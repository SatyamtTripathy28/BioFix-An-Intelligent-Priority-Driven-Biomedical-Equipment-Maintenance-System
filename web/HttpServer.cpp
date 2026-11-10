// HttpServer.cpp
// The network code is different on Windows (Winsock) and Linux/macOS,
// so the few lines that differ are inside #ifdef _WIN32.

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>            // must come before any other header
    typedef SOCKET socket_t;
    #define CLOSE_SOCKET closesocket
    static const socket_t BAD_SOCKET = INVALID_SOCKET;
#else
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <sys/time.h>
    #include <unistd.h>
    typedef int socket_t;
    #define CLOSE_SOCKET close
    static const socket_t BAD_SOCKET = -1;
#endif

#include <cctype>
#include <cstdlib>
#include <iostream>
#include "HttpServer.h"

static const size_t MAX_HEADER = 16 * 1024;     // refuse very large requests
static const size_t MAX_BODY = 64 * 1024;

// -------------------- URL DECODING --------------------
static int hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// "X%20Ray+Unit" -> "X Ray Unit"
static std::string urlDecode(const std::string& s) {
    std::string out;

    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '+') {
            out += ' ';
        }
        else if (s[i] == '%' && i + 2 < s.size() &&
                 hexValue(s[i + 1]) >= 0 && hexValue(s[i + 2]) >= 0) {
            out += (char)(hexValue(s[i + 1]) * 16 + hexValue(s[i + 2]));
            i += 2;
        }
        else {
            out += s[i];
        }
    }

    return out;
}

std::map<std::string, std::string> parseUrlEncoded(const std::string& text) {
    std::map<std::string, std::string> values;
    size_t start = 0;

    while (start <= text.size()) {
        size_t end = text.find('&', start);
        if (end == std::string::npos)
            end = text.size();

        std::string pair = text.substr(start, end - start);
        size_t eq = pair.find('=');

        if (!pair.empty()) {
            if (eq == std::string::npos)
                values[urlDecode(pair)] = "";
            else
                values[urlDecode(pair.substr(0, eq))] = urlDecode(pair.substr(eq + 1));
        }

        start = end + 1;
    }

    return values;
}

// -------------------- SENDING AND RECEIVING --------------------
static bool sendAll(socket_t client, const std::string& data) {
    size_t sent = 0;

    while (sent < data.size()) {
        int n = send(client, data.c_str() + sent, (int)(data.size() - sent), 0);
        if (n <= 0)
            return false;
        sent += (size_t)n;
    }

    return true;
}

static std::string statusText(int status) {
    if (status == 200) return "OK";
    if (status == 400) return "Bad Request";
    if (status == 404) return "Not Found";
    if (status == 405) return "Method Not Allowed";
    if (status == 413) return "Payload Too Large";
    return "Error";
}

static void sendResponse(socket_t client, const HttpResponse& response) {
    std::string head =
        "HTTP/1.1 " + std::to_string(response.status) + " " + statusText(response.status) + "\r\n"
        "Content-Type: " + response.contentType + "\r\n"
        "Content-Length: " + std::to_string(response.body.size()) + "\r\n"
        "Cache-Control: no-store\r\n"
        "X-Content-Type-Options: nosniff\r\n"
        "Connection: close\r\n\r\n";

    sendAll(client, head + response.body);
}

static std::string toLowerText(std::string s) {
    for (char& c : s)
        c = (char)std::tolower((unsigned char)c);
    return s;
}

// Reads one request, calls the handler, and sends the answer back.
static void handleClient(socket_t client, const HttpHandler& handler) {
    std::string data;
    char buffer[4096];
    size_t headerEnd;

    // 1. Read until the empty line that ends the headers.
    while ((headerEnd = data.find("\r\n\r\n")) == std::string::npos) {
        if (data.size() > MAX_HEADER) {
            sendResponse(client, {413, "text/plain; charset=utf-8", "Request too large"});
            return;
        }

        int n = recv(client, buffer, (int)sizeof(buffer), 0);
        if (n <= 0)
            return;                  // the browser closed the connection or was too slow
        data.append(buffer, (size_t)n);
    }

    // 2. First line: "POST /api/equipment?x=1 HTTP/1.1"
    std::string firstLine = data.substr(0, data.find("\r\n"));
    size_t space1 = firstLine.find(' ');
    size_t space2 = firstLine.find(' ', space1 + 1);

    if (space1 == std::string::npos || space2 == std::string::npos) {
        sendResponse(client, {400, "text/plain; charset=utf-8", "Bad request"});
        return;
    }

    HttpRequest request;
    request.method = firstLine.substr(0, space1);
    std::string target = firstLine.substr(space1 + 1, space2 - space1 - 1);

    size_t question = target.find('?');
    request.path = target.substr(0, question);
    if (question != std::string::npos)
        request.query = parseUrlEncoded(target.substr(question + 1));

    // 3. Find Content-Length, so we know how much body to read.
    size_t contentLength = 0;
    std::string headers = toLowerText(data.substr(0, headerEnd));
    size_t pos = headers.find("\r\ncontent-length:");

    if (pos != std::string::npos)
        contentLength = (size_t)std::strtoul(headers.c_str() + pos + 17, nullptr, 10);

    if (contentLength > MAX_BODY) {
        sendResponse(client, {413, "text/plain; charset=utf-8", "Request too large"});
        return;
    }

    // 4. Read the rest of the body.
    std::string body = data.substr(headerEnd + 4);

    while (body.size() < contentLength) {
        int n = recv(client, buffer, (int)sizeof(buffer), 0);
        if (n <= 0)
            return;
        body.append(buffer, (size_t)n);
    }

    if (request.method == "POST")
        request.form = parseUrlEncoded(body.substr(0, contentLength));

    // 5. Let BioFix answer, then send it back.
    sendResponse(client, handler(request));
}

// -------------------- MAIN LOOP --------------------
bool runServer(int port, HttpHandler handler) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cout << "Could not start Windows networking.\n";
        return false;
    }
#endif

    socket_t server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == BAD_SOCKET) {
        std::cout << "Could not create the server socket.\n";
        return false;
    }

    int yes = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));

    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_port = htons((unsigned short)port);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);   // 127.0.0.1: this computer only

    if (bind(server, (sockaddr*)&address, sizeof(address)) != 0 || listen(server, 16) != 0) {
        std::cout << "Port " << port << " is busy. Close the other program or use another port.\n";
        CLOSE_SOCKET(server);
        return false;
    }

    std::cout << "BioFix website is running at http://localhost:" << port << "\n";
    std::cout << "Open that address in your browser. Press Ctrl+C here to stop.\n";
    std::cout.flush();

    while (true) {
        socket_t client = accept(server, nullptr, nullptr);
        if (client == BAD_SOCKET)
            continue;

        // Don't wait forever for a slow or broken connection (5 seconds).
#ifdef _WIN32
        DWORD timeout = 5000;
        setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#else
        timeval timeout = {5, 0};
        setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#endif

        handleClient(client, handler);
        CLOSE_SOCKET(client);
    }
}

// HttpServer.h
// A very small HTTP server: enough for one person using BioFix in a browser.
// It answers one request at a time and only listens on this computer (127.0.0.1).
//
// No project headers here, and no "using namespace std", so the Windows
// network headers can be included safely in HttpServer.cpp.

#ifndef BIOFIX_HTTPSERVER_H
#define BIOFIX_HTTPSERVER_H

#include <functional>
#include <map>
#include <string>

struct HttpRequest {
    std::string method;                          // "GET" or "POST"
    std::string path;                            // for example "/api/equipment"
    std::map<std::string, std::string> query;    // values after '?' in the URL
    std::map<std::string, std::string> form;     // values sent in a POST body
};

struct HttpResponse {
    int status = 200;                            // 200 OK, 400 Bad Request, 404 Not Found
    std::string contentType = "application/json; charset=utf-8";
    std::string body;
};

using HttpHandler = std::function<HttpResponse(const HttpRequest&)>;

// Runs until the program is stopped (Ctrl+C). Returns false if the port can't be opened.
bool runServer(int port, HttpHandler handler);

// "id=104&name=X%20Ray" -> {"id": "104", "name": "X Ray"}
std::map<std::string, std::string> parseUrlEncoded(const std::string& text);

#endif

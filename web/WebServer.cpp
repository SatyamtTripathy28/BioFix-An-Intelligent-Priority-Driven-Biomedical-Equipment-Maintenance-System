// WebServer.cpp
// BioFix as a website. Run it from the project folder:
//     build/BioFixWeb          (Linux, macOS, Git Bash)
//     BioFixWeb.exe            (Windows)
// then open http://localhost:8080 in a browser.
//
// The browser only shows data. Everything is kept in the same linked list,
// hash table, priority queue and history stack as the CLI, and saved to the
// same .txt files in data/.

#ifdef _WIN32
    #include <direct.h>
    #define CHANGE_DIR _chdir
#else
    #include <unistd.h>
    #define CHANGE_DIR chdir
#endif

#include <fstream>
#include <sstream>
#include "HttpServer.h"
#include "../src/EquipmentList.h"
#include "../src/MaintenanceQueue.h"
#include "../src/RepairHistory.h"
#include "../src/FileManager.h"
#include "../src/EquipmentMenu.h"
#include "../src/MaintenanceManager.h"
#include "../src/HistoryManager.h"
#include "../src/SearchSort.h"
#include "../src/InputHelper.h"

// The program's data, loaded once at start.
static EquipmentList equipmentList;
static MaintenanceQueue maintenanceQueue;
static RepairHistory repairHistory;
static string indexPage;

// -------------------- JSON HELPERS --------------------
// JSON is the text format the browser understands, for example {"id":104}.

static string jsonText(const string& s) {
    string out = "\"";

    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if ((unsigned char)c < 0x20) out += " ";
        else if (c == '<') out += "\\u003c";     // safe to place inside HTML
        else out += c;
    }

    return out + "\"";
}

static string equipmentJson(const Equipment* e) {
    return "{\"id\":" + to_string(e->getId()) +
           ",\"name\":" + jsonText(e->getName()) +
           ",\"department\":" + jsonText(e->getDepartment()) +
           ",\"type\":" + jsonText(e->getCategory()) +
           ",\"status\":" + jsonText(e->getStatus()) +
           ",\"priority\":" + to_string(e->getPriority()) +
           ",\"waiting\":" + to_string(maintenanceQueue.countFor(e->getId())) + "}";
}

static HttpResponse json(const string& body, int status = 200) {
    HttpResponse response;
    response.status = status;
    response.body = body;
    return response;
}

static HttpResponse resultJson(const ActionResult& result) {
    string body = string("{\"ok\":") + (result.ok ? "true" : "false") +
                  ",\"message\":" + jsonText(result.message) + "}";
    return json(body, result.ok ? 200 : 400);
}

// Reads a whole number sent by the browser. Returns -1 if it isn't one.
static int toInt(const map<string, string>& values, const string& key) {
    auto it = values.find(key);
    if (it == values.end() || !isNumber(it->second) || it->second.size() > 9)
        return -1;
    return stoi(it->second);
}

// Priority from a form: "" or "auto" = 0 (automatic); "1" to "3" = by hand; anything else = -1.
static int priorityOf(const map<string, string>& values) {
    auto it = values.find("priority");
    if (it == values.end() || it->second.empty() || it->second == "auto")
        return 0;
    return (it->second.size() == 1 && isNumber(it->second)) ? stoi(it->second) : -1;
}

static string valueOf(const map<string, string>& values, const string& key) {
    auto it = values.find(key);
    return it == values.end() ? "" : it->second;
}

// -------------------- READ-ONLY PAGES OF DATA --------------------

// GET /api/summary : the numbers on the dashboard cards
static HttpResponse summary() {
    int machines = 0, working = 0, maintenance = 0, unavailable = 0;

    for (Node* n = equipmentList.getHead(); n != nullptr; n = n->next) {
        machines++;
        string s = n->data->getStatus();
        if (s == "Working") working++;
        else if (s == "Under Maintenance") maintenance++;
        else unavailable++;
    }

    return json("{\"machines\":" + to_string(machines) +
                ",\"working\":" + to_string(working) +
                ",\"underMaintenance\":" + to_string(maintenance) +
                ",\"unavailable\":" + to_string(unavailable) +
                ",\"waiting\":" + to_string(maintenanceQueue.count()) +
                ",\"urgent\":" + to_string(maintenanceQueue.count(1)) + "}");
}

// GET /api/equipment?search=icu&department=ICU&sort=name
static HttpResponse equipment(const HttpRequest& request) {
    string search = trim(valueOf(request.query, "search"));
    string department = valueOf(request.query, "department");
    bool byName = valueOf(request.query, "sort") == "name";

    // Copy the pointers into an array so they can be sorted (insertion sort).
    int n;
    Equipment** arr = equipmentList.toArray(n);
    if (byName && n > 0)
        insertionSortByName(arr, n);

    string body = "[";
    bool first = true;

    for (int i = 0; i < n; i++) {
        Equipment* e = arr[i];

        if (!department.empty() && e->getDepartment() != department)
            continue;
        if (!search.empty() && !matchesKeyword(e, search))     // linear search
            continue;

        body += (first ? "" : ",") + equipmentJson(e);
        first = false;
    }

    delete[] arr;
    return json(body + "]");
}

// GET /api/find?id=104 : exact lookup through the hash table
static HttpResponse findById(const HttpRequest& request) {
    Equipment* e = equipmentList.findById(toInt(request.query, "id"));
    if (e == nullptr)
        return json("{\"ok\":false,\"message\":\"Equipment not found.\"}", 404);
    return json(equipmentJson(e));
}

// GET /api/queue : waiting requests in queue order (heap sort of the min-heap)
static HttpResponse queue() {
    int n;
    MaintenanceRequest* list = maintenanceQueue.sortedCopy(n);
    string body = "[";

    for (int i = 0; i < n; i++) {
        const MaintenanceRequest& r = list[i];
        Equipment* e = equipmentList.findById(r.equipmentId);

        body += string(i ? "," : "") +
                "{\"requestId\":" + to_string(r.requestId) +
                ",\"id\":" + to_string(r.equipmentId) +
                ",\"name\":" + jsonText(e ? e->getName() : "(deleted)") +
                ",\"priority\":" + to_string(r.effectivePriority) +
                ",\"priorityLabel\":" + jsonText(priorityLabel(r.effectivePriority)) +
                ",\"basePriority\":" + to_string(r.priority) +
                ",\"aged\":" + (r.effectivePriority != r.priority ? "true" : "false") +
                ",\"manual\":" + (r.manualPriority ? "true" : "false") +
                ",\"pinned\":" + (r.manualRank > 0 ? "true" : "false") +
                ",\"position\":" + to_string(i + 1) +
                ",\"date\":" + jsonText(r.date) +
                ",\"description\":" + jsonText(r.description) + "}";
    }

    delete[] list;
    return json(body + "]");
}

// GET /api/history?id=146 : finished repairs of one machine, newest first
static HttpResponse history(const HttpRequest& request) {
    int id = toInt(request.query, "id");
    Equipment* e = equipmentList.findById(id);

    if (e == nullptr)
        return json("{\"ok\":false,\"message\":\"Equipment not found.\"}", 404);

    string records = "[";
    bool first = true;
    int total = 0, repairs = 0;

    for (HistoryNode* h = repairHistory.getTop(); h != nullptr; h = h->next) {
        const RepairRecord& r = h->data;
        if (r.equipmentId != id)
            continue;

        records += string(first ? "" : ",") +
                   "{\"requestDate\":" + jsonText(r.requestDate) +
                   ",\"completedDate\":" + jsonText(r.completedDate) +
                   ",\"days\":" + to_string(r.downtimeDays) +
                   ",\"description\":" + jsonText(r.description) + "}";
        first = false;
        total += r.downtimeDays;
        repairs++;
    }

    return json("{\"equipment\":" + equipmentJson(e) +
                ",\"repairs\":" + to_string(repairs) +
                ",\"totalDays\":" + to_string(total) +
                ",\"records\":" + records + "]}");
}

// GET /api/ranking : downtime ranking (insertion sort, most days first)
static HttpResponse ranking() {
    RankEntry* rows = nullptr;
    int used = computeRanking(equipmentList, repairHistory, rows);

    string body = "[";
    for (int i = 0; i < used; i++) {
        body += string(i ? "," : "") +
                "{\"rank\":" + to_string(i + 1) +
                ",\"id\":" + to_string(rows[i].equipment->getId()) +
                ",\"name\":" + jsonText(rows[i].equipment->getName()) +
                ",\"days\":" + to_string(rows[i].downtime) +
                ",\"repairs\":" + to_string(rows[i].repairs) + "}";
    }

    delete[] rows;
    return json(body + "]");
}

// GET /api/notifications?days=7&today=2026-10-30
// Requests that are overdue, due today, or due within 'days' days, earliest first.
static HttpResponse notifications(const HttpRequest& request) {
    string today = valueOf(request.query, "today");
    if (!isValidDate(today))
        today = todayDate();                     // the browser didn't send a date

    int days = toInt(request.query, "days");
    if (days < 0 || days > 365)
        days = 7;

    MaintenanceRequest* due = nullptr;
    int used = dueRequests(maintenanceQueue, today, days, due);

    int overdue = 0;
    string items = "[";

    for (int i = 0; i < used; i++) {
        Equipment* e = equipmentList.findById(due[i].equipmentId);
        int daysLeft = daysBetween(today, due[i].date);   // negative = overdue
        if (daysLeft < 0)
            overdue++;

        items += string(i ? "," : "") +
                 "{\"requestId\":" + to_string(due[i].requestId) +
                 ",\"id\":" + to_string(due[i].equipmentId) +
                 ",\"name\":" + jsonText(e ? e->getName() : "(deleted)") +
                 ",\"priority\":" + to_string(due[i].effectivePriority) +
                 ",\"date\":" + jsonText(due[i].date) +
                 ",\"description\":" + jsonText(due[i].description) +
                 ",\"daysLeft\":" + to_string(daysLeft) + "}";
    }

    delete[] due;

    return json("{\"today\":" + jsonText(today) +
                ",\"days\":" + to_string(days) +
                ",\"count\":" + to_string(used) +
                ",\"overdue\":" + to_string(overdue) +
                ",\"items\":" + items + "]}");
}

// -------------------- ROUTER --------------------
// Looks at the address the browser asked for and calls the right function.
static HttpResponse route(const HttpRequest& request) {
    // Aging depends on today's date (the server may run past midnight).
    maintenanceQueue.setToday(todayDate());

    const string& path = request.path;
    const map<string, string>& form = request.form;

    if (request.method == "GET") {
        if (path == "/" || path == "/index.html") {
            HttpResponse page;
            page.contentType = "text/html; charset=utf-8";
            page.body = indexPage;
            return page;
        }
        if (path == "/api/summary")   return summary();
        if (path == "/api/equipment") return equipment(request);
        if (path == "/api/find")      return findById(request);
        if (path == "/api/queue")     return queue();
        if (path == "/api/history")   return history(request);
        if (path == "/api/ranking")   return ranking();
        if (path == "/api/notifications") return notifications(request);
    }

    // Everything that changes data must be a POST.
    if (request.method == "POST") {
        if (path == "/api/equipment/add")
            return resultJson(addEquipmentRecord(equipmentList, toInt(form, "id"),
                valueOf(form, "name"), valueOf(form, "department"),
                valueOf(form, "type"), valueOf(form, "status")));

        if (path == "/api/equipment/update")
            return resultJson(updateEquipmentRecord(equipmentList, maintenanceQueue,
                toInt(form, "id"), valueOf(form, "name"), valueOf(form, "department"),
                valueOf(form, "status")));

        if (path == "/api/equipment/delete")
            return resultJson(deleteEquipmentRecord(equipmentList, maintenanceQueue,
                toInt(form, "id")));

        if (path == "/api/maintenance/schedule")
            return resultJson(scheduleRequest(equipmentList, maintenanceQueue,
                toInt(form, "id"), valueOf(form, "date"), valueOf(form, "description"),
                priorityOf(form)));

        if (path == "/api/maintenance/update") {
            ActionResult result = updateRequest(maintenanceQueue, toInt(form, "requestId"),
                valueOf(form, "date"), valueOf(form, "description"));

            // The edit form may also send a new priority.
            if (result.ok && form.count("priority")) {
                ActionResult changed = setRequestPriority(equipmentList, maintenanceQueue,
                    toInt(form, "requestId"), priorityOf(form));
                if (!changed.ok)
                    return resultJson(changed);
                result.message += "\n" + changed.message;
            }

            return resultJson(result);
        }

        if (path == "/api/maintenance/priority")
            return resultJson(setRequestPriority(equipmentList, maintenanceQueue,
                toInt(form, "requestId"), priorityOf(form)));

        if (path == "/api/maintenance/cancel")
            return resultJson(cancelRequest(equipmentList, maintenanceQueue,
                toInt(form, "requestId")));

        if (path == "/api/maintenance/position")
            return resultJson(moveRequestToPosition(maintenanceQueue,
                toInt(form, "requestId"), toInt(form, "position")));

        if (path == "/api/maintenance/unpin")
            return resultJson(unpinRequest(maintenanceQueue, toInt(form, "requestId")));

        if (path == "/api/maintenance/reset-order")
            return resultJson(resetManualOrder(maintenanceQueue));

        if (path == "/api/maintenance/complete")
            return resultJson(completeRequest(equipmentList, maintenanceQueue, repairHistory,
                toInt(form, "requestId"), valueOf(form, "completed")));

        if (path == "/api/maintenance/process")
            return resultJson(completeNextRequest(equipmentList, maintenanceQueue,
                repairHistory, valueOf(form, "completed")));
    }

    return json("{\"ok\":false,\"message\":\"Not found.\"}", 404);
}

int main(int argc, char* argv[]) {
    int port = 8080;
    if (argc > 1 && isNumber(argv[1]) && string(argv[1]).size() <= 5)
        port = stoi(argv[1]);

    // 1. Read the web page (from the project folder).
    ifstream page("web/index.html");
    if (!page) {
        cout << "Cannot find web/index.html. Start BioFixWeb from the project folder.\n";
        return 1;
    }
    stringstream ss;
    ss << page.rdbuf();
    indexPage = ss.str();

    // 2. Work inside data/, like the CLI, so the same .txt files are used.
    if (CHANGE_DIR("data") != 0) {
        cout << "Cannot find the data folder. Start BioFixWeb from the project folder.\n";
        return 1;
    }

    loadEquipment(equipmentList);
    loadMaintenance(maintenanceQueue, equipmentList);
    loadHistory(repairHistory);
    maintenanceQueue.setToday(todayDate());

    // 3. Answer the browser until Ctrl+C. Every change is saved immediately.
    return runServer(port, route) ? 0 : 1;
}

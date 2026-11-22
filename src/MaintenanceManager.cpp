// MaintenanceManager.cpp
// Author: Rajat Rathore

#include <fstream>
#include <sstream>
#include "MaintenanceManager.h"
#include "FileManager.h"
#include "HistoryManager.h"
#include "InputHelper.h"

const string MAINTENANCE_FILE = "maintenance.txt";

// File line: requestNumber|equipmentId|date|priority|auto-or-manual|manualRank|description
// (manualRank: 0 = normal order, 1, 2, 3 ... = position chosen by hand)
// Older files (without manual rank, priority or request number) are still read.
// "auto" priorities are taken again from the machine type when loading.
void loadMaintenance(MaintenanceQueue& queue, const EquipmentList& list) {
    ifstream file(MAINTENANCE_FILE);

    if (!file)
        return;

    string line;

    while (getline(file, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        if (line.empty())
            continue;

        // Split the line at every '|'.
        string parts[8];
        int count = 0;
        stringstream ss(line);
        while (count < 8 && getline(ss, parts[count], '|'))
            count++;

        string reqStr, idStr, date, description;
        string priorityStr = "", mode = "auto", rankStr = "0";

        if (count == 7) {                    // current format
            reqStr = parts[0]; idStr = parts[1]; date = parts[2];
            priorityStr = parts[3]; mode = parts[4]; rankStr = parts[5]; description = parts[6];
        }
        else if (count == 6) {               // older format, no manual position
            reqStr = parts[0]; idStr = parts[1]; date = parts[2];
            priorityStr = parts[3]; mode = parts[4]; description = parts[5];
        }
        else if (count == 4) {               // older format, no priority
            reqStr = parts[0]; idStr = parts[1]; date = parts[2]; description = parts[3];
        }
        else if (count == 3) {               // old format, no request number
            reqStr = "0"; idStr = parts[0]; date = parts[1]; description = parts[2];
        }
        else {
            cout << "Skipping a damaged line in maintenance.txt.\n";
            continue;
        }

        if (!isNumber(reqStr) || reqStr.size() > 9 || !isNumber(idStr) || idStr.size() > 9) {
            cout << "Skipping a line in maintenance.txt: bad number.\n";
            continue;
        }

        if (!isValidDate(date)) {
            cout << "Skipping a line in maintenance.txt: bad date \"" << date << "\".\n";
            continue;
        }

        int id = stoi(idStr);
        Equipment* e = list.findById(id);

        if (e == nullptr) {
            cout << "Skipping a request in maintenance.txt: equipment " << id << " not found.\n";
            continue;
        }

        MaintenanceRequest request;
        request.requestId = stoi(reqStr);
        if (queue.findRequest(request.requestId) != nullptr)
            request.requestId = 0;          // duplicate number: give it a new one
        request.equipmentId = id;
        request.date = date;
        request.description = description;
        request.manualRank = (isNumber(rankStr) && rankStr.size() <= 6) ? stoi(rankStr) : 0;

        // A hand-set priority is kept; otherwise it comes from the machine type.
        if (mode == "manual" && priorityStr.size() == 1 && isNumber(priorityStr) &&
            isValidPriority(stoi(priorityStr))) {
            request.priority = stoi(priorityStr);
            request.manualPriority = true;
        }
        else {
            request.priority = e->getPriority();     // polymorphic call
            request.manualPriority = false;
        }

        queue.enqueue(request);
    }

    file.close();
}

// Writes the queue from front to back (most urgent first).
void saveMaintenanceQueue(const MaintenanceQueue& queue) {
    ofstream file(MAINTENANCE_FILE);

    if (!file) {
        cout << "Error opening maintenance.txt\n";
        return;
    }

    // Saved in queue order, so the file is easy to read.
    int n;
    MaintenanceRequest* list = queue.sortedCopy(n);

    for (int i = 0; i < n; i++) {
        file << list[i].requestId << "|"
             << list[i].equipmentId << "|"
             << list[i].date << "|"
             << list[i].priority << "|"
             << (list[i].manualPriority ? "manual" : "auto") << "|"
             << list[i].manualRank << "|"
             << list[i].description << "\n";
    }

    delete[] list;
    file.close();
}

// ==================== LOGIC ====================

// priority 0 = automatic (from the machine type); 1 to 3 = set by hand.
ActionResult scheduleRequest(EquipmentList& list, MaintenanceQueue& queue,
                             int id, const string& date, string description, int priority) {
    description = trim(description);
    Equipment* e = list.findById(id);

    if (e == nullptr)
        return {false, "Equipment not found."};
    if (!isValidDate(date))
        return {false, "Invalid date. Use YYYY-MM-DD, for example 2026-10-21."};
    if (!isValidText(description))
        return {false, "Description must be 1 to 60 characters, without the | character."};
    if (priority != 0 && !isValidPriority(priority))
        return {false, "Priority must be Auto, 1, 2 or 3."};

    MaintenanceRequest request;
    request.equipmentId = id;
    request.date = date;
    request.description = description;

    if (priority == 0) {
        request.priority = e->getPriority();   // each subclass gives its own priority
        request.manualPriority = false;
    }
    else {
        request.priority = priority;
        request.manualPriority = true;
    }

    queue.enqueue(request);
    e->setStatus("Under Maintenance");

    saveFiles(list);
    saveMaintenanceQueue(queue);

    return {true, "Maintenance scheduled successfully.\nPriority: " + priorityLabel(request.priority) +
                  (request.manualPriority ? " (set by hand)" : " (from machine type)")};
}

// Finishes one request (requestId 0 = the first in the queue) and records it in the history.
ActionResult completeRequest(EquipmentList& list, MaintenanceQueue& queue,
                             RepairHistory& history, int requestId, const string& completedDate) {
    MaintenanceRequest request;

    if (requestId == 0) {
        if (!queue.peek(request))
            return {false, "Maintenance queue is empty."};
    }
    else {
        const MaintenanceRequest* found = queue.findRequest(requestId);
        if (found == nullptr)
            return {false, "Maintenance request not found."};
        request = *found;
    }

    if (!isValidDate(completedDate))
        return {false, "Invalid date. Use YYYY-MM-DD, for example 2026-10-21."};
    if (daysBetween(request.date, completedDate) < 0)
        return {false, "Completion date cannot be before the request date (" + request.date + ")."};

    queue.removeRequest(request.requestId);

    RepairRecord record;
    record.equipmentId = request.equipmentId;
    record.requestDate = request.date;
    record.completedDate = completedDate;
    record.downtimeDays = daysBetween(request.date, completedDate);
    record.description = request.description;

    history.push(record);

    // The machine is working again unless another request for it is still waiting.
    Equipment* e = list.findById(request.equipmentId);
    if (e != nullptr && !queue.hasRequestFor(request.equipmentId))
        e->setStatus("Working");

    saveFiles(list);
    saveMaintenanceQueue(queue);
    saveHistory(history);

    return {true, "Request #" + to_string(request.requestId) + " completed. Downtime: " +
                  to_string(record.downtimeDays) + " day(s)."};
}

ActionResult completeNextRequest(EquipmentList& list, MaintenanceQueue& queue,
                                 RepairHistory& history, const string& completedDate) {
    return completeRequest(list, queue, history, 0, completedDate);
}

// Changes the date and description of one waiting request.
// The date takes part in the order, so the request may move.
ActionResult updateRequest(MaintenanceQueue& queue, int requestId,
                           const string& date, string description) {
    description = trim(description);
    const MaintenanceRequest* request = queue.findRequest(requestId);

    if (request == nullptr)
        return {false, "Maintenance request not found."};
    if (!isValidDate(date))
        return {false, "Invalid date. Use YYYY-MM-DD, for example 2026-10-21."};
    if (!isValidText(description))
        return {false, "Description must be 1 to 60 characters, without the | character."};

    queue.changeDetails(requestId, date, description);
    saveMaintenanceQueue(queue);

    return {true, "Maintenance request #" + to_string(requestId) + " updated."};
}

// Changes one request's priority. 0 = automatic (back to the machine type's priority).
// The request moves to its new place in the queue.
ActionResult setRequestPriority(const EquipmentList& list, MaintenanceQueue& queue,
                                int requestId, int priority) {
    const MaintenanceRequest* request = queue.findRequest(requestId);

    if (request == nullptr)
        return {false, "Maintenance request not found."};
    if (priority != 0 && !isValidPriority(priority))
        return {false, "Priority must be Auto, 1, 2 or 3."};

    bool manual = priority != 0;

    if (!manual) {
        Equipment* e = list.findById(request->equipmentId);
        priority = e != nullptr ? e->getPriority() : 3;   // polymorphic call
    }

    queue.changePriority(requestId, priority, manual);
    saveMaintenanceQueue(queue);

    return {true, "Request #" + to_string(requestId) + " is now " + priorityLabel(priority) +
                  (manual ? " (set by hand)." : " (from machine type).")};
}

// Pins a request at a position of the pinned group at the top (1 = done first),
// or moves it inside that group.
ActionResult moveRequestToPosition(MaintenanceQueue& queue, int requestId, int position) {
    if (queue.findRequest(requestId) == nullptr)
        return {false, "Maintenance request not found."};

    int highest = queue.maxPosition(requestId);
    if (position < 1 || position > highest)
        return {false, "Position must be between 1 and " + to_string(highest) +
                       " (the pinned requests at the top)."};

    queue.moveToPosition(requestId, position);
    saveMaintenanceQueue(queue);

    return {true, "Request #" + to_string(requestId) + " is pinned at position " +
                  to_string(position) + "."};
}

// Puts one request back into the normal priority order.
ActionResult unpinRequest(MaintenanceQueue& queue, int requestId) {
    const MaintenanceRequest* request = queue.findRequest(requestId);

    if (request == nullptr)
        return {false, "Maintenance request not found."};
    if (request->manualRank == 0)
        return {false, "Request #" + to_string(requestId) + " is already in the normal order."};

    queue.unpin(requestId);
    saveMaintenanceQueue(queue);

    return {true, "Request #" + to_string(requestId) + " is back in the normal priority order."};
}

// Puts every request back into the normal priority order.
ActionResult resetManualOrder(MaintenanceQueue& queue) {
    int pinned = queue.pinnedCount();
    if (pinned == 0)
        return {false, "No request has an order set by hand."};

    queue.clearManualOrder();
    saveMaintenanceQueue(queue);

    return {true, "Order reset: " + to_string(pinned) + " request(s) are back in the normal priority order."};
}

// Removes one waiting request without repairing anything.
// If the machine has no other waiting request, it is Working again.
ActionResult cancelRequest(EquipmentList& list, MaintenanceQueue& queue, int requestId) {
    const MaintenanceRequest* request = queue.findRequest(requestId);

    if (request == nullptr)
        return {false, "Maintenance request not found."};

    int equipmentId = request->equipmentId;
    queue.removeRequest(requestId);

    string message = "Maintenance request #" + to_string(requestId) + " cancelled.";

    Equipment* e = list.findById(equipmentId);
    if (e != nullptr && !queue.hasRequestFor(equipmentId) && e->getStatus() == "Under Maintenance") {
        e->setStatus("Working");
        message += "\n" + e->getName() + " is Working again.";
    }

    saveFiles(list);
    saveMaintenanceQueue(queue);

    return {true, message};
}

// Earlier date first; same date: lower priority number (more urgent) first;
// then the older request (lower request number) first.
static bool earlierThan(const MaintenanceRequest& a, const MaintenanceRequest& b) {
    if (a.date != b.date) return a.date < b.date;     // YYYY-MM-DD sorts like text
    if (a.effectivePriority != b.effectivePriority) return a.effectivePriority < b.effectivePriority;
    return a.requestId < b.requestId;
}

int dueRequests(const MaintenanceQueue& queue, const string& today, int withinDays,
                MaintenanceRequest*& due) {
    int n = queue.count();
    due = new MaintenanceRequest[n > 0 ? n : 1];
    int used = 0;

    // 1. Keep only requests that are overdue, today, or coming soon.
    for (int i = 0; i < queue.getSize(); i++)
        if (daysBetween(today, queue.at(i).date) <= withinDays)
            due[used++] = queue.at(i);

    // 2. Insertion sort by date.
    for (int i = 1; i < used; i++) {
        MaintenanceRequest key = due[i];
        int j = i - 1;

        while (j >= 0 && earlierThan(key, due[j])) {
            due[j + 1] = due[j];
            j--;
        }

        due[j + 1] = key;
    }

    return used;
}

// ==================== CLI SCREENS ====================

void scheduleMaintenance(EquipmentList& list, MaintenanceQueue& queue) {
    int id = readInt("\nEnter equipment ID: ");
    if (inputEnded())
        return;

    if (list.findById(id) == nullptr) {
        cout << "Equipment not found.\n";
        return;
    }

    string date = readDate("Enter maintenance date (YYYY-MM-DD): ");
    if (date.empty()) return;

    string description = readText("Enter maintenance description: ");
    if (description.empty()) return;

    ActionResult result = scheduleRequest(list, queue, id, date, description);
    cout << "\n" << result.message << "\n";
}

void processNextRequest(EquipmentList& list, MaintenanceQueue& queue,
                        RepairHistory& history) {
    MaintenanceRequest request;

    if (!queue.peek(request)) {
        cout << "\nMaintenance queue is empty.\n";
        return;
    }

    cout << "\nNext maintenance request (first in the queue):\n";
    cout << "Request: #" << request.requestId << endl;
    cout << "Equipment ID: " << request.equipmentId << endl;
    cout << "Priority: " << priorityLabel(request.effectivePriority)
         << (request.effectivePriority != request.priority ? " (aged from " + priorityLabel(request.priority) + ")"
             : request.manualPriority ? " (set by hand)" : "") << endl;
    cout << "Date: " << request.date << endl;
    cout << "Description: " << request.description << endl;

    // Ask until the completion date is accepted.
    while (true) {
        string completed = readDate("Enter completion date (YYYY-MM-DD): ");
        if (completed.empty())          // input ended: nothing was removed
            return;

        ActionResult result = completeNextRequest(list, queue, history, completed);

        if (result.ok) {
            cout << "\n" << result.message << "\n";
            return;
        }

        cout << result.message << "\n";
    }
}

// Menu option 14: change the date or description of one request.
void editMaintenanceRequest(MaintenanceQueue& queue) {
    if (queue.isEmpty()) {
        cout << "\nMaintenance queue is empty.\n";
        return;
    }

    queue.display();

    int number = readInt("\nEnter request number to edit (the number after #): ");
    if (inputEnded())
        return;

    const MaintenanceRequest* request = queue.findRequest(number);
    if (request == nullptr) {
        cout << "Maintenance request not found.\n";
        return;
    }

    cout << "Current: " << request->date << ", " << request->description << "\n";

    string date = readDate("Enter new date (YYYY-MM-DD): ");
    if (date.empty()) return;

    string description = readText("Enter new description: ");
    if (description.empty()) return;

    ActionResult result = updateRequest(queue, number, date, description);
    cout << "\n" << result.message << "\n";
}

// Menu option 15: remove one request without repairing.
void cancelMaintenanceRequest(EquipmentList& list, MaintenanceQueue& queue) {
    if (queue.isEmpty()) {
        cout << "\nMaintenance queue is empty.\n";
        return;
    }

    queue.display();

    int number = readInt("\nEnter request number to cancel (the number after #): ");
    if (inputEnded())
        return;

    ActionResult result = cancelRequest(list, queue, number);
    cout << "\n" << result.message << "\n";
}

// Menu option 16: change the priority of one request.
void changeRequestPriority(const EquipmentList& list, MaintenanceQueue& queue) {
    if (queue.isEmpty()) {
        cout << "\nMaintenance queue is empty.\n";
        return;
    }

    queue.display();

    int number = readInt("\nEnter request number (the number after #): ");
    if (inputEnded())
        return;

    const MaintenanceRequest* request = queue.findRequest(number);
    if (request == nullptr) {
        cout << "Maintenance request not found.\n";
        return;
    }

    Equipment* e = list.findById(request->equipmentId);
    int automatic = e != nullptr ? e->getPriority() : 3;

    int priority;
    while (true) {
        cout << "\nSelect Priority:\n";
        cout << "0. Auto, from machine type (" << priorityLabel(automatic) << ")\n";
        cout << "1. P1 Urgent\n";
        cout << "2. P2 High\n";
        cout << "3. P3 Normal\n";

        priority = readInt("Enter choice: ");
        if (inputEnded())
            return;
        if (priority >= 0 && priority <= 3)
            break;

        cout << "Invalid choice. Try again.\n";
    }

    ActionResult result = setRequestPriority(list, queue, number, priority);
    cout << "\n" << result.message << "\n";
}

// Menu option 17: complete any request, not only the first one
// (for example when the first one is waiting for a spare part).
void completeSpecificRequest(EquipmentList& list, MaintenanceQueue& queue,
                             RepairHistory& history) {
    if (queue.isEmpty()) {
        cout << "\nMaintenance queue is empty.\n";
        return;
    }

    queue.display();

    int number = readInt("\nEnter request number to complete (the number after #): ");
    if (inputEnded())
        return;

    const MaintenanceRequest* request = queue.findRequest(number);
    if (request == nullptr) {
        cout << "Maintenance request not found.\n";
        return;
    }

    cout << "Request #" << number << ": equipment " << request->equipmentId << ", "
         << request->date << ", " << request->description << "\n";

    while (true) {
        string completed = readDate("Enter completion date (YYYY-MM-DD): ");
        if (completed.empty())
            return;

        ActionResult result = completeRequest(list, queue, history, number, completed);

        if (result.ok) {
            cout << "\n" << result.message << "\n";
            return;
        }

        cout << result.message << "\n";
    }
}

// Menu option 18: put a request at the position you want.
void changeRequestPosition(MaintenanceQueue& queue) {
    if (queue.isEmpty()) {
        cout << "\nMaintenance queue is empty.\n";
        return;
    }

    queue.display();

    int number = readInt("\nEnter request number to move (or 0 to reset the whole order): ");
    if (inputEnded())
        return;

    if (number == 0) {
        cout << "\n" << resetManualOrder(queue).message << "\n";
        return;
    }

    const MaintenanceRequest* request = queue.findRequest(number);
    if (request == nullptr) {
        cout << "Maintenance request not found.\n";
        return;
    }

    int highest = queue.maxPosition(number);
    cout << "Pinned requests stay at the top in the order you choose.\n";
    int position = readInt("Position among the pinned requests (1 to " + to_string(highest) +
                           "), or 0 to put it back in the normal order: ");
    if (inputEnded())
        return;

    ActionResult result = position == 0 ? unpinRequest(queue, number)
                                        : moveRequestToPosition(queue, number, position);
    cout << "\n" << result.message << "\n";
}

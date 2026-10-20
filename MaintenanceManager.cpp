// MaintenanceManager.cpp
// Author: Rajat Rathore

#include <fstream>
#include <sstream>
#include "MaintenanceManager.h"

const string MAINTENANCE_FILE = "maintenance.txt";

void saveMaintenance(const MaintenanceQueue& queue) {
    // Since queue nodes are private, this function uses a temporary copy.
    // Dequeueing from the real queue would change the user's data.
    // Therefore the queue class provides no direct file traversal.
    // For simplicity, maintenance data is saved when requests are added
    // and processed using a separate append/rebuild approach in main.
    //
    // This function is intentionally unused.
    (void)queue;
}

void loadMaintenance(MaintenanceQueue& queue) {
    ifstream file(MAINTENANCE_FILE);

    if (!file)
        return;

    string line;

    while (getline(file, line)) {
        if (line.empty())
            continue;

        stringstream ss(line);

        string idStr, date, description;

        getline(ss, idStr, '|');
        getline(ss, date, '|');
        getline(ss, description, '|');

        if (idStr.empty())
            continue;

        MaintenanceRequest request;
        request.equipmentId = stoi(idStr);
        request.date = date;
        request.description = description;

        queue.enqueue(request);
    }

    file.close();
}

void saveMaintenanceQueue(MaintenanceQueue& queue) {
    // Preserve the queue while writing it by moving elements temporarily.
    ofstream file(MAINTENANCE_FILE);

    if (!file) {
        cout << "Error opening maintenance.txt\n";
        return;
    }

    MaintenanceQueue temp;
    MaintenanceRequest request;

    while (queue.dequeue(request)) {
        file << request.equipmentId << "|"
             << request.date << "|"
             << request.description << "\n";

        temp.enqueue(request);
    }

    while (temp.dequeue(request))
        queue.enqueue(request);

    file.close();
}

// -------------------- SCHEDULE (menu option 9) --------------------
void scheduleMaintenance(const EquipmentList& equipmentList,
                         MaintenanceQueue& maintenanceQueue) {
    int id;
    string date, description;

    cout << "\nEnter equipment ID: ";
    cin >> id;

    if (equipmentList.findById(id) == nullptr) {
        cout << "Equipment not found.\n";
        return;
    }

    cout << "Enter maintenance date (YYYY-MM-DD): ";
    cin >> date;

    cout << "Enter maintenance description: ";
    getline(cin >> ws, description);

    MaintenanceRequest request;
    request.equipmentId = id;
    request.date = date;
    request.description = description;

    maintenanceQueue.enqueue(request);
    saveMaintenanceQueue(maintenanceQueue);

    cout << "\nMaintenance scheduled successfully.\n";
}

// -------------------- PROCESS (menu option 11) --------------------
void processNextRequest(MaintenanceQueue& maintenanceQueue) {
    MaintenanceRequest request;

    if (maintenanceQueue.dequeue(request)) {
        saveMaintenanceQueue(maintenanceQueue);

        cout << "\nNext maintenance request processed:\n";
        cout << "Equipment ID: " << request.equipmentId << endl;
        cout << "Date: " << request.date << endl;
        cout << "Description: " << request.description << endl;
    }
    else {
        cout << "\nMaintenance queue is empty.\n";
    }
}

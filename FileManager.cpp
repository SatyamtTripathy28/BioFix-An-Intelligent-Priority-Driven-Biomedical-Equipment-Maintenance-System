// FileManager.cpp
// Author: Satyam Tripathy

#include <fstream>
#include <sstream>
#include "FileManager.h"

const string MASTER_FILE = "equipment.txt";

string getDepartmentFile(string department) {
    if (department == "Radiology")
        return "radiology.txt";
    if (department == "ICU")
        return "icu.txt";
    if (department == "Cardiology")
        return "cardiology.txt";
    if (department == "Operation Theatre")
        return "operation_theatre.txt";
    if (department == "Laboratory")
        return "laboratory.txt";
    if (department == "Emergency")
        return "emergency.txt";

    return "";
}

void saveFiles(const EquipmentList& list, const MaintenanceQueue& queue) {
    // Master file
    ofstream master(MASTER_FILE);

    if (!master) {
        cout << "Error opening equipment.txt\n";
        return;
    }

    Node* current = list.getHead();

    while (current != nullptr) {
        Equipment* e = current->data;

        master << e->getId() << "|"
               << e->getName() << "|"
               << e->getDepartment() << "|"
               << e->getType() << "|"
               << e->getStatus() << "\n";

        current = current->next;
    }

    master.close();

    // Department files are synchronized copies of the master data.
    string departments[] = {
        "Radiology",
        "ICU",
        "Cardiology",
        "Operation Theatre",
        "Laboratory",
        "Emergency"
    };

    for (string department : departments) {
        ofstream file(getDepartmentFile(department));

        current = list.getHead();

        while (current != nullptr) {
            Equipment* e = current->data;

            if (e->getDepartment() == department) {
                file << e->getId() << "|"
                     << e->getName() << "|"
                     << e->getDepartment() << "|"
                     << e->getType() << "|"
                     << e->getStatus() << "\n";
            }

            current = current->next;
        }

        file.close();
    }

    // maintenance.txt is handled separately by saveMaintenanceQueue().
    // Keeping it separate prevents equipment updates from erasing requests.
}

void loadEquipment(EquipmentList& list) {
    ifstream file(MASTER_FILE);

    if (!file)
        return;

    string line;

    while (getline(file, line)) {
        if (line.empty())
            continue;

        stringstream ss(line);

        string idStr, name, department, type, status;

        getline(ss, idStr, '|');
        getline(ss, name, '|');
        getline(ss, department, '|');
        getline(ss, type, '|');
        getline(ss, status, '|');

        if (idStr.empty())
            continue;

        int id = stoi(idStr);

        list.add(createEquipment(id, name, department, type, status));
    }

    file.close();
}

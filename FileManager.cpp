// FileManager.cpp
// Author: Satyam Tripathy

#include <fstream>
#include <sstream>
#include "FileManager.h"
#include "InputHelper.h"

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

// One line of a data file: id|name|department|type|status
static void writeRecord(ofstream& file, const Equipment* e) {
    file << e->getId() << "|"
         << e->getName() << "|"
         << e->getDepartment() << "|"
         << e->getType() << "|"
         << e->getStatus() << "\n";
}

void saveFiles(const EquipmentList& list) {
    // Master file
    ofstream master(MASTER_FILE);

    if (!master) {
        cout << "Error opening equipment.txt\n";
        return;
    }

    for (Node* current = list.getHead(); current != nullptr; current = current->next)
        writeRecord(master, current->data);

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

        for (Node* current = list.getHead(); current != nullptr; current = current->next)
            if (current->data->getDepartment() == department)
                writeRecord(file, current->data);

        file.close();
    }
}

void loadEquipment(EquipmentList& list) {
    ifstream file(MASTER_FILE);

    if (!file)
        return;

    string line;
    int lineNumber = 0;

    while (getline(file, line)) {
        lineNumber++;

        if (!line.empty() && line.back() == '\r')   // file saved on Windows
            line.pop_back();

        if (line.empty())
            continue;

        stringstream ss(line);
        string idStr, name, department, type, status;

        getline(ss, idStr, '|');
        getline(ss, name, '|');
        getline(ss, department, '|');
        getline(ss, type, '|');
        getline(ss, status, '|');

        // A damaged line is skipped instead of crashing the program.
        if (!isNumber(idStr) || idStr.size() > 9) {
            cout << "Skipping line " << lineNumber << " of equipment.txt: bad ID.\n";
            continue;
        }

        int id = stoi(idStr);

        if (list.findById(id) != nullptr) {
            cout << "Skipping line " << lineNumber << " of equipment.txt: duplicate ID " << id << ".\n";
            continue;
        }

        Equipment* e = createEquipment(id, name, department, type, status);

        if (e == nullptr) {
            cout << "Skipping line " << lineNumber << " of equipment.txt: unknown type \"" << type << "\".\n";
            continue;
        }

        list.add(e);
    }

    file.close();
}

// EquipmentMenu.cpp
// Author: Satyam Tripathy

#include "EquipmentMenu.h"
#include "FileManager.h"

void displayDepartment(const EquipmentList& list, string department) {
    cout << "\n========== " << department << " EQUIPMENT ==========\n";

    bool found = false;

    Node* current = list.getHead();

    cout << "ID\tName\t\tType\t\tStatus\n";
    cout << "------------------------------------------------------------\n";

    while (current != nullptr) {
        Equipment* e = current->data;

        if (e->getDepartment() == department) {
            e->display();
            found = true;
        }

        current = current->next;
    }

    if (!found)
        cout << "No equipment found in this department.\n";
}

// -------------------- ADD --------------------
void addEquipment(EquipmentList& list, MaintenanceQueue& queue) {
    int id;
    string name, department, type, status;

    cout << "\nEnter equipment ID: ";
    cin >> id;

    if (list.findById(id) != nullptr) {
        cout << "ID already exists.\n";
        return;
    }

    cout << "Enter equipment name: ";
    getline(cin >> ws, name);

    cout << "\nSelect Department:\n";
    cout << "1. Radiology\n";
    cout << "2. ICU\n";
    cout << "3. Cardiology\n";
    cout << "4. Operation Theatre\n";
    cout << "5. Laboratory\n";
    cout << "6. Emergency\n";

    int d;
    cout << "Enter choice: ";
    cin >> d;

    switch (d) {
        case 1: department = "Radiology"; break;
        case 2: department = "ICU"; break;
        case 3: department = "Cardiology"; break;
        case 4: department = "Operation Theatre"; break;
        case 5: department = "Laboratory"; break;
        case 6: department = "Emergency"; break;
        default:
            cout << "Invalid department.\n";
            return;
    }

    cout << "\nSelect Equipment Type:\n";
    cout << "1. Life Support\n";
    cout << "2. Imaging\n";
    cout << "3. Diagnostic\n";

    int t;
    cout << "Enter choice: ";
    cin >> t;

    switch (t) {
        case 1: type = "Life Support"; break;
        case 2: type = "Imaging"; break;
        case 3: type = "Diagnostic"; break;
        default:
            cout << "Invalid type.\n";
            return;
    }

    cout << "\nSelect Status:\n";
    cout << "1. Working\n";
    cout << "2. Under Maintenance\n";
    cout << "3. Unavailable\n";

    int s;
    cout << "Enter choice: ";
    cin >> s;

    switch (s) {
        case 1: status = "Working"; break;
        case 2: status = "Under Maintenance"; break;
        case 3: status = "Unavailable"; break;
        default:
            cout << "Invalid status.\n";
            return;
    }

    list.add(createEquipment(id, name, department, type, status));

    saveFiles(list, queue);

    cout << "\nEquipment added successfully.\n";
}

// -------------------- UPDATE --------------------
void updateEquipment(EquipmentList& list, MaintenanceQueue& queue) {
    int id;

    cout << "\nEnter equipment ID to update: ";
    cin >> id;

    Equipment* e = list.findById(id);

    if (e == nullptr) {
        cout << "Equipment not found.\n";
        return;
    }

    string name, status;

    cout << "Enter new name: ";
    getline(cin >> ws, name);

    cout << "\nSelect New Status:\n";
    cout << "1. Working\n";
    cout << "2. Under Maintenance\n";
    cout << "3. Unavailable\n";

    int choice;
    cout << "Enter choice: ";
    cin >> choice;

    switch (choice) {
        case 1: status = "Working"; break;
        case 2: status = "Under Maintenance"; break;
        case 3: status = "Unavailable"; break;
        default:
            cout << "Invalid status.\n";
            return;
    }

    e->setName(name);
    e->setStatus(status);

    saveFiles(list, queue);

    cout << "\nEquipment updated successfully.\n";
}

// -------------------- DELETE --------------------
void deleteEquipment(EquipmentList& list, MaintenanceQueue& queue) {
    int id;

    cout << "\nEnter equipment ID to delete: ";
    cin >> id;

    if (list.removeById(id)) {
        saveFiles(list, queue);
        cout << "\nEquipment deleted successfully.\n";
    }
    else {
        cout << "\nEquipment not found.\n";
    }
}

// -------------------- DEPARTMENT MENU --------------------
void departmentMenu(const EquipmentList& list) {
    int choice;

    do {
        cout << "\n========== DEPARTMENT VIEW ==========\n";
        cout << "1. Radiology\n";
        cout << "2. ICU\n";
        cout << "3. Cardiology\n";
        cout << "4. Operation Theatre\n";
        cout << "5. Laboratory\n";
        cout << "6. Emergency\n";
        cout << "0. Back\n";
        cout << "Enter choice: ";
        cin >> choice;

        switch (choice) {
            case 1: displayDepartment(list, "Radiology"); break;
            case 2: displayDepartment(list, "ICU"); break;
            case 3: displayDepartment(list, "Cardiology"); break;
            case 4: displayDepartment(list, "Operation Theatre"); break;
            case 5: displayDepartment(list, "Laboratory"); break;
            case 6: displayDepartment(list, "Emergency"); break;
            case 0: break;
            default: cout << "Invalid choice.\n";
        }

    } while (choice != 0);
}

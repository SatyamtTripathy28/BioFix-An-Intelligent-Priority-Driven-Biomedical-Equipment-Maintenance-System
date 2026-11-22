// EquipmentMenu.cpp
// Author: Satyam Tripathy

#include "EquipmentMenu.h"
#include "FileManager.h"
#include "MaintenanceManager.h"
#include "InputHelper.h"

const string DEPARTMENTS[DEPARTMENT_COUNT] = {
    "Radiology", "ICU", "Cardiology", "Operation Theatre", "Laboratory", "Emergency"
};

bool isValidDepartment(const string& department) {
    for (int i = 0; i < DEPARTMENT_COUNT; i++)
        if (DEPARTMENTS[i] == department)
            return true;
    return false;
}

bool isValidType(const string& type) {
    return type == "Life Support" || type == "Imaging" || type == "Diagnostic";
}

bool isValidStatus(const string& status) {
    return status == "Working" || status == "Under Maintenance" || status == "Unavailable";
}

// ==================== LOGIC ====================
// These check everything themselves, because the website can send any values.

ActionResult addEquipmentRecord(EquipmentList& list, int id, string name,
                                const string& department, const string& type,
                                const string& status) {
    name = trim(name);

    if (id <= 0)
        return {false, "ID must be greater than 0."};
    if (list.findById(id) != nullptr)
        return {false, "ID already exists."};
    if (!isValidText(name))
        return {false, "Name must be 1 to 60 characters, without the | character."};
    if (!isValidDepartment(department))
        return {false, "Invalid department."};
    if (!isValidType(type))
        return {false, "Invalid type."};
    if (!isValidStatus(status))
        return {false, "Invalid status."};
    if (status == "Under Maintenance")
        return {false, "New equipment can't start Under Maintenance. Add it, then schedule a repair."};

    list.add(createEquipment(id, name, department, type, status));
    saveFiles(list);

    return {true, "Equipment added successfully."};
}

// The type cannot change, because the object's class (subclass) is fixed
// when it is created. Name, department and status can change.
//
// Status and the maintenance queue must always agree:
//  - "Under Maintenance" means a repair is waiting, so it can only be chosen
//    when the machine already has a request (schedule a repair to get one).
//  - Choosing Working or Unavailable cancels the machine's waiting requests.
ActionResult updateEquipmentRecord(EquipmentList& list, MaintenanceQueue& queue, int id,
                                   string name, const string& department,
                                   const string& status) {
    name = trim(name);
    Equipment* e = list.findById(id);

    if (e == nullptr)
        return {false, "Equipment not found."};
    if (!isValidText(name))
        return {false, "Name must be 1 to 60 characters, without the | character."};
    if (!isValidDepartment(department))
        return {false, "Invalid department."};
    if (!isValidStatus(status))
        return {false, "Invalid status."};

    bool hasRequests = queue.hasRequestFor(id);

    if (status == "Under Maintenance" && !hasRequests)
        return {false, "To mark it Under Maintenance, schedule a repair for it instead."};

    string message = "Equipment updated successfully.";

    if (status != "Under Maintenance" && hasRequests) {
        int cancelled = queue.removeRequestsFor(id);
        message += "\nCancelled " + to_string(cancelled) + " waiting maintenance request(s).";
        saveMaintenanceQueue(queue);
    }

    e->setName(name);
    e->setDepartment(department);
    e->setStatus(status);
    saveFiles(list);

    return {true, message};
}

// Also removes the equipment's waiting maintenance requests,
// so the queue never points to a machine that no longer exists.
ActionResult deleteEquipmentRecord(EquipmentList& list, MaintenanceQueue& queue, int id) {
    if (list.findById(id) == nullptr)
        return {false, "Equipment not found."};

    int removedRequests = queue.removeRequestsFor(id);
    list.removeById(id);

    saveFiles(list);
    saveMaintenanceQueue(queue);

    string message = "Equipment deleted successfully.";
    if (removedRequests > 0)
        message += "\nAlso removed " + to_string(removedRequests) +
                   " waiting maintenance request(s).";

    return {true, message};
}

// ==================== CLI SCREENS ====================
// Each helper asks until a valid number is chosen.
// If 'current' is not empty, 0 keeps the current value (used when updating).
// Returns "" only if the input has ended.

static string chooseDepartment(const string& current) {
    while (true) {
        cout << "\nSelect Department:\n";
        for (int i = 0; i < DEPARTMENT_COUNT; i++)
            cout << i + 1 << ". " << DEPARTMENTS[i] << "\n";
        if (!current.empty())
            cout << "0. Keep current (" << current << ")\n";

        int choice = readInt("Enter choice: ");
        if (inputEnded())
            return "";

        if (choice == 0 && !current.empty())
            return current;
        if (choice >= 1 && choice <= DEPARTMENT_COUNT)
            return DEPARTMENTS[choice - 1];

        cout << "Invalid choice. Try again.\n";
    }
}

static string chooseType() {
    while (true) {
        cout << "\nSelect Equipment Type:\n";
        cout << "1. Life Support\n";
        cout << "2. Imaging\n";
        cout << "3. Diagnostic\n";

        int choice = readInt("Enter choice: ");
        if (inputEnded())
            return "";

        if (choice == 1) return "Life Support";
        if (choice == 2) return "Imaging";
        if (choice == 3) return "Diagnostic";

        cout << "Invalid choice. Try again.\n";
    }
}

// For new equipment: Under Maintenance is not offered (schedule a repair instead).
static string chooseNewStatus() {
    while (true) {
        cout << "\nSelect Status:\n";
        cout << "1. Working\n";
        cout << "2. Unavailable\n";

        int choice = readInt("Enter choice: ");
        if (inputEnded())
            return "";

        if (choice == 1) return "Working";
        if (choice == 2) return "Unavailable";

        cout << "Invalid choice. Try again.\n";
    }
}

static string chooseStatus(const string& current) {
    while (true) {
        cout << "\nSelect Status:\n";
        cout << "1. Working\n";
        cout << "2. Under Maintenance\n";
        cout << "3. Unavailable\n";
        if (!current.empty())
            cout << "0. Keep current (" << current << ")\n";

        int choice = readInt("Enter choice: ");
        if (inputEnded())
            return "";

        if (choice == 0 && !current.empty()) return current;
        if (choice == 1) return "Working";
        if (choice == 2) return "Under Maintenance";
        if (choice == 3) return "Unavailable";

        cout << "Invalid choice. Try again.\n";
    }
}

void displayDepartment(const EquipmentList& list, string department) {
    cout << "\n========== " << department << " EQUIPMENT ==========\n";
    printEquipmentHeader();

    bool found = false;

    for (Node* current = list.getHead(); current != nullptr; current = current->next) {
        if (current->data->getDepartment() == department) {
            current->data->display();
            found = true;
        }
    }

    if (!found)
        cout << "No equipment found in this department.\n";
}

void addEquipment(EquipmentList& list) {
    int id = readInt("\nEnter equipment ID: ");
    if (inputEnded())
        return;

    // Check the ID first, so the user doesn't type everything for nothing.
    if (id <= 0) {
        cout << "ID must be greater than 0.\n";
        return;
    }
    if (list.findById(id) != nullptr) {
        cout << "ID already exists.\n";
        return;
    }

    string name = readText("Enter equipment name: ");
    if (name.empty()) return;

    string department = chooseDepartment("");
    if (department.empty()) return;

    string type = chooseType();
    if (type.empty()) return;

    string status = chooseNewStatus();
    if (status.empty()) return;

    ActionResult result = addEquipmentRecord(list, id, name, department, type, status);
    cout << "\n" << result.message << "\n";
}

void updateEquipment(EquipmentList& list, MaintenanceQueue& queue) {
    int id = readInt("\nEnter equipment ID to update: ");
    if (inputEnded())
        return;

    Equipment* e = list.findById(id);
    if (e == nullptr) {
        cout << "Equipment not found.\n";
        return;
    }

    string name = readText("Enter new name: ");
    if (name.empty()) return;

    string department = chooseDepartment(e->getDepartment());
    if (department.empty()) return;

    string status = chooseStatus(e->getStatus());
    if (status.empty()) return;

    // Leaving maintenance cancels waiting repairs, so ask first.
    int waiting = queue.countFor(id);
    if (status != "Under Maintenance" && waiting > 0) {
        cout << "This machine has " << waiting << " waiting maintenance request(s). "
             << "Changing the status will cancel them.\n";
        int answer = readInt("Type 1 to continue or 0 to stop: ");
        if (answer != 1) {
            cout << "\nNothing was changed.\n";
            return;
        }
    }

    ActionResult result = updateEquipmentRecord(list, queue, id, name, department, status);
    cout << "\n" << result.message << "\n";
}

void deleteEquipment(EquipmentList& list, MaintenanceQueue& queue) {
    int id = readInt("\nEnter equipment ID to delete: ");
    if (inputEnded())
        return;

    Equipment* e = list.findById(id);
    if (e == nullptr) {
        cout << "\nEquipment not found.\n";
        return;
    }

    // Deleting can't be undone, so ask first.
    int waiting = queue.countFor(id);
    cout << "Delete " << e->getName() << " (" << id << ")";
    if (waiting > 0)
        cout << " and its " << waiting << " waiting maintenance request(s)";
    cout << "?\n";

    if (readInt("Type 1 to delete or 0 to keep it: ") != 1) {
        cout << "\nNothing was deleted.\n";
        return;
    }

    ActionResult result = deleteEquipmentRecord(list, queue, id);
    cout << "\n" << result.message << "\n";
}

void departmentMenu(const EquipmentList& list) {
    int choice;

    do {
        cout << "\n========== DEPARTMENT VIEW ==========\n";
        for (int i = 0; i < DEPARTMENT_COUNT; i++)
            cout << i + 1 << ". " << DEPARTMENTS[i] << "\n";
        cout << "0. Back\n";

        choice = readInt("Enter choice: ");

        if (choice >= 1 && choice <= DEPARTMENT_COUNT)
            displayDepartment(list, DEPARTMENTS[choice - 1]);
        else if (choice != 0)
            cout << "Invalid choice.\n";

    } while (choice != 0);
}

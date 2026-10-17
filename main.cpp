// main.cpp
// Program entry point: loads the data files, shows the menu and calls the
// other modules.
// Author: Bhaskar Dev

#include <iostream>
#include <string>
#include "Equipment.h"
#include "EquipmentList.h"
#include "MaintenanceQueue.h"
#include "FileManager.h"
#include "EquipmentMenu.h"
#include "MaintenanceManager.h"
#include "SearchSort.h"
using namespace std;

/* =========================================================
   BIOFIX - Biomedical Equipment Maintenance Management
   Concepts used:
   OOP, Inheritance, Polymorphism, Linked List, Queue,
   Linear Search, Binary Search, Insertion Sort,
   Dynamic Memory Allocation and File Handling
   ========================================================= */

int main() {
    EquipmentList equipmentList;
    MaintenanceQueue maintenanceQueue;

    loadEquipment(equipmentList);
    loadMaintenance(maintenanceQueue);

    int choice;

    do {
        cout << "\n================================================\n";
        cout << "     BIOFIX - BIOMEDICAL EQUIPMENT SYSTEM\n";
        cout << "================================================\n";
        cout << "1. Add Equipment\n";
        cout << "2. Display All Equipment\n";
        cout << "3. View Department Equipment\n";
        cout << "4. Update Equipment\n";
        cout << "5. Delete Equipment\n";
        cout << "6. Linear Search\n";
        cout << "7. Binary Search by ID\n";
        cout << "8. Sort by Name (Insertion Sort)\n";
        cout << "9. Schedule Maintenance\n";
        cout << "10. View Maintenance Queue\n";
        cout << "11. Process Next Maintenance Request\n";
        cout << "0. Exit\n";
        cout << "Enter choice: ";
        cin >> choice;

        switch (choice) {
            case 1:
                addEquipment(equipmentList, maintenanceQueue);
                break;

            case 2:
                equipmentList.displayAll();
                break;

            case 3:
                departmentMenu(equipmentList);
                break;

            case 4:
                updateEquipment(equipmentList, maintenanceQueue);
                break;

            case 5:
                deleteEquipment(equipmentList, maintenanceQueue);
                break;

            case 6:
                linearSearch(equipmentList);
                break;

            case 7:
                binarySearchById(equipmentList);
                break;

            case 8:
                sortByName(equipmentList);
                break;

            case 9:
                scheduleMaintenance(equipmentList, maintenanceQueue);
                break;

            case 10:
                maintenanceQueue.display();
                break;

            case 11:
                processNextRequest(maintenanceQueue);
                break;

            case 0:
                cout << "\nThank you for using BioFix.\n";
                break;

            default:
                cout << "\nInvalid choice.\n";
        }

    } while (choice != 0);

    // Final save
    saveFiles(equipmentList, maintenanceQueue);
    saveMaintenanceQueue(maintenanceQueue);

    return 0;
}

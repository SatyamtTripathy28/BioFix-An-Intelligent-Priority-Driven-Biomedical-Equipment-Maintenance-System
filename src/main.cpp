// main.cpp
// Program entry point: loads the data files, shows the menu and calls the
// other modules.
// Author: Bhaskar Dev

#include <iostream>
#include <string>
#include "Equipment.h"
#include "EquipmentList.h"
#include "MaintenanceQueue.h"
#include "RepairHistory.h"
#include "FileManager.h"
#include "EquipmentMenu.h"
#include "MaintenanceManager.h"
#include "HistoryManager.h"
#include "SearchSort.h"
#include "InputHelper.h"

using namespace std;

/* =========================================================
   BIOFIX - Biomedical Equipment Maintenance Management

   Concepts used:
   OOP: classes, encapsulation, abstract class, inheritance,
        runtime polymorphism (getCategory, getPriority)
   Data structures: linked list (with tail pointer), hash table
        (separate chaining), priority queue (binary min-heap with aging),
        stack (repair history)
   Algorithms: hashing, keyword search, insertion sort, heap sort
   Also: dynamic memory and file handling
   ========================================================= */

int main() {
    EquipmentList equipmentList;
    MaintenanceQueue maintenanceQueue;
    RepairHistory repairHistory;

    loadEquipment(equipmentList);
    loadMaintenance(maintenanceQueue, equipmentList);   // needs the equipment for priorities
    loadHistory(repairHistory);

    int choice;

    do {
        // Aging depends on today's date, so update it before each choice.
        maintenanceQueue.setToday(todayDate());

        cout << "\n================================================\n";
        cout << "     BIOFIX - BIOMEDICAL EQUIPMENT SYSTEM\n";
        cout << "================================================\n";
        cout << "1. Add Equipment\n";
        cout << "2. Display All Equipment\n";
        cout << "3. View Department Equipment\n";
        cout << "4. Update Equipment\n";
        cout << "5. Delete Equipment\n";
        cout << "6. Search (ID, name, department, type, status)\n";
        cout << "7. Find by ID (hash table)\n";
        cout << "8. Sort by Name (Insertion Sort)\n";
        cout << "9. Schedule Maintenance\n";
        cout << "10. View Maintenance Queue (most urgent first)\n";
        cout << "11. Process Next Maintenance Request\n";
        cout << "12. View Repair History of a Machine\n";
        cout << "13. Downtime Ranking\n";
        cout << "14. Edit a Maintenance Request\n";
        cout << "15. Cancel a Maintenance Request\n";
        cout << "16. Change a Request's Priority\n";
        cout << "17. Complete a Specific Request\n";
        cout << "18. Change a Request's Position (manual order)\n";
        cout << "0. Exit\n";

        choice = readInt("Enter choice: ");   // returns 0 when input ends

        switch (choice) {
            case 1:  addEquipment(equipmentList); break;
            case 2:  equipmentList.displayAll(); break;
            case 3:  departmentMenu(equipmentList); break;
            case 4:  updateEquipment(equipmentList, maintenanceQueue); break;
            case 5:  deleteEquipment(equipmentList, maintenanceQueue); break;
            case 6:  searchEquipment(equipmentList); break;
            case 7:  findEquipmentById(equipmentList); break;
            case 8:  sortByName(equipmentList); break;
            case 9:  scheduleMaintenance(equipmentList, maintenanceQueue); break;
            case 10: maintenanceQueue.display(); break;
            case 11: processNextRequest(equipmentList, maintenanceQueue, repairHistory); break;
            case 12: showHistory(equipmentList, repairHistory); break;
            case 13: downtimeRanking(equipmentList, repairHistory); break;
            case 14: editMaintenanceRequest(maintenanceQueue); break;
            case 15: cancelMaintenanceRequest(equipmentList, maintenanceQueue); break;
            case 16: changeRequestPriority(equipmentList, maintenanceQueue); break;
            case 17: completeSpecificRequest(equipmentList, maintenanceQueue, repairHistory); break;
            case 18: changeRequestPosition(maintenanceQueue); break;
            case 0:  cout << "\nThank you for using BioFix.\n"; break;
            default: cout << "\nInvalid choice.\n";
        }

    } while (choice != 0);

    // Final save
    saveFiles(equipmentList);
    saveMaintenanceQueue(maintenanceQueue);
    saveHistory(repairHistory);

    return 0;
}

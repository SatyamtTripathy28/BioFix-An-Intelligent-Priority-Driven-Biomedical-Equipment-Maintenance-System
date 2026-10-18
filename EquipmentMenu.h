// EquipmentMenu.h
// Screens for adding, updating, deleting and viewing equipment by department.
// Author: Satyam Tripathy

#ifndef BIOFIX_EQUIPMENTMENU_H
#define BIOFIX_EQUIPMENTMENU_H

#include "EquipmentList.h"

class MaintenanceQueue;   // only passed through to saveFiles()

void displayDepartment(const EquipmentList& list, string department);
void addEquipment(EquipmentList& list, MaintenanceQueue& queue);
void updateEquipment(EquipmentList& list, MaintenanceQueue& queue);
void deleteEquipment(EquipmentList& list, MaintenanceQueue& queue);
void departmentMenu(const EquipmentList& list);

#endif

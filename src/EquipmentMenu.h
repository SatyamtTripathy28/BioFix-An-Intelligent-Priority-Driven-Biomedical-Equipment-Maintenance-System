// EquipmentMenu.h
// Equipment logic (add, update, delete) and the CLI screens that use it.
// The logic functions have no cin/cout, so the website can use them too.
// Author: Satyam Tripathy

#ifndef BIOFIX_EQUIPMENTMENU_H
#define BIOFIX_EQUIPMENTMENU_H

#include "EquipmentList.h"
#include "MaintenanceQueue.h"
#include "ActionResult.h"

// -------------------- CHOICES --------------------
const int DEPARTMENT_COUNT = 6;
extern const string DEPARTMENTS[DEPARTMENT_COUNT];

bool isValidDepartment(const string& department);
bool isValidType(const string& type);
bool isValidStatus(const string& status);

// -------------------- LOGIC (no cin/cout) --------------------
ActionResult addEquipmentRecord(EquipmentList& list, int id, string name,
                                const string& department, const string& type,
                                const string& status);

// Keeps status and queue in agreement (see EquipmentMenu.cpp).
ActionResult updateEquipmentRecord(EquipmentList& list, MaintenanceQueue& queue, int id,
                                   string name, const string& department,
                                   const string& status);

ActionResult deleteEquipmentRecord(EquipmentList& list, MaintenanceQueue& queue, int id);

// -------------------- CLI SCREENS --------------------
void displayDepartment(const EquipmentList& list, string department);
void addEquipment(EquipmentList& list);
void updateEquipment(EquipmentList& list, MaintenanceQueue& queue);
void deleteEquipment(EquipmentList& list, MaintenanceQueue& queue);
void departmentMenu(const EquipmentList& list);

#endif

// FileManager.h
// Loading and saving of equipment.txt and the six department files.
// Author: Satyam Tripathy

#ifndef BIOFIX_FILEMANAGER_H
#define BIOFIX_FILEMANAGER_H

#include "EquipmentList.h"

// saveFiles() receives the queue but does not use it. The queue is saved
// separately by saveMaintenanceQueue() in MaintenanceManager.
class MaintenanceQueue;

string getDepartmentFile(string department);
void saveFiles(const EquipmentList& list, const MaintenanceQueue& queue);
void loadEquipment(EquipmentList& list);

#endif

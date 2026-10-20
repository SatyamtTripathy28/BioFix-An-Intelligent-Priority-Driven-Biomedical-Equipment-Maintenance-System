// MaintenanceManager.h
// Saving/loading maintenance.txt and the schedule and process options.
// Author: Rajat Rathore

#ifndef BIOFIX_MAINTENANCEMANAGER_H
#define BIOFIX_MAINTENANCEMANAGER_H

#include "EquipmentList.h"
#include "MaintenanceQueue.h"

void saveMaintenance(const MaintenanceQueue& queue);
void loadMaintenance(MaintenanceQueue& queue);
void saveMaintenanceQueue(MaintenanceQueue& queue);

// Menu option 9: schedule a maintenance request for an existing equipment ID.
void scheduleMaintenance(const EquipmentList& equipmentList,
                         MaintenanceQueue& maintenanceQueue);

// Menu option 11: take the next request out of the queue.
void processNextRequest(MaintenanceQueue& maintenanceQueue);

#endif

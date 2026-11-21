// MaintenanceManager.h
// Maintenance logic (schedule, complete) and the CLI screens that use it.
// The logic functions have no cin/cout, so the website can use them too.
// Author: Rajat Rathore

#ifndef BIOFIX_MAINTENANCEMANAGER_H
#define BIOFIX_MAINTENANCEMANAGER_H

#include "EquipmentList.h"
#include "MaintenanceQueue.h"
#include "RepairHistory.h"
#include "ActionResult.h"

// Needs the list to give each request its equipment's priority.
void loadMaintenance(MaintenanceQueue& queue, const EquipmentList& list);
void saveMaintenanceQueue(const MaintenanceQueue& queue);

// -------------------- LOGIC (no cin/cout) --------------------
// Adds a request; the machine becomes "Under Maintenance".
// priority 0 = automatic (from the machine type); 1 to 3 = set by hand.
ActionResult scheduleRequest(EquipmentList& list, MaintenanceQueue& queue,
                             int id, const string& date, string description,
                             int priority = 0);

// Changes one request's priority (0 = back to automatic); the request moves in the queue.
ActionResult setRequestPriority(const EquipmentList& list, MaintenanceQueue& queue,
                                int requestId, int priority);

// Finishes one request (requestId 0 = the first in the queue) and records it in the history.
ActionResult completeRequest(EquipmentList& list, MaintenanceQueue& queue,
                             RepairHistory& history, int requestId, const string& completedDate);

// Finishes the first request in the queue (same as completeRequest with 0).
ActionResult completeNextRequest(EquipmentList& list, MaintenanceQueue& queue,
                                 RepairHistory& history, const string& completedDate);

// Changes the date and description of one waiting request.
ActionResult updateRequest(MaintenanceQueue& queue, int requestId,
                           const string& date, string description);

// Manual order: move a request to a position (1 = first), put it back, or reset all.
ActionResult moveRequestToPosition(MaintenanceQueue& queue, int requestId, int position);
ActionResult unpinRequest(MaintenanceQueue& queue, int requestId);
ActionResult resetManualOrder(MaintenanceQueue& queue);

// Removes one waiting request; the machine is Working again if nothing else waits.
ActionResult cancelRequest(EquipmentList& list, MaintenanceQueue& queue, int requestId);

// Notifications: requests whose date has passed or is within 'withinDays' days of today.
// Fills a new array sorted by date (earliest first; same date: most urgent first)
// using insertion sort, and returns how many. The caller must delete[] the array.
int dueRequests(const MaintenanceQueue& queue, const string& today, int withinDays,
                MaintenanceRequest*& due);

// -------------------- CLI SCREENS --------------------
void scheduleMaintenance(EquipmentList& list, MaintenanceQueue& queue);           // menu 9
void processNextRequest(EquipmentList& list, MaintenanceQueue& queue,
                        RepairHistory& history);                                 // menu 11
void editMaintenanceRequest(MaintenanceQueue& queue);                                // menu 14
void cancelMaintenanceRequest(EquipmentList& list, MaintenanceQueue& queue);         // menu 15
void changeRequestPriority(const EquipmentList& list, MaintenanceQueue& queue);      // menu 16
void completeSpecificRequest(EquipmentList& list, MaintenanceQueue& queue,
                             RepairHistory& history);                                // menu 17
void changeRequestPosition(MaintenanceQueue& queue);                                 // menu 18

#endif

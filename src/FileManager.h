// FileManager.h
// Loading and saving of equipment.txt and the six department files.
// Author: Satyam Tripathy

#ifndef BIOFIX_FILEMANAGER_H
#define BIOFIX_FILEMANAGER_H

#include "EquipmentList.h"

string getDepartmentFile(string department);
void saveFiles(const EquipmentList& list);
void loadEquipment(EquipmentList& list);

#endif

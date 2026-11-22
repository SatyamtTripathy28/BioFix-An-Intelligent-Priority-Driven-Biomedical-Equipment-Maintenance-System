// SearchSort.h
// Keyword search, fast ID lookup (hash table) and insertion sort by name.
// Author: Keshav Aggarwal

#ifndef BIOFIX_SEARCHSORT_H
#define BIOFIX_SEARCHSORT_H

#include "EquipmentList.h"

// Logic (no cin/cout): does this machine match the keyword?
// ID must match exactly; other fields match part of a word, ignoring capitals.
bool matchesKeyword(const Equipment* e, const string& keyword);

// Menu option 6: one search box for ID, name, department, type or status.
void searchEquipment(const EquipmentList& list);

// Menu option 7: exact ID lookup using the hash table inside EquipmentList.
void findEquipmentById(const EquipmentList& list);

void insertionSortByName(Equipment** arr, int n);
void sortByName(const EquipmentList& list);

#endif

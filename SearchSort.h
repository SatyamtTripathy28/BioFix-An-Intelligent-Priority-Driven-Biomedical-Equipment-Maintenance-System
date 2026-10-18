// SearchSort.h
// Linear search, binary search and insertion sort.
// Author: Keshav Aggarwal

#ifndef BIOFIX_SEARCHSORT_H
#define BIOFIX_SEARCHSORT_H

#include "EquipmentList.h"

void linearSearch(const EquipmentList& list);
void insertionSortById(Equipment** arr, int n);
void insertionSortByName(Equipment** arr, int n);
void binarySearchById(const EquipmentList& list);
void sortByName(const EquipmentList& list);

#endif

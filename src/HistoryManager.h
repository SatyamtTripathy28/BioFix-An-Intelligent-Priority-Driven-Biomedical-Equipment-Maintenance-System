// HistoryManager.h
// Saving/loading history.txt, the history screen and the downtime ranking.
// Module owner: Bhaskar Dev

#ifndef BIOFIX_HISTORYMANAGER_H
#define BIOFIX_HISTORYMANAGER_H

#include "EquipmentList.h"
#include "RepairHistory.h"

void saveHistory(const RepairHistory& history);
void loadHistory(RepairHistory& history);

// Menu option 12: all finished repairs of one machine, newest first.
void showHistory(const EquipmentList& list, const RepairHistory& history);

// One row of the downtime ranking.
struct RankEntry {
    Equipment* equipment;
    int downtime;      // total days out of service
    int repairs;       // number of finished repairs
};

// Logic (no cin/cout): fills a new array with every repaired machine,
// sorted with insertion sort, most downtime first. Returns how many rows.
// The caller must delete[] the array.
int computeRanking(const EquipmentList& list, const RepairHistory& history,
                   RankEntry*& ranking);

// Menu option 13: machines ranked by total downtime (most first).
void downtimeRanking(const EquipmentList& list, const RepairHistory& history);

#endif

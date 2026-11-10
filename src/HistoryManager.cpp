// HistoryManager.cpp
// Module owner: Bhaskar Dev

#include <fstream>
#include <iomanip>
#include <sstream>
#include "HistoryManager.h"
#include "InputHelper.h"

const string HISTORY_FILE = "history.txt";

// File line: id|requestDate|completedDate|downtimeDays|description
// Saved from the top of the stack, so the file is newest first.
void saveHistory(const RepairHistory& history) {
    ofstream file(HISTORY_FILE);

    if (!file) {
        cout << "Error opening history.txt\n";
        return;
    }

    for (HistoryNode* current = history.getTop(); current != nullptr; current = current->next) {
        const RepairRecord& r = current->data;
        file << r.equipmentId << "|" << r.requestDate << "|" << r.completedDate << "|"
             << r.downtimeDays << "|" << r.description << "\n";
    }

    file.close();
}

void loadHistory(RepairHistory& history) {
    ifstream file(HISTORY_FILE);

    if (!file)
        return;

    string line;

    while (getline(file, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        if (line.empty())
            continue;

        stringstream ss(line);
        string idStr, requestDate, completedDate, daysStr, description;

        getline(ss, idStr, '|');
        getline(ss, requestDate, '|');
        getline(ss, completedDate, '|');
        getline(ss, daysStr, '|');
        getline(ss, description, '|');

        if (!isNumber(idStr) || idStr.size() > 9 || !isNumber(daysStr) || daysStr.size() > 9) {
            cout << "Skipping a damaged line in history.txt.\n";
            continue;
        }

        RepairRecord record;
        record.equipmentId = stoi(idStr);
        record.requestDate = requestDate;
        record.completedDate = completedDate;
        record.downtimeDays = stoi(daysStr);
        record.description = description;

        history.addOldest(record);   // file is newest first, so each line goes below
    }

    file.close();
}

// -------------------- HISTORY (menu option 12) --------------------
void showHistory(const EquipmentList& list, const RepairHistory& history) {
    int id = readInt("\nEnter equipment ID: ");
    if (inputEnded())
        return;

    Equipment* e = list.findById(id);
    if (e == nullptr) {
        cout << "Equipment not found.\n";
        return;
    }

    cout << "\n========== REPAIR HISTORY: " << e->getName() << " (" << id << ") ==========\n";
    cout << left << setw(13) << "Requested" << setw(13) << "Completed" << setw(6) << "Days" << "Description\n";
    cout << string(70, '-') << "\n";

    int repairs = 0;
    int totalDays = 0;

    // Walk from the top of the stack: newest repair first.
    for (HistoryNode* current = history.getTop(); current != nullptr; current = current->next) {
        const RepairRecord& r = current->data;

        if (r.equipmentId == id) {
            cout << left << setw(13) << r.requestDate << setw(13) << r.completedDate
                 << setw(6) << r.downtimeDays << r.description << "\n";
            repairs++;
            totalDays += r.downtimeDays;
        }
    }

    if (repairs == 0)
        cout << "No finished repairs yet.\n";
    else
        cout << repairs << " repair(s), total downtime " << totalDays << " day(s).\n";
}

// -------------------- DOWNTIME RANKING --------------------
// Insertion sort, most downtime first. Ties: more repairs first, then lower ID.
static bool comesBefore(const RankEntry& a, const RankEntry& b) {
    if (a.downtime != b.downtime) return a.downtime > b.downtime;
    if (a.repairs != b.repairs) return a.repairs > b.repairs;
    return a.equipment->getId() < b.equipment->getId();
}

static void insertionSortRanking(RankEntry* arr, int n) {
    for (int i = 1; i < n; i++) {
        RankEntry key = arr[i];
        int j = i - 1;

        while (j >= 0 && comesBefore(key, arr[j])) {
            arr[j + 1] = arr[j];
            j--;
        }

        arr[j + 1] = key;
    }
}

int computeRanking(const EquipmentList& list, const RepairHistory& history,
                   RankEntry*& ranking) {
    int n = list.count();
    ranking = new RankEntry[n > 0 ? n : 1];
    int used = 0;

    // Add up downtime and repairs for every machine that has been repaired.
    for (Node* node = list.getHead(); node != nullptr; node = node->next) {
        int total = 0, repairs = 0;

        for (HistoryNode* h = history.getTop(); h != nullptr; h = h->next) {
            if (h->data.equipmentId == node->data->getId()) {
                total += h->data.downtimeDays;
                repairs++;
            }
        }

        if (repairs > 0)
            ranking[used++] = {node->data, total, repairs};
    }

    insertionSortRanking(ranking, used);
    return used;
}

// Menu option 13
void downtimeRanking(const EquipmentList& list, const RepairHistory& history) {
    RankEntry* ranking = nullptr;
    int used = computeRanking(list, history, ranking);

    if (used == 0) {
        cout << "\nNo finished repairs yet, so there is nothing to rank.\n";
        delete[] ranking;
        return;
    }

    cout << "\n========== DOWNTIME RANKING ==========\n";
    cout << left << setw(6) << "Rank" << setw(6) << "ID" << setw(7) << "Days"
         << setw(9) << "Repairs" << "Name\n";
    cout << string(60, '-') << "\n";

    for (int i = 0; i < used; i++) {
        cout << left << setw(6) << i + 1 << setw(6) << ranking[i].equipment->getId()
             << setw(7) << ranking[i].downtime << setw(9) << ranking[i].repairs
             << ranking[i].equipment->getName() << "\n";
    }

    delete[] ranking;
}

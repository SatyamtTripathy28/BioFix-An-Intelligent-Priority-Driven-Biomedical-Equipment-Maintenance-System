// RepairHistory.h
// Repair history kept as a stack (last in, first out):
// every finished repair is pushed on top, so the newest repair is seen first.
// Module owner: Bhaskar Dev

#ifndef BIOFIX_REPAIRHISTORY_H
#define BIOFIX_REPAIRHISTORY_H

#include <string>
using namespace std;

struct RepairRecord {
    int equipmentId;
    string requestDate;
    string completedDate;
    int downtimeDays;      // days from request to completion
    string description;
};

struct HistoryNode {
    RepairRecord data;
    HistoryNode* next;
};

class RepairHistory {
private:
    HistoryNode* top;      // newest repair
    HistoryNode* bottom;   // oldest repair (used only while loading the file)

public:
    RepairHistory() {
        top = nullptr;
        bottom = nullptr;
    }

    ~RepairHistory() {
        clear();
    }

    RepairHistory(const RepairHistory&) = delete;
    RepairHistory& operator=(const RepairHistory&) = delete;

    void clear() {
        while (top != nullptr) {
            HistoryNode* temp = top;
            top = top->next;
            delete temp;
        }
        bottom = nullptr;
    }

    // Stack push: the new record goes on top. O(1)
    void push(RepairRecord record) {
        HistoryNode* node = new HistoryNode{record, top};
        top = node;

        if (bottom == nullptr)
            bottom = node;
    }

    // Used only when loading history.txt, which is saved newest first,
    // so each older line goes under the ones already loaded. O(1)
    void addOldest(RepairRecord record) {
        HistoryNode* node = new HistoryNode{record, nullptr};

        if (top == nullptr)
            top = bottom = node;
        else {
            bottom->next = node;
            bottom = node;
        }
    }

    HistoryNode* getTop() const {
        return top;
    }

    bool isEmpty() const {
        return top == nullptr;
    }
};

#endif

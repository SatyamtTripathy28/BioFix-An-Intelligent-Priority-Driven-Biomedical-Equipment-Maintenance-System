// bench/benchmark.cpp
// Measures two choices made in BioFix, with real numbers:
//   1. Finding equipment by ID:   hash table   vs  walking the linked list
//   2. The maintenance queue:     binary heap  vs  sorted linked list (v3)
// Build and run:  make bench     (or see README for the g++ command)
// The numbers depend on the computer; the growth pattern is what matters.

#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include "../src/EquipmentList.h"
#include "../src/MaintenanceQueue.h"

using namespace std;
using Clock = chrono::steady_clock;

static double millisecondsSince(Clock::time_point start) {
    return chrono::duration<double, milli>(Clock::now() - start).count();
}

// ---------------- the old way: walk the list until the ID matches ----------------
static Equipment* linearFind(const EquipmentList& list, int id) {
    for (Node* n = list.getHead(); n != nullptr; n = n->next)
        if (n->data->getId() == id)
            return n->data;
    return nullptr;
}

// ---------------- the v3 queue: a sorted linked list ----------------
struct SortedNode {
    MaintenanceRequest data;
    SortedNode* next;
};

static bool before(const MaintenanceRequest& a, const MaintenanceRequest& b) {
    if (a.priority != b.priority) return a.priority < b.priority;
    if (a.date != b.date) return a.date < b.date;
    return a.requestId < b.requestId;
}

class SortedListQueue {
    SortedNode* front = nullptr;
public:
    ~SortedListQueue() {
        while (front) { SortedNode* t = front; front = front->next; delete t; }
    }
    void enqueue(const MaintenanceRequest& r) {          // O(n): walk to the right place
        SortedNode* node = new SortedNode{r, nullptr};
        if (!front || before(r, front->data)) { node->next = front; front = node; return; }
        SortedNode* cur = front;
        while (cur->next && !before(r, cur->next->data)) cur = cur->next;
        node->next = cur->next;
        cur->next = node;
    }
    bool dequeue(MaintenanceRequest& r) {                // O(1): take the front
        if (!front) return false;
        SortedNode* t = front; r = t->data; front = t->next; delete t; return true;
    }
};

int main() {
    mt19937 random(42);                                  // fixed seed: same numbers every run
    int sizes[] = {1000, 5000, 20000};

    cout << "BioFix benchmark (times in milliseconds)\n\n";

    // ---------------- 1. find by ID ----------------
    cout << "1. Find every machine once by ID\n";
    cout << left << setw(10) << "Machines" << setw(18) << "Linked list" << setw(14) << "Hash table" << "Faster by\n";

    for (int n : sizes) {
        EquipmentList list;
        for (int i = 0; i < n; i++)
            list.add(createEquipment(100000 + i, "Machine " + to_string(i), "ICU", "Imaging", "Working"));

        vector<int> ids;
        for (int i = 0; i < n; i++)
            ids.push_back(100000 + (int)(random() % n));

        long found = 0;
        Clock::time_point t1 = Clock::now();
        for (int id : ids) found += linearFind(list, id) != nullptr;
        double linearMs = millisecondsSince(t1);

        Clock::time_point t2 = Clock::now();
        for (int id : ids) found += list.findById(id) != nullptr;
        double hashMs = millisecondsSince(t2);

        if (found != 2L * n) cout << "(error: some IDs were not found)\n";
        cout << left << setw(10) << n << setw(18) << fixed << setprecision(2) << linearMs
             << setw(14) << hashMs << setprecision(0) << linearMs / (hashMs > 0 ? hashMs : 0.001) << "x\n";
    }

    // ---------------- 2. priority queue ----------------
    cout << "\n2. Add every request, then take them all out in order\n";
    cout << left << setw(10) << "Requests" << setw(18) << "Sorted list" << setw(14) << "Heap" << "Faster by\n";

    for (int n : sizes) {
        vector<MaintenanceRequest> requests(n);
        for (int i = 0; i < n; i++) {
            requests[i].requestId = i + 1;
            requests[i].equipmentId = 100 + i;
            requests[i].priority = 1 + (int)(random() % 3);
            requests[i].effectivePriority = requests[i].priority;
            int day = 1 + (int)(random() % 28);
            requests[i].date = string("2026-10-") + (day < 10 ? "0" : "") + to_string(day);
            requests[i].description = "Benchmark";
        }

        MaintenanceRequest out;
        Clock::time_point t1 = Clock::now();
        {
            SortedListQueue listQueue;
            for (const MaintenanceRequest& r : requests) listQueue.enqueue(r);
            while (listQueue.dequeue(out)) {}
        }
        double listMs = millisecondsSince(t1);

        Clock::time_point t2 = Clock::now();
        {
            MaintenanceQueue heapQueue;
            for (const MaintenanceRequest& r : requests) heapQueue.enqueue(r);
            while (heapQueue.dequeue(out)) {}
        }
        double heapMs = millisecondsSince(t2);

        cout << left << setw(10) << n << setw(18) << fixed << setprecision(2) << listMs
             << setw(14) << heapMs << setprecision(0) << listMs / (heapMs > 0 ? heapMs : 0.001) << "x\n";
    }

    cout << "\nFor tens of machines both ways are instant; the difference grows with size.\n";
    return 0;
}

// tests/queue_fuzz.cpp
// Random test for the priority queue (the heap in src/MaintenanceQueue.h).
//
// Idea: do thousands of random operations (add, take the first, change a priority,
// change a date, remove one, remove all of a machine, start a new day,
// move a request to a position / unpin it / reset the manual order) on the heap
// AND on a plain sorted list that is obviously correct. After every step, both
// must give the same queue order and the same answers.
//
// Run:  make fuzz        (prints "mismatches: 0" when everything is right)
//
// This test found a real bug in removeRequestsFor() that the normal tests missed,
// so keep it and run it after every change to MaintenanceQueue.h.

#include <algorithm>
#include <random>
#include <vector>
#include "../src/MaintenanceQueue.h"

static string dateFor(int day) {
    return string("2026-10-") + (day < 10 ? "0" : "") + to_string(day);
}

// The order we expect: pinned requests first (by their position), then
// effective priority, then date, then request number.
static bool expectedBefore(const MaintenanceRequest& a, const MaintenanceRequest& b) {
    bool aPinned = a.manualRank > 0, bPinned = b.manualRank > 0;
    if (aPinned != bPinned) return aPinned;
    if (aPinned && a.manualRank != b.manualRank) return a.manualRank < b.manualRank;
    if (a.effectivePriority != b.effectivePriority) return a.effectivePriority < b.effectivePriority;
    if (a.date != b.date) return a.date < b.date;
    return a.requestId < b.requestId;
}

int main() {
    const int RUNS = 2000;       // each run starts from an empty queue with its own random numbers
    const int STEPS = 60;        // operations per run
    int mismatches = 0;
    int firstBadRun = 0;
    long checked = 0;

    for (int run = 1; run <= RUNS; run++) {
        mt19937 random(run);                         // same numbers every time we run it
        MaintenanceQueue queue;
        vector<MaintenanceRequest> expected;         // the simple list
        string today = dateFor(1 + random() % 28);
        queue.setToday(today);

        for (int step = 0; step < STEPS; step++) {
            int op = random() % 12;
            bool wrong = false;

            if (op <= 2) {                           // add a request
                MaintenanceRequest r;
                r.equipmentId = 100 + random() % 4;  // only 4 machines, so each has many requests
                r.priority = 1 + random() % 3;
                r.date = dateFor(1 + random() % 28);
                r.description = "x";
                r.requestId = queue.enqueue(r);
                r.effectivePriority = agedPriority(r, today);
                expected.push_back(r);
            }
            else if (op == 3 && !expected.empty()) { // take the first request
                sort(expected.begin(), expected.end(), expectedBefore);
                MaintenanceRequest got;
                queue.dequeue(got);
                wrong = got.requestId != expected[0].requestId;
                expected.erase(expected.begin());
            }
            else if (op == 4 && !expected.empty()) { // change one priority
                int k = random() % expected.size();
                int p = 1 + random() % 3;
                queue.changePriority(expected[k].requestId, p, true);
                expected[k].priority = p;
                expected[k].effectivePriority = agedPriority(expected[k], today);
            }
            else if (op == 5 && !expected.empty()) { // change one date
                int k = random() % expected.size();
                string d = dateFor(1 + random() % 28);
                queue.changeDetails(expected[k].requestId, d, "y");
                expected[k].date = d;
                expected[k].effectivePriority = agedPriority(expected[k], today);
            }
            else if (op == 6) {                      // remove ALL requests of one machine
                int id = 100 + random() % 4;
                queue.removeRequestsFor(id);
                expected.erase(remove_if(expected.begin(), expected.end(),
                    [&](const MaintenanceRequest& r) { return r.equipmentId == id; }), expected.end());
            }
            else if (op == 7) {                      // a new day: aging may change the order
                today = dateFor(1 + random() % 28);
                queue.setToday(today);
                for (MaintenanceRequest& r : expected)
                    r.effectivePriority = agedPriority(r, today);
            }
            else if (op == 9 && !expected.empty()) { // move one request to a position
                int n;
                MaintenanceRequest* before = queue.sortedCopy(n);
                int id = before[random() % n].requestId;
                int position = 1 + random() % queue.maxPosition(id);   // a position the group allows

                // remember which OTHER requests are pinned now
                vector<int> wasPinned;
                for (int i = 0; i < n; i++)
                    if (before[i].requestId != id && before[i].manualRank > 0)
                        wasPinned.push_back(before[i].requestId);

                queue.moveToPosition(id, position);

                // only the moved request may change group: the others stay pinned or not pinned
                for (const MaintenanceRequest& r : expected) {
                    if (r.requestId == id) continue;
                    bool nowPinned = queue.findRequest(r.requestId)->manualRank > 0;
                    bool before_ = find(wasPinned.begin(), wasPinned.end(), r.requestId) != wasPinned.end();
                    if (nowPinned != before_) wrong = true;
                }
                if (queue.findRequest(id)->manualRank == 0)      // the moved one is pinned
                    wrong = true;
                MaintenanceRequest* after = queue.sortedCopy(n);

                // it must be exactly at that position ...
                if (after[position - 1].requestId != id)
                    wrong = true;
                // ... and all the others must keep their order
                int a = 0;
                for (int b = 0; b < n; b++) {
                    if (before[b].requestId == id) continue;
                    if (after[a].requestId == id) a++;
                    if (a >= n || after[a].requestId != before[b].requestId) { wrong = true; break; }
                    a++;
                }
                delete[] before;
                delete[] after;

                // copy the new positions into the simple list
                for (MaintenanceRequest& r : expected)
                    r.manualRank = queue.findRequest(r.requestId)->manualRank;
            }
            else if (op == 10 && !expected.empty()) { // unpin one request
                int k = random() % expected.size();
                queue.unpin(expected[k].requestId);
                expected[k].manualRank = 0;
                if (queue.findRequest(expected[k].requestId)->manualRank != 0)
                    wrong = true;
            }
            else if (op == 11) {                     // reset the manual order
                queue.clearManualOrder();
                for (MaintenanceRequest& r : expected)
                    r.manualRank = 0;
                if (queue.pinnedCount() != 0)
                    wrong = true;
            }
            else if (op == 8 && !expected.empty()) { // remove one request
                int k = random() % expected.size();
                queue.removeRequest(expected[k].requestId);
                expected.erase(expected.begin() + k);
            }

            // ---- compare the heap with the simple list ----
            sort(expected.begin(), expected.end(), expectedBefore);

            int n;
            MaintenanceRequest* ordered = queue.sortedCopy(n);
            if (n != (int)expected.size())
                wrong = true;
            for (int i = 0; !wrong && i < n; i++)
                wrong = ordered[i].requestId != expected[i].requestId;
            delete[] ordered;

            if (queue.count() != (int)expected.size() || queue.isEmpty() != expected.empty())
                wrong = true;

            for (int id = 100; id < 104; id++) {     // counts per machine
                int c = 0;
                for (const MaintenanceRequest& r : expected)
                    if (r.equipmentId == id) c++;
                if (queue.countFor(id) != c || queue.hasRequestFor(id) != (c > 0))
                    wrong = true;
            }

            for (int p = 1; p <= 3; p++) {           // counts per priority
                int c = 0;
                for (const MaintenanceRequest& r : expected)
                    if (r.effectivePriority == p) c++;
                if (queue.count(p) != c)
                    wrong = true;
            }

            MaintenanceRequest first;                // peek = the first of the list
            if (queue.peek(first) != !expected.empty() ||
                (!expected.empty() && first.requestId != expected[0].requestId))
                wrong = true;

            if (!expected.empty()) {                 // find one request by its number
                int middle = expected.size() / 2;
                const MaintenanceRequest* found = queue.findRequest(expected[middle].requestId);
                if (found == nullptr || found->requestId != expected[middle].requestId)
                    wrong = true;
            }
            if (queue.findRequest(999999) != nullptr)
                wrong = true;

            checked++;
            if (wrong) {
                mismatches++;
                if (firstBadRun == 0) firstBadRun = run;
                break;                               // after one mistake the rest is not meaningful
            }
        }
    }

    cout << "queue_fuzz: " << checked << " steps checked, mismatches: " << mismatches;
    if (mismatches > 0)
        cout << " (first failing run: " << firstBadRun << ")";
    cout << "\n";

    return mismatches == 0 ? 0 : 1;
}

// MaintenanceQueue.h
// Priority queue of maintenance requests, built as a BINARY MIN-HEAP in an array.
//
// What comes first (comesBefore):
//   0. requests placed by hand ("pinned"), in the order staff chose
//   1. effective priority: 1 = Urgent, 2 = High, 3 = Normal
//   2. earlier date
//   3. lower request number (the request that arrived first)
//
// Manual order: staff can "pin" requests. Pinned requests form a group at the
// top, in the order staff choose (moveToPosition); all other requests keep the
// normal priority order below them. unpin() / clearManualOrder() undo it.
//
// Effective priority = the request's priority, after "aging": a P3 Normal request
// that is AGING_DAYS or more days overdue counts as P2 High, so it can't wait
// forever. Aging never makes anything P1 Urgent: that level stays for
// life-critical work (Life Support machines, or set by hand by staff).
//
// The priority itself comes from the machine type (Life Support 1, Imaging 2,
// Diagnostic 3) unless it was set by hand.
//
// Heap in an array: the parent of index i is (i - 1) / 2,
// its children are 2i + 1 and 2i + 2. The most urgent request is always heap[0].
//   enqueue, dequeue, change priority: O(log n)    peek: O(1)
//   find one request by number: O(n)               sorted list (heap sort): O(n log n)
// Author: Rajat Rathore

#ifndef BIOFIX_MAINTENANCEQUEUE_H
#define BIOFIX_MAINTENANCEQUEUE_H

#include <iomanip>
#include <utility>
#include <iostream>
#include <string>
#include "InputHelper.h"
using namespace std;

const int AGING_DAYS = 7;          // days overdue before a P3 request counts as P2

struct MaintenanceRequest {
    int requestId = 0;             // unique number of this request (0 = give it a new one)
    int equipmentId = 0;
    string date;
    string description;
    int priority = 3;              // 1 = Urgent, 2 = High, 3 = Normal (before aging)
    bool manualPriority = false;   // false: from the machine type; true: set by hand
    int effectivePriority = 3;     // after aging; this decides the order
    int manualRank = 0;            // 0 = normal order; 1, 2, 3 ... = position chosen by hand
};

inline string priorityLabel(int priority) {
    if (priority == 1) return "P1 Urgent";
    if (priority == 2) return "P2 High";
    return "P3 Normal";
}

inline bool isValidPriority(int priority) {
    return priority >= 1 && priority <= 3;
}

// Priority after aging, for the given day ("" = no aging).
inline int agedPriority(const MaintenanceRequest& r, const string& today) {
    if (today.empty() || !isValidDate(r.date) || r.priority != 3)
        return r.priority;                     // only P3 Normal can age

    int overdue = daysBetween(r.date, today);  // days since the request date
    return overdue >= AGING_DAYS ? 2 : 3;      // never higher than P2 High
}

class MaintenanceQueue {
private:
    MaintenanceRequest* heap;      // the array
    int size;                      // how many requests are stored
    int capacity;                  // how many fit before the array must grow
    int lastRequestId;             // highest request number used so far
    string today;                  // the day used for aging

    // ---------- heap helpers (static, so heap sort can use them on a copy) ----------

    static bool comesBefore(const MaintenanceRequest& a, const MaintenanceRequest& b) {
        // 0. pinned requests first, in the order chosen by hand
        bool aPinned = a.manualRank > 0, bPinned = b.manualRank > 0;
        if (aPinned != bPinned)
            return aPinned;
        if (aPinned && a.manualRank != b.manualRank)
            return a.manualRank < b.manualRank;

        if (a.effectivePriority != b.effectivePriority)
            return a.effectivePriority < b.effectivePriority;
        if (a.date != b.date)
            return a.date < b.date;          // YYYY-MM-DD compares like text
        return a.requestId < b.requestId;
    }

    // Move item i up while it should come before its parent.
    static void siftUp(MaintenanceRequest* arr, int i) {
        while (i > 0) {
            int parent = (i - 1) / 2;
            if (!comesBefore(arr[i], arr[parent]))
                break;
            swap(arr[i], arr[parent]);
            i = parent;
        }
    }

    // Move item i down while one of its children should come before it.
    static void siftDown(MaintenanceRequest* arr, int n, int i) {
        while (true) {
            int left = 2 * i + 1;
            int right = 2 * i + 2;
            int best = i;

            if (left < n && comesBefore(arr[left], arr[best]))
                best = left;
            if (right < n && comesBefore(arr[right], arr[best]))
                best = right;
            if (best == i)
                break;

            swap(arr[i], arr[best]);
            i = best;
        }
    }

    // After one item changed, move it up or down to its correct place.
    void fix(int i) {
        if (i > 0 && comesBefore(heap[i], heap[(i - 1) / 2]))
            siftUp(heap, i);
        else
            siftDown(heap, size, i);
    }

    void grow() {
        capacity *= 2;
        MaintenanceRequest* bigger = new MaintenanceRequest[capacity];
        for (int i = 0; i < size; i++)
            bigger[i] = heap[i];
        delete[] heap;
        heap = bigger;
    }

    // Rebuild the heap from scratch after many items changed ("heapify", O(n)).
    void heapify() {
        for (int i = size / 2 - 1; i >= 0; i--)
            siftDown(heap, size, i);
    }

    int indexOf(int requestId) const {
        for (int i = 0; i < size; i++)
            if (heap[i].requestId == requestId)
                return i;
        return -1;
    }

    // Remove the item at index i (O(log n)).
    void removeAt(int i) {
        size--;
        if (i < size) {
            heap[i] = heap[size];        // move the last item into the gap
            fix(i);
        }
    }

public:
    MaintenanceQueue() {
        capacity = 16;
        heap = new MaintenanceRequest[capacity];
        size = 0;
        lastRequestId = 0;
    }

    ~MaintenanceQueue() {
        delete[] heap;
    }

    // The queue owns its array, so copying is not allowed.
    MaintenanceQueue(const MaintenanceQueue&) = delete;
    MaintenanceQueue& operator=(const MaintenanceQueue&) = delete;

    void clear() {
        size = 0;
    }

    // Sets the day used for aging and puts everything back in heap order.
    // Building a heap from scratch ("heapify") is O(n).
    void setToday(const string& day) {
        if (day == today)
            return;
        today = day;

        for (int i = 0; i < size; i++)
            heap[i].effectivePriority = agedPriority(heap[i], today);
        for (int i = size / 2 - 1; i >= 0; i--)
            siftDown(heap, size, i);
    }

    const string& getToday() const {
        return today;
    }

    // Adds a request. Returns its number. O(log n)
    int enqueue(MaintenanceRequest request) {
        // Give every request its own number, so one request can be edited or cancelled.
        if (request.requestId <= 0)
            request.requestId = ++lastRequestId;
        else if (request.requestId > lastRequestId)
            lastRequestId = request.requestId;

        request.effectivePriority = agedPriority(request, today);

        if (size == capacity)
            grow();

        heap[size] = request;
        siftUp(heap, size);
        size++;
        return request.requestId;
    }

    // Takes the request that should be done first. O(log n)
    bool dequeue(MaintenanceRequest& request) {
        if (size == 0)
            return false;

        request = heap[0];
        removeAt(0);
        return true;
    }

    // Looks at the first request without removing it. O(1)
    bool peek(MaintenanceRequest& request) const {
        if (size == 0)
            return false;

        request = heap[0];
        return true;
    }

    // Finds one request by its number (nullptr if there is none). Read only:
    // use the change... functions so the heap stays in order.
    const MaintenanceRequest* findRequest(int requestId) const {
        int i = indexOf(requestId);
        return i < 0 ? nullptr : &heap[i];
    }

    // Removes one request by its number. Returns false if there is none.
    bool removeRequest(int requestId) {
        int i = indexOf(requestId);
        if (i < 0)
            return false;
        removeAt(i);
        return true;
    }

    // Changes one request's priority and moves it to its new place.
    bool changePriority(int requestId, int priority, bool manual) {
        int i = indexOf(requestId);
        if (i < 0)
            return false;

        heap[i].priority = priority;
        heap[i].manualPriority = manual;
        heap[i].effectivePriority = agedPriority(heap[i], today);
        fix(i);
        return true;
    }

    // Changes one request's date and description (the date affects the order).
    bool changeDetails(int requestId, const string& date, const string& description) {
        int i = indexOf(requestId);
        if (i < 0)
            return false;

        heap[i].date = date;
        heap[i].description = description;
        heap[i].effectivePriority = agedPriority(heap[i], today);
        fix(i);
        return true;
    }

    // ---------------- manual order ----------------

    // How many requests are pinned (placed by hand).
    int pinnedCount() const {
        int c = 0;
        for (int i = 0; i < size; i++)
            if (heap[i].manualRank > 0)
                c++;
        return c;
    }

    // Highest position a request can get in the pinned group
    // (pinned requests can move inside the group; a new one can join at the end).
    int maxPosition(int requestId) const {
        int i = indexOf(requestId);
        if (i < 0)
            return 0;
        return pinnedCount() + (heap[i].manualRank > 0 ? 0 : 1);
    }

    // Pins one request at a position (1 = first) of the pinned group, or moves it
    // inside the group. Only this request changes group; the others keep their order.
    // Returns false if the number or position is wrong.
    bool moveToPosition(int requestId, int position) {
        int target = indexOf(requestId);
        if (target < 0 || position < 1 || position > maxPosition(requestId))
            return false;

        // 1. the pinned requests in their current order, without the one we move
        int n;
        MaintenanceRequest* order = sortedCopy(n);
        int* pinned = new int[n + 1];
        int k = 0;
        for (int i = 0; i < n; i++)
            if (order[i].manualRank > 0 && order[i].requestId != requestId)
                pinned[k++] = order[i].requestId;
        delete[] order;

        // 2. put it in at the chosen position
        for (int i = k; i > position - 1; i--)
            pinned[i] = pinned[i - 1];
        pinned[position - 1] = requestId;
        k++;

        // 3. number the pinned group 1, 2, 3 ... (everything else stays 0)
        for (int i = 0; i < size; i++) {
            heap[i].manualRank = 0;
            for (int j = 0; j < k; j++)
                if (pinned[j] == heap[i].requestId)
                    heap[i].manualRank = j + 1;
        }

        delete[] pinned;
        heapify();
        return true;
    }

    // Puts one request back into the normal priority order.
    bool unpin(int requestId) {
        int i = indexOf(requestId);
        if (i < 0 || heap[i].manualRank == 0)
            return false;
        heap[i].manualRank = 0;
        fix(i);
        return true;
    }

    // Puts every request back into the normal priority order.
    void clearManualOrder() {
        for (int i = 0; i < size; i++)
            heap[i].manualRank = 0;
        heapify();
    }

    // Removes every waiting request for this equipment. Returns how many.
    // Step 1: keep only the other requests (squeeze them to the front of the array).
    // Step 2: rebuild the heap once ("heapify", O(n)).
    // (Removing one by one is not safe: the item moved into the gap can climb to a
    // place the loop has already passed, so it would be skipped.)
    int removeRequestsFor(int equipmentId) {
        int kept = 0;

        for (int i = 0; i < size; i++)
            if (heap[i].equipmentId != equipmentId)
                heap[kept++] = heap[i];

        int removed = size - kept;
        size = kept;

        for (int i = size / 2 - 1; i >= 0; i--)
            siftDown(heap, size, i);

        return removed;
    }

    // Number of waiting requests (onlyPriority = count only that effective priority).
    int count(int onlyPriority = 0) const {
        if (onlyPriority == 0)
            return size;

        int c = 0;
        for (int i = 0; i < size; i++)
            if (heap[i].effectivePriority == onlyPriority)
                c++;
        return c;
    }

    int countFor(int equipmentId) const {
        int c = 0;
        for (int i = 0; i < size; i++)
            if (heap[i].equipmentId == equipmentId)
                c++;
        return c;
    }

    bool hasRequestFor(int equipmentId) const {
        return countFor(equipmentId) > 0;
    }

    bool isEmpty() const {
        return size == 0;
    }

    // Requests in heap order (NOT sorted), for saving or counting.
    int getSize() const { return size; }
    const MaintenanceRequest& at(int i) const { return heap[i]; }

    // A new array with every request in queue order, made with HEAP SORT:
    // copy the heap, then take the first item out again and again. O(n log n).
    // The caller must delete[] the array.
    MaintenanceRequest* sortedCopy(int& n) const {
        n = size;
        MaintenanceRequest* work = new MaintenanceRequest[size > 0 ? size : 1];
        MaintenanceRequest* sorted = new MaintenanceRequest[size > 0 ? size : 1];

        for (int i = 0; i < size; i++)
            work[i] = heap[i];           // the copy is already a valid heap

        int left = size;
        for (int k = 0; k < size; k++) {
            sorted[k] = work[0];         // the first item
            left--;
            work[0] = work[left];        // last item to the top ...
            siftDown(work, left, 0);     // ... and push it down
        }

        delete[] work;
        return sorted;
    }

    void display() const {
        if (size == 0) {
            cout << "\nMaintenance queue is empty.\n";
            return;
        }

        int n;
        MaintenanceRequest* list = sortedCopy(n);

        cout << "\n" << left << setw(6) << "Req" << setw(6) << "ID" << setw(28) << "Priority"
             << setw(12) << "Date" << "Description\n";
        cout << string(76, '-') << "\n";

        for (int i = 0; i < n; i++) {
            const MaintenanceRequest& r = list[i];
            string label = priorityLabel(r.effectivePriority);
            if (r.manualRank > 0)
                label += " (pinned)";
            else if (r.effectivePriority != r.priority)
                label += " (aged)";
            else if (r.manualPriority)
                label += " (manual)";

            cout << left << setw(6) << ("#" + to_string(r.requestId)) << setw(6) << r.equipmentId
                 << setw(28) << label << setw(12) << r.date << r.description << "\n";
        }

        delete[] list;
    }
};

#endif

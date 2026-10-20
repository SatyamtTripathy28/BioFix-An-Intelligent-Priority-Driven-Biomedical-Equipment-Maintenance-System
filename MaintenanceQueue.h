// MaintenanceQueue.h
// Linked queue of maintenance requests (first in, first out).
// Author: Rajat Rathore

#ifndef BIOFIX_MAINTENANCEQUEUE_H
#define BIOFIX_MAINTENANCEQUEUE_H

#include <iostream>
#include <string>
using namespace std;

// -------------------- MAINTENANCE QUEUE --------------------
struct MaintenanceRequest {
    int equipmentId;
    string date;
    string description;
};

struct QueueNode {
    MaintenanceRequest data;
    QueueNode* next;

    QueueNode(MaintenanceRequest r) {
        data = r;
        next = nullptr;
    }
};

class MaintenanceQueue {
private:
    QueueNode* front;
    QueueNode* rear;

public:
    MaintenanceQueue() {
        front = nullptr;
        rear = nullptr;
    }

    ~MaintenanceQueue() {
        clear();
    }

    void clear() {
        while (front != nullptr) {
            QueueNode* temp = front;
            front = front->next;
            delete temp;
        }
        rear = nullptr;
    }

    void enqueue(MaintenanceRequest request) {
        QueueNode* newNode = new QueueNode(request);

        if (rear == nullptr) {
            front = rear = newNode;
            return;
        }

        rear->next = newNode;
        rear = newNode;
    }

    bool dequeue(MaintenanceRequest& request) {
        if (front == nullptr)
            return false;

        QueueNode* temp = front;
        request = front->data;

        front = front->next;

        if (front == nullptr)
            rear = nullptr;

        delete temp;
        return true;
    }

    void display() const {
        if (front == nullptr) {
            cout << "\nMaintenance queue is empty.\n";
            return;
        }

        cout << "\nID\tDate\t\tDescription\n";
        cout << "------------------------------------------------------------\n";

        QueueNode* current = front;

        while (current != nullptr) {
            cout << current->data.equipmentId << "\t"
                 << current->data.date << "\t"
                 << current->data.description << endl;

            current = current->next;
        }
    }

    bool isEmpty() const {
        return front == nullptr;
    }
};

#endif

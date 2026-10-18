// EquipmentList.h
// Singly linked list that holds all Equipment objects.
// Author: Satyam Tripathy

#ifndef BIOFIX_EQUIPMENTLIST_H
#define BIOFIX_EQUIPMENTLIST_H

#include "Equipment.h"

// -------------------- LINKED LIST NODE --------------------
struct Node {
    Equipment* data;
    Node* next;

    Node(Equipment* e) {
        data = e;
        next = nullptr;
    }
};

// -------------------- LINKED LIST --------------------
class EquipmentList {
private:
    Node* head;

public:
    EquipmentList() {
        head = nullptr;
    }

    ~EquipmentList() {
        clear();
    }

    void clear() {
        Node* current = head;
        while (current != nullptr) {
            Node* temp = current;
            current = current->next;
            delete temp->data;
            delete temp;
        }
        head = nullptr;
    }

    void add(Equipment* e) {
        Node* newNode = new Node(e);

        if (head == nullptr) {
            head = newNode;
            return;
        }

        Node* current = head;
        while (current->next != nullptr)
            current = current->next;

        current->next = newNode;
    }

    Node* getHead() const {
        return head;
    }

    Equipment* findById(int id) const {
        Node* current = head;

        while (current != nullptr) {
            if (current->data->getId() == id)
                return current->data;

            current = current->next;
        }

        return nullptr;
    }

    bool removeById(int id) {
        Node* current = head;
        Node* previous = nullptr;

        while (current != nullptr) {
            if (current->data->getId() == id) {
                if (previous == nullptr)
                    head = current->next;
                else
                    previous->next = current->next;

                delete current->data;
                delete current;
                return true;
            }

            previous = current;
            current = current->next;
        }

        return false;
    }

    void displayAll() const {
        if (head == nullptr) {
            cout << "\nNo equipment found.\n";
            return;
        }

        cout << "\nID\tName\t\tType\t\tStatus\n";
        cout << "------------------------------------------------------------\n";

        Node* current = head;
        while (current != nullptr) {
            current->data->display();
            current = current->next;
        }
    }

    int count() const {
        int c = 0;
        Node* current = head;

        while (current != nullptr) {
            c++;
            current = current->next;
        }

        return c;
    }

    // Copy pointers into a dynamic array for sorting/binary search.
    // The equipment objects themselves remain owned by the linked list.
    Equipment** toArray(int& n) const {
        n = count();

        if (n == 0)
            return nullptr;

        Equipment** arr = new Equipment*[n];

        Node* current = head;
        int i = 0;

        while (current != nullptr) {
            arr[i++] = current->data;
            current = current->next;
        }

        return arr;
    }
};

#endif

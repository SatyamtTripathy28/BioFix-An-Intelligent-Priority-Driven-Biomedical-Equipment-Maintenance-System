// EquipmentList.h
// Singly linked list that holds all Equipment objects.
// A tail pointer makes add() O(1), and a hash table makes findById() about O(1).
// Author: Satyam Tripathy

#ifndef BIOFIX_EQUIPMENTLIST_H
#define BIOFIX_EQUIPMENTLIST_H

#include "Equipment.h"
#include "EquipmentHash.h"

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
    Node* tail;           // last node, so add() does not walk the list
    EquipmentHash index;  // fast lookup by ID

public:
    EquipmentList() {
        head = nullptr;
        tail = nullptr;
    }

    // The list owns its objects, so copying is not allowed.
    EquipmentList(const EquipmentList&) = delete;
    EquipmentList& operator=(const EquipmentList&) = delete;

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
        tail = nullptr;
        index.clear();
    }

    // Adds at the end in O(1) using the tail pointer.
    void add(Equipment* e) {
        Node* newNode = new Node(e);

        if (head == nullptr)
            head = tail = newNode;
        else {
            tail->next = newNode;
            tail = newNode;
        }

        index.insert(e);
    }

    Node* getHead() const {
        return head;
    }

    // Uses the hash table: about one step instead of walking the list.
    Equipment* findById(int id) const {
        return index.find(id);
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

                if (current == tail)
                    tail = previous;

                index.remove(id);          // before the object is deleted
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

        cout << "\n";
        printEquipmentHeader();

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

    // Copy pointers into a dynamic array for sorting.
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

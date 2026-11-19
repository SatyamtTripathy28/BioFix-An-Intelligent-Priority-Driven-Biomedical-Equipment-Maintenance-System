// EquipmentHash.h
// Hash table that finds equipment by ID in about one step.
// Each "bucket" is a small linked list (separate chaining), so two IDs that
// land in the same bucket are both kept.
//
// Load factor = machines / buckets. If it goes above 0.75, the table
// REHASHES: it makes about twice as many buckets and moves every node to its
// new bucket. This keeps the chains short, so lookups stay about O(1) even
// for thousands of machines.
//
// The table only stores pointers. EquipmentList owns and deletes the objects.
// Module owner: Satyam Tripathy

#ifndef BIOFIX_EQUIPMENTHASH_H
#define BIOFIX_EQUIPMENTHASH_H

#include "Equipment.h"

struct HashNode {
    Equipment* data;
    HashNode* next;
};

class EquipmentHash {
private:
    static const int START_BUCKETS = 101;
    HashNode** buckets;      // array of chains
    int bucketCount;
    int itemCount;

    int hashOf(int id, int count) const {
        return (id % count + count) % count;   // always 0 to count - 1
    }

    // Make a bigger table and move every node into it (the nodes are reused).
    void rehash(int newCount) {
        HashNode** bigger = new HashNode*[newCount];
        for (int i = 0; i < newCount; i++)
            bigger[i] = nullptr;

        for (int i = 0; i < bucketCount; i++) {
            HashNode* current = buckets[i];

            while (current != nullptr) {
                HashNode* next = current->next;
                int index = hashOf(current->data->getId(), newCount);
                current->next = bigger[index];      // put at the front of its new chain
                bigger[index] = current;
                current = next;
            }
        }

        delete[] buckets;
        buckets = bigger;
        bucketCount = newCount;
    }

public:
    EquipmentHash() {
        bucketCount = START_BUCKETS;
        itemCount = 0;
        buckets = new HashNode*[bucketCount];
        for (int i = 0; i < bucketCount; i++)
            buckets[i] = nullptr;
    }

    ~EquipmentHash() {
        clear();
        delete[] buckets;
    }

    // Copying would make two tables share the same nodes, so it is not allowed.
    EquipmentHash(const EquipmentHash&) = delete;
    EquipmentHash& operator=(const EquipmentHash&) = delete;

    void insert(Equipment* e) {
        // Keep the load factor at or below 0.75 (written as 4 * items <= 3 * buckets).
        if (4 * (itemCount + 1) > 3 * bucketCount)
            rehash(bucketCount * 2 + 1);

        int index = hashOf(e->getId(), bucketCount);
        buckets[index] = new HashNode{e, buckets[index]};   // add at the front
        itemCount++;
    }

    Equipment* find(int id) const {
        HashNode* current = buckets[hashOf(id, bucketCount)];

        while (current != nullptr) {
            if (current->data->getId() == id)
                return current->data;
            current = current->next;
        }

        return nullptr;
    }

    // Call this BEFORE the equipment object is deleted.
    void remove(int id) {
        int index = hashOf(id, bucketCount);
        HashNode* current = buckets[index];
        HashNode* previous = nullptr;

        while (current != nullptr) {
            if (current->data->getId() == id) {
                if (previous == nullptr)
                    buckets[index] = current->next;
                else
                    previous->next = current->next;

                delete current;            // deletes the bucket node only
                itemCount--;
                return;
            }

            previous = current;
            current = current->next;
        }
    }

    // Deletes all bucket nodes (not the equipment objects).
    void clear() {
        for (int i = 0; i < bucketCount; i++) {
            HashNode* current = buckets[i];

            while (current != nullptr) {
                HashNode* next = current->next;
                delete current;
                current = next;
            }

            buckets[i] = nullptr;
        }
        itemCount = 0;
    }

    int getBucketCount() const { return bucketCount; }
    int size() const { return itemCount; }
};

#endif

// SearchSort.cpp
// Author: Keshav Aggarwal

#include "SearchSort.h"
#include "InputHelper.h"

// Prints one machine's details. Used by both searches.
static void showEquipment(const Equipment* e) {
    cout << "ID: " << e->getId() << endl;
    cout << "Name: " << e->getName() << endl;
    cout << "Department: " << e->getDepartment() << endl;
    cout << "Type: " << e->getCategory() << endl;
    cout << "Status: " << e->getStatus() << endl;
}

bool matchesKeyword(const Equipment* e, const string& keyword) {
    return to_string(e->getId()) == keyword ||
           containsText(e->getName(), keyword) ||
           containsText(e->getDepartment(), keyword) ||
           containsText(e->getCategory(), keyword) ||
           containsText(e->getStatus(), keyword);
}

// -------------------- KEYWORD SEARCH (menu option 6) --------------------
// Checks every machine once (O(n)), because finding part of a word
// (for example "mri" inside "MRI Machine") needs to look at each name.
// Capital letters are ignored. The ID must match exactly.
void searchEquipment(const EquipmentList& list) {
    string keyword = readText("\nSearch by ID, name, department, type or status: ");
    if (keyword.empty())
        return;

    if (keyword.size() < 2 && !isNumber(keyword)) {
        cout << "Please type at least 2 characters.\n";
        return;
    }

    int count = 0;

    for (Node* current = list.getHead(); current != nullptr; current = current->next) {
        Equipment* e = current->data;

        if (matchesKeyword(e, keyword)) {
            cout << "\nEquipment Found:\n";
            showEquipment(e);
            count++;
        }
    }

    if (count == 0)
        cout << "\nEquipment not found.\n";
    else
        cout << "\n" << count << " result(s) found.\n";
}

// -------------------- FIND BY ID (menu option 7) --------------------
// The hash table jumps straight to the right bucket: about one step.
void findEquipmentById(const EquipmentList& list) {
    int id = readInt("\nEnter equipment ID: ");
    if (inputEnded())
        return;

    Equipment* e = list.findById(id);

    if (e == nullptr) {
        cout << "\nEquipment not found.\n";
        return;
    }

    cout << "\nEquipment Found:\n";
    showEquipment(e);
}

// -------------------- INSERTION SORT BY NAME --------------------
void insertionSortByName(Equipment** arr, int n) {
    for (int i = 1; i < n; i++) {
        Equipment* key = arr[i];
        int j = i - 1;

        while (j >= 0 && arr[j]->getName() > key->getName()) {
            arr[j + 1] = arr[j];
            j--;
        }

        arr[j + 1] = key;
    }
}

// -------------------- SORT BY NAME (menu option 8) --------------------
void sortByName(const EquipmentList& list) {
    int n;
    Equipment** arr = list.toArray(n);

    if (n == 0) {
        cout << "\nNo equipment found.\n";
        return;
    }

    insertionSortByName(arr, n);

    cout << "\n========== SORTED BY NAME ==========\n";
    printEquipmentHeader();

    for (int i = 0; i < n; i++)
        arr[i]->display();

    delete[] arr;   // frees the array only; the list still owns the objects
}

// SearchSort.cpp
// Author: Keshav Aggarwal

#include "SearchSort.h"

void linearSearch(const EquipmentList& list) {
    string keyword;

    cout << "\nEnter equipment ID or name: ";
    getline(cin >> ws, keyword);

    bool found = false;

    Node* current = list.getHead();

    while (current != nullptr) {
        Equipment* e = current->data;

        if (to_string(e->getId()) == keyword ||
            e->getName() == keyword) {

            cout << "\nEquipment Found:\n";
            cout << "ID: " << e->getId() << endl;
            cout << "Name: " << e->getName() << endl;
            cout << "Department: " << e->getDepartment() << endl;
            cout << "Type: " << e->getType() << endl;
            cout << "Status: " << e->getStatus() << endl;

            found = true;
        }

        current = current->next;
    }

    if (!found)
        cout << "\nEquipment not found.\n";
}

void insertionSortById(Equipment** arr, int n) {
    for (int i = 1; i < n; i++) {
        Equipment* key = arr[i];
        int j = i - 1;

        while (j >= 0 && arr[j]->getId() > key->getId()) {
            arr[j + 1] = arr[j];
            j--;
        }

        arr[j + 1] = key;
    }
}

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

void binarySearchById(const EquipmentList& list) {
    int id;

    cout << "\nEnter equipment ID: ";
    cin >> id;

    int n;
    Equipment** arr = list.toArray(n);

    if (n == 0) {
        cout << "\nNo equipment found.\n";
        return;
    }

    insertionSortById(arr, n);

    int low = 0;
    int high = n - 1;

    while (low <= high) {
        int mid = (low + high) / 2;

        if (arr[mid]->getId() == id) {
            Equipment* e = arr[mid];

            cout << "\nEquipment Found:\n";
            cout << "ID: " << e->getId() << endl;
            cout << "Name: " << e->getName() << endl;
            cout << "Department: " << e->getDepartment() << endl;
            cout << "Type: " << e->getType() << endl;
            cout << "Status: " << e->getStatus() << endl;

            delete[] arr;
            return;
        }

        if (arr[mid]->getId() < id)
            low = mid + 1;
        else
            high = mid - 1;
    }

    cout << "\nEquipment not found.\n";

    delete[] arr;
}

void sortByName(const EquipmentList& list) {
    int n;
    Equipment** arr = list.toArray(n);

    if (n == 0) {
        cout << "\nNo equipment found.\n";
        return;
    }

    insertionSortByName(arr, n);

    cout << "\n========== SORTED BY NAME ==========\n";
    cout << "ID\tName\t\tType\t\tStatus\n";
    cout << "------------------------------------------------------------\n";

    for (int i = 0; i < n; i++)
        arr[i]->display();

    delete[] arr;
}

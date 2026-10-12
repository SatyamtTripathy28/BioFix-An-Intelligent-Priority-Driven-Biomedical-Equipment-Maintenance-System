#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
using namespace std;

/* =========================================================
   BIOFIX - Biomedical Equipment Maintenance Management
   Concepts used:
   OOP, Inheritance, Polymorphism, Linked List, Queue,
   Linear Search, Binary Search, Insertion Sort,
   Dynamic Memory Allocation and File Handling
   ========================================================= */

// -------------------- OOP: BASE CLASS --------------------
class Equipment {
protected:
    int id;
    string name;
    string department;
    string type;
    string status;

public:
    Equipment(int i, string n, string d, string t, string s)
        : id(i), name(n), department(d), type(t), status(s) {}

    virtual ~Equipment() {}

    int getId() const { return id; }
    string getName() const { return name; }
    string getDepartment() const { return department; }
    string getType() const { return type; }
    string getStatus() const { return status; }

    void setName(string n) { name = n; }
    void setDepartment(string d) { department = d; }
    void setType(string t) { type = t; }
    void setStatus(string s) { status = s; }

    // Runtime polymorphism
    virtual string getCategory() const = 0;

    virtual void display() const {
        cout << id << "\t" << name << "\t\t"
             << type << "\t\t" << status << endl;
    }
};

// -------------------- INHERITANCE --------------------
class LifeSupportEquipment : public Equipment {
public:
    LifeSupportEquipment(int i, string n, string d, string s)
        : Equipment(i, n, d, "Life Support", s) {}

    string getCategory() const override {
        return "Life Support";
    }
};

class ImagingEquipment : public Equipment {
public:
    ImagingEquipment(int i, string n, string d, string s)
        : Equipment(i, n, d, "Imaging", s) {}

    string getCategory() const override {
        return "Imaging";
    }
};

class DiagnosticEquipment : public Equipment {
public:
    DiagnosticEquipment(int i, string n, string d, string s)
        : Equipment(i, n, d, "Diagnostic", s) {}

    string getCategory() const override {
        return "Diagnostic";
    }
};

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

// -------------------- FILE FUNCTIONS --------------------
const string MASTER_FILE = "equipment.txt";
const string MAINTENANCE_FILE = "maintenance.txt";

string getDepartmentFile(string department) {
    if (department == "Radiology")
        return "radiology.txt";
    if (department == "ICU")
        return "icu.txt";
    if (department == "Cardiology")
        return "cardiology.txt";
    if (department == "Operation Theatre")
        return "operation_theatre.txt";
    if (department == "Laboratory")
        return "laboratory.txt";
    if (department == "Emergency")
        return "emergency.txt";

    return "";
}

Equipment* createEquipment(int id, string name, string department,
                           string type, string status) {
    if (type == "Life Support")
        return new LifeSupportEquipment(id, name, department, status);

    if (type == "Imaging")
        return new ImagingEquipment(id, name, department, status);

    return new DiagnosticEquipment(id, name, department, status);
}

void saveFiles(const EquipmentList& list, const MaintenanceQueue& queue) {
    // Master file
    ofstream master(MASTER_FILE);

    if (!master) {
        cout << "Error opening equipment.txt\n";
        return;
    }

    Node* current = list.getHead();

    while (current != nullptr) {
        Equipment* e = current->data;

        master << e->getId() << "|"
               << e->getName() << "|"
               << e->getDepartment() << "|"
               << e->getType() << "|"
               << e->getStatus() << "\n";

        current = current->next;
    }

    master.close();

    // Department files are synchronized copies of the master data.
    string departments[] = {
        "Radiology",
        "ICU",
        "Cardiology",
        "Operation Theatre",
        "Laboratory",
        "Emergency"
    };

    for (string department : departments) {
        ofstream file(getDepartmentFile(department));

        current = list.getHead();

        while (current != nullptr) {
            Equipment* e = current->data;

            if (e->getDepartment() == department) {
                file << e->getId() << "|"
                     << e->getName() << "|"
                     << e->getDepartment() << "|"
                     << e->getType() << "|"
                     << e->getStatus() << "\n";
            }

            current = current->next;
        }

        file.close();
    }

    // maintenance.txt is handled separately by saveMaintenanceQueue().
    // Keeping it separate prevents equipment updates from erasing requests.
}

void saveMaintenance(const MaintenanceQueue& queue) {
    // Since queue nodes are private, this function uses a temporary copy.
    // Dequeueing from the real queue would change the user's data.
    // Therefore the queue class provides no direct file traversal.
    // For simplicity, maintenance data is saved when requests are added
    // and processed using a separate append/rebuild approach in main.
    //
    // This function is intentionally unused.
    (void)queue;
}

void loadEquipment(EquipmentList& list) {
    ifstream file(MASTER_FILE);

    if (!file)
        return;

    string line;

    while (getline(file, line)) {
        if (line.empty())
            continue;

        stringstream ss(line);

        string idStr, name, department, type, status;

        getline(ss, idStr, '|');
        getline(ss, name, '|');
        getline(ss, department, '|');
        getline(ss, type, '|');
        getline(ss, status, '|');

        if (idStr.empty())
            continue;

        int id = stoi(idStr);

        list.add(createEquipment(id, name, department, type, status));
    }

    file.close();
}

void loadMaintenance(MaintenanceQueue& queue) {
    ifstream file(MAINTENANCE_FILE);

    if (!file)
        return;

    string line;

    while (getline(file, line)) {
        if (line.empty())
            continue;

        stringstream ss(line);

        string idStr, date, description;

        getline(ss, idStr, '|');
        getline(ss, date, '|');
        getline(ss, description, '|');

        if (idStr.empty())
            continue;

        MaintenanceRequest request;
        request.equipmentId = stoi(idStr);
        request.date = date;
        request.description = description;

        queue.enqueue(request);
    }

    file.close();
}

void saveMaintenanceQueue(MaintenanceQueue& queue) {
    // Preserve the queue while writing it by moving elements temporarily.
    ofstream file(MAINTENANCE_FILE);

    if (!file) {
        cout << "Error opening maintenance.txt\n";
        return;
    }

    MaintenanceQueue temp;
    MaintenanceRequest request;

    while (queue.dequeue(request)) {
        file << request.equipmentId << "|"
             << request.date << "|"
             << request.description << "\n";

        temp.enqueue(request);
    }

    while (temp.dequeue(request))
        queue.enqueue(request);

    file.close();
}

// -------------------- SEARCH --------------------
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

// -------------------- DISPLAY DEPARTMENT --------------------
void displayDepartment(const EquipmentList& list, string department) {
    cout << "\n========== " << department << " EQUIPMENT ==========\n";

    bool found = false;

    Node* current = list.getHead();

    cout << "ID\tName\t\tType\t\tStatus\n";
    cout << "------------------------------------------------------------\n";

    while (current != nullptr) {
        Equipment* e = current->data;

        if (e->getDepartment() == department) {
            e->display();
            found = true;
        }

        current = current->next;
    }

    if (!found)
        cout << "No equipment found in this department.\n";
}

// -------------------- ADD --------------------
void addEquipment(EquipmentList& list, MaintenanceQueue& queue) {
    int id;
    string name, department, type, status;

    cout << "\nEnter equipment ID: ";
    cin >> id;

    if (list.findById(id) != nullptr) {
        cout << "ID already exists.\n";
        return;
    }

    cout << "Enter equipment name: ";
    getline(cin >> ws, name);

    cout << "\nSelect Department:\n";
    cout << "1. Radiology\n";
    cout << "2. ICU\n";
    cout << "3. Cardiology\n";
    cout << "4. Operation Theatre\n";
    cout << "5. Laboratory\n";
    cout << "6. Emergency\n";

    int d;
    cout << "Enter choice: ";
    cin >> d;

    switch (d) {
        case 1: department = "Radiology"; break;
        case 2: department = "ICU"; break;
        case 3: department = "Cardiology"; break;
        case 4: department = "Operation Theatre"; break;
        case 5: department = "Laboratory"; break;
        case 6: department = "Emergency"; break;
        default:
            cout << "Invalid department.\n";
            return;
    }

    cout << "\nSelect Equipment Type:\n";
    cout << "1. Life Support\n";
    cout << "2. Imaging\n";
    cout << "3. Diagnostic\n";

    int t;
    cout << "Enter choice: ";
    cin >> t;

    switch (t) {
        case 1: type = "Life Support"; break;
        case 2: type = "Imaging"; break;
        case 3: type = "Diagnostic"; break;
        default:
            cout << "Invalid type.\n";
            return;
    }

    cout << "\nSelect Status:\n";
    cout << "1. Working\n";
    cout << "2. Under Maintenance\n";
    cout << "3. Unavailable\n";

    int s;
    cout << "Enter choice: ";
    cin >> s;

    switch (s) {
        case 1: status = "Working"; break;
        case 2: status = "Under Maintenance"; break;
        case 3: status = "Unavailable"; break;
        default:
            cout << "Invalid status.\n";
            return;
    }

    list.add(createEquipment(id, name, department, type, status));

    saveFiles(list, queue);

    cout << "\nEquipment added successfully.\n";
}

// -------------------- UPDATE --------------------
void updateEquipment(EquipmentList& list, MaintenanceQueue& queue) {
    int id;

    cout << "\nEnter equipment ID to update: ";
    cin >> id;

    Equipment* e = list.findById(id);

    if (e == nullptr) {
        cout << "Equipment not found.\n";
        return;
    }

    string name, status;

    cout << "Enter new name: ";
    getline(cin >> ws, name);

    cout << "\nSelect New Status:\n";
    cout << "1. Working\n";
    cout << "2. Under Maintenance\n";
    cout << "3. Unavailable\n";

    int choice;
    cout << "Enter choice: ";
    cin >> choice;

    switch (choice) {
        case 1: status = "Working"; break;
        case 2: status = "Under Maintenance"; break;
        case 3: status = "Unavailable"; break;
        default:
            cout << "Invalid status.\n";
            return;
    }

    e->setName(name);
    e->setStatus(status);

    saveFiles(list, queue);

    cout << "\nEquipment updated successfully.\n";
}

// -------------------- DELETE --------------------
void deleteEquipment(EquipmentList& list, MaintenanceQueue& queue) {
    int id;

    cout << "\nEnter equipment ID to delete: ";
    cin >> id;

    if (list.removeById(id)) {
        saveFiles(list, queue);
        cout << "\nEquipment deleted successfully.\n";
    }
    else {
        cout << "\nEquipment not found.\n";
    }
}

// -------------------- SORT --------------------
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

// -------------------- DEPARTMENT MENU --------------------
void departmentMenu(const EquipmentList& list) {
    int choice;

    do {
        cout << "\n========== DEPARTMENT VIEW ==========\n";
        cout << "1. Radiology\n";
        cout << "2. ICU\n";
        cout << "3. Cardiology\n";
        cout << "4. Operation Theatre\n";
        cout << "5. Laboratory\n";
        cout << "6. Emergency\n";
        cout << "0. Back\n";
        cout << "Enter choice: ";
        cin >> choice;

        switch (choice) {
            case 1: displayDepartment(list, "Radiology"); break;
            case 2: displayDepartment(list, "ICU"); break;
            case 3: displayDepartment(list, "Cardiology"); break;
     

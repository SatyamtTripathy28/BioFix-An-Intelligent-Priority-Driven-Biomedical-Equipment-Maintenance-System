// Equipment.h
// The Equipment base class and its three subclasses.
// Shows inheritance (subclasses) and runtime polymorphism: getCategory() and
// getPriority() are virtual, and each subclass gives its own answer.
// Author: Bhaskar Dev

#ifndef BIOFIX_EQUIPMENT_H
#define BIOFIX_EQUIPMENT_H

#include <iomanip>
#include <iostream>
#include <string>
using namespace std;

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

    // Runtime polymorphism: each subclass answers these differently.
    virtual string getCategory() const = 0;

    // Repair priority used by the maintenance queue (1 = most urgent).
    virtual int getPriority() const = 0;

    // display() calls getCategory(), so the subclass's own version runs.
    // setw(n) gives each column a fixed width, so the table lines up.
    virtual void display() const {
        cout << left << setw(6) << id << setw(32) << name
             << setw(15) << getCategory() << status << endl;
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

    int getPriority() const override {
        return 1;   // a patient may depend on it
    }
};

class ImagingEquipment : public Equipment {
public:
    ImagingEquipment(int i, string n, string d, string s)
        : Equipment(i, n, d, "Imaging", s) {}

    string getCategory() const override {
        return "Imaging";
    }

    int getPriority() const override {
        return 2;
    }
};

class DiagnosticEquipment : public Equipment {
public:
    DiagnosticEquipment(int i, string n, string d, string s)
        : Equipment(i, n, d, "Diagnostic", s) {}

    string getCategory() const override {
        return "Diagnostic";
    }

    int getPriority() const override {
        return 3;
    }
};

// The header line for a table of equipment (matches display()).
inline void printEquipmentHeader() {
    cout << left << setw(6) << "ID" << setw(32) << "Name" << setw(15) << "Type" << "Status\n";
    cout << string(70, '-') << "\n";
}

// Picks the right subclass for a type name (defined in Equipment.cpp).
// Returns nullptr for an unknown type.
Equipment* createEquipment(int id, string name, string department,
                           string type, string status);

#endif

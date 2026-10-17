// Equipment.h
// The Equipment base class and its three subclasses.
// Shows inheritance (subclasses) and polymorphism (getCategory is virtual).
// Author: Bhaskar Dev

#ifndef BIOFIX_EQUIPMENT_H
#define BIOFIX_EQUIPMENT_H

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

// Picks the right subclass for a type name (defined in Equipment.cpp).
Equipment* createEquipment(int id, string name, string department,
                           string type, string status);

#endif

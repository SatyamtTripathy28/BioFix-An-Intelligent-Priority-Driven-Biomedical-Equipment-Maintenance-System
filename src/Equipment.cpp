// Equipment.cpp
// Factory function that creates the correct Equipment subclass.
// Author: Bhaskar Dev

#include "Equipment.h"

Equipment* createEquipment(int id, string name, string department,
                           string type, string status) {
    if (type == "Life Support")
        return new LifeSupportEquipment(id, name, department, status);

    if (type == "Imaging")
        return new ImagingEquipment(id, name, department, status);

    if (type == "Diagnostic")
        return new DiagnosticEquipment(id, name, department, status);

    return nullptr;   // unknown type: the caller decides what to do
}

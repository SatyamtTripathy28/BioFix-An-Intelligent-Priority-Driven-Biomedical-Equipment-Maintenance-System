# BioFix

BioFix is a command-line program, written in C++17, that keeps track of the biomedical equipment in a hospital and the maintenance requests for it. It was built for the Data Structure and Programming in C++ PBL project (Team ID DSCPP-III-2026-T173).

Equipment is stored by department, can be searched and sorted, and is saved to text files so the data is still there the next time the program runs. Maintenance requests wait in a queue and are processed one at a time.

## What the menu does

```
1. Add Equipment
2. Display All Equipment
3. View Department Equipment
4. Update Equipment
5. Delete Equipment
6. Linear Search
7. Binary Search by ID
8. Sort by Name (Insertion Sort)
9. Schedule Maintenance
10. View Maintenance Queue
11. Process Next Maintenance Request
0. Exit
```

The six departments are Radiology, ICU, Cardiology, Operation Theatre, Laboratory and Emergency. Equipment type is Life Support, Imaging or Diagnostic. Status is Working, Under Maintenance or Unavailable.

## Build and run

With make (Linux, macOS, WSL, Git Bash):

```
make
make run
```

Without make (for example on Windows):

```
g++ -std=c++17 src/main.cpp src/Equipment.cpp src/FileManager.cpp src/EquipmentMenu.cpp src/MaintenanceManager.cpp src/SearchSort.cpp -o BioFix
cd data
../BioFix
```

The program reads and writes its .txt files in the folder it is started from, so always run it from inside `data/`. `make run` does this for you.

## Folder layout

| File | What it holds | Author |
|---|---|---|
| `src/main.cpp` | Program entry point and the main menu | Bhaskar Dev |
| `src/Equipment.h`, `src/Equipment.cpp` | `Equipment` base class, three subclasses, `createEquipment()` | Bhaskar Dev |
| `src/EquipmentList.h` | Linked list of equipment | Satyam Tripathy |
| `src/FileManager.h`, `src/FileManager.cpp` | Loading and saving `equipment.txt` and the six department files | Satyam Tripathy |
| `src/EquipmentMenu.h`, `src/EquipmentMenu.cpp` | Add, update, delete and department view screens | Satyam Tripathy |
| `src/MaintenanceQueue.h` | Linked queue of maintenance requests | Rajat Rathore |
| `src/MaintenanceManager.h`, `src/MaintenanceManager.cpp` | Saving and loading `maintenance.txt`, schedule and process options | Rajat Rathore |
| `src/SearchSort.h`, `src/SearchSort.cpp` | Linear search, binary search, insertion sort | Keshav Aggarwal |
| `tests/run_tests.sh` | Test script | Keshav Aggarwal |
| `data/` | Sample data: 18 equipment records, one maintenance request | Satyam Tripathy |

## Data files

`equipment.txt` is the master file. Each line is one machine, with fields separated by the `|` character:

```
ID|Name|Department|Type|Status
```

Every add, update or delete rewrites `equipment.txt` and the six department files (`radiology.txt`, `icu.txt`, `cardiology.txt`, `operation_theatre.txt`, `laboratory.txt`, `emergency.txt`) so they always match. `maintenance.txt` holds the queue, one request per line:

```
EquipmentID|Date|Description
```

## Tests

After building, run `bash tests/run_tests.sh`. It works on a temporary copy of `data/`, so your data files are not changed. It needs bash and the `timeout` command.

## Known problems

- Maintenance requests are served in the order they were scheduled. They are not yet ordered by priority.
- Typing letters at a prompt other than the main menu makes the program print the same error over and over.
- An equipment name that contains `|` breaks the record when the file is loaded again.
- Name search needs an exact match and is case sensitive.
- Processing a request removes it from the queue but does not change the equipment status or keep a maintenance history.

## Team

Bhaskar Dev (team lead), Rajat Rathore, Keshav Aggarwal, Satyam Tripathy.
Mentor: Ms. Priyanka Rastogi.

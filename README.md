# BioFix v5

BioFix is a command-line program, written in C++17, that keeps track of the biomedical equipment in a hospital and its maintenance. It was built for the Data Structure and Programming in C++ PBL project (Team ID DSCPP-III-2026-T173).

Equipment is stored by department, can be searched and sorted, and is saved to text files so the data is still there the next time the program runs. Maintenance requests wait in a priority queue, so the most urgent repair is always done first. Finished repairs are kept in a history, and machines can be ranked by how long they were out of service.

## What's new in v5

**Manual order.** Staff can pin requests and put them in the order they want. Pinned requests form a group at the top of the queue, in that order, and processing takes them first. Every other request keeps the normal priority order below them, so a new emergency still goes straight after the pinned group. The priority queue itself (levels, aging, the heap) is unchanged; the order rule just checks "pinned?" first.

- CLI: menu **18** (pin at a position, move inside the group, 0 to unpin, or request 0 to reset everything)
- Website: **Pin** on normal rows; **up / down** and **Unpin** on pinned rows; **Reset order** above the queue
- Saved in `maintenance.txt` (new column), so the order survives a restart; v4 files still load

## What's new in v4

| Change | Why |
|---|---|
| Priority queue is now a **binary min-heap** in an array | Adding and taking the next request are O(log n) instead of O(n); measured 76x faster at 20,000 requests |
| **Heap sort** gives the queue in order (screens, file) | O(n log n), reuses the heap's own sift-down |
| Same priority: **earlier date first**, then request number | An older repair isn't overtaken by a newer one of the same priority |
| **Aging**: a P3 request 7+ days overdue counts as P2 | Low-priority work can't wait forever; aging never reaches P1, which stays for life-critical work |
| **Complete any request** (menu 17, Done button on the website) | When the first request waits for a part, the next job can still be done |
| Hash table **rehashes** when more than 75% full | Lookups stay about O(1) at any size (fixed 101 buckets slowed down above ~100 machines) |
| **Benchmark program** (`make bench`) | Real measurements for the design choices |
| Aligned CLI tables (`setw`), confirmation before delete | Easier to read; no accidental deletes |
| Makefile notices header (`.h`) changes | v3 could keep stale code after editing a header |
| `BIOFIX_TODAY` setting for tests | Aging depends on the date, so tests fix the day |

## What the menu does

```
1. Add Equipment
2. Display All Equipment
3. View Department Equipment
4. Update Equipment
5. Delete Equipment
6. Search (ID, name, department, type, status)
7. Find by ID (hash table)
8. Sort by Name (Insertion Sort)
9. Schedule Maintenance
10. View Maintenance Queue (most urgent first)
11. Process Next Maintenance Request
12. View Repair History of a Machine
13. Downtime Ranking
14. Edit a Maintenance Request
15. Cancel a Maintenance Request
16. Change a Request's Priority
17. Complete a Specific Request
18. Change a Request's Position (manual order)
0. Exit
```

The six departments are Radiology, ICU, Cardiology, Operation Theatre, Laboratory and Emergency. Equipment type is Life Support, Imaging or Diagnostic. Status is Working, Under Maintenance or Unavailable.

## How priority works

There are three levels: **P1 Urgent**, **P2 High** and **P3 Normal**.

By default, each equipment type gives its own priority through the virtual function `getPriority()`:

| Type | Default priority |
|---|---|
| Life Support | P1 Urgent |
| Imaging | P2 High |
| Diagnostic | P3 Normal |

The priority can also be set **by hand**: when scheduling (website), or later for any waiting request (website edit panel, or menu 16). It can be set back to Auto at any time. Hand-set priorities are marked "manual" and are saved.

**Queue order:** (1) effective priority, (2) earlier date, (3) lower request number. When a priority or date changes, the request moves to its new place in the heap.

**Aging:** a P3 Normal request that is 7 or more days past its date counts as P2 High and is marked "aged". Aging never makes a request P1 Urgent: that level stays for Life Support machines or for staff who set it by hand. The stored priority doesn't change, so it goes back to normal if the date is edited. Scheduling a request sets the machine to Under Maintenance. Processing it asks for the completion date, records the repair in the history with its downtime in days, and sets the machine back to Working unless another request for it is still waiting.

## Status and the maintenance queue always agree

- **Under Maintenance** means a repair is waiting. It is set by scheduling a repair, not chosen by hand. New equipment starts as Working or Unavailable.
- Changing a machine with waiting repairs to **Working** or **Unavailable** cancels those repairs (the program asks first).
- Every request has its own number (#1, #2 ...). Any request can be **edited** (date, description) or **cancelled**. Cancelling the last request for a machine makes it Working again.
- **Processing** (menu 11) takes the first request in the queue. **Complete a specific request** (menu 17, or Done on the website) finishes any request, for example when the first one is waiting for a spare part.

## Data structures and algorithms

| What | Where | Why |
|---|---|---|
| Singly linked list with a tail pointer | `EquipmentList.h` | Holds all equipment; adding at the end is O(1) |
| Hash table with separate chaining and **rehashing** (load factor 0.75) | `EquipmentHash.h` | Finds equipment by ID in about one step, at any size |
| Priority queue as a **binary min-heap** in an array | `MaintenanceQueue.h` | Enqueue, dequeue and priority change in O(log n); peek O(1) |
| **Heap sort** | `MaintenanceQueue.h` (`sortedCopy`) | The queue in order for the screens and the file, O(n log n) |
| Stack | `RepairHistory.h` | Newest repair is shown first |
| Keyword search | `SearchSort.cpp` | One search box; ignores capital letters and finds part of a word |
| Insertion sort | `SearchSort.cpp`, `HistoryManager.cpp`, `MaintenanceManager.cpp` | Sort by name; downtime ranking; notifications by date |

## The website

BioFix also runs as a website on your own computer. The browser only shows the data; the C++ program keeps it in the same linked list, hash table, priority queue and history stack, and saves it to the same `.txt` files in `data/`.

The server only listens on `127.0.0.1` (this computer), answers one request at a time, and needs no extra libraries. It has no login, so don't make it reachable from other computers.

| Tab | What it does |
|---|---|
| Bell (top right) | Waiting requests that are overdue, due today, or due within 3/7/14/30 days, earliest date first. Click one to open it. |
| Dashboard | Counts, next urgent repairs, machines with most downtime |
| Equipment | Search, department filter, sort by name, find by ID (hash table), add, edit, delete |
| Maintenance | Schedule a repair, process the most urgent request, full queue |
| History | Finished repairs of one machine, newest first |
| Reports | Downtime ranking |

## Build and run

With make (Linux, macOS, WSL, Git Bash):

```
make
make run
```

`make` builds both programs: `build/BioFix` (command line) and `build/BioFixWeb` (website). `make web` starts the website; then open http://localhost:8080.

Without make (for example on Windows), from the project folder:

Command line version:
```
g++ -std=c++17 src/*.cpp -o BioFix
cd data
..\BioFix.exe
```

Website version (`-lws2_32` is the Windows network library):
```
g++ -std=c++17 src/Equipment.cpp src/EquipmentMenu.cpp src/FileManager.cpp src/HistoryManager.cpp src/InputHelper.cpp src/MaintenanceManager.cpp src/SearchSort.cpp web/HttpServer.cpp web/WebServer.cpp -o BioFixWeb -lws2_32
BioFixWeb.exe
```
Then open http://localhost:8080 in a browser. Stop it with Ctrl+C. On Linux or macOS, leave out `-lws2_32`.

The command line version must run inside `data/`. The website must be started from the project folder (it reads `web/index.html`, then works inside `data/` itself). Don't run both at the same time on the same data, or one may overwrite the other's changes.

## Folder layout

| File | What it holds | Module owner |
|---|---|---|
| `src/main.cpp` | Program entry point and the main menu | Bhaskar Dev |
| `src/Equipment.h`, `src/Equipment.cpp` | `Equipment` base class, three subclasses, `createEquipment()` | Bhaskar Dev |
| `src/InputHelper.h`, `src/InputHelper.cpp` | Safe number, text and date input | Bhaskar Dev |
| `src/RepairHistory.h` | Repair history stack | Bhaskar Dev |
| `src/HistoryManager.h`, `src/HistoryManager.cpp` | `history.txt`, history screen, downtime ranking | Bhaskar Dev |
| `src/EquipmentList.h` | Linked list of equipment | Satyam Tripathy |
| `src/EquipmentHash.h` | Hash table for ID lookup | Satyam Tripathy |
| `src/FileManager.h`, `src/FileManager.cpp` | Loading and saving `equipment.txt` and the six department files | Satyam Tripathy |
| `src/EquipmentMenu.h`, `src/EquipmentMenu.cpp` | Add, update, delete and department view screens | Satyam Tripathy |
| `src/MaintenanceQueue.h` | Priority queue of maintenance requests | Rajat Rathore |
| `src/MaintenanceManager.h`, `src/MaintenanceManager.cpp` | `maintenance.txt`, schedule and process options | Rajat Rathore |
| `src/SearchSort.h`, `src/SearchSort.cpp` | Keyword search, find by ID, insertion sort | Keshav Aggarwal |
| `src/ActionResult.h` | Result of a logic function (ok + message), shared by CLI and website | Bhaskar Dev |
| `web/HttpServer.h`, `web/HttpServer.cpp` | Small HTTP server (Windows and Linux) | Bhaskar Dev |
| `web/WebServer.cpp` | Website routes: connects the browser to the data structures | Bhaskar Dev |
| `web/index.html` | The web page (HTML, CSS, JavaScript) | Bhaskar Dev |
| `tests/run_tests.sh` | Command line tests (141 checks, incl. a heap stress test, rehashing, the removal bug and the manual order) | Keshav Aggarwal |
| `tests/queue_fuzz.cpp` | Random test of the heap: 120,000 steps compared with a simple list (`make fuzz`) | Keshav Aggarwal |
| `bench/benchmark.cpp` | Benchmark: hash table vs list, heap vs sorted list | Keshav Aggarwal |
| `tests/web_tests.sh` | Website tests (89 checks, needs curl) | Keshav Aggarwal |
| `data/` | Demo data: 50 machines, 16 waiting requests (3 in November), 34 past repairs | Satyam Tripathy |
| `tests/fixtures/` | Fixed test data: 18 machines, one request (used only by the tests) | Keshav Aggarwal |

## Data files

`equipment.txt` is the master file. Each line is one machine, with fields separated by the `|` character:

```
ID|Name|Department|Type|Status
```

Every add, update or delete rewrites `equipment.txt` and the six department files (`radiology.txt`, `icu.txt`, `cardiology.txt`, `operation_theatre.txt`, `laboratory.txt`, `emergency.txt`) so they always match. Damaged lines are skipped with a message instead of stopping the program.

`maintenance.txt` holds the waiting requests, most urgent first:

```
RequestNumber|EquipmentID|Date|Priority|auto-or-manual|ManualPosition|Description
```

ManualPosition is 0 for the normal order, or 1, 2, 3 ... for pinned requests.

Older files (without priority or request number) still load; they are saved in the new format on the next save.

`history.txt` holds the finished repairs, newest first. It is created after the first repair is processed:

```
EquipmentID|RequestDate|CompletedDate|DowntimeDays|Description
```

## Benchmark

`make bench` (or, without make: `g++ -std=c++17 -O2 src/Equipment.cpp src/EquipmentMenu.cpp src/FileManager.cpp src/HistoryManager.cpp src/InputHelper.cpp src/MaintenanceManager.cpp src/SearchSort.cpp bench/benchmark.cpp -o benchmark`) measures the two main design choices. Results on the development computer (Linux); your numbers will be different, but the pattern is the same:

```
BioFix benchmark (times in milliseconds)

1. Find every machine once by ID
Machines  Linked list       Hash table    Faster by
1000      1.72              0.00          474x
5000      48.89             0.04          1381x
20000     1826.00           0.25          7169x

2. Add every request, then take them all out in order
Requests  Sorted list       Heap          Faster by
1000      1.13              0.39          3x
5000      40.46             2.68          15x
20000     1115.72           14.66         76x

For tens of machines both ways are instant; the difference grows with size.
```

The benchmark also found a real problem: with a fixed 101 buckets, the hash table was only about 60x faster than the list, because each bucket's chain grew long. Adding rehashing made it thousands of times faster at large sizes.

## Tests

`make fuzz` runs a random test of the priority queue: thousands of random operations (add, take, change priority or date, remove one, remove all of a machine, new day) are done on the heap and on a plain sorted list, and every step must give the same order and answers. It found a real bug in `removeRequestsFor` that the normal tests missed (deleting a machine could leave one of its requests in the queue), so it stays in the project.

After building, run `bash tests/run_tests.sh` and `bash tests/web_tests.sh`. They set `BIOFIX_TODAY=2026-10-14` so aging gives the same result on any day. They use the fixed data in `tests/fixtures/` (copied to a temporary folder), so the demo data in `data/` can change without breaking the tests, and is never modified. It needs bash and the `timeout` command.

The demo data in `data/` is invented sample data for demonstration. It is not real hospital data.

## Running the tests on GitHub

`.github/workflows/tests.yml` makes GitHub Actions test the project after every push (see the **Actions** tab in the repository):

- **Linux job:** builds both programs and runs the command line tests, the website tests and `make fuzz`.
- **Windows job:** builds both programs with MinGW g++ (like on our laptops), runs the command line program once and checks that the website answers. This is the only automatic check of the Windows network code.

The workflow only works when this folder layout (`Makefile`, `src/`, `tests/`, `web/`, `data/`) is at the top of the repository.

`.gitattributes` keeps Unix line endings for `.sh` files and the `Makefile`. Without it, Git on Windows can change them to Windows line endings, and the test scripts then fail with `$'\r': command not found`.

## Limits

- One user at a time. Two copies (or the CLI and the website together) on the same files would overwrite each other's changes.
- The website has no login and serves only this computer.
- Data is kept in plain text files, with no backup and no login.
- Keyword search checks every record, which is fine for a hospital department but slow for very large data.
- Downtime is counted in whole days from the request date to the completion date.

## Team

Bhaskar Dev (team lead), Rajat Rathore, Keshav Aggarwal, Satyam Tripathy.
Mentor: Ms. Priyanka Rastogi.

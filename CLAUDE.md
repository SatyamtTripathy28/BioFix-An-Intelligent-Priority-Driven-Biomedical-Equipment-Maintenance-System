# BioFix v5: notes for Claude Code

## What this project is
BioFix is a C++17 command-line program for a 3rd-semester DS-CPP PBL project
(Team DSCPP-III-2026-T173, Graphic Era). It tracks hospital equipment and its
maintenance. Team: Bhaskar Dev (lead), Satyam Tripathy, Rajat Rathore, Keshav Aggarwal.

## How to explain things
The team are beginners. Explain in very simple words, with small examples.
Before big changes, say what you plan and why. Each member must be able to
explain their own module in a viva, so keep the code simple and readable.

## Build, run, test
- Build: `make` (or `g++ -std=c++17 src/*.cpp -o build/BioFix`)
- Run: `make run` (the program must run inside `data/` to find its .txt files)
- Website: `make web`, then open http://localhost:8080 (start from the project folder)
- Test: `bash tests/run_tests.sh` and `bash tests/web_tests.sh` (need bash; the web tests need curl).
  Tests set BIOFIX_TODAY=2026-10-14; todayDate() returns it when set.
- GitHub Actions: `.github/workflows/tests.yml` (Linux: all tests; Windows/MSYS2: build + smoke test).
  `.gitattributes` keeps .sh and Makefile as LF.
- Benchmark: `make bench`. Queue random test: `make fuzz` (run it after ANY change to MaintenanceQueue.h).
- All tests must pass after every change. Add a test for every new feature.
- Compile with `-Wall -Wextra` and keep zero warnings.

## Structure (module owner in brackets)
- `src/Equipment.h/.cpp`: abstract `Equipment`, 3 subclasses, virtual
  `getCategory()` and `getPriority()`, factory `createEquipment()` [Bhaskar]
- `src/InputHelper.*`: safe number, text and date input [Bhaskar]
- `src/RepairHistory.h`, `src/HistoryManager.*`: history stack, downtime ranking [Bhaskar]
- `src/main.cpp`: menu (options 0 to 18); calls queue.setToday(todayDate()) each loop [Bhaskar]
- `src/EquipmentList.h`: singly linked list with tail pointer [Satyam]
- `src/EquipmentHash.h`: hash table, separate chaining, starts at 101 buckets,
  rehashes to 2n+1 when load factor > 0.75 [Satyam]
- `src/FileManager.*`, `src/EquipmentMenu.*`: files, add/update/delete screens [Satyam]
- `src/MaintenanceQueue.h`: priority queue as a binary MIN-HEAP in an array.
  Order: pinned first (manualRank 1, 2, 3 ...), then effectivePriority, then date,
  then requestId. v5 manual order: moveToPosition() pins ONLY the chosen request
  inside the pinned group (max position = pinnedCount + 1 for a new pin);
  never pin other requests implicitly (it would freeze the queue). Aging: P3 that is 7+ days
  overdue counts as P2 (never P1). setToday() recomputes and heapifies.
  sortedCopy() = heap sort. Change entries only through changePriority /
  changeDetails / removeRequest / removeRequestsFor so the heap stays valid.
  removeRequestsFor filters the array, then heapifies (removing one by one in a
  loop is unsafe: the moved item can climb past the loop index). [Rajat]
- `src/MaintenanceManager.*`: schedule and process requests [Rajat]
- `src/SearchSort.*`: keyword search (linear), find by ID (hash), insertion sort [Keshav]
- `tests/run_tests.sh` + `tests/fixtures/` (fixed 18-machine test data) [Keshav]
- `data/`: demo data (50 machines, 16 waiting requests, 34 past repairs)
- `web/`: website. `HttpServer.*` is a tiny HTTP server (Winsock on Windows,
  needs `-lws2_32`); `WebServer.cpp` has the routes; `index.html` is the page.
  Logic lives in functions with no cin/cout (`addEquipmentRecord`,
  `scheduleRequest`, `completeNextRequest`, `computeRanking`, `matchesKeyword`),
  shared by the CLI and the website. Keep it that way.

## Rules
- Data files use `|` between fields; input helpers reject `|` in text.
- `EquipmentList` owns the Equipment objects; the hash table and arrays only hold pointers.
- The hash table must be updated in `add()` and `removeById()` (remove before delete).
- Tests use `tests/fixtures/`, never `data/`. Don't change fixtures without updating tests.
- Keep manual memory management (new/delete); the course teaches it.
- No new external libraries without asking the team.
- The web server must stay on 127.0.0.1. All data changes use POST.
- Status and queue must agree: "Under Maintenance" only via scheduling; setting
  Working/Unavailable cancels waiting requests; cancelling a machine's last
  request sets it Working. Requests have unique numbers (requestId).
- In index.html, always pass data through esc() before putting it in the page.

## Status
Done: bug fixes, hash table, keyword search, priority queue, repair history,
downtime ranking, date validation, website (5 tabs), edit/cancel one request, status rules, notification bell
(`dueRequests` + `/api/notifications`, sorted by date), priority by hand
(menu 16, `setRequestPriority`). v4: heap queue, aging, date tie-break,
complete any request (menu 17, `completeRequest`), rehashing, benchmark,
setw tables, delete confirmation. 141 CLI tests + 89 web + queue fuzz test (incl. manual order)
tests pass, AddressSanitizer clean. The Windows website build is not yet
confirmed on a real Windows machine.

## Ideas for next steps (ask the team which one)
1. Update the Phase-II slides and report to match this code.
2. Benchmark: time findById with 10,000 machines (list vs hash table).
3. Undo last delete/update with a stack, or a heap-based priority queue.
4. Aligned tables with setw, and a confirmation before delete.
5. Website: confirm the Windows build; add a mutex if it ever handles requests in parallel.

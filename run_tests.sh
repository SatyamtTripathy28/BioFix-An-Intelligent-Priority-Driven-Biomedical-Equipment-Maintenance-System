#!/bin/bash
# tests/run_tests.sh
# Author: Keshav Aggarwal
#
# Checks every menu option, the priority order, the repair history,
# the downtime ranking and the input fixes.
# Run it from the project root after building:
#
#     make
#     bash tests/run_tests.sh
#
# It uses its own fixed data in tests/fixtures/ (18 machines, one request),
# copied to a temporary folder, so data/ can change without breaking the tests.
# Needs bash and the "timeout" command (Linux, WSL or Git Bash).

# Aging depends on today's date, so the tests always pretend it is this day.
export BIOFIX_TODAY="2026-10-14"

ROOT="$(pwd)"
BIN="$ROOT/build/BioFix"

if [ ! -x "$BIN" ]; then
    echo "Build the program first: make"
    exit 1
fi

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

fresh() {                      # start every test from the fixed test data
    rm -f "$WORK"/*.txt
    cp "$ROOT"/tests/fixtures/*.txt "$WORK"/
}

run() {                        # run() "keys typed, one per line"
    (cd "$WORK" && printf "%b" "$1" | timeout 5 "$BIN" 2>&1 | head -c 200000)
}

PASS=0
FAIL=0

check() {                      # check "name" command...
    local name="$1"; shift
    if "$@"; then
        echo "PASS   $name"; PASS=$((PASS + 1))
    else
        echo "FAIL   $name"; FAIL=$((FAIL + 1))
    fi
}

contains() { grep -q -- "$2" <<< "$1"; }
lacks()    { ! grep -q -- "$2" <<< "$1"; }

# ------------------------------------------------------------ equipment

fresh
OUT=$(run "2\n0\n")
check "Load and display all equipment (18 records)" \
    test "$(grep -cE '^[0-9]{3}[[:space:]]' <<< "$OUT")" -eq 18

OUT=$(run "1\n119\nMammography Unit\n1\n2\n1\n0\n")
check "Add equipment: success message" contains "$OUT" "Equipment added successfully."
check "Add equipment: written to equipment.txt" grep -q "^119|Mammography Unit|Radiology|Imaging|Working" "$WORK/equipment.txt"
check "Add equipment: written to radiology.txt" grep -q "^119|" "$WORK/radiology.txt"

OUT=$(run "1\n101\n0\n")
check "Duplicate ID is rejected" contains "$OUT" "ID already exists."

OUT=$(run "1\n0\n0\n")
check "ID 0 is rejected" contains "$OUT" "ID must be greater than 0."

OUT=$(run "4\n119\nMammography Unit B\n0\n3\n0\n")
check "Update: success message" contains "$OUT" "Equipment updated successfully."
check "Update: name and status changed, department kept" \
    grep -q "^119|Mammography Unit B|Radiology|Imaging|Unavailable" "$WORK/equipment.txt" "$WORK/radiology.txt"

OUT=$(run "4\n119\nMammography Unit B\n0\n2\n0\n")
check "Update: Under Maintenance without a repair is refused" contains "$OUT" "schedule a repair for it instead"
check "Update: refused change is not saved" grep -q "^119|Mammography Unit B|Radiology|Imaging|Unavailable" "$WORK/equipment.txt"

OUT=$(run "1\n150\nSpare Monitor\n2\n3\n2\n0\n")
check "Add: status 2 is Unavailable (Under Maintenance is not offered)" \
    grep -q "^150|Spare Monitor|ICU|Diagnostic|Unavailable" "$WORK/equipment.txt"

OUT=$(run "4\n119\nMammography Unit B\n2\n0\n0\n")
check "Update: department can change (moved to ICU)" grep -q "^119|Mammography Unit B|ICU|" "$WORK/icu.txt"
check "Update: old department file no longer has it" bash -c "! grep -q '^119|' '$WORK/radiology.txt'"

OUT=$(run "4\n999\n0\n")
check "Update with a missing ID is reported" contains "$OUT" "Equipment not found."

OUT=$(run "5\n119\n1\n0\n")
check "Delete: success message" contains "$OUT" "Equipment deleted successfully."
check "Delete: removed from all files" \
    bash -c "! grep -q '^119|' '$WORK/equipment.txt' '$WORK/icu.txt' '$WORK/radiology.txt'"
OUT=$(run "5\n119\n0\n")
check "Delete with a missing ID is reported" contains "$OUT" "Equipment not found."
OUT=$(run "5\n101\n0\n0\n")
check "Delete asks first; 0 keeps it" contains "$OUT" "Nothing was deleted."
check "...still in the file" grep -q "^101|" "$WORK/equipment.txt"

# ------------------------------------------------------------ search and sort

fresh
OUT=$(run "6\n104\n0\n")
check "Search by exact ID" contains "$OUT" "Name: Ventilator"
OUT=$(run "6\nct scanner\n0\n")
check "Search ignores capital letters" contains "$OUT" "Name: CT Scanner"
OUT=$(run "6\nmri\n0\n")
check "Search finds part of a name" contains "$OUT" "Name: MRI Machine"
OUT=$(run "6\nicu\n0\n")
check "Search by department (ICU has 3)" contains "$OUT" "3 result(s) found."
OUT=$(run "6\npump\n0\n")
check "Search finds every match (2 pumps)" contains "$OUT" "2 result(s) found."
OUT=$(run "6\nzzzz\n0\n")
check "Search: nothing found is reported" contains "$OUT" "Equipment not found."

OUT=$(run "7\n116\n0\n")
check "Find by ID (hash table): existing ID" contains "$OUT" "Name: Operating Table"
OUT=$(run "7\n555\n0\n")
check "Find by ID (hash table): missing ID" contains "$OUT" "Equipment not found."

OUT=$(run "8\n0\n")
NAMES=$(sed -n '/SORTED BY NAME/,$p' <<< "$OUT" | grep -E '^[0-9]{3}[[:space:]]' | cut -c7-38 | sed 's/ *$//')
SORTED=$(LC_ALL=C sort <<< "$NAMES")
check "Sort by name: all 18 shown" test "$(wc -l <<< "$NAMES")" -eq 18
check "Sort by name: names are in order" test "$NAMES" = "$SORTED"

for n in 1 2 3 4 5 6; do
    OUT=$(run "3\n$n\n0\n0\n")
    check "Department view $n opens" contains "$OUT" "EQUIPMENT =========="
done
TOTAL=0
for f in radiology icu cardiology operation_theatre laboratory emergency; do
    TOTAL=$((TOTAL + $(wc -l < "$WORK/$f.txt")))
done
check "Department files add up to the master file" test "$TOTAL" -eq "$(wc -l < "$WORK/equipment.txt")"

# ------------------------------------------------------------ priority queue

# Sample data already has request 103 (Imaging, 2026-10-10).
# Add 105 (Diagnostic), 101 (Imaging), 104 (Life Support).
# Expected order: 104 (P1), 103 (P2, older), 101 (P2), 105 (P3).
fresh
OUT=$(run "9\n105\n2026-10-12\nMonitor check\n9\n101\n2026-10-12\nX-ray tube check\n9\n104\n2026-10-13\nVentilator alarm\n0\n")
check "Scheduling shows the priority" contains "$OUT" "Priority: P1 Urgent (from machine type)"
check "Scheduling sets status to Under Maintenance" grep -q "^104|Ventilator|ICU|Life Support|Under Maintenance" "$WORK/equipment.txt"

OUT=$(run "10\n0\n")
QUEUE=$(sed -n '/Description/,$p' <<< "$OUT" | grep -E '^#[0-9]+' | awk '{print $2}' | tr '\n' ' ')
check "Queue view: most urgent first (104 103 101 105)" test "$QUEUE" = "104 103 101 105 "

OUT=$(run "11\n2026-10-15\n11\n2026-10-15\n11\n2026-10-16\n11\n2026-10-16\n11\n0\n")
ORDER=$(grep "Equipment ID:" <<< "$OUT" | grep -oE '[0-9]+' | tr '\n' ' ')
check "Processing order follows priority (104 103 101 105)" test "$ORDER" = "104 103 101 105 "
check "Empty queue gives a message" contains "$OUT" "Maintenance queue is empty."
check "Processing sets status back to Working" grep -q "^104|Ventilator|ICU|Life Support|Working" "$WORK/equipment.txt"
check "Downtime is calculated (104: 2026-10-13 to 2026-10-15 = 2 days)" contains "$OUT" "Downtime: 2 day(s)."

# ------------------------------------------------------------ history and ranking

OUT=$(run "12\n104\n0\n")
check "History shows the finished repair" contains "$OUT" "Ventilator alarm"
check "History shows the total downtime" contains "$OUT" "1 repair(s), total downtime 2 day(s)."

OUT=$(run "13\n0\n")
FIRST=$(sed -n '/DOWNTIME RANKING/,$p' <<< "$OUT" | grep -E '^1[[:space:]]' | awk '{print $2}')
check "Ranking: most downtime first (103 had 5 days)" test "$FIRST" = "103"

OUT=$(run "12\n106\n0\n")
check "History of a machine with no repairs" contains "$OUT" "No finished repairs yet."

# ------------------------------------------------------------ validation and persistence

fresh
OUT=$(run "9\n104\n2026-13-40\n2026-10-20\nFan noise\n0\n")
check "Impossible date is rejected" contains "$OUT" "Invalid date."
OUT=$(run "11\n2026-10-01\n2026-10-21\n0\n")
check "Completion date before the request is rejected" contains "$OUT" "Completion date cannot be before"

fresh
run "9\n102\n2026-10-14\nCT calibration\n0\n" > /dev/null
OUT=$(run "10\n0\n")
check "Queue is still there after restarting" contains "$OUT" "CT calibration"

OUT=$(run "5\n102\n1\n0\n")
check "Deleting equipment also removes its waiting requests" contains "$OUT" "Also removed 1 waiting maintenance request(s)."
OUT=$(run "10\n0\n")
check "Queue no longer has the deleted machine's request" lacks "$OUT" "CT calibration"

OUT=$(run "9\n999\n0\n")
check "Scheduling with a missing ID is rejected" contains "$OUT" "Equipment not found."

OUT=$(run "99\n0\n")
check "Invalid menu number is handled" contains "$OUT" "Invalid choice."

# ------------------------------------------------------------ status and queue stay in agreement

fresh
OUT=$(run "4\n103\nMRI Machine\n0\n1\n0\n0\n")
check "Leaving maintenance asks first; 0 changes nothing" contains "$OUT" "Nothing was changed."
check "...and the request is still waiting" grep -q "MRI regular inspection" "$WORK/maintenance.txt"

OUT=$(run "4\n103\nMRI Machine\n0\n1\n1\n0\n")
check "Status Working cancels the waiting request" contains "$OUT" "Cancelled 1 waiting maintenance request(s)."
check "...machine is Working" grep -q "^103|MRI Machine|Radiology|Imaging|Working" "$WORK/equipment.txt"
OUT=$(run "10\n0\n")
check "...and the queue is empty" contains "$OUT" "Maintenance queue is empty."

# ------------------------------------------------------------ edit or cancel one request

fresh
run "2\n0\n" > /dev/null
check "Old maintenance.txt is saved with a request number" grep -q "^1|103|2026-10-10|2|auto|0|MRI regular inspection" "$WORK/maintenance.txt"

OUT=$(run "14\n1\n2026-10-11\nMRI full inspection\n0\n")
check "Edit one request (menu 14)" contains "$OUT" "Maintenance request #1 updated."
check "...new date and description are saved" grep -q "^1|103|2026-10-11|2|auto|0|MRI full inspection" "$WORK/maintenance.txt"
OUT=$(run "14\n77\n0\n")
check "Edit a missing request number" contains "$OUT" "Maintenance request not found."

run "9\n104\n2026-10-13\nVentilator alarm\n0\n" > /dev/null
OUT=$(run "15\n2\n0\n")
check "Cancel one request (menu 15)" contains "$OUT" "Maintenance request #2 cancelled."
check "...machine with no other request is Working again" contains "$OUT" "Ventilator is Working again."
check "...status saved as Working" grep -q "^104|Ventilator|ICU|Life Support|Working" "$WORK/equipment.txt"
check "...the other request is still waiting" grep -q "^1|103|" "$WORK/maintenance.txt"
OUT=$(run "15\n2\n0\n")
check "Cancel a missing request number" contains "$OUT" "Maintenance request not found."

run "9\n104\n2026-10-20\nFirst check\n9\n104\n2026-10-21\nSecond check\n0\n" > /dev/null
OUT=$(run "15\n3\n0\n")
check "Cancel one of two requests: machine stays Under Maintenance" lacks "$OUT" "Working again"
check "...status still Under Maintenance" grep -q "^104|Ventilator|ICU|Life Support|Under Maintenance" "$WORK/equipment.txt"

# ------------------------------------------------------------ change priority by hand (menu 16)

queue_order() { sed -n '/Description/,$p' <<< "$(run "10\n0\n")" | grep -E '^#[0-9]+' | awk '{print $2}' | tr '\n' ' '; }

fresh
run "9\n101\n2026-10-12\nX-ray tube\n9\n105\n2026-10-12\nMonitor check\n0\n" > /dev/null
check "Start: 103 (#1 P2), 101 (#2 P2), 105 (#3 P3)" test "$(queue_order)" = "103 101 105 "

OUT=$(run "16\n3\n1\n0\n")
check "Raise #3 to P1 by hand" contains "$OUT" "Request #3 is now P1 Urgent (set by hand)."
check "...it moves to the front" test "$(queue_order)" = "105 103 101 "
check "...saved as manual" grep -q "^3|105|2026-10-12|1|manual|0|Monitor check" "$WORK/maintenance.txt"
check "...queue view marks it (manual)" contains "$(run "10\n0\n")" "P1 Urgent (manual)"

run "16\n1\n3\n0\n" > /dev/null
check "Lower #1 to P3: it moves to the back" test "$(queue_order)" = "105 101 103 "
run "16\n1\n0\n0\n" > /dev/null
check "Back to Auto: #1 returns before #2 (arrival order kept)" test "$(queue_order)" = "105 103 101 "
check "...saved as auto again" grep -q "^1|103|2026-10-10|2|auto|0|" "$WORK/maintenance.txt"

OUT=$(run "11\n2026-10-15\n0\n")
check "Processing takes the hand-raised request first" contains "$OUT" "Equipment ID: 105"
check "...and shows it was set by hand" contains "$OUT" "Priority: P1 Urgent (set by hand)"

OUT=$(run "16\n99\n0\n")
check "Change priority of a missing request" contains "$OUT" "Maintenance request not found."
OUT=$(run "16\n1\n7\n2\n0\n")
check "Invalid priority choice is asked again" contains "$OUT" "Invalid choice. Try again."

# ------------------------------------------------------------ v4: date tie-break, aging, any request

fresh
run "9\n101\n2026-10-05\nOlder X-ray job\n0\n" > /dev/null
check "Same priority: earlier date first, even if scheduled later (101 before 103)" \
    test "$(queue_order)" = "101 103 "

fresh
run "9\n105\n2026-10-12\nMonitor check\n9\n101\n2026-10-13\nX-ray tube\n0\n" > /dev/null
check "Before aging: P2s first, then the P3 (103 101 105)" test "$(queue_order)" = "103 101 105 "
OUT=$(BIOFIX_TODAY=2026-10-25 run "10\n0\n")
check "13 days overdue: the P3 is shown as aged" contains "$OUT" "P2 High (aged)"
check "...and moves among the P2s by date (103 105 101)" \
    test "$(sed -n '/Description/,$p' <<< "$OUT" | grep -E '^#' | awk '{print $2}' | tr '\n' ' ')" = "103 105 101 "
OUT=$(BIOFIX_TODAY=2027-06-01 run "10\n0\n")
check "Aging never reaches P1 Urgent, even after months" lacks "$OUT" "P1 Urgent"
OUT=$(BIOFIX_TODAY=2026-10-25 run "11\n2026-10-25\n0\n")
check "Processing shows where an aged priority came from" lacks "$OUT" "aged from"

fresh
run "9\n101\n2026-10-12\nX-ray tube\n9\n105\n2026-10-12\nMonitor check\n0\n" > /dev/null
OUT=$(run "17\n3\n2026-10-15\n0\n")
check "Complete a specific request (menu 17), not the first one" contains "$OUT" "Request #3 completed. Downtime: 3 day(s)."
check "...it is in the history" contains "$(run "12\n105\n0\n")" "Monitor check"
check "...the others are still waiting, in order" test "$(queue_order)" = "103 101 "
check "...machine 105 is Working again" grep -q "^105|Patient Monitor|ICU|Diagnostic|Working" "$WORK/equipment.txt"
OUT=$(run "17\n42\n0\n")
check "Complete a missing request number" contains "$OUT" "Maintenance request not found."

# Heap stress test: 15 requests added in mixed order must come out sorted
# by (priority, date, request number), both in the view and when processed.
fresh
KEYS=""
for spec in "112 2026-10-09" "106 2026-10-03" "113 2026-10-03" "104 2026-10-11" "110 2026-10-01" \
            "102 2026-10-02" "107 2026-10-11" "111 2026-10-05" "118 2026-10-04" "109 2026-10-06" \
            "116 2026-10-08" "117 2026-10-02" "101 2026-10-07" "115 2026-10-09" "114 2026-10-01"; do
    set -- $spec
    KEYS="${KEYS}9\n$1\n$2\nStress $1\n"
done
run "${KEYS}0\n" > /dev/null
VIEW=$(sed -n '/Description/,$p' <<< "$(run "10\n0\n")" | grep -E '^#')
check "Stress: all 16 requests are in the queue" test "$(wc -l <<< "$VIEW")" -eq 16
# One line per request: priority digit, date, request number
KEYLIST=$(awk '{ for (i = 1; i <= NF; i++) if ($i ~ /^[0-9][0-9][0-9][0-9]-/) d = $i;
                 print substr($3, 2, 1), d, substr($1, 2) }' <<< "$VIEW")
check "Stress: queue view is in (priority, date, number) order" \
    bash -c "sort -c -k1,1n -k2,2 -k3,3n <<< \"$KEYLIST\""
PROC=""
for i in $(seq 1 16); do PROC="${PROC}11\n2026-10-20\n"; done
PROCESSED=$(run "${PROC}0\n" | grep "^Request: #" | grep -oE '[0-9]+' | tr '\n' ' ')
VIEWNUMS=$(awk '{print substr($1,2)}' <<< "$VIEW" | tr '\n' ' ')
check "Stress: processing order equals the queue view" test "$PROCESSED" = "$VIEWNUMS"
check "Stress: queue is empty at the end" contains "$(run "10\n0\n")" "Maintenance queue is empty."

# ------------------------------------------------------------ manual order (menu 18)
# Pinned requests form a group at the top in the order chosen by hand;
# all other requests keep the normal priority order below them.
fresh
run "9\n104\n2026-10-12\nVent\n9\n105\n2026-10-12\nMonitor\n0\n" > /dev/null
check "Manual order: start in normal order (104 103 105)" test "$(queue_order)" = "104 103 105 "

OUT=$(run "18\n3\n1\n0\n")
check "Pin #3 at position 1" contains "$OUT" "Request #3 is pinned at position 1."
check "...it is first, the rest keep their order (105 104 103)" test "$(queue_order)" = "105 104 103 "
check "...shown as pinned" contains "$(run "10\n0\n")" "P3 Normal (pinned)"
check "...saved with its position" grep -q "^3|105|2026-10-12|3|auto|1|Monitor" "$WORK/maintenance.txt"

OUT=$(run "18\n3\n0\n0\n")
check "Unpin #3: back in the normal order" contains "$OUT" "Request #3 is back in the normal priority order."
check "...order is normal again (104 103 105)" test "$(queue_order)" = "104 103 105 "
check "Unpin a request that is not pinned" contains "$(run "18\n3\n0\n0\n")" "is already in the normal order."

run "18\n1\n1\n18\n3\n2\n0\n" > /dev/null
check "Pin #1 first, then #3 second (103 105 104)" test "$(queue_order)" = "103 105 104 "
run "18\n3\n1\n0\n" > /dev/null
check "Move #3 up inside the pinned group (105 103 104)" test "$(queue_order)" = "105 103 104 "
check "...only the 2 chosen requests are pinned" test "$(grep -c "(pinned)" <<< "$(run "10\n0\n")")" -eq 2
run "9\n107\n2026-10-13\nDefib check\n0\n" > /dev/null
check "A new urgent request goes right after the pinned ones (105 103 104 107)" test "$(queue_order)" = "105 103 104 107 "

check "A new pin can only go up to the end of the pinned group" contains "$(run "18\n2\n4\n0\n")" "Position must be between 1 and 3"
check "A pinned request can only move inside the group" contains "$(run "18\n3\n3\n0\n")" "Position must be between 1 and 2"
check "Missing request number" contains "$(run "18\n99\n0\n")" "Maintenance request not found."

OUT=$(run "18\n0\n0\n")
check "Reset the whole order" contains "$OUT" "Order reset: 2 request(s) are back in the normal priority order."
check "...normal priority order again (104 107 103 105)" test "$(queue_order)" = "104 107 103 105 "
check "Reset when nothing is pinned" contains "$(run "18\n0\n0\n")" "No request has an order set by hand."

run "18\n3\n1\n0\n" > /dev/null
OUT=$(run "11\n2026-10-15\n0\n")
check "Processing takes the pinned request first (105)" contains "$OUT" "Equipment ID: 105"
check "...the others are in normal order (104 107 103)" test "$(queue_order)" = "104 107 103 "

fresh
printf "5|104|2026-10-12|1|auto|Vent check\n" > "$WORK/maintenance.txt"
run "2\n0\n" > /dev/null
check "An older maintenance.txt (no position column) loads and is saved in the new format" \
    grep -q "^5|104|2026-10-12|1|auto|0|Vent check" "$WORK/maintenance.txt"

# ------------------------------------------------------------ removing all requests of one machine
# Regression test for a heap bug: with these 6 requests, deleting machine 105 used to
# leave one of its requests behind (a moved request climbed past the loop's position).
# Everything runs in ONE session: a program that reloads the file builds a tidier heap
# and would not show the bug.
fresh
OUT=$(run "15\n1\n9\n105\n2026-10-08\nA\n9\n104\n2026-10-04\nB\n9\n104\n2026-10-06\nC\n9\n101\n2026-10-09\nD\n9\n101\n2026-10-08\nE\n9\n105\n2026-10-06\nF\n10\n5\n105\n1\n10\n0\n")
check "Delete a machine with 2 waiting requests: both are removed" contains "$OUT" "Also removed 2 waiting maintenance request(s)."
AFTER=$(sed -n '/Also removed/,$p' <<< "$OUT")
check "...none of its requests is left in the queue" test "$(grep -cE '^#[0-9]+ +105 ' <<< "$AFTER")" -eq 0
check "...the other 4 requests are still waiting" test "$(grep -cE '^#' <<< "$AFTER")" -eq 4
check "...maintenance.txt has no line for it" bash -c "! grep -q '|105|' '$WORK/maintenance.txt'"

# Same bug through the status rule: Working cancels all of a machine's requests.
fresh
OUT=$(run "15\n1\n9\n105\n2026-10-08\nA\n9\n104\n2026-10-04\nB\n9\n104\n2026-10-06\nC\n9\n101\n2026-10-09\nD\n9\n101\n2026-10-08\nE\n9\n105\n2026-10-06\nF\n4\n105\nPatient Monitor\n0\n1\n1\n10\n0\n")
check "Status Working cancels both requests of a machine" contains "$OUT" "Cancelled 2 waiting maintenance request(s)."
AFTER=$(sed -n '/Cancelled/,$p' <<< "$OUT")
check "...none of its requests is left in the queue" test "$(grep -cE '^#[0-9]+ +105 ' <<< "$AFTER")" -eq 0

# ------------------------------------------------------------ hash table rehashing
# 18 + 100 machines is more than 0.75 x 101 buckets, so the table must grow (rehash).
fresh
ADDS=""
for i in $(seq 200 299); do ADDS="${ADDS}1\n$i\nBulk machine $i\n1\n3\n1\n"; done
OUT=$(run "${ADDS}0\n")
check "Rehash: 100 machines added" test "$(grep -c "Equipment added successfully." <<< "$OUT")" -eq 100
OUT=$(run "7\n250\n7\n299\n7\n101\n7\n118\n0\n")
check "Rehash: new machines are found by ID" test "$(grep -c "Name: Bulk machine" <<< "$OUT")" -eq 2
check "Rehash: old machines are still found by ID" contains "$OUT" "Name: Portable Oxygen Concentrator"
OUT=$(run "5\n250\n1\n7\n250\n7\n251\n0\n")
check "Rehash: a deleted machine is gone, its neighbour is still there" \
    bash -c "grep -q 'Equipment not found.' <<< \"\$0\" && grep -q 'Name: Bulk machine 251' <<< \"\$0\"" "$OUT"
check "Rehash: duplicate IDs are still rejected" contains "$(run "1\n299\n0\n")" "ID already exists."

# ------------------------------------------------------------ old bugs, now fixed

fresh
OUT=$(run "abc\n0\n")
check "Letters at the main menu are rejected" contains "$OUT" "Please enter a number."

OUT=$(run "1\nabc\n130\nTest Pump\n1\n1\n1\n0\n")
check "Letters at a sub-prompt: asked again, no endless loop" contains "$OUT" "Equipment added successfully."

OUT=$(run "1\n131\nBad|Name\nGood Name\n1\n1\n1\n0\n")
check "A | in a name is rejected" contains "$OUT" "Text cannot contain the | character."
check "The corrected name is saved" grep -q "^131|Good Name|" "$WORK/equipment.txt"

(cd "$WORK" && printf "2\n" | timeout 5 "$BIN" > /dev/null 2>&1)
check "Input ending without 0 does not hang" test $? -ne 124

fresh
printf "xyz|Bad Line|ICU|Imaging|Working\n140|Robot|ICU|Robot Arm|Working\n" >> "$WORK/equipment.txt"
OUT=$(run "2\n0\n")
check "Damaged lines in equipment.txt are skipped" contains "$OUT" "Skipping line"
check "The 18 good records still load" \
    test "$(grep -cE '^[0-9]{3}[[:space:]]' <<< "$OUT")" -eq 18

fresh
printf "9|104|2026-99-99|Broken date\n" >> "$WORK/maintenance.txt"
OUT=$(run "10\n0\n")
check "A request with a broken date is skipped, not crashing" contains "$OUT" "bad date"
check "...the good request still loads" contains "$OUT" "MRI regular inspection"

# ------------------------------------------------------------ demo data in data/

DEMO="$(mktemp -d)"
cp "$ROOT"/data/*.txt "$DEMO"/
OUT=$(cd "$DEMO" && printf "2\n10\n13\n0\n" | timeout 5 "$BIN" 2>&1)
rm -rf "$DEMO"
check "Demo data loads with no skipped lines" lacks "$OUT" "Skipping"
check "Demo data: all 50 machines load" \
    test "$(sed -n '1,/Thank you/p' <<< "$OUT" | grep -cE '^[0-9]{3}[[:space:]]+[A-Za-z]')" -ge 50
check "Demo data: queue and ranking have entries" contains "$OUT" "DOWNTIME RANKING"

echo
echo "Passed: $PASS   Failed: $FAIL"
[ "$FAIL" -eq 0 ]

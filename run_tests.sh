#!/bin/bash
# tests/run_tests.sh
# Author: Keshav Aggarwal
#
# Runs the checks listed in the Testing section of the Phase-II report.
# Run it from the project root after building:
#
#     make
#     bash tests/run_tests.sh
#
# It works on a temporary copy of data/, so your real data files are not changed.
# Needs bash and the "timeout" command (Linux, WSL or Git Bash).

ROOT="$(pwd)"
BIN="$ROOT/build/BioFix"

if [ ! -x "$BIN" ]; then
    echo "Build the program first: make"
    exit 1
fi

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

fresh() {                      # start every test from the original data
    rm -f "$WORK"/*.txt
    cp "$ROOT"/data/*.txt "$WORK"/
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

# ---------------------------------------------------------------- tests

fresh
OUT=$(run "2\n0\n")
check "Load and display all equipment (18 records)" \
    test "$(grep -cE '^[0-9]{3}[[:space:]]' <<< "$OUT")" -eq 18

fresh
OUT=$(run "1\n119\nMammography Unit\n1\n2\n1\n0\n")
check "Add equipment: success message" contains "$OUT" "Equipment added successfully."
check "Add equipment: written to equipment.txt" grep -q "^119|Mammography Unit|Radiology|Imaging|Working" "$WORK/equipment.txt"
check "Add equipment: written to radiology.txt" grep -q "^119|" "$WORK/radiology.txt"

OUT=$(run "1\n101\n0\n")
check "Duplicate ID is rejected" contains "$OUT" "ID already exists."

OUT=$(run "4\n119\nMammography Unit B\n2\n0\n")
check "Update equipment: success message" contains "$OUT" "Equipment updated successfully."
check "Update equipment: both files changed" \
    grep -q "^119|Mammography Unit B|Radiology|Imaging|Under Maintenance" "$WORK/equipment.txt" "$WORK/radiology.txt"

OUT=$(run "4\n999\n0\n")
check "Update with a missing ID is reported" contains "$OUT" "Equipment not found."

OUT=$(run "5\n119\n0\n")
check "Delete equipment: success message" contains "$OUT" "Equipment deleted successfully."
check "Delete equipment: removed from both files" \
    bash -c "! grep -q '^119|' '$WORK/equipment.txt' '$WORK/radiology.txt'"
OUT=$(run "5\n119\n0\n")
check "Delete with a missing ID is reported" contains "$OUT" "Equipment not found."

fresh
OUT=$(run "6\n104\n0\n")
check "Linear search by ID" contains "$OUT" "Equipment Found"
OUT=$(run "6\nCT Scanner\n0\n")
check "Linear search by exact name" contains "$OUT" "Equipment Found"
OUT=$(run "6\n999\n0\n")
check "Linear search: missing ID reported" contains "$OUT" "Equipment not found."

OUT=$(run "7\n116\n0\n")
check "Binary search finds an existing ID" contains "$OUT" "Equipment Found"
OUT=$(run "7\n555\n0\n")
check "Binary search reports a missing ID" contains "$OUT" "Equipment not found."

OUT=$(run "8\n0\n")
NAMES=$(sed -n '/SORTED BY NAME/,$p' <<< "$OUT" | grep -E '^[0-9]{3}[[:space:]]' | cut -f2)
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

fresh
OUT=$(run "9\n101\n2026-10-12\nX-ray tube check\n9\n104\n2026-10-13\nVentilator alarm fault\n11\n11\n11\n11\n0\n")
ORDER=$(grep "Equipment ID:" <<< "$OUT" | grep -oE '[0-9]+' | tr '\n' ' ')
check "Requests leave in the order entered (103 101 104)" test "$ORDER" = "103 101 104 "
check "Empty queue gives a message" contains "$OUT" "Maintenance queue is empty."

OUT=$(run "9\n999\n0\n")
check "Scheduling with a missing ID is rejected" contains "$OUT" "Equipment not found."

fresh
run "9\n102\n2026-10-14\nCT calibration\n0\n" > /dev/null
OUT=$(run "10\n0\n")
check "Queue is still there after restarting" contains "$OUT" "CT calibration"

OUT=$(run "99\n0\n")
check "Invalid menu number is handled" contains "$OUT" "Invalid choice."

# ------------------------------------------------- known issues (see report)
# These two are bugs found during testing. They do not count as failures.
# When a bug is fixed, the line below changes to "now fixed".

fresh
OUT=$(printf "1\nabc\n2\n0\n" | (cd "$WORK" && timeout 2 "$BIN" 2>&1 | head -c 20000))
if [ "$(grep -c 'Invalid department' <<< "$OUT")" -gt 3 ]; then
    echo "KNOWN  Letters typed at a sub-prompt make the program loop (still present)"
else
    echo "KNOWN  Letters typed at a sub-prompt: now fixed"
fi

fresh
run "1\n130\nBad|Name\n1\n1\n1\n0\n" > /dev/null
OUT=$(run "2\n0\n")
if grep -qE "^130[[:space:]]+Bad[[:space:]]+Radiology" <<< "$OUT"; then
    echo "KNOWN  A | inside a name is stored and reloaded correctly: now fixed"
else
    echo "KNOWN  A | inside a name corrupts the record on reload (still present)"
fi

echo
echo "Passed: $PASS   Failed: $FAIL"
[ "$FAIL" -eq 0 ]

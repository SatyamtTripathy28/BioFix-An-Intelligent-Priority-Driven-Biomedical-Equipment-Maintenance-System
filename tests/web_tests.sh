#!/bin/bash
# tests/web_tests.sh
# Starts the website on a temporary copy of tests/fixtures/ and checks every
# address the web page uses. Run from the project root after "make":
#
#     bash tests/web_tests.sh
#
# Needs bash and curl (Linux, WSL or Git Bash).

# Aging depends on today's date, so the tests always pretend it is this day.
export BIOFIX_TODAY="2026-10-14"

ROOT="$(pwd)"
BIN="$ROOT/build/BioFixWeb"
PORT=18080
URL="http://127.0.0.1:$PORT"

if [ ! -x "$BIN" ]; then
    echo "Build the program first: make"
    exit 1
fi

WORK="$(mktemp -d)"
mkdir -p "$WORK/web" "$WORK/data"
cp "$ROOT/web/index.html" "$WORK/web/"
cp "$ROOT"/tests/fixtures/*.txt "$WORK/data/"

(cd "$WORK" && exec "$BIN" "$PORT" > "$WORK/server.log" 2>&1) &
SERVER=$!
trap 'kill $SERVER 2>/dev/null; rm -rf "$WORK"' EXIT

for i in $(seq 1 50); do                     # wait until the server answers
    curl -s "$URL/api/summary" > /dev/null && break
    sleep 0.1
done

# If our server could not start (for example the port is busy), stop here,
# so we never test some other program by mistake.
if ! kill -0 "$SERVER" 2>/dev/null; then
    echo "The test server did not start:"
    cat "$WORK/server.log"
    exit 1
fi

PASS=0
FAIL=0
check() {
    local name="$1"; shift
    if "$@"; then echo "PASS   $name"; PASS=$((PASS + 1))
    else echo "FAIL   $name"; FAIL=$((FAIL + 1)); fi
}
contains() { grep -q -- "$2" <<< "$1"; }
lacks()    { ! grep -q -- "$2" <<< "$1"; }
count()    { grep -o -- "$2" <<< "$1" | wc -l; }
get()      { curl -s "$URL$1"; }
post()     { curl -s -X POST "$URL$1" "${@:2}"; }
code()     { curl -s -o /dev/null -w "%{http_code}" "$@"; }

# ------------------------------------------------------------ pages and reading
OUT=$(get "/")
check "Home page is served" contains "$OUT" "<title>BioFix</title>"
check "Home page has the light/dark mode button" contains "$OUT" 'id="theme-toggle"'
check "Home page has the notification bell" contains "$OUT" 'id="bell"'

# The fixture queue has one request: 103, date 2026-10-10.
OUT=$(get "/api/notifications?days=7&today=2026-10-05")
check "Notification: due in 5 days" contains "$OUT" '"daysLeft":5'
check "Notification: count is 1" contains "$OUT" '"count":1,'
OUT=$(get "/api/notifications?days=7&today=2026-10-01")
check "Notification: 9 days away is outside a 7-day window" contains "$OUT" '"count":0,'
OUT=$(get "/api/notifications?days=3&today=2026-10-12")
check "Notification: overdue by 2 days" contains "$OUT" '"daysLeft":-2'
check "Notification: overdue is counted" contains "$OUT" '"overdue":1,'
OUT=$(get "/api/notifications?today=nonsense")
check "Notification: a bad date falls back to the program's date" contains "$OUT" "\"today\":\"$BIOFIX_TODAY\""

OUT=$(get "/api/summary")
check "Summary: 18 machines, 1 waiting request" contains "$OUT" '"machines":18,'
check "Summary: waiting count" contains "$OUT" '"waiting":1,'

OUT=$(get "/api/equipment")
check "Equipment list has 18 machines" test "$(count "$OUT" '"id":')" -eq 18
OUT=$(get "/api/equipment?search=icu")
check "Search 'icu' finds 3" test "$(count "$OUT" '"id":')" -eq 3
OUT=$(get "/api/equipment?search=PUMP")
check "Search ignores capital letters (2 pumps)" test "$(count "$OUT" '"id":')" -eq 2
OUT=$(get "/api/equipment?department=Cardiology")
check "Department filter (Cardiology has 3)" test "$(count "$OUT" '"id":')" -eq 3
OUT=$(get "/api/equipment?sort=name")
check "Sort by name: first is Anaesthesia Machine" contains "${OUT:0:60}" "Anaesthesia Machine"

OUT=$(get "/api/find?id=104")
check "Find by ID (hash table)" contains "$OUT" '"name":"Ventilator"'
check "Find a missing ID gives 404" test "$(code "$URL/api/find?id=555")" = "404"

# ------------------------------------------------------------ equipment changes
OUT=$(post "/api/equipment/add" -d id=119 --data-urlencode "name=Mammography Unit" -d department=Radiology -d type=Imaging -d status=Working)
check "Add equipment" contains "$OUT" "Equipment added successfully."
check "Add is saved to equipment.txt" grep -q "^119|Mammography Unit|Radiology|Imaging|Working" "$WORK/data/equipment.txt"
check "Add is saved to radiology.txt" grep -q "^119|" "$WORK/data/radiology.txt"

check "Duplicate ID is rejected (400)" test "$(code -X POST "$URL/api/equipment/add" -d id=101 -d name=X -d department=ICU -d type=Imaging -d status=Working)" = "400"
OUT=$(post "/api/equipment/add" -d id=120 --data-urlencode "name=Bad|Name" -d department=ICU -d type=Imaging -d status=Working)
check "A | in the name is rejected" contains "$OUT" "without the | character"
OUT=$(post "/api/equipment/add" -d id=121 -d name=Thing -d department=Kitchen -d type=Imaging -d status=Working)
check "Unknown department is rejected" contains "$OUT" "Invalid department."
OUT=$(post "/api/equipment/add" -d id=abc -d name=Thing -d department=ICU -d type=Imaging -d status=Working)
check "Letters as ID are rejected" contains "$OUT" "ID must be greater than 0."

OUT=$(post "/api/equipment/update" -d id=119 --data-urlencode "name=Mammography Unit B" -d department=ICU -d status=Working)
check "Update equipment" contains "$OUT" "Equipment updated successfully."
check "Update moves it to icu.txt" grep -q "^119|Mammography Unit B|ICU|" "$WORK/data/icu.txt"

# ------------------------------------------------------------ maintenance
OUT=$(post "/api/maintenance/schedule" -d id=104 -d date=2026-10-13 --data-urlencode "description=Ventilator alarm")
check "Schedule maintenance shows priority" contains "$OUT" "P1 Urgent (from machine type)"
OUT=$(get "/api/queue")
check "Life Support request goes to the front" contains "${OUT:0:40}" '"id":104'
check "Status becomes Under Maintenance" grep -q "^104|Ventilator|ICU|Life Support|Under Maintenance" "$WORK/data/equipment.txt"

OUT=$(post "/api/maintenance/schedule" -d id=104 -d date=2026-02-30 -d description=X)
check "Impossible date is rejected" contains "$OUT" "Invalid date."

OUT=$(post "/api/maintenance/process" -d completed=2026-10-01)
check "Completion before the request is rejected" contains "$OUT" "cannot be before"
OUT=$(post "/api/maintenance/process" -d completed=2026-10-15)
check "Process next: downtime 2 days" contains "$OUT" "Downtime: 2 day(s)."
check "Status back to Working" grep -q "^104|Ventilator|ICU|Life Support|Working" "$WORK/data/equipment.txt"

OUT=$(get "/api/history?id=104")
check "History shows the repair" contains "$OUT" '"repairs":1,"totalDays":2'
OUT=$(get "/api/ranking")
check "Ranking includes the machine" contains "$OUT" '"id":104'

# ------------------------------------------------------------ delete
post "/api/maintenance/schedule" -d id=119 -d date=2026-10-20 -d description=Check > /dev/null
OUT=$(post "/api/equipment/delete" -d id=119)
check "Delete also removes its waiting request" contains "$OUT" "Also removed 1 waiting maintenance request(s)."
OUT=$(get "/api/queue")
check "Queue no longer has the deleted machine" lacks "$OUT" '"id":119'

# ------------------------------------------------------------ status rules and one request
OUT=$(get "/api/find?id=103")
check "Equipment shows how many repairs are waiting" contains "$OUT" '"waiting":1'
OUT=$(get "/api/queue")
check "Queue rows have a request number" contains "$OUT" '"requestId":1,"id":103'

OUT=$(post "/api/equipment/add" -d id=140 -d name=Pump -d department=ICU -d type=Imaging --data-urlencode "status=Under Maintenance")
check "New equipment can't start Under Maintenance" contains "$OUT" "schedule a repair"
OUT=$(post "/api/equipment/update" -d id=101 --data-urlencode "name=X-Ray Machine" -d department=Radiology --data-urlencode "status=Under Maintenance")
check "Under Maintenance without a repair is refused" contains "$OUT" "schedule a repair for it instead"

OUT=$(post "/api/maintenance/update" -d requestId=1 -d date=2026-10-11 --data-urlencode "description=MRI full inspection")
check "Edit one request" contains "$OUT" "Maintenance request #1 updated."
check "...saved to maintenance.txt" grep -q "^1|103|2026-10-11|2|auto|0|MRI full inspection" "$WORK/data/maintenance.txt"
OUT=$(post "/api/maintenance/update" -d requestId=1 -d date=2026-99-99 -d description=X)
check "Edit with a bad date is refused" contains "$OUT" "Invalid date."

OUT=$(post "/api/equipment/update" -d id=103 --data-urlencode "name=MRI Machine" -d department=Radiology -d status=Working)
check "Status Working cancels waiting requests" contains "$OUT" "Cancelled 1 waiting maintenance request(s)."
check "...queue no longer has 103" lacks "$(get /api/queue)" '"id":103'

OUT=$(post "/api/maintenance/schedule" -d id=101 -d date=2026-10-20 -d description=Check)
NUM=$(get "/api/queue" | grep -oE '"requestId":[0-9]+,"id":101' | grep -oE '[0-9]+' | head -1)
OUT=$(post "/api/maintenance/cancel" -d requestId=$NUM)
check "Cancel one request" contains "$OUT" "cancelled."
check "...machine is Working again" contains "$OUT" "X-Ray Machine is Working again."
check "Cancel a missing request is refused (400)" test "$(code -X POST "$URL/api/maintenance/cancel" -d requestId=999)" = "400"

# ------------------------------------------------------------ priority by hand
OUT=$(post "/api/maintenance/schedule" -d id=110 -d date=2026-11-20 -d description=Blood-check -d priority=1)
check "Schedule with a priority chosen by hand" contains "$OUT" "P1 Urgent (set by hand)"
NUM=$(get "/api/queue" | grep -oE '"requestId":[0-9]+,"id":110' | grep -oE '[0-9]+' | head -1)
check "Queue shows it as manual" contains "$(get /api/queue)" "\"requestId\":$NUM,\"id\":110,\"name\":\"Blood Analyzer\",\"priority\":1,\"priorityLabel\":\"P1 Urgent\",\"basePriority\":1,\"aged\":false,\"manual\":true"
OUT=$(post "/api/maintenance/priority" -d requestId=$NUM -d priority=auto)
check "Change priority back to Auto (Diagnostic = P3)" contains "$OUT" "P3 Normal (from machine type)"
OUT=$(post "/api/maintenance/update" -d requestId=$NUM -d date=2026-11-21 -d description=Blood-check -d priority=2)
check "Edit form can change date and priority together" contains "$OUT" "is now P2 High (set by hand)"
check "...saved to the file" grep -q "^$NUM|110|2026-11-21|2|manual|0|Blood-check" "$WORK/data/maintenance.txt"
check "Bad priority is refused (400)" test "$(code -X POST "$URL/api/maintenance/priority" -d requestId=$NUM -d priority=9)" = "400"
check "Schedule with a bad priority is refused (400)" test "$(code -X POST "$URL/api/maintenance/schedule" -d id=111 -d date=2026-11-20 -d description=X -d priority=x)" = "400"
post "/api/maintenance/cancel" -d requestId=$NUM > /dev/null

# ------------------------------------------------------------ v4: aging and completing any request
# BIOFIX_TODAY is 2026-10-14, so a P3 request dated 2026-09-30 is 14 days overdue.
post "/api/maintenance/schedule" -d id=111 -d date=2026-09-30 -d description=Old-lens-check > /dev/null
OUT=$(get "/api/queue")
check "Aged request: shown as P2 with base P3" contains "$OUT" '"id":111,"name":"Microscope","priority":2,"priorityLabel":"P2 High","basePriority":3,"aged":true'
NUM=$(grep -oE '"requestId":[0-9]+,"id":111' <<< "$OUT" | grep -oE '[0-9]+' | head -1)
OUT=$(post "/api/maintenance/complete" -d requestId=$NUM -d completed=2026-10-14)
check "Complete a specific request" contains "$OUT" "Request #$NUM completed. Downtime: 14 day(s)."
check "...it is no longer waiting" lacks "$(get /api/queue)" '"id":111'
check "...and is in its history" contains "$(get "/api/history?id=111")" "Old-lens-check"
check "Complete a missing request is refused (400)" test "$(code -X POST "$URL/api/maintenance/complete" -d requestId=999 -d completed=2026-10-14)" = "400"

# ------------------------------------------------------------ removing all requests of one machine
# Regression test for a heap bug: with these 6 requests (all in one server session),
# deleting machine 106 used to leave one of its requests behind.
# 106 = ECG Machine (Diagnostic, P3), 107 = Defibrillator (Life Support, P1), 102 = CT Scanner (Imaging, P2).
# These machines are not used by other tests, and the queue is emptied again at the end.
for spec in "106 2026-10-08" "107 2026-10-04" "107 2026-10-06" "102 2026-10-09" "102 2026-10-08" "106 2026-10-06"; do
    set -- $spec
    post "/api/maintenance/schedule" -d id=$1 -d date=$2 -d description=Heap-test > /dev/null
done
check "Heap bug test: 6 requests are waiting" test "$(count "$(get /api/queue)" '"requestId":')" -eq 6
OUT=$(post "/api/equipment/delete" -d id=106)
check "Delete a machine with 2 waiting requests: both are removed" contains "$OUT" "Also removed 2 waiting maintenance request(s)."
OUT=$(get "/api/queue")
check "...none of its requests is left in the queue" lacks "$OUT" '"id":106,'
check "...the other 4 are still waiting" test "$(count "$OUT" '"requestId":')" -eq 4
for number in $(grep -oE '"requestId":[0-9]+' <<< "$OUT" | grep -oE '[0-9]+'); do     # clean up
    post "/api/maintenance/cancel" -d requestId=$number > /dev/null
done
check "Cleanup: the queue is empty again" test "$(count "$(get /api/queue)" '"requestId":')" -eq 0

# ------------------------------------------------------------ manual order
# 108 Anaesthesia Machine (P1), 113 Ultrasound Machine (P2), 109 Surgical Monitor (P3)
for spec in "108 2026-10-12" "113 2026-10-12" "109 2026-10-12"; do
    set -- $spec
    post "/api/maintenance/schedule" -d id=$1 -d date=$2 -d description=Order-test > /dev/null
done
order() { get "/api/queue" | grep -oE '"id":1[0-9][0-9],' | grep -oE '[0-9]+' | tr '\n' ' '; }
num()   { get "/api/queue" | grep -oE "\"requestId\":[0-9]+,\"id\":$1," | grep -oE '[0-9]+' | head -1; }
check "Manual order: normal order first (108 113 109)" test "$(order)" = "108 113 109 "

OUT=$(post "/api/maintenance/position" -d requestId=$(num 109) -d position=1)
check "Pin a request at position 1" contains "$OUT" "is pinned at position 1."
check "...it is first (109 108 113)" test "$(order)" = "109 108 113 "
check "...the queue marks it pinned at position 1" contains "$(get /api/queue)" '"pinned":true,"position":1,'

OUT=$(post "/api/maintenance/unpin" -d requestId=$(num 109))
check "Unpin puts it back" contains "$OUT" "back in the normal priority order."
check "...normal order again (108 113 109)" test "$(order)" = "108 113 109 "

post "/api/maintenance/position" -d requestId=$(num 113) -d position=1 > /dev/null
post "/api/maintenance/position" -d requestId=$(num 109) -d position=2 > /dev/null
check "Pin two requests (113 109 108)" test "$(order)" = "113 109 108 "
post "/api/maintenance/position" -d requestId=$(num 109) -d position=1 > /dev/null
check "Move up inside the pinned group (109 113 108)" test "$(order)" = "109 113 108 "
check "A new pin can only go to the end of the group (400)" test "$(code -X POST "$URL/api/maintenance/position" -d requestId=$(num 108) -d position=4)" = "400"
check "A pinned request can only move inside the group (400)" test "$(code -X POST "$URL/api/maintenance/position" -d requestId=$(num 113) -d position=3)" = "400"
check "Unknown request is refused (400)" test "$(code -X POST "$URL/api/maintenance/position" -d requestId=999 -d position=1)" = "400"

OUT=$(post "/api/maintenance/reset-order")
check "Reset order" contains "$OUT" "Order reset: 2 request(s)"
check "...normal order again" test "$(order)" = "108 113 109 "

for id in 108 113 109; do post "/api/maintenance/cancel" -d requestId=$(num $id) > /dev/null; done
check "Cleanup: the queue is empty again" test "$(count "$(get /api/queue)" '"requestId":')" -eq 0

# ------------------------------------------------------------ notifications are sorted by date
post "/api/maintenance/schedule" -d id=101 -d date=2027-01-03 -d description=Later-P2 > /dev/null
post "/api/maintenance/schedule" -d id=105 -d date=2027-01-02 -d description=First > /dev/null
post "/api/maintenance/schedule" -d id=104 -d date=2027-01-03 -d description=Later-P1 > /dev/null
OUT=$(get "/api/notifications?days=10&today=2026-12-31")
ORDER=$(grep -oE '"id":[0-9]+' <<< "$OUT" | grep -oE '[0-9]+' | tr '\n' ' ')
check "Notifications: earliest date first, same date most urgent first (105 104 101)" test "$ORDER" = "105 104 101 "

# ------------------------------------------------------------ safety
post "/api/equipment/add" -d id=130 --data-urlencode "name=<script>alert(1)</script>" -d department=ICU -d type=Imaging -d status=Working > /dev/null
OUT=$(get "/api/equipment?search=alert")
check "HTML in a name is escaped in JSON" lacks "$OUT" "<script>"
check "Unknown address gives 404" test "$(code "$URL/api/nothing")" = "404"
check "Changing data needs POST (GET is refused)" test "$(code "$URL/api/equipment/delete?id=101")" = "404"
BIG=$(head -c 70000 /dev/zero | tr '\0' 'a')
check "Very large request is refused (413)" test "$(code -X POST "$URL/api/equipment/add" --data "name=$BIG")" = "413"
printf 'garbage\r\n\r\n' > /dev/tcp/127.0.0.1/$PORT 2>/dev/null
check "Server still works after a broken request" contains "$(get /api/summary)" '"machines"'
check "Start message tells the address" contains "$(cat "$WORK/server.log")" "http://localhost:$PORT"
if [ -r /proc/net/tcp ]; then                 # Linux: look at the real listening address
    HEXPORT=$(printf '%04X' "$PORT")
    check "Server listens only on this computer (127.0.0.1)" \
        grep -qi "0100007F:$HEXPORT 00000000:0000 0A" /proc/net/tcp
fi

echo
echo "Passed: $PASS   Failed: $FAIL"
[ "$FAIL" -eq 0 ]

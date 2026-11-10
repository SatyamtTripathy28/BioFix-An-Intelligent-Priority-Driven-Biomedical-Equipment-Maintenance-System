// InputHelper.cpp
// Module owner: Bhaskar Dev

#include <iostream>
#include <cctype>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include "InputHelper.h"

// Removes spaces (and a Windows '\r') from both ends of a line.
string trim(const string& s) {
    size_t start = 0;
    while (start < s.size() && isspace((unsigned char)s[start]))
        start++;

    size_t end = s.size();
    while (end > start && isspace((unsigned char)s[end - 1]))
        end--;

    return s.substr(start, end - start);
}

bool isValidText(const string& s) {
    if (s.empty() || s.size() > 60)
        return false;

    for (char c : s)
        if (c == '|' || c == '\n' || c == '\r')
            return false;

    return true;
}

bool inputEnded() {
    return cin.eof();
}

bool isNumber(const string& s) {
    if (s.empty())
        return false;

    for (char c : s)
        if (!isdigit((unsigned char)c))
            return false;

    return true;
}

int readInt(const string& prompt) {
    string line;

    while (true) {
        cout << prompt;

        if (!getline(cin >> ws, line))      // no more input
            return 0;

        line = trim(line);

        if (isNumber(line) && line.size() <= 9)
            return stoi(line);

        cout << "Please enter a number.\n";
    }
}

string readText(const string& prompt) {
    string line;

    while (true) {
        cout << prompt;

        if (!getline(cin >> ws, line))      // no more input
            return "";

        line = trim(line);

        if (line.find('|') != string::npos) {
            cout << "Text cannot contain the | character.\n";
            continue;
        }

        if (!line.empty())
            return line;
    }
}

static int daysInMonth(int year, int month) {
    int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    if (month == 2 && leap)
        return 29;

    return days[month - 1];
}

bool isValidDate(const string& date) {
    if (date.size() != 10 || date[4] != '-' || date[7] != '-')
        return false;

    for (int i = 0; i < 10; i++) {
        if (i == 4 || i == 7)
            continue;
        if (!isdigit((unsigned char)date[i]))
            return false;
    }

    int year = stoi(date.substr(0, 4));
    int month = stoi(date.substr(5, 2));
    int day = stoi(date.substr(8, 2));

    if (year < 2000 || year > 2100)
        return false;
    if (month < 1 || month > 12)
        return false;
    if (day < 1 || day > daysInMonth(year, month))
        return false;

    return true;
}

string readDate(const string& prompt) {
    while (true) {
        string date = readText(prompt);

        if (date.empty())                   // no more input
            return "";

        if (isValidDate(date))
            return date;

        cout << "Invalid date. Use YYYY-MM-DD, for example 2026-10-21.\n";
    }
}

// Turns a valid date into a time value (midday avoids daylight-saving problems).
static time_t toTime(const string& date) {
    tm t = {};
    t.tm_year = stoi(date.substr(0, 4)) - 1900;
    t.tm_mon = stoi(date.substr(5, 2)) - 1;
    t.tm_mday = stoi(date.substr(8, 2));
    t.tm_hour = 12;
    t.tm_isdst = -1;
    return mktime(&t);
}

int daysBetween(const string& from, const string& to) {
    double seconds = difftime(toTime(to), toTime(from));
    return (int)lround(seconds / 86400.0);
}

string todayDate() {
    // For testing: BIOFIX_TODAY=2026-10-14 makes the program act as if it is that day.
    const char* fixed = getenv("BIOFIX_TODAY");
    if (fixed != nullptr && isValidDate(fixed))
        return fixed;

    time_t now = time(nullptr);
    tm* t = localtime(&now);

    char text[11];
    strftime(text, sizeof(text), "%Y-%m-%d", t);
    return text;
}

string toLower(string s) {
    for (char& c : s)
        c = (char)tolower((unsigned char)c);
    return s;
}

bool containsText(const string& text, const string& word) {
    return toLower(text).find(toLower(word)) != string::npos;
}

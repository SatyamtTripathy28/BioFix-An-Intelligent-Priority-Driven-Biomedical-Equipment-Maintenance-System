// InputHelper.h
// Safe input functions used by every screen.
// They fix two old bugs: letters typed at a number prompt made the program
// loop forever, and a '|' typed in a name broke the saved files.
// Module owner: Bhaskar Dev

#ifndef BIOFIX_INPUTHELPER_H
#define BIOFIX_INPUTHELPER_H

#include <string>
using namespace std;

// Asks again until a whole number is typed. Returns 0 if the input has ended.
int readInt(const string& prompt);

// Asks again until some text without '|' is typed. Returns "" if the input has ended.
string readText(const string& prompt);

// Asks again until a real date in the form YYYY-MM-DD is typed. Returns "" if the input has ended.
string readDate(const string& prompt);

bool inputEnded();                                   // true after the last line of input
string trim(const string& s);                        // removes spaces at both ends
bool isValidText(const string& s);                   // not empty, no '|', at most 60 characters
bool isNumber(const string& s);                      // only digits, at least one
bool isValidDate(const string& date);                // YYYY-MM-DD and a real calendar day
int daysBetween(const string& from, const string& to);   // number of days from 'from' to 'to'
string todayDate();                                  // this computer's date, YYYY-MM-DD
string toLower(string s);
bool containsText(const string& text, const string& word); // ignores capital letters

#endif

// ActionResult.h
// What a logic function returns: did it work, and a message to show.
// The CLI prints the message; the website sends it back to the browser.

#ifndef BIOFIX_ACTIONRESULT_H
#define BIOFIX_ACTIONRESULT_H

#include <string>
using namespace std;

struct ActionResult {
    bool ok;
    string message;
};

#endif

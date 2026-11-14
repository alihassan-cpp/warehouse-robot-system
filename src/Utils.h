#ifndef UTILS_H
#define UTILS_H

#include <string>
using namespace std;

// Small helper functions used by every module.

// Removes spaces and tabs from both ends of a string
string trim(const string& text);

// Returns an uppercase copy, so "l01" and "L01" are treated the same
string toUpper(const string& text);

// Checks an ID such as "L01" or "RK04": the prefix followed by at least one digit
bool isValidId(const string& id, const string& prefix);

// Compares two IDs with the same prefix by their number, so "L9" comes before "L10"
bool idLess(const string& a, const string& b);

// Converts text to an int; returns false if it is not a clean whole number
bool parseInt(const string& text, int& result);

// Menu input helpers. They keep asking until the input is valid,
// so bad input can never crash the program.
int readInt(const string& prompt, int minValue, int maxValue);
string readLine(const string& prompt);
string readId(const string& prompt);

#endif

#include "Utils.h"
#include "UI.h"
#include <iostream>
#include <cstdlib>
#include <sstream>

string trim(const string& text) {
    int start = 0;
    int end = (int)text.length() - 1;
    while (start <= end && (text[start] == ' ' || text[start] == '\t' || text[start] == '\r')) {
        start++;
    }
    while (end >= start && (text[end] == ' ' || text[end] == '\t' || text[end] == '\r')) {
        end--;
    }
    return text.substr(start, end - start + 1);
}

string toUpper(const string& text) {
    string result = text;
    for (int i = 0; i < (int)result.length(); i++) {
        if (result[i] >= 'a' && result[i] <= 'z') {
            result[i] = result[i] - 'a' + 'A';
        }
    }
    return result;
}

bool isValidId(const string& id, const string& prefix) {
    int prefixLength = (int)prefix.length();

    // Needs the prefix plus at least one digit
    if ((int)id.length() <= prefixLength) {
        return false;
    }
    if (id.substr(0, prefixLength) != prefix) {
        return false;
    }
    for (int i = prefixLength; i < (int)id.length(); i++) {
        if (id[i] < '0' || id[i] > '9') {
            return false;
        }
    }
    return true;
}

bool idLess(const string& a, const string& b) {
    // A shorter ID has a smaller number (L9 < L10), otherwise compare normally
    if (a.length() != b.length()) {
        return a.length() < b.length();
    }
    return a < b;
}

bool parseInt(const string& text, int& result) {
    string clean = trim(text);
    if (clean.empty() || clean.length() > 9) {
        return false;
    }

    int start = 0;
    bool negative = false;
    if (clean[0] == '-') {
        negative = true;
        start = 1;
        if (clean.length() == 1) {
            return false;
        }
    }

    int value = 0;
    for (int i = start; i < (int)clean.length(); i++) {
        if (clean[i] < '0' || clean[i] > '9') {
            return false;
        }
        value = value * 10 + (clean[i] - '0');
    }

    if (negative) {
        result = -value;
    } else {
        result = value;
    }
    return true;
}

bool parseDouble(const string& text, double& result) {
    string clean = trim(text);
    if (clean.empty()) {
        return false;
    }

    stringstream stream(clean);
    double value;
    stream >> value;
    if (stream.fail()) {
        return false;
    }

    // Anything left over (like "2.5kg") means the input was not a clean number
    char extra;
    if (stream >> extra) {
        return false;
    }

    result = value;
    return true;
}

string readLine(const string& prompt) {
    cout << UI::paint(prompt, "36");
    string line;

    // If input ends completely (for example Ctrl+Z), exit cleanly instead of looping forever
    if (!getline(cin, line)) {
        cout << "\nInput stream closed. Exiting program.\n";
        exit(0);
    }
    return trim(line);
}

int readInt(const string& prompt, int minValue, int maxValue) {
    while (true) {
        string line = readLine(prompt);
        int value;
        if (!parseInt(line, value)) {
            cout << UI::err() << "Invalid input. Please enter a whole number.\n";
        } else if (value < minValue || value > maxValue) {
            cout << "  Please enter a number between " << minValue << " and " << maxValue << ".\n";
        } else {
            return value;
        }
    }
}

double readDouble(const string& prompt, double minValue, double maxValue) {
    while (true) {
        string line = readLine(prompt);
        double value;
        if (!parseDouble(line, value)) {
            cout << UI::err() << "Invalid input. Please enter a number (e.g. 2.5).\n";
        } else if (value < minValue || value > maxValue) {
            cout << "  Please enter a value between " << minValue << " and " << maxValue << ".\n";
        } else {
            return value;
        }
    }
}

string readId(const string& prompt) {
    return toUpper(readLine(prompt));
}

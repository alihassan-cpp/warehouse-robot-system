#ifndef UI_H
#define UI_H
#include <iostream>
#include <string>
using namespace std;
class UI {
public:
    static bool useColor() {
        return true;
    }
    static string start(const string& code) {
        if (!useColor()) {
            return "";
        }
        return "\033[" + code + "m";
    }
    static string reset() {
        if (!useColor()) {
            return "";
        }
        return "\033[0m";
    }
    static string paint(const string& text, const string& code) {
        return start(code) + text + reset();
    }
    static void clear() {
        if (useColor()) {
            cout << "\033[2J\033[H";
        }
    }
    static string err() {
        return "  " + paint("ERROR", "1;31") + "  ";
    }
    static string good() {
        return "  " + paint("OK", "1;32") + "     ";
    }
    static string note() {
        return "  " + paint("NOTE", "1;33") + "   ";
    }
    static string statusColor(const string& status) {
        if (status == "Available" || status == "Completed") {
            return "32";
        }
        if (status == "Pending" || status == "Reserved" || status == "Partially Fulfilled" || status == "Waiting for Charging") {
            return "33";
        }
        if (status == "Assigned" || status == "Charging") {
            return "36";
        }
        if (status == "Unavailable" || status == "Cancelled") {
            return "31";
        }
        return "0";
    }
    static void header(const string& title) {
        int width = 50;
        int space = width - 2 - (int)title.size();
        if (space < 0) {
            space = 0;
        }
        int left = space / 2;
        int right = space - left;
        cout << "\n";
        cout << paint("+" + string(width - 2, '-') + "+", "36") << "\n";
        cout << paint("|", "36") << string(left, ' ') << paint(title, "1;97") << string(right, ' ') << paint("|", "36") << "\n";
        cout << paint("+" + string(width - 2, '-') + "+", "36") << "\n";
    }
    static void banner(const string& title) {
        int width = 50;
        int space = width - (int)title.size();
        if (space < 0) {
            space = 0;
        }
        int left = space / 2;
        cout << paint(string(width, '='), "35") << "\n";
        cout << string(left, ' ') << paint(title, "1;95") << "\n";
        cout << paint(string(width, '='), "35") << "\n";
    }
    static void option(int number, const string& text) {
        string label = "[" + to_string(number) + "]";
        if (number < 10) {
            label = " " + label;
        }
        if (number == 0) {
            cout << "  " << paint(label, "31") << " " << paint(text, "90") << "\n";
        } else {
            cout << "  " << paint(label, "1;33") << " " << text << "\n";
        }
    }
};
#endif

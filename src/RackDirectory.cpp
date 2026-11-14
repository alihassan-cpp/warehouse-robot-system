#include "RackDirectory.h"
#include "Utils.h"
#include <iostream>
#include <fstream>
#include <sstream>

RackInfo::RackInfo(const string& rack, const string& location) {
    rackId = rack;
    locationId = location;
    next = nullptr;
}

RackDirectory::RackDirectory() {
    head = nullptr;
    tail = nullptr;
    count = 0;
}

RackDirectory::~RackDirectory() {
    RackInfo* current = head;
    while (current != nullptr) {
        RackInfo* temp = current;
        current = current->next;
        delete temp;
    }
}

bool RackDirectory::load(const string& fileName, const Graph& warehouse) {
    ifstream file(fileName);
    if (!file.is_open()) {
        cout << "Error: could not open " << fileName << "\n";
        return false;
    }

    string line;
    int lineNumber = 0;
    int loaded = 0;
    while (getline(file, line)) {
        lineNumber++;
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        stringstream parts(line);
        string rackId, locationId;
        getline(parts, rackId, ',');
        getline(parts, locationId);
        rackId = toUpper(trim(rackId));
        locationId = toUpper(trim(locationId));

        if (addRack(rackId, locationId, warehouse)) {
            loaded++;
        } else {
            cout << "  (" << fileName << " line " << lineNumber << " skipped)\n";
        }
    }

    cout << "Loaded " << loaded << " racks from " << fileName << ".\n";
    return true;
}

bool RackDirectory::addRack(const string& rackId, const string& locationId, const Graph& warehouse) {
    if (!isValidId(rackId, "RK")) {
        cout << "  Invalid rack ID '" << rackId << "'. It must look like RK01.\n";
        return false;
    }
    if (find(rackId) != nullptr) {
        cout << "  Rack " << rackId << " already exists.\n";
        return false;
    }

    Location* location = warehouse.findLocation(locationId);
    if (location == nullptr) {
        cout << "  Location " << locationId << " does not exist.\n";
        return false;
    }
    if (location->type != "RACK") {
        cout << "  Location " << locationId << " is not a RACK location.\n";
        return false;
    }

    RackInfo* info = new RackInfo(rackId, locationId);
    if (head == nullptr) {
        head = info;
        tail = info;
    } else {
        tail->next = info;
        tail = info;
    }
    count++;
    return true;
}

RackInfo* RackDirectory::find(const string& rackId) const {
    RackInfo* current = head;
    while (current != nullptr) {
        if (current->rackId == rackId) {
            return current;
        }
        current = current->next;
    }
    return nullptr;
}

string RackDirectory::locationOf(const string& rackId) const {
    RackInfo* info = find(rackId);
    if (info == nullptr) {
        return "";
    }
    return info->locationId;
}

void RackDirectory::display() const {
    cout << "\n  Racks and their locations:\n";
    RackInfo* current = head;
    while (current != nullptr) {
        cout << "  " << current->rackId << " -> " << current->locationId << "\n";
        current = current->next;
    }
}

int RackDirectory::size() const {
    return count;
}

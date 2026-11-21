#include "RobotFleet.h"
#include "UI.h"
#include "Utils.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

RobotFleet::RobotFleet() {
    head = nullptr;
    tail = nullptr;
    robotCount = 0;
}

RobotFleet::~RobotFleet() {
    // Robot has a virtual destructor, so this correctly deletes Pickers, Carriers, etc.
    Robot* current = head;
    while (current != nullptr) {
        Robot* temp = current;
        current = current->next;
        delete temp;
    }
}

// ---------------------------------------------------------------
// Loading and registering
// ---------------------------------------------------------------

bool RobotFleet::loadRobots(const string& fileName, const Graph& warehouse) {
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

        // Split "RB01,PICKER,L01,85,10"
        stringstream parts(line);
        string id, type, location, batteryText, capacityText;
        getline(parts, id, ',');
        getline(parts, type, ',');
        getline(parts, location, ',');
        getline(parts, batteryText, ',');
        getline(parts, capacityText);

        double battery, capacity;
        if (!parseDouble(batteryText, battery) || !parseDouble(capacityText, capacity)) {
            cout << "  " << fileName << " line " << lineNumber << ": battery or capacity is not a number, skipped.\n";
            continue;
        }

        type = toUpper(trim(type));
        Robot* robot = createRobot(type, toUpper(trim(id)), toUpper(trim(location)), battery, capacity);
        if (robot == nullptr) {
            cout << "  " << fileName << " line " << lineNumber << ": unknown robot type '" << type << "', skipped.\n";
            continue;
        }

        if (registerRobot(robot, warehouse)) {
            loaded++;
        } else {
            delete robot;
            cout << "  (" << fileName << " line " << lineNumber << " skipped)\n";
        }
    }

    cout << "Loaded " << loaded << " robots from " << fileName << ".\n";
    return true;
}

bool RobotFleet::registerRobot(Robot* robot, const Graph& warehouse) {
    if (!isValidId(robot->id, "RB")) {
        cout << UI::err() << "Invalid robot ID '" << robot->id << "'. It must look like RB01.\n";
        return false;
    }
    if (findRobot(robot->id) != nullptr) {
        cout << UI::err() << "Robot " << robot->id << " already exists.\n";
        return false;
    }
    if (warehouse.findLocation(robot->locationId) == nullptr) {
        cout << UI::err() << "Location " << robot->locationId << " does not exist.\n";
        return false;
    }
    if (robot->battery < 0 || robot->battery > 100) {
        cout << UI::err() << "Battery must be between 0 and 100.\n";
        return false;
    }
    // Each type checks its own limit (virtual call)
    if (!robot->isCapacityValid()) {
        cout << "  Robot " << robot->id << ": capacity " << robot->capacity
             << " kg is outside the " << robot->typeName() << " limit.\n";
        return false;
    }

    if (head == nullptr) {
        head = robot;
        tail = robot;
    } else {
        tail->next = robot;
        tail = robot;
    }
    robotCount++;
    return true;
}

Robot* RobotFleet::findRobot(const string& robotId) const {
    Robot* current = head;
    while (current != nullptr) {
        if (current->id == robotId) {
            return current;
        }
        current = current->next;
    }
    return nullptr;
}

// ---------------------------------------------------------------
// Displays
// ---------------------------------------------------------------

void RobotFleet::displayHeader() const {
    cout << "\n  " << UI::start("1;36") << left << setw(6) << "Robot" << setw(12) << "Type" << setw(10) << "Location"
         << right << setw(8) << "Battery" << setw(10) << "Capacity" << "   "
         << left << setw(22) << "Status" << "Task" << UI::reset() << "\n";
    cout << UI::start("90") << "  ----------------------------------------------------------------------------" << UI::reset() << "\n";
}

void RobotFleet::displayRow(const Robot* robot) const {
    cout << "  " << left << setw(6) << robot->id << setw(12) << robot->typeName()
         << setw(10) << robot->locationId
         << right << setw(7) << fixed << setprecision(1) << robot->battery << "%"
         << setw(7) << setprecision(1) << robot->capacity << " kg   "
         << left << UI::start(UI::statusColor(robotStatusToString(robot->status))) << setw(22) << robotStatusToString(robot->status) << UI::reset();
    if (robot->hasActiveTask()) {
        cout << robot->assignedTask;
    } else {
        cout << "-";
    }
    if (robot->needsCharging()) {
        cout << "  " << UI::paint("[needs charging]", "1;31");
    }
    cout << "\n";
}

void RobotFleet::displayAll() const {
    if (head == nullptr) {
        cout << "  No robots in the fleet.\n";
        return;
    }
    displayHeader();
    Robot* current = head;
    while (current != nullptr) {
        displayRow(current);
        current = current->next;
    }
    cout << "  (" << robotCount << " robots)\n";
}

void RobotFleet::displayAvailable() const {
    int found = 0;
    displayHeader();
    Robot* current = head;
    while (current != nullptr) {
        if (current->status == RobotStatus::Available) {
            displayRow(current);
            found++;
        }
        current = current->next;
    }
    if (found == 0) {
        cout << "  No robots are available right now.\n";
    }
}

void RobotFleet::displayRobot(const string& robotId) const {
    Robot* robot = findRobot(robotId);
    if (robot == nullptr) {
        cout << UI::err() << "Robot " << robotId << " not found.\n";
        return;
    }
    displayHeader();
    displayRow(robot);
    cout << "  Battery rate: " << setprecision(2) << robot->batteryRate() << "% per distance unit\n";
}

void RobotFleet::displayNeedingCharge() const {
    int found = 0;
    cout << "\n  Robots with battery below 20%:\n";
    Robot* current = head;
    while (current != nullptr) {
        if (current->needsCharging()) {
            displayRow(current);
            found++;
        }
        current = current->next;
    }
    if (found == 0) {
        cout << "  None.\n";
    }
}

// ---------------------------------------------------------------
// Updates and assignments
// ---------------------------------------------------------------

bool RobotFleet::updateBattery(const string& robotId, double newBattery) {
    Robot* robot = findRobot(robotId);
    if (robot == nullptr) {
        cout << UI::err() << "Robot " << robotId << " not found.\n";
        return false;
    }
    if (newBattery < 0 || newBattery > 100) {
        cout << UI::err() << "Battery must be between 0 and 100.\n";
        return false;
    }
    robot->battery = newBattery;
    if (robot->needsCharging()) {
        cout << UI::note() << "Note: " << robotId << " is below 20% and needs charging.\n";
    }
    return true;
}

bool RobotFleet::assignTask(const string& robotId, const string& taskId) {
    Robot* robot = findRobot(robotId);
    if (robot == nullptr) {
        cout << UI::err() << "Robot " << robotId << " not found.\n";
        return false;
    }
    // One robot, one active assignment
    if (robot->hasActiveTask()) {
        cout << UI::err() << "Robot " << robotId << " already has an active assignment (" << robot->assignedTask << ").\n";
        return false;
    }
    if (robot->status != RobotStatus::Available) {
        cout << "  Robot " << robotId << " is " << robotStatusToString(robot->status) << ", not Available.\n";
        return false;
    }

    robot->assignedTask = taskId;
    robot->status = RobotStatus::Assigned;
    return true;
}

bool RobotFleet::releaseRobot(const string& robotId) {
    Robot* robot = findRobot(robotId);
    if (robot == nullptr) {
        cout << UI::err() << "Robot " << robotId << " not found.\n";
        return false;
    }
    if (!robot->hasActiveTask()) {
        cout << "  Robot " << robotId << " has no active task.\n";
        return false;
    }

    robot->assignedTask = "";
    robot->status = RobotStatus::Available;
    if (robot->needsCharging()) {
        cout << "  Note: " << robotId << " battery is " << fixed << setprecision(1)
             << robot->battery << "%, it needs charging.\n";
    }
    return true;
}

bool RobotFleet::setUnavailable(const string& robotId, RobotStatus& oldStatus) {
    Robot* robot = findRobot(robotId);
    if (robot == nullptr) {
        cout << UI::err() << "Robot " << robotId << " not found.\n";
        return false;
    }
    if (robot->hasActiveTask()) {
        cout << UI::err() << "Robot " << robotId << " has an active task and cannot go to maintenance.\n";
        return false;
    }
    if (robot->status == RobotStatus::Charging || robot->status == RobotStatus::WaitingForCharging) {
        cout << UI::err() << "Robot " << robotId << " is in the charging system and cannot go to maintenance.\n";
        return false;
    }
    if (robot->status == RobotStatus::Unavailable) {
        cout << UI::err() << "Robot " << robotId << " is already in maintenance.\n";
        return false;
    }

    oldStatus = robot->status;
    robot->status = RobotStatus::Unavailable;
    return true;
}

bool RobotFleet::returnFromMaintenance(const string& robotId, RobotStatus& oldStatus) {
    Robot* robot = findRobot(robotId);
    if (robot == nullptr) {
        cout << UI::err() << "Robot " << robotId << " not found.\n";
        return false;
    }
    if (robot->status != RobotStatus::Unavailable) {
        cout << "  Robot " << robotId << " is not in maintenance.\n";
        return false;
    }

    oldStatus = robot->status;
    robot->status = RobotStatus::Available;
    return true;
}

int RobotFleet::size() const {
    return robotCount;
}

Robot* RobotFleet::getHead() const {
    return head;
}

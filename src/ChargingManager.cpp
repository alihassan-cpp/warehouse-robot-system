#include "ChargingManager.h"
#include "Utils.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

// ---------------------------------------------------------------
// ChargingStation
// ---------------------------------------------------------------

ChargingStation::ChargingStation(const string& stationId, const string& location, int numberOfSlots) {
    id = stationId;
    locationId = location;
    slotCount = numberOfSlots;
    slots = new Robot*[slotCount];
    for (int i = 0; i < slotCount; i++) {
        slots[i] = nullptr;
    }
    next = nullptr;
}

ChargingStation::~ChargingStation() {
    // Only the array is ours; the robots belong to the fleet
    delete[] slots;
}

int ChargingStation::freeSlots() const {
    int free = 0;
    for (int i = 0; i < slotCount; i++) {
        if (slots[i] == nullptr) {
            free++;
        }
    }
    return free;
}

int ChargingStation::slotOf(const Robot* robot) const {
    for (int i = 0; i < slotCount; i++) {
        if (slots[i] == robot) {
            return i;
        }
    }
    return -1;
}

bool ChargingStation::startCharging(Robot* robot) {
    for (int i = 0; i < slotCount; i++) {
        if (slots[i] == nullptr) {
            slots[i] = robot;
            robot->status = RobotStatus::Charging;
            robot->locationId = locationId;
            return true;
        }
    }
    return false;
}

void ChargingStation::display() const {
    cout << "  " << id << " at " << locationId << ": " << slotCount << " slot(s), "
         << freeSlots() << " free\n";

    cout << "     Charging: ";
    bool any = false;
    for (int i = 0; i < slotCount; i++) {
        if (slots[i] != nullptr) {
            cout << slots[i]->id << " (slot " << (i + 1) << ") ";
            any = true;
        }
    }
    if (!any) {
        cout << "none";
    }

    cout << "\n     Queue:    Front -> ";
    QueueNode<Robot*>* node = waiting.getFront();
    while (node != nullptr) {
        cout << node->data->id << " -> ";
        node = node->next;
    }
    cout << "NULL\n";
}

// ---------------------------------------------------------------
// ChargingManager: setup
// ---------------------------------------------------------------

ChargingManager::ChargingManager() {
    head = nullptr;
    tail = nullptr;
    stationCount = 0;
}

ChargingManager::~ChargingManager() {
    ChargingStation* current = head;
    while (current != nullptr) {
        ChargingStation* temp = current;
        current = current->next;
        delete temp;
    }
}

bool ChargingManager::load(const string& fileName, const Graph& warehouse) {
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

        // Split "C01,L04,1"
        stringstream parts(line);
        string id, location, slotsText;
        getline(parts, id, ',');
        getline(parts, location, ',');
        getline(parts, slotsText);

        int slots;
        if (!parseInt(slotsText, slots)) {
            cout << "  " << fileName << " line " << lineNumber << ": slots is not a number, skipped.\n";
            continue;
        }
        if (addStation(toUpper(trim(id)), toUpper(trim(location)), slots, warehouse)) {
            loaded++;
        } else {
            cout << "  (" << fileName << " line " << lineNumber << " skipped)\n";
        }
    }

    cout << "Loaded " << loaded << " charging stations from " << fileName << ".\n";
    return true;
}

bool ChargingManager::addStation(const string& id, const string& locationId, int slots, const Graph& warehouse) {
    if (!isValidId(id, "C")) {
        cout << "  Invalid station ID '" << id << "'. It must look like C01.\n";
        return false;
    }
    if (find(id) != nullptr) {
        cout << "  Station " << id << " already exists.\n";
        return false;
    }
    Location* location = warehouse.findLocation(locationId);
    if (location == nullptr || location->type != "CHARGING") {
        cout << "  Location " << locationId << " is not a CHARGING location.\n";
        return false;
    }
    // Slots are a small bounded number, which is why a fixed array is fine here
    if (slots < 1 || slots > 10) {
        cout << "  A station must have between 1 and 10 slots.\n";
        return false;
    }

    ChargingStation* station = new ChargingStation(id, locationId, slots);
    if (head == nullptr) {
        head = station;
        tail = station;
    } else {
        tail->next = station;
        tail = station;
    }
    stationCount++;
    return true;
}

ChargingStation* ChargingManager::find(const string& stationId) const {
    ChargingStation* current = head;
    while (current != nullptr) {
        if (current->id == stationId) {
            return current;
        }
        current = current->next;
    }
    return nullptr;
}

ChargingStation* ChargingManager::stationOf(const Robot* robot) const {
    ChargingStation* station = head;
    while (station != nullptr) {
        if (station->slotOf(robot) != -1 || station->waiting.contains(const_cast<Robot*>(robot))) {
            return station;
        }
        station = station->next;
    }
    return nullptr;
}

// ---------------------------------------------------------------
// Station choice rule
// ---------------------------------------------------------------

ChargingStation* ChargingManager::chooseStation(const Robot* robot, const Graph& warehouse, int& distance) const {
    ChargingStation* bestFree = nullptr;
    int bestFreeDistance = -1;
    ChargingStation* bestAny = nullptr;
    int bestAnyDistance = -1;

    ChargingStation* station = head;
    while (station != nullptr) {
        int d = warehouse.shortestDistance(robot->locationId, station->locationId);

        // Reachable means: a route exists AND the robot has enough battery to drive it (empty)
        if (d >= 0 && robot->legConsumption(d, 0) <= robot->battery) {
            if (station->freeSlots() > 0) {
                if (bestFree == nullptr || d < bestFreeDistance
                    || (d == bestFreeDistance && idLess(station->id, bestFree->id))) {
                    bestFree = station;
                    bestFreeDistance = d;
                }
            }
            if (bestAny == nullptr || d < bestAnyDistance
                || (d == bestAnyDistance && idLess(station->id, bestAny->id))) {
                bestAny = station;
                bestAnyDistance = d;
            }
        }
        station = station->next;
    }

    // Prefer a station with a free slot; otherwise queue at the nearest one
    if (bestFree != nullptr) {
        distance = bestFreeDistance;
        return bestFree;
    }
    distance = bestAnyDistance;
    return bestAny;
}

// ---------------------------------------------------------------
// Charging operations
// ---------------------------------------------------------------

bool ChargingManager::sendToCharge(Robot* robot, const Graph& warehouse) {
    // Robots with an active task cannot be charged
    if (robot->hasActiveTask()) {
        cout << "  " << robot->id << " has an active task (" << robot->assignedTask << ") and cannot be charged.\n";
        return false;
    }
    if (robot->status != RobotStatus::Available) {
        cout << "  " << robot->id << " is " << robotStatusToString(robot->status)
             << ". Only Available robots can be sent to charge.\n";
        return false;
    }
    if (!robot->needsCharging()) {
        cout << "  Note: " << robot->id << " is at " << fixed << setprecision(1) << robot->battery
             << "%, it does not strictly need charging (below 20%).\n";
    }

    int distance;
    ChargingStation* station = chooseStation(robot, warehouse, distance);
    if (station == nullptr) {
        // No station can be reached: hand over to a human
        robot->status = RobotStatus::Unavailable;
        cout << "  " << robot->id << " cannot reach any charging station with "
             << fixed << setprecision(1) << robot->battery << "% battery.\n";
        cout << "  It has been set Unavailable and must be handled manually.\n";
        return false;
    }

    // Drive to the station (empty), using battery for the trip
    robot->battery -= robot->legConsumption(distance, 0);
    if (robot->battery < 0) {
        robot->battery = 0;
    }
    robot->locationId = station->locationId;

    if (station->startCharging(robot)) {
        cout << "  " << robot->id << " drove " << distance << " units to " << station->id
             << " and started charging.\n";
    } else {
        robot->status = RobotStatus::WaitingForCharging;
        station->waiting.enqueue(robot);
        cout << "  All slots at " << station->id << " are busy. " << robot->id
             << " joined its queue (position " << station->waiting.size() << ").\n";
    }
    return true;
}

void ChargingManager::startNextInQueue(ChargingStation* station) {
    Robot* nextRobot;
    // FIFO: the robot that has waited longest gets the slot
    while (station->freeSlots() > 0 && station->waiting.dequeue(nextRobot)) {
        station->startCharging(nextRobot);
        cout << "  " << nextRobot->id << " (front of the queue) started charging at " << station->id << ".\n";
    }
}

bool ChargingManager::completeCharging(Robot* robot) {
    ChargingStation* station = stationOf(robot);
    int slot = -1;
    if (station != nullptr) {
        slot = station->slotOf(robot);
    }
    if (station == nullptr || slot == -1) {
        cout << "  " << robot->id << " is not charging at any station.\n";
        return false;
    }

    // Charged: full battery, slot freed, robot Available at the station's location
    robot->battery = 100;
    robot->status = RobotStatus::Available;
    station->slots[slot] = nullptr;
    cout << "  " << robot->id << " is fully charged (100%) and Available at " << station->locationId << ".\n";

    startNextInQueue(station);
    return true;
}

bool ChargingManager::removeFromQueue(Robot* robot) {
    ChargingStation* station = stationOf(robot);
    if (station == nullptr || !station->waiting.remove(robot)) {
        cout << "  " << robot->id << " is not waiting in any charging queue.\n";
        return false;
    }
    robot->status = RobotStatus::Available;
    cout << "  " << robot->id << " removed from " << station->id << "'s queue and is Available.\n";
    return true;
}

// ---------------------------------------------------------------
// Displays
// ---------------------------------------------------------------

void ChargingManager::displayAll() const {
    if (head == nullptr) {
        cout << "  No charging stations.\n";
        return;
    }
    cout << "\n  Charging stations:\n";
    ChargingStation* station = head;
    while (station != nullptr) {
        station->display();
        station = station->next;
    }
}

void ChargingManager::displayStation(const string& stationId) const {
    ChargingStation* station = find(stationId);
    if (station == nullptr) {
        cout << "  Station " << stationId << " not found.\n";
        return;
    }
    station->display();
    if (station->freeSlots() > 0) {
        cout << "  A slot is available.\n";
    } else {
        cout << "  No slot is available; new robots will join the queue.\n";
    }
}

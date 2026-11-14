#include "PickingPlan.h"
#include <iostream>
#include <iomanip>

// ---------------------------------------------------------------
// Pickup and TripStop
// ---------------------------------------------------------------

Pickup::Pickup(const string& product, int amount, double weight) {
    productId = product;
    quantity = amount;
    unitWeight = weight;
    next = nullptr;
}

TripStop::TripStop(const string& rack, const string& location) {
    rackId = rack;
    locationId = location;
    routeIn = new Route();
    weightIn = 0;
    batteryIn = 0;
    collected = false;
    pickups = nullptr;
    next = nullptr;
}

TripStop::~TripStop() {
    delete routeIn;
    Pickup* current = pickups;
    while (current != nullptr) {
        Pickup* temp = current;
        current = current->next;
        delete temp;
    }
}

void TripStop::addPickup(const string& productId, int quantity, double unitWeight) {
    Pickup* current = pickups;
    Pickup* last = nullptr;
    while (current != nullptr) {
        if (current->productId == productId) {
            current->quantity += quantity;
            return;
        }
        last = current;
        current = current->next;
    }

    Pickup* pickup = new Pickup(productId, quantity, unitWeight);
    if (last == nullptr) {
        pickups = pickup;
    } else {
        last->next = pickup;
    }
}

double TripStop::pickupWeight() const {
    double weight = 0;
    Pickup* current = pickups;
    while (current != nullptr) {
        weight += current->quantity * current->unitWeight;
        current = current->next;
    }
    return weight;
}

// ---------------------------------------------------------------
// Trip
// ---------------------------------------------------------------

Trip::Trip(int tripNumber, const string& start) {
    number = tripNumber;
    startLocation = start;
    stops = nullptr;
    lastStop = nullptr;
    packingStation = "";
    routeToPacking = new Route();
    batteryToPacking = 0;
    weight = 0;
    executed = false;
    next = nullptr;
}

Trip::~Trip() {
    delete routeToPacking;
    TripStop* current = stops;
    while (current != nullptr) {
        TripStop* temp = current;
        current = current->next;
        delete temp;
    }
}

void Trip::appendStop(TripStop* stop) {
    if (stops == nullptr) {
        stops = stop;
    } else {
        lastStop->next = stop;
    }
    lastStop = stop;
}

int Trip::distance() const {
    int total = routeToPacking->getTotalDistance();
    TripStop* current = stops;
    while (current != nullptr) {
        total += current->routeIn->getTotalDistance();
        current = current->next;
    }
    return total;
}

double Trip::battery() const {
    double total = batteryToPacking;
    TripStop* current = stops;
    while (current != nullptr) {
        total += current->batteryIn;
        current = current->next;
    }
    return total;
}

// ---------------------------------------------------------------
// PickingPlan
// ---------------------------------------------------------------

PickingPlan::PickingPlan(const string& order, const string& robot) {
    orderId = order;
    robotId = robot;
    trips = nullptr;
    lastTrip = nullptr;
    tripCount = 0;
    next = nullptr;
}

PickingPlan::~PickingPlan() {
    Trip* current = trips;
    while (current != nullptr) {
        Trip* temp = current;
        current = current->next;
        delete temp;
    }
}

void PickingPlan::appendTrip(Trip* trip) {
    if (trips == nullptr) {
        trips = trip;
    } else {
        lastTrip->next = trip;
    }
    lastTrip = trip;
    tripCount++;
}

int PickingPlan::totalDistance() const {
    int total = 0;
    Trip* current = trips;
    while (current != nullptr) {
        total += current->distance();
        current = current->next;
    }
    return total;
}

double PickingPlan::totalBattery() const {
    double total = 0;
    Trip* current = trips;
    while (current != nullptr) {
        total += current->battery();
        current = current->next;
    }
    return total;
}

void PickingPlan::displayTrip(const Trip* trip) const {
    cout << "\n  Trip " << trip->number;
    if (trip->executed) {
        cout << " (EXECUTED)";
    }
    cout << ":\n  Start: " << trip->startLocation << "\n";

    TripStop* stop = trip->stops;
    while (stop != nullptr) {
        cout << "    Route: ";
        stop->routeIn->display();
        cout << "  (" << stop->routeIn->getTotalDistance() << " units)\n";
        cout << "  Visit Rack " << stop->rackId << " at " << stop->locationId;
        if (stop->collected) {
            cout << " [collected]";
        }
        cout << "\n";
        Pickup* pickup = stop->pickups;
        while (pickup != nullptr) {
            cout << "    Collect " << pickup->productId << ": " << pickup->quantity << " units\n";
            pickup = pickup->next;
        }
        stop = stop->next;
    }

    cout << "    Route: ";
    trip->routeToPacking->display();
    cout << "  (" << trip->routeToPacking->getTotalDistance() << " units)\n";
    cout << "  Destination: Packing Station " << trip->packingStation << "\n";
    cout << "  Trip distance: " << trip->distance() << " units   Weight: "
         << fixed << setprecision(1) << trip->weight << " kg   Battery: "
         << trip->battery() << "%\n";
}

void PickingPlan::display() const {
    cout << "\n  Order ID: " << orderId << "\n";
    cout << "  Assigned Robot: " << robotId << "\n";
    Trip* trip = trips;
    while (trip != nullptr) {
        displayTrip(trip);
        trip = trip->next;
    }
    cout << "\n  Trips: " << tripCount
         << "   Total Distance: " << totalDistance() << " units"
         << "   Estimated Battery Consumption: " << fixed << setprecision(1) << totalBattery() << "%\n";
}

// ---------------------------------------------------------------
// PlanManager
// ---------------------------------------------------------------

PlanManager::PlanManager() {
    head = nullptr;
}

PlanManager::~PlanManager() {
    PickingPlan* current = head;
    while (current != nullptr) {
        PickingPlan* temp = current;
        current = current->next;
        delete temp;
    }
}

void PlanManager::add(PickingPlan* plan) {
    // Add at the end so plans are listed in the order they were assigned
    plan->next = nullptr;
    if (head == nullptr) {
        head = plan;
        return;
    }
    PickingPlan* current = head;
    while (current->next != nullptr) {
        current = current->next;
    }
    current->next = plan;
}

PickingPlan* PlanManager::find(const string& orderId) const {
    PickingPlan* current = head;
    while (current != nullptr) {
        if (current->orderId == orderId) {
            return current;
        }
        current = current->next;
    }
    return nullptr;
}

bool PlanManager::remove(const string& orderId) {
    PickingPlan* current = head;
    PickingPlan* previous = nullptr;
    while (current != nullptr) {
        if (current->orderId == orderId) {
            if (previous == nullptr) {
                head = current->next;
            } else {
                previous->next = current->next;
            }
            delete current;
            return true;
        }
        previous = current;
        current = current->next;
    }
    return false;
}

void PlanManager::displayAll() const {
    if (head == nullptr) {
        cout << "  No active picking plans.\n";
        return;
    }
    PickingPlan* current = head;
    while (current != nullptr) {
        cout << "\n  =========================================";
        current->display();
        current = current->next;
    }
}

PickingPlan* PlanManager::getHead() const {
    return head;
}

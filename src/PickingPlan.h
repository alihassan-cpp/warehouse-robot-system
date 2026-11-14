#ifndef PICKINGPLAN_H
#define PICKINGPLAN_H

#include <string>
#include "Route.h"
using namespace std;

// One product picked at a rack stop: "take 4 units of P103 here"
class Pickup {
public:
    string productId;
    int quantity;
    double unitWeight;
    Pickup* next;

    Pickup(const string& product, int amount, double weight);
};

// One rack visit inside a trip. Holds the route used to get here,
// the battery used on that leg, and what is picked up on arrival.
class TripStop {
public:
    string rackId;
    string locationId;
    Route* routeIn;        // route from the previous point to this rack
    double weightIn;       // weight carried while travelling to this stop
    double batteryIn;      // battery used on that leg
    bool collected;        // set in Module 6 so a stop can never be collected twice
    Pickup* pickups;
    TripStop* next;

    TripStop(const string& rack, const string& location);
    ~TripStop();

    TripStop(const TripStop&) = delete;
    TripStop& operator=(const TripStop&) = delete;

    // Adds units of a product here; merges if the product is already picked at this stop
    void addPickup(const string& productId, int quantity, double unitWeight);
    double pickupWeight() const;
};

// One trip: start somewhere, visit racks, unload at a packing station
class Trip {
public:
    int number;
    string startLocation;
    TripStop* stops;
    TripStop* lastStop;
    string packingStation;
    Route* routeToPacking;   // route from the last rack to the packing station
    double batteryToPacking; // battery used on that final (loaded) leg
    double weight;           // total kg carried on this trip
    bool executed;           // set in Module 6 once the trip has been delivered
    Trip* next;

    Trip(int tripNumber, const string& start);
    ~Trip();

    Trip(const Trip&) = delete;
    Trip& operator=(const Trip&) = delete;

    void appendStop(TripStop* stop);
    int distance() const;   // sum of all legs
    double battery() const; // sum of all legs
};

// A complete plan for one order and one robot
class PickingPlan {
public:
    string orderId;
    string robotId;
    Trip* trips;
    Trip* lastTrip;
    int tripCount;
    PickingPlan* next; // used by PlanManager's linked list

    PickingPlan(const string& order, const string& robot);
    ~PickingPlan();

    PickingPlan(const PickingPlan&) = delete;
    PickingPlan& operator=(const PickingPlan&) = delete;

    void appendTrip(Trip* trip);
    int totalDistance() const;
    double totalBattery() const;

    // Prints every trip with its routes, pickups, weight, distance and battery
    void display() const;
    void displayTrip(const Trip* trip) const;
};

// Stores all confirmed (assigned) plans so Module 6 can execute them later
class PlanManager {
private:
    PickingPlan* head;

public:
    PlanManager();
    ~PlanManager();

    PlanManager(const PlanManager&) = delete;
    PlanManager& operator=(const PlanManager&) = delete;

    void add(PickingPlan* plan);              // takes ownership
    PickingPlan* find(const string& orderId) const;
    bool remove(const string& orderId);       // deletes the plan
    void displayAll() const;
    PickingPlan* getHead() const;
};

#endif

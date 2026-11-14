#ifndef RACKENTRY_H
#define RACKENTRY_H

#include <string>
using namespace std;

// Records that an order has reserved some units in a rack.
// Keeping one record per order means a cancelled order can release
// exactly what it reserved, nothing more and nothing less.
class Reservation {
public:
    string orderId;
    int quantity;
    Reservation* next;

    Reservation(const string& order, int amount);
};

// One rack in a product's rack list: how many units of that product the rack holds.
// Rule that must always hold: reservedQuantity == sum of all reservation records.
class RackEntry {
public:
    string rackId;
    int totalQuantity;
    int reservedQuantity;
    Reservation* reservationHead;
    RackEntry* next;

    RackEntry(const string& id, int quantity);
    ~RackEntry(); // frees the reservation list

    RackEntry(const RackEntry&) = delete;
    RackEntry& operator=(const RackEntry&) = delete;

    // Units that are in the rack and not promised to any order
    int available() const;
    bool hasReservations() const;

    // Reserves units for an order. If the order already has a record here,
    // the amounts are merged. Fails if not enough units are available.
    bool addReservation(const string& orderId, int amount);

    // Removes an order's reservation completely; returns how many units were freed
    int releaseReservation(const string& orderId);

    // Used when a robot picks up reserved units (Module 6):
    // total, reserved and the order's record all go down by the same amount
    bool collect(const string& orderId, int amount);

    // How many units a specific order has reserved here (0 if none)
    int reservedBy(const string& orderId) const;

    void displayReservations() const;
};

#endif

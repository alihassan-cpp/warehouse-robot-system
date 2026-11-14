#include "Robot.h"

string robotStatusToString(RobotStatus status) {
    switch (status) {
        case RobotStatus::Available: return "Available";
        case RobotStatus::Assigned: return "Assigned";
        case RobotStatus::WaitingForCharging: return "Waiting for Charging";
        case RobotStatus::Charging: return "Charging";
        case RobotStatus::Unavailable: return "Unavailable";
    }
    return "Unknown";
}

// ---------------------------------------------------------------
// Base class
// ---------------------------------------------------------------

Robot::Robot(const string& robotId, const string& location, double batteryLevel, double maxCapacity) {
    id = robotId;
    locationId = location;
    battery = batteryLevel;
    capacity = maxCapacity;
    status = RobotStatus::Available;
    assignedTask = "";
    next = nullptr;
}

Robot::~Robot() {
}

double Robot::legConsumption(int distance, double carriedWeight) const {
    // batteryRate() is virtual, so this automatically uses 0.20, 0.25 or 0.35
    return distance * batteryRate() * (1 + 0.5 * carriedWeight / capacity);
}

bool Robot::canCarryUnit(double unitWeight) const {
    return unitWeight <= capacity;
}

bool Robot::needsCharging() const {
    return battery < 20;
}

bool Robot::hasActiveTask() const {
    return assignedTask != "";
}

// ---------------------------------------------------------------
// Picker: light items only
// ---------------------------------------------------------------

Picker::Picker(const string& robotId, const string& location, double batteryLevel, double maxCapacity)
    : Robot(robotId, location, batteryLevel, maxCapacity) {
}

string Picker::typeName() const {
    return "Picker";
}

double Picker::batteryRate() const {
    return 0.20;
}

bool Picker::isCapacityValid() const {
    return capacity > 0 && capacity <= 15;
}

bool Picker::typeAllowsOrder(double orderWeight, double heaviestUnit) const {
    (void)orderWeight; // Pickers only care about single-unit weight
    // Cannot carry any product whose unit weight is above 5 kg
    return heaviestUnit <= 5;
}

// ---------------------------------------------------------------
// Carrier: general purpose
// ---------------------------------------------------------------

Carrier::Carrier(const string& robotId, const string& location, double batteryLevel, double maxCapacity)
    : Robot(robotId, location, batteryLevel, maxCapacity) {
}

string Carrier::typeName() const {
    return "Carrier";
}

double Carrier::batteryRate() const {
    return 0.25;
}

bool Carrier::isCapacityValid() const {
    return capacity >= 15 && capacity <= 30;
}

bool Carrier::typeAllowsOrder(double orderWeight, double heaviestUnit) const {
    (void)orderWeight;
    (void)heaviestUnit;
    return true; // no restriction
}

// ---------------------------------------------------------------
// HeavyDuty: reserved for heavy work
// ---------------------------------------------------------------

HeavyDuty::HeavyDuty(const string& robotId, const string& location, double batteryLevel, double maxCapacity)
    : Robot(robotId, location, batteryLevel, maxCapacity) {
}

string HeavyDuty::typeName() const {
    return "Heavy-duty";
}

double HeavyDuty::batteryRate() const {
    return 0.35;
}

bool HeavyDuty::isCapacityValid() const {
    return capacity > 0 && capacity <= 60;
}

bool HeavyDuty::typeAllowsOrder(double orderWeight, double heaviestUnit) const {
    // Only for orders above 15 kg in total, or with a product above 10 kg per unit
    return orderWeight > 15 || heaviestUnit > 10;
}

// ---------------------------------------------------------------
// Factory
// ---------------------------------------------------------------

Robot* createRobot(const string& type, const string& id, const string& location, double battery, double capacity) {
    if (type == "PICKER") {
        return new Picker(id, location, battery, capacity);
    }
    if (type == "CARRIER") {
        return new Carrier(id, location, battery, capacity);
    }
    if (type == "HEAVYDUTY" || type == "HEAVY-DUTY" || type == "HEAVY") {
        return new HeavyDuty(id, location, battery, capacity);
    }
    return nullptr;
}

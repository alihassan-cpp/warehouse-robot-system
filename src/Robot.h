#ifndef ROBOT_H
#define ROBOT_H

#include <string>
using namespace std;

enum class RobotStatus {
    Available,
    Assigned,
    WaitingForCharging,
    Charging,
    Unavailable
};

string robotStatusToString(RobotStatus status);

// Base class for every robot. It holds everything robots have in common.
// The parts that differ by type (battery rate, capacity limits, which orders
// it may take) are pure virtual functions, so Robot itself cannot be created;
// only Picker, Carrier and HeavyDuty can.
class Robot {
public:
    string id;            // e.g. RB01
    string locationId;    // where the robot is right now
    double battery;       // 0 - 100 (%)
    double capacity;      // max kg per trip
    RobotStatus status;
    string assignedTask;  // order ID or task ID, "" if none
    Robot* next;          // next robot in the fleet's linked list

    Robot(const string& robotId, const string& location, double batteryLevel, double maxCapacity);

    // Virtual destructor: deleting a Picker through a Robot* must call the right destructor
    virtual ~Robot();

    Robot(const Robot&) = delete;
    Robot& operator=(const Robot&) = delete;

    // ---- Type-specific behaviour (each derived class gives its own version) ----
    virtual string typeName() const = 0;
    virtual double batteryRate() const = 0;          // % per distance unit
    virtual bool isCapacityValid() const = 0;        // capacity within the type's limit

    // The type's eligibility rule from the PDF table
    virtual bool typeAllowsOrder(double orderWeight, double heaviestUnit) const = 0;

    // ---- Shared behaviour ----

    // Battery used for one leg: distance x rate x (1 + 0.5 x carried / capacity)
    double legConsumption(int distance, double carriedWeight) const;

    bool canCarryUnit(double unitWeight) const;  // a single unit must fit
    bool needsCharging() const;                  // battery below 20%
    bool hasActiveTask() const;
};

class Picker : public Robot {
public:
    Picker(const string& robotId, const string& location, double batteryLevel, double maxCapacity);
    string typeName() const override;
    double batteryRate() const override;
    bool isCapacityValid() const override;
    bool typeAllowsOrder(double orderWeight, double heaviestUnit) const override;
};

class Carrier : public Robot {
public:
    Carrier(const string& robotId, const string& location, double batteryLevel, double maxCapacity);
    string typeName() const override;
    double batteryRate() const override;
    bool isCapacityValid() const override;
    bool typeAllowsOrder(double orderWeight, double heaviestUnit) const override;
};

class HeavyDuty : public Robot {
public:
    HeavyDuty(const string& robotId, const string& location, double batteryLevel, double maxCapacity);
    string typeName() const override;
    double batteryRate() const override;
    bool isCapacityValid() const override;
    bool typeAllowsOrder(double orderWeight, double heaviestUnit) const override;
};

// Creates the right kind of robot from a type name (PICKER / CARRIER / HEAVYDUTY).
// Returns nullptr if the type is unknown.
Robot* createRobot(const string& type, const string& id, const string& location, double battery, double capacity);

#endif

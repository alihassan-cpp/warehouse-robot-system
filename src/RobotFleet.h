#ifndef ROBOTFLEET_H
#define ROBOTFLEET_H

#include <string>
#include "Robot.h"
#include "Graph.h"
using namespace std;

// Module 4 manager: all robots in a linked list.
// The list stores Robot* so it can hold Pickers, Carriers and HeavyDuty robots together
// (polymorphism); calling robot->batteryRate() runs the correct version for each type.
class RobotFleet {
private:
    Robot* head;
    Robot* tail;
    int robotCount;

    void displayRow(const Robot* robot) const;
    void displayHeader() const;

public:
    RobotFleet();
    ~RobotFleet();

    RobotFleet(const RobotFleet&) = delete;
    RobotFleet& operator=(const RobotFleet&) = delete;

    bool loadRobots(const string& fileName, const Graph& warehouse);

    // Validates and adds a robot. Takes ownership on success.
    bool registerRobot(Robot* robot, const Graph& warehouse);

    Robot* findRobot(const string& robotId) const;
    void displayAll() const;
    void displayAvailable() const;
    void displayRobot(const string& robotId) const;

    bool updateBattery(const string& robotId, double newBattery);

    // Gives a robot a task. Fails if it is not Available or already has a task,
    // so a robot can never hold more than one active assignment.
    bool assignTask(const string& robotId, const string& taskId);

    // Ends the robot's task and makes it Available again.
    // Prints a warning if the battery is now below 20%.
    bool releaseRobot(const string& robotId);

    // Maintenance. Only allowed with no active task and not charging.
    // oldStatus is returned so the undo stack (Module 8) can restore it.
    bool setUnavailable(const string& robotId, RobotStatus& oldStatus);
    bool returnFromMaintenance(const string& robotId, RobotStatus& oldStatus);

    void displayNeedingCharge() const;

    int size() const;
    Robot* getHead() const;
};

#endif

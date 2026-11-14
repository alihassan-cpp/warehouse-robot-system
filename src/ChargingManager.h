#ifndef CHARGINGMANAGER_H
#define CHARGINGMANAGER_H

#include <string>
#include "Robot.h"
#include "Graph.h"
#include "Queue.h"
using namespace std;

// One charging station. Its slots are a small fixed-size array (allowed by the PDF
// for bounded data), and robots waiting for a slot are in a FIFO queue.
class ChargingStation {
public:
    string id;          // e.g. C01
    string locationId;  // a CHARGING location, e.g. L04
    int slotCount;
    Robot** slots;      // slots[i] is the robot charging in slot i, or nullptr if free
    Queue<Robot*> waiting;
    ChargingStation* next;

    ChargingStation(const string& stationId, const string& location, int numberOfSlots);
    ~ChargingStation();

    ChargingStation(const ChargingStation&) = delete;
    ChargingStation& operator=(const ChargingStation&) = delete;

    int freeSlots() const;
    int slotOf(const Robot* robot) const; // -1 if the robot is not charging here

    // Puts a robot into the first free slot. Returns false if all slots are taken.
    bool startCharging(Robot* robot);

    void display() const;
};

// Module 7 manager: all charging stations in a linked list
class ChargingManager {
private:
    ChargingStation* head;
    ChargingStation* tail;
    int stationCount;

    // Where a robot currently is in the charging system (charging or waiting)
    ChargingStation* stationOf(const Robot* robot) const;

    // After a slot frees up, the robot at the front of the queue starts charging
    void startNextInQueue(ChargingStation* station);

public:
    ChargingManager();
    ~ChargingManager();

    ChargingManager(const ChargingManager&) = delete;
    ChargingManager& operator=(const ChargingManager&) = delete;

    bool load(const string& fileName, const Graph& warehouse);
    bool addStation(const string& id, const string& locationId, int slots, const Graph& warehouse);
    ChargingStation* find(const string& stationId) const;

    // Station choice rule (PDF Module 7):
    //  - only stations the robot can reach (route exists AND enough battery to drive there)
    //  - nearest station with a free slot
    //  - if none has a free slot, the nearest reachable station (to join its queue)
    //  - ties go to the smaller station ID
    // Returns nullptr if no station can be reached.
    ChargingStation* chooseStation(const Robot* robot, const Graph& warehouse, int& distance) const;

    // Sends a robot to charge: drives it to the chosen station, then it either
    // starts charging (free slot) or joins that station's FIFO queue.
    // If no station is reachable, the robot is set Unavailable for manual handling.
    bool sendToCharge(Robot* robot, const Graph& warehouse);

    // Battery -> 100%, slot freed, robot Available, next robot in the queue starts
    bool completeCharging(Robot* robot);

    // Takes a waiting robot out of its station's queue (it becomes Available)
    bool removeFromQueue(Robot* robot);

    void displayAll() const;
    void displayStation(const string& stationId) const;
};

#endif

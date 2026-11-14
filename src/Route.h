#ifndef ROUTE_H
#define ROUTE_H

#include <string>
using namespace std;

// One stop on a route, stored as a linked list node
class RouteNode {
public:
    string locationId;
    RouteNode* next;

    RouteNode(const string& id);
};

// A route is the sequence of locations a robot passes through, plus the total distance.
// Dijkstra fills it in, and later modules (picking plans) will store routes for each trip.
class Route {
private:
    RouteNode* head;
    RouteNode* tail;
    int stopCount;
    int totalDistance;

public:
    Route();
    ~Route();

    // Disabled so two routes never share and double-delete the same nodes
    Route(const Route&) = delete;
    Route& operator=(const Route&) = delete;

    void addStop(const string& locationId);       // adds at the end
    void addStopAtFront(const string& locationId); // used when rebuilding a path backwards
    void clear();

    void setTotalDistance(int distance);
    int getTotalDistance() const;
    int getStopCount() const;
    bool isEmpty() const;
    RouteNode* getHead() const;

    // Prints like: L01 -> L02 -> L06 -> L08
    void display() const;
};

#endif

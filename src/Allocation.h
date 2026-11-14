#ifndef ALLOCATION_H
#define ALLOCATION_H

#include <string>
using namespace std;

// One line of an allocation: "take this many units from this rack"
class AllocationNode {
public:
    string rackId;
    string locationId;
    int quantity;
    int distance; // Dijkstra distance from the reference location
    AllocationNode* next;

    AllocationNode(const string& rack, const string& location, int amount, int dist);
};

// The result of the rack allocation rule for one product:
// a linked list of racks and how many units to take from each.
class Allocation {
private:
    AllocationNode* head;
    AllocationNode* tail;
    int totalQuantity;

public:
    Allocation();
    ~Allocation();

    Allocation(const Allocation&) = delete;
    Allocation& operator=(const Allocation&) = delete;

    void add(const string& rackId, const string& locationId, int quantity, int distance);
    void clear();

    AllocationNode* getHead() const;
    int getTotalQuantity() const;
    bool isEmpty() const;
    void display() const;
};

#endif

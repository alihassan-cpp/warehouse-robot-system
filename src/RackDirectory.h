#ifndef RACKDIRECTORY_H
#define RACKDIRECTORY_H

#include <string>
#include "Graph.h"
using namespace std;

// Which location each rack is installed at (loaded from racks.txt)
class RackInfo {
public:
    string rackId;     // e.g. RK04
    string locationId; // e.g. L05
    RackInfo* next;

    RackInfo(const string& rack, const string& location);
};

// Linked list of all racks in the warehouse.
// Routing "to a rack" really means routing to its location, so other modules
// ask this directory for a rack's location before calling Dijkstra.
class RackDirectory {
private:
    RackInfo* head;
    RackInfo* tail;
    int count;

public:
    RackDirectory();
    ~RackDirectory();

    RackDirectory(const RackDirectory&) = delete;
    RackDirectory& operator=(const RackDirectory&) = delete;

    bool load(const string& fileName, const Graph& warehouse);

    // The location must exist in the graph and be of type RACK
    bool addRack(const string& rackId, const string& locationId, const Graph& warehouse);

    RackInfo* find(const string& rackId) const;
    string locationOf(const string& rackId) const; // "" if the rack is unknown
    void display() const;
    int size() const;
};

#endif

#ifndef LOCATION_H
#define LOCATION_H

#include <string>
using namespace std;

class Location;

// One pathway in a location's adjacency list.
// Because the warehouse graph is undirected, every pathway is stored twice:
// once in each endpoint's list. Both copies must always have the same status.
class Edge {
public:
    Location* destination; // the location at the other end of this pathway
    int distance;
    bool blocked;          // true = BLOCKED, false = OPEN
    Edge* next;            // next pathway in the same adjacency list

    Edge(Location* dest, int dist, bool isBlocked);
};

// One node of the warehouse graph (a rack spot, packing station or charging spot).
// Locations are kept in a linked list inside the Graph, and each one owns
// its own linked list of edges (the adjacency list).
class Location {
public:
    string id;      // e.g. L01
    string type;    // RACK, PACKING or CHARGING
    string name;    // e.g. Aisle A Rack
    int index;      // position 0, 1, 2... used for Dijkstra/BFS arrays
    Edge* edgeHead; // first pathway leaving this location
    Location* next; // next location in the graph's list

    Location(const string& locationId, const string& locationType, const string& locationName, int locationIndex);
    ~Location(); // deletes all of this location's edges

    Location(const Location&) = delete;
    Location& operator=(const Location&) = delete;
};

#endif

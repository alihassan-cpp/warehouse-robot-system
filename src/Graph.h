#ifndef GRAPH_H
#define GRAPH_H

#include <string>
#include "Location.h"
#include "Route.h"
using namespace std;

// The warehouse layout: an undirected weighted graph stored as an adjacency list.
// Locations are in a linked list; each location has a linked list of edges.
class Graph {
private:
    Location* head;
    Location* tail;
    int locationCount;

    // Finds the edge going from one location to another (nullptr if none)
    Edge* findEdge(Location* from, Location* to) const;

    // Adds an edge at the end of a location's adjacency list
    void appendEdge(Location* from, Edge* edge);

    // Deletes the edge from -> to in one direction only
    bool removeEdgeOneWay(Location* from, Location* to);

    // Builds an array where table[i] is the location with index i.
    // Dijkstra uses this to go from an array position back to a location.
    Location** buildIndexTable() const;

    bool isValidType(const string& type) const;

public:
    Graph();
    ~Graph();

    Graph(const Graph&) = delete;
    Graph& operator=(const Graph&) = delete;

    // File loading
    bool loadLocations(const string& fileName);
    bool loadPathways(const string& fileName);

    // Editing the layout
    bool addLocation(const string& id, const string& type, const string& name);
    bool addPathway(const string& fromId, const string& toId, int distance, bool blocked);
    bool removePathway(const string& fromId, const string& toId);
    bool blockPathway(const string& fromId, const string& toId);
    bool reopenPathway(const string& fromId, const string& toId);

    // Queries
    Location* findLocation(const string& id) const;
    bool pathwayExists(const string& fromId, const string& toId) const;
    bool isPathwayBlocked(const string& fromId, const string& toId) const;
    void displayAll() const;
    void displayLocation(const string& id) const;

    // BFS: can we get from A to B using only OPEN pathways?
    bool isConnected(const string& fromId, const string& toId) const;

    // Dijkstra: fills route with the shortest path. Returns false if no route exists.
    bool shortestRoute(const string& fromId, const string& toId, Route& route) const;

    // Shortcut for other modules: shortest distance only, or -1 if unreachable
    int shortestDistance(const string& fromId, const string& toId) const;

    int getLocationCount() const;
    Location* getHead() const;
};

#endif

#include "Graph.h"
#include "Queue.h"
#include "Utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <climits>

Graph::Graph() {
    head = nullptr;
    tail = nullptr;
    locationCount = 0;
}

Graph::~Graph() {
    // Deleting a Location also deletes its edges (see Location destructor)
    Location* current = head;
    while (current != nullptr) {
        Location* temp = current;
        current = current->next;
        delete temp;
    }
}

// ---------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------

bool Graph::isValidType(const string& type) const {
    return type == "RACK" || type == "PACKING" || type == "CHARGING";
}

Edge* Graph::findEdge(Location* from, Location* to) const {
    Edge* current = from->edgeHead;
    while (current != nullptr) {
        if (current->destination == to) {
            return current;
        }
        current = current->next;
    }
    return nullptr;
}

void Graph::appendEdge(Location* from, Edge* edge) {
    // Adding at the end keeps pathways in the same order as the file,
    // which makes the display easier to read
    if (from->edgeHead == nullptr) {
        from->edgeHead = edge;
        return;
    }
    Edge* current = from->edgeHead;
    while (current->next != nullptr) {
        current = current->next;
    }
    current->next = edge;
}

bool Graph::removeEdgeOneWay(Location* from, Location* to) {
    Edge* current = from->edgeHead;
    Edge* previous = nullptr;
    while (current != nullptr) {
        if (current->destination == to) {
            if (previous == nullptr) {
                from->edgeHead = current->next;
            } else {
                previous->next = current->next;
            }
            delete current;
            return true;
        }
        previous = current;
        current = current->next;
    }
    return false;
}

Location** Graph::buildIndexTable() const {
    Location** table = new Location*[locationCount];
    Location* current = head;
    while (current != nullptr) {
        table[current->index] = current;
        current = current->next;
    }
    return table;
}

// ---------------------------------------------------------------
// File loading
// ---------------------------------------------------------------

bool Graph::loadLocations(const string& fileName) {
    ifstream file(fileName);
    if (!file.is_open()) {
        cout << "Error: could not open " << fileName << "\n";
        return false;
    }

    string line;
    int lineNumber = 0;
    int loaded = 0;

    while (getline(file, line)) {
        lineNumber++;
        line = trim(line);

        // Skip blank lines and comment lines starting with #
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Split "L01,RACK,Aisle A Rack" at the commas
        stringstream parts(line);
        string id, type, name;
        getline(parts, id, ',');
        getline(parts, type, ',');
        getline(parts, name);

        id = toUpper(trim(id));
        type = toUpper(trim(type));
        name = trim(name);

        if (id.empty() || type.empty() || name.empty()) {
            cout << "  " << fileName << " line " << lineNumber << ": missing fields, skipped.\n";
            continue;
        }

        if (addLocation(id, type, name)) {
            loaded++;
        } else {
            cout << "  (" << fileName << " line " << lineNumber << " skipped)\n";
        }
    }

    cout << "Loaded " << loaded << " locations from " << fileName << ".\n";
    return true;
}

bool Graph::loadPathways(const string& fileName) {
    ifstream file(fileName);
    if (!file.is_open()) {
        cout << "Error: could not open " << fileName << "\n";
        return false;
    }

    string line;
    int lineNumber = 0;
    int loaded = 0;

    while (getline(file, line)) {
        lineNumber++;
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Split "L01,L02,5,OPEN" at the commas
        stringstream parts(line);
        string fromId, toId, distanceText, status;
        getline(parts, fromId, ',');
        getline(parts, toId, ',');
        getline(parts, distanceText, ',');
        getline(parts, status);

        fromId = toUpper(trim(fromId));
        toId = toUpper(trim(toId));
        status = toUpper(trim(status));

        int distance;
        if (!parseInt(distanceText, distance)) {
            cout << "  " << fileName << " line " << lineNumber << ": distance is not a number, skipped.\n";
            continue;
        }

        // Status is optional in the file; a missing status means OPEN
        bool blocked = false;
        if (status == "BLOCKED") {
            blocked = true;
        } else if (status != "" && status != "OPEN") {
            cout << "  " << fileName << " line " << lineNumber << ": status must be OPEN or BLOCKED, skipped.\n";
            continue;
        }

        if (addPathway(fromId, toId, distance, blocked)) {
            loaded++;
        } else {
            cout << "  (" << fileName << " line " << lineNumber << " skipped)\n";
        }
    }

    cout << "Loaded " << loaded << " pathways from " << fileName << ".\n";
    return true;
}

// ---------------------------------------------------------------
// Editing the layout
// ---------------------------------------------------------------

bool Graph::addLocation(const string& id, const string& type, const string& name) {
    if (!isValidId(id, "L")) {
        cout << "  Invalid location ID '" << id << "'. It must look like L01.\n";
        return false;
    }
    if (!isValidType(type)) {
        cout << "  Invalid type '" << type << "'. Use RACK, PACKING or CHARGING.\n";
        return false;
    }
    if (name.empty()) {
        cout << "  Location name cannot be empty.\n";
        return false;
    }
    if (findLocation(id) != nullptr) {
        cout << "  Location " << id << " already exists.\n";
        return false;
    }

    // The new location gets the next free index (0, 1, 2...)
    Location* location = new Location(id, type, name, locationCount);
    if (head == nullptr) {
        head = location;
        tail = location;
    } else {
        tail->next = location;
        tail = location;
    }
    locationCount++;
    return true;
}

bool Graph::addPathway(const string& fromId, const string& toId, int distance, bool blocked) {
    Location* from = findLocation(fromId);
    Location* to = findLocation(toId);

    if (from == nullptr) {
        cout << "  Location " << fromId << " does not exist.\n";
        return false;
    }
    if (to == nullptr) {
        cout << "  Location " << toId << " does not exist.\n";
        return false;
    }
    if (from == to) {
        cout << "  A pathway cannot connect a location to itself.\n";
        return false;
    }
    if (distance <= 0) {
        cout << "  Distance must be greater than 0.\n";
        return false;
    }
    if (findEdge(from, to) != nullptr) {
        cout << "  A pathway between " << fromId << " and " << toId << " already exists.\n";
        return false;
    }

    // Undirected graph: store the pathway in both adjacency lists
    appendEdge(from, new Edge(to, distance, blocked));
    appendEdge(to, new Edge(from, distance, blocked));
    return true;
}

bool Graph::removePathway(const string& fromId, const string& toId) {
    Location* from = findLocation(fromId);
    Location* to = findLocation(toId);
    if (from == nullptr || to == nullptr) {
        cout << "  One or both locations do not exist.\n";
        return false;
    }
    if (findEdge(from, to) == nullptr) {
        cout << "  No pathway exists between " << fromId << " and " << toId << ".\n";
        return false;
    }

    // Remove both directions so the lists stay consistent
    removeEdgeOneWay(from, to);
    removeEdgeOneWay(to, from);
    return true;
}

bool Graph::blockPathway(const string& fromId, const string& toId) {
    Location* from = findLocation(fromId);
    Location* to = findLocation(toId);
    if (from == nullptr || to == nullptr) {
        cout << "  One or both locations do not exist.\n";
        return false;
    }

    Edge* forward = findEdge(from, to);
    Edge* backward = findEdge(to, from);
    if (forward == nullptr || backward == nullptr) {
        cout << "  No pathway exists between " << fromId << " and " << toId << ".\n";
        return false;
    }
    if (forward->blocked) {
        cout << "  This pathway is already blocked.\n";
        return false;
    }

    // Unlike remove, the edge stays in the lists; only its status changes
    forward->blocked = true;
    backward->blocked = true;
    return true;
}

bool Graph::reopenPathway(const string& fromId, const string& toId) {
    Location* from = findLocation(fromId);
    Location* to = findLocation(toId);
    if (from == nullptr || to == nullptr) {
        cout << "  One or both locations do not exist.\n";
        return false;
    }

    Edge* forward = findEdge(from, to);
    Edge* backward = findEdge(to, from);
    if (forward == nullptr || backward == nullptr) {
        cout << "  No pathway exists between " << fromId << " and " << toId << ".\n";
        return false;
    }
    if (!forward->blocked) {
        cout << "  This pathway is already open.\n";
        return false;
    }

    forward->blocked = false;
    backward->blocked = false;
    return true;
}

// ---------------------------------------------------------------
// Queries
// ---------------------------------------------------------------

Location* Graph::findLocation(const string& id) const {
    Location* current = head;
    while (current != nullptr) {
        if (current->id == id) {
            return current;
        }
        current = current->next;
    }
    return nullptr;
}

bool Graph::pathwayExists(const string& fromId, const string& toId) const {
    Location* from = findLocation(fromId);
    Location* to = findLocation(toId);
    if (from == nullptr || to == nullptr) {
        return false;
    }
    return findEdge(from, to) != nullptr;
}

bool Graph::isPathwayBlocked(const string& fromId, const string& toId) const {
    Location* from = findLocation(fromId);
    Location* to = findLocation(toId);
    if (from == nullptr || to == nullptr) {
        return false;
    }
    Edge* edge = findEdge(from, to);
    return edge != nullptr && edge->blocked;
}

void Graph::displayLocation(const string& id) const {
    Location* location = findLocation(id);
    if (location == nullptr) {
        cout << "  Location " << id << " not found.\n";
        return;
    }

    cout << "  " << location->id << " [" << location->type << "] " << location->name << "\n";
    cout << "     Connections: ";
    if (location->edgeHead == nullptr) {
        cout << "none";
    }

    Edge* edge = location->edgeHead;
    while (edge != nullptr) {
        cout << edge->destination->id << "(" << edge->distance;
        if (edge->blocked) {
            cout << ", BLOCKED";
        }
        cout << ")";
        if (edge->next != nullptr) {
            cout << ", ";
        }
        edge = edge->next;
    }
    cout << "\n";
}

void Graph::displayAll() const {
    if (head == nullptr) {
        cout << "  The warehouse has no locations.\n";
        return;
    }

    cout << "\n  Warehouse layout (" << locationCount << " locations)\n";
    cout << "  Format: neighbour(distance), blocked pathways are marked BLOCKED\n";
    cout << "  ------------------------------------------------------------\n";
    Location* current = head;
    while (current != nullptr) {
        displayLocation(current->id);
        current = current->next;
    }
}

// ---------------------------------------------------------------
// BFS - reachability
// ---------------------------------------------------------------

bool Graph::isConnected(const string& fromId, const string& toId) const {
    Location* start = findLocation(fromId);
    Location* target = findLocation(toId);
    if (start == nullptr || target == nullptr) {
        return false;
    }
    if (start == target) {
        return true;
    }

    // visited[i] is true once the location with index i has been queued
    bool* visited = new bool[locationCount];
    for (int i = 0; i < locationCount; i++) {
        visited[i] = false;
    }

    Queue<Location*> queue;
    queue.enqueue(start);
    visited[start->index] = true;
    bool found = false;

    // Standard BFS: explore level by level, skipping BLOCKED pathways
    Location* current;
    while (!found && queue.dequeue(current)) {
        Edge* edge = current->edgeHead;
        while (edge != nullptr) {
            Location* neighbour = edge->destination;
            if (!edge->blocked && !visited[neighbour->index]) {
                if (neighbour == target) {
                    found = true;
                    break;
                }
                visited[neighbour->index] = true;
                queue.enqueue(neighbour);
            }
            edge = edge->next;
        }
    }

    delete[] visited;
    return found;
}

// ---------------------------------------------------------------
// Dijkstra - shortest route
// ---------------------------------------------------------------
// Tie-break rules (required to make results the same every time):
//   1. Smaller total distance wins.
//   2. Equal distance: the route with fewer locations (fewer hops) wins.
//   3. Still equal: the route whose previous location has the smaller ID wins.
// Because we compare (distance, hops) together, Dijkstra still finds the correct answer.
// We use the simple O(V^2) version that scans for the closest unvisited location,
// since priority_queue (STL) is not allowed.

bool Graph::shortestRoute(const string& fromId, const string& toId, Route& route) const {
    route.clear();
    Location* start = findLocation(fromId);
    Location* target = findLocation(toId);
    if (start == nullptr || target == nullptr) {
        return false;
    }

    int n = locationCount;
    Location** table = buildIndexTable();
    int* dist = new int[n];          // best distance found so far
    int* hops = new int[n];          // number of steps on that best route
    bool* done = new bool[n];        // true once a location's distance is final
    Location** parent = new Location*[n]; // previous location on the best route

    for (int i = 0; i < n; i++) {
        dist[i] = INT_MAX;
        hops[i] = INT_MAX;
        done[i] = false;
        parent[i] = nullptr;
    }
    dist[start->index] = 0;
    hops[start->index] = 0;

    for (int step = 0; step < n; step++) {
        // Pick the unfinished location with the smallest (distance, hops), then smallest ID
        int u = -1;
        for (int i = 0; i < n; i++) {
            if (done[i] || dist[i] == INT_MAX) {
                continue;
            }
            if (u == -1 || dist[i] < dist[u]
                || (dist[i] == dist[u] && hops[i] < hops[u])
                || (dist[i] == dist[u] && hops[i] == hops[u] && idLess(table[i]->id, table[u]->id))) {
                u = i;
            }
        }

        // Nothing reachable is left
        if (u == -1) {
            break;
        }
        done[u] = true;
        if (table[u] == target) {
            break;
        }

        // Relax every OPEN pathway leaving u
        Edge* edge = table[u]->edgeHead;
        while (edge != nullptr) {
            int v = edge->destination->index;
            if (!edge->blocked && !done[v]) {
                int newDist = dist[u] + edge->distance;
                int newHops = hops[u] + 1;

                bool better = false;
                if (newDist < dist[v]) {
                    better = true;
                } else if (newDist == dist[v] && newHops < hops[v]) {
                    better = true;
                } else if (newDist == dist[v] && newHops == hops[v]
                           && idLess(table[u]->id, parent[v]->id)) {
                    better = true;
                }

                if (better) {
                    dist[v] = newDist;
                    hops[v] = newHops;
                    parent[v] = table[u];
                }
            }
            edge = edge->next;
        }
    }

    bool found = dist[target->index] != INT_MAX;
    if (found) {
        // Walk backwards from the target using parent pointers,
        // adding each stop at the front so the route ends up in the right order
        Location* current = target;
        while (current != nullptr) {
            route.addStopAtFront(current->id);
            current = parent[current->index];
        }
        route.setTotalDistance(dist[target->index]);
    }

    delete[] table;
    delete[] dist;
    delete[] hops;
    delete[] done;
    delete[] parent;
    return found;
}

int Graph::shortestDistance(const string& fromId, const string& toId) const {
    Route route;
    if (!shortestRoute(fromId, toId, route)) {
        return -1;
    }
    return route.getTotalDistance();
}

int Graph::getLocationCount() const {
    return locationCount;
}

Location* Graph::getHead() const {
    return head;
}

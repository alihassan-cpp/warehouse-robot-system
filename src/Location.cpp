#include "Location.h"

Edge::Edge(Location* dest, int dist, bool isBlocked) {
    destination = dest;
    distance = dist;
    blocked = isBlocked;
    next = nullptr;
}

Location::Location(const string& locationId, const string& locationType, const string& locationName, int locationIndex) {
    id = locationId;
    type = locationType;
    name = locationName;
    index = locationIndex;
    edgeHead = nullptr;
    next = nullptr;
}

Location::~Location() {
    // Each location owns its own edges, so it frees them here.
    // The destination locations themselves are deleted by the Graph.
    Edge* current = edgeHead;
    while (current != nullptr) {
        Edge* temp = current;
        current = current->next;
        delete temp;
    }
}

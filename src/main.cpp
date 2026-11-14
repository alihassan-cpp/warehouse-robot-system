#include <iostream>
#include <string>
#include "Graph.h"
#include "Route.h"
#include "Utils.h"
using namespace std;

// ---------------------------------------------------------------
// Module 1 menu: Warehouse Layout and Pathway Management
// ---------------------------------------------------------------

void handleAddLocation(Graph& warehouse) {
    string id = readId("  Enter new location ID (e.g. L16): ");
    string type = toUpper(readLine("  Enter type (RACK / PACKING / CHARGING): "));
    string name = readLine("  Enter location name: ");
    if (warehouse.addLocation(id, type, name)) {
        cout << "  Location " << id << " added.\n";
    }
}

void handleAddPathway(Graph& warehouse) {
    string fromId = readId("  Enter first location ID: ");
    string toId = readId("  Enter second location ID: ");
    int distance = readInt("  Enter distance (1 - 10000): ", 1, 10000);
    if (warehouse.addPathway(fromId, toId, distance, false)) {
        cout << "  Pathway " << fromId << " <-> " << toId << " added with distance " << distance << ".\n";
    }
}

void handleRemovePathway(Graph& warehouse) {
    string fromId = readId("  Enter first location ID: ");
    string toId = readId("  Enter second location ID: ");
    if (warehouse.removePathway(fromId, toId)) {
        cout << "  Pathway " << fromId << " <-> " << toId << " removed.\n";
    }
}

void handleBlockPathway(Graph& warehouse) {
    string fromId = readId("  Enter first location ID: ");
    string toId = readId("  Enter second location ID: ");
    if (warehouse.blockPathway(fromId, toId)) {
        cout << "  Pathway " << fromId << " <-> " << toId << " is now BLOCKED.\n";
    }
}

void handleReopenPathway(Graph& warehouse) {
    string fromId = readId("  Enter first location ID: ");
    string toId = readId("  Enter second location ID: ");
    if (warehouse.reopenPathway(fromId, toId)) {
        cout << "  Pathway " << fromId << " <-> " << toId << " is now OPEN.\n";
    }
}

void handleSearchLocation(Graph& warehouse) {
    string id = readId("  Enter location ID: ");
    warehouse.displayLocation(id);
}

void handleCheckConnected(Graph& warehouse) {
    string fromId = readId("  Enter start location ID: ");
    string toId = readId("  Enter end location ID: ");

    if (warehouse.findLocation(fromId) == nullptr) {
        cout << "  Location " << fromId << " does not exist.\n";
        return;
    }
    if (warehouse.findLocation(toId) == nullptr) {
        cout << "  Location " << toId << " does not exist.\n";
        return;
    }

    if (warehouse.isConnected(fromId, toId)) {
        cout << "  " << fromId << " and " << toId << " are connected (BFS, open pathways only).\n";
    } else {
        cout << "  " << fromId << " and " << toId << " are NOT connected.\n";
    }
}

void handleShortestRoute(Graph& warehouse) {
    string fromId = readId("  Enter start location ID: ");
    string toId = readId("  Enter end location ID: ");

    // The PDF requires a clear failure if either location is missing
    if (warehouse.findLocation(fromId) == nullptr) {
        cout << "  Route failed: start location " << fromId << " does not exist.\n";
        return;
    }
    if (warehouse.findLocation(toId) == nullptr) {
        cout << "  Route failed: end location " << toId << " does not exist.\n";
        return;
    }

    Route route;
    if (warehouse.shortestRoute(fromId, toId, route)) {
        cout << "  Shortest route: ";
        route.display();
        cout << "\n  Total distance: " << route.getTotalDistance() << " units\n";
    } else {
        cout << "  No route exists between " << fromId << " and " << toId << ".\n";
    }
}

void layoutMenu(Graph& warehouse) {
    while (true) {
        cout << "\n========== WAREHOUSE LAYOUT & PATHWAYS ==========\n";
        cout << "  1. Display all locations and connections\n";
        cout << "  2. Add a new location\n";
        cout << "  3. Add a pathway\n";
        cout << "  4. Remove a pathway\n";
        cout << "  5. Block a pathway\n";
        cout << "  6. Reopen a blocked pathway\n";
        cout << "  7. Search location by ID\n";
        cout << "  8. Check if two locations are connected (BFS)\n";
        cout << "  9. Find shortest route (Dijkstra)\n";
        cout << "  0. Back to main menu\n";

        int choice = readInt("Enter choice: ", 0, 9);
        switch (choice) {
            case 1: warehouse.displayAll(); break;
            case 2: handleAddLocation(warehouse); break;
            case 3: handleAddPathway(warehouse); break;
            case 4: handleRemovePathway(warehouse); break;
            case 5: handleBlockPathway(warehouse); break;
            case 6: handleReopenPathway(warehouse); break;
            case 7: handleSearchLocation(warehouse); break;
            case 8: handleCheckConnected(warehouse); break;
            case 9: handleShortestRoute(warehouse); break;
            case 0: return;
        }
    }
}

// ---------------------------------------------------------------
// Program start
// ---------------------------------------------------------------

int main() {
    cout << "=================================================\n";
    cout << "   INTELLIGENT WAREHOUSE ROBOT SYSTEM\n";
    cout << "=================================================\n";

    // All data comes from files; nothing is hardcoded.
    // Run the program from the project folder so "data/..." can be found.
    Graph warehouse;
    warehouse.loadLocations("data/locations.txt");
    warehouse.loadPathways("data/pathways.txt");

    while (true) {
        cout << "\n================== MAIN MENU ==================\n";
        cout << "  1. Warehouse Layout & Pathways\n";
        cout << "  0. Exit\n";

        int choice = readInt("Enter choice: ", 0, 1);
        if (choice == 1) {
            layoutMenu(warehouse);
        } else {
            cout << "Goodbye!\n";
            break;
        }
    }
    return 0;
}

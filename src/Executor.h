#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <string>
#include "Graph.h"
#include "Inventory.h"
#include "RobotFleet.h"
#include "OrderManager.h"
#include "PickingPlan.h"
using namespace std;

// Module 6: Order picking and delivery execution.
// Runs an assigned plan one trip at a time.
// Golden rule: VALIDATE FIRST, THEN APPLY. If any check fails, no data changes.
class Executor {
private:
    Graph& warehouse;
    Inventory& inventory;
    RobotFleet& fleet;
    OrderManager& orders;
    PlanManager& plans;

    // The first trip of the plan that has not been executed yet
    Trip* nextTrip(PickingPlan* plan) const;

    // True if every pathway along the route exists and is OPEN
    bool routeIsOpen(const Route& route) const;

    // All checks for one trip: routes, battery (+10 reserve), capacity, stock.
    // Prints every problem found and returns false if the trip cannot run.
    bool validateTrip(const PickingPlan* plan, const Trip* trip, const Robot* robot) const;

    // Performs the trip. Only called after validateTrip() passed.
    void applyTrip(PickingPlan* plan, Trip* trip, Robot* robot);

    // Re-runs Dijkstra between the same stops for every trip not executed yet
    bool recalculateRoutes(PickingPlan* plan, Robot* robot);

    // Releases uncollected reservations, frees the robot, and puts the order
    // back at the FRONT of the queue. Items already delivered stay delivered.
    void cancelAssignment(PickingPlan* plan);

    void displayTaskScreen(const PickingPlan* plan, const Trip* trip) const;

public:
    Executor(Graph& graph, Inventory& stock, RobotFleet& robots, OrderManager& orderManager, PlanManager& planManager);

    // Shows the EXECUTE PICKING TASK screen for an order and handles its options
    void runOrder(const string& orderId);

    // Lists orders that have an assigned plan
    void listAssignedOrders() const;
};

#endif

#ifndef PLANNER_H
#define PLANNER_H

#include <string>
#include "Graph.h"
#include "Inventory.h"
#include "RobotFleet.h"
#include "OrderManager.h"
#include "PickingPlan.h"
using namespace std;

// Module 5: Intelligent robot assignment and picking plan generation.
// Uses every earlier module: the graph (routes), inventory (allocation and reservation),
// the fleet (eligibility and battery) and the order queue.
class Planner {
private:
    Graph& warehouse;
    Inventory& inventory;
    RobotFleet& fleet;
    OrderManager& orders;
    PlanManager& plans;

    // Nearest PACKING location from 'from' (Dijkstra). Ties go to the smaller ID.
    // Fills 'route' and returns the station ID, or "" if none is reachable.
    string nearestPackingStation(const string& from, Route& route) const;

    // Units still to deliver for an item (ordered - already delivered)
    int unitsNeeded(const OrderItem* item) const;

    // Prints and returns true if any product lacks enough available stock
    bool hasShortage(const Order* order, bool print) const;

    // Total weight and heaviest unit of what still has to be picked
    void measureOrder(const Order* order, bool allowPartial, double& totalWeight, double& heaviestUnit) const;

    // Builds a plan for one specific robot using the nearest-reachable-rack heuristic.
    // Returns nullptr (and a reason) if the plan cannot be built.
    PickingPlan* buildPlan(const Order* order, const Robot* robot, bool allowPartial, string& failReason) const;

    // Builds a plan for every eligible robot and keeps the best one:
    // fewest trips, then smallest distance, then smallest RobotID.
    PickingPlan* chooseBestPlan(const Order* order, bool allowPartial, bool showComparison, string& failReason) const;

    // All-or-nothing reservation of every pickup in the plan
    bool reservePlan(const PickingPlan* plan);

    // Reserves stock, assigns the robot, updates statuses and stores the plan
    bool confirmAndAssign(Order* order, PickingPlan* plan);

public:
    Planner(Graph& graph, Inventory& stock, RobotFleet& robots, OrderManager& orderManager, PlanManager& planManager);

    // Takes the oldest plannable order from the queue, shows its plan and asks to confirm
    void processNextOrder();

    // Shows the robot comparison and best plan for any pending order without reserving
    void previewOrder(const string& orderId) const;
};

#endif

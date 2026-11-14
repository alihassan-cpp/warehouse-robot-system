#include "Executor.h"
#include "Utils.h"
#include <iostream>
#include <iomanip>

Executor::Executor(Graph& graph, Inventory& stock, RobotFleet& robots, OrderManager& orderManager, PlanManager& planManager)
    : warehouse(graph), inventory(stock), fleet(robots), orders(orderManager), plans(planManager) {
}

Trip* Executor::nextTrip(PickingPlan* plan) const {
    Trip* trip = plan->trips;
    while (trip != nullptr && trip->executed) {
        trip = trip->next;
    }
    return trip;
}

bool Executor::routeIsOpen(const Route& route) const {
    // Check every consecutive pair of stops on the route
    RouteNode* current = route.getHead();
    while (current != nullptr && current->next != nullptr) {
        string from = current->locationId;
        string to = current->next->locationId;
        if (!warehouse.pathwayExists(from, to) || warehouse.isPathwayBlocked(from, to)) {
            return false;
        }
        current = current->next;
    }
    return true;
}

// ---------------------------------------------------------------
// Validation (no data is changed here)
// ---------------------------------------------------------------

bool Executor::validateTrip(const PickingPlan* plan, const Trip* trip, const Robot* robot) const {
    bool ok = true;

    // The robot must still be working on this order
    if (robot->assignedTask != plan->orderId || robot->status != RobotStatus::Assigned) {
        cout << "  Check failed: robot " << robot->id << " is not assigned to " << plan->orderId << ".\n";
        return false;
    }

    // The trip must start where the robot actually is
    if (robot->locationId != trip->startLocation) {
        cout << "  Check failed: robot is at " << robot->locationId << " but the trip starts at "
             << trip->startLocation << ". Use Recalculate Route.\n";
        ok = false;
    }

    // 1. Routes: every pathway on every leg must still be OPEN
    TripStop* stop = trip->stops;
    while (stop != nullptr) {
        if (!routeIsOpen(*stop->routeIn)) {
            cout << "  Check failed: the route to rack " << stop->rackId << " uses a blocked or removed pathway.\n";
            ok = false;
        }
        stop = stop->next;
    }
    if (!routeIsOpen(*trip->routeToPacking)) {
        cout << "  Check failed: the route to packing station " << trip->packingStation
             << " uses a blocked or removed pathway.\n";
        ok = false;
    }

    // 2. Battery: this trip's consumption plus the 10% safety reserve
    double needed = trip->battery() + 10;
    if (robot->battery < needed) {
        cout << "  Check failed: battery " << fixed << setprecision(1) << robot->battery
             << "% is below the " << needed << "% needed (trip + 10% reserve). Charge the robot first.\n";
        ok = false;
    }

    // 3. Capacity
    if (trip->weight > robot->capacity) {
        cout << "  Check failed: trip weight " << trip->weight << " kg exceeds capacity "
             << robot->capacity << " kg.\n";
        ok = false;
    }

    // 4. Stock: each pickup must still be reserved for this order and physically present
    stop = trip->stops;
    while (stop != nullptr) {
        if (stop->collected) {
            cout << "  Check failed: rack " << stop->rackId << " was already collected.\n";
            ok = false;
        }
        Pickup* pickup = stop->pickups;
        while (pickup != nullptr) {
            Product* product = inventory.findProduct(pickup->productId);
            RackEntry* rack = nullptr;
            if (product != nullptr) {
                rack = product->findRack(stop->rackId);
            }
            if (rack == nullptr) {
                cout << "  Check failed: " << pickup->productId << " is no longer in rack " << stop->rackId << ".\n";
                ok = false;
            } else if (rack->reservedBy(plan->orderId) < pickup->quantity || rack->totalQuantity < pickup->quantity) {
                cout << "  Check failed: rack " << stop->rackId << " does not hold the " << pickup->quantity
                     << " reserved units of " << pickup->productId << ".\n";
                ok = false;
            }
            pickup = pickup->next;
        }
        stop = stop->next;
    }

    return ok;
}

// ---------------------------------------------------------------
// Applying a trip (only after validation passed)
// ---------------------------------------------------------------

void Executor::applyTrip(PickingPlan* plan, Trip* trip, Robot* robot) {
    Order* order = orders.findOrder(plan->orderId);

    // Collect at each rack. collect() lowers total, reserved and the order's record together.
    TripStop* stop = trip->stops;
    while (stop != nullptr) {
        Pickup* pickup = stop->pickups;
        while (pickup != nullptr) {
            RackEntry* rack = inventory.findProduct(pickup->productId)->findRack(stop->rackId);
            rack->collect(plan->orderId, pickup->quantity);
            cout << "  Collected " << pickup->quantity << " x " << pickup->productId
                 << " from " << stop->rackId << "\n";
            pickup = pickup->next;
        }
        stop->collected = true; // this stop can never be collected again
        stop = stop->next;
    }

    // Move the robot and use its battery
    double used = trip->battery();
    robot->battery -= used;
    if (robot->battery < 0) {
        robot->battery = 0;
    }
    robot->locationId = trip->packingStation;

    // Deliver: record delivered units on the order (status updates itself)
    stop = trip->stops;
    while (stop != nullptr) {
        Pickup* pickup = stop->pickups;
        while (pickup != nullptr) {
            order->recordDelivery(pickup->productId, pickup->quantity);
            pickup = pickup->next;
        }
        stop = stop->next;
    }
    trip->executed = true;

    cout << "  Delivered to packing station " << trip->packingStation << ". Robot " << robot->id
         << " battery now " << fixed << setprecision(1) << robot->battery << "%.\n";
    cout << "  Order " << order->orderId << " is now " << statusToString(order->status) << ".\n";

    // After the final trip, release the robot and retire the plan
    if (nextTrip(plan) == nullptr) {
        fleet.releaseRobot(robot->id);
        order->robotId = "";
        cout << "  All trips done. Robot " << robot->id << " released.\n";
        if (order->status != OrderStatus::Completed) {
            cout << "  (Only part of the order could be reserved, so it stays Partially Fulfilled.)\n";
        }
        plans.remove(plan->orderId); // 'plan' is deleted here, so it is not used after this line
    }
}

// ---------------------------------------------------------------
// Recalculate Route
// ---------------------------------------------------------------

bool Executor::recalculateRoutes(PickingPlan* plan, Robot* robot) {
    // First check that every leg is still reachable, without changing anything
    Trip* trip = nextTrip(plan);
    string here = robot->locationId;
    while (trip != nullptr) {
        TripStop* stop = trip->stops;
        while (stop != nullptr) {
            if (warehouse.shortestDistance(here, stop->locationId) < 0) {
                cout << "  Rack " << stop->rackId << " (" << stop->locationId << ") is now unreachable.\n";
                cout << "  Choose Cancel Assignment so the order can be re-planned.\n";
                return false;
            }
            here = stop->locationId;
            stop = stop->next;
        }
        if (warehouse.shortestDistance(here, trip->packingStation) < 0) {
            cout << "  Packing station " << trip->packingStation << " is now unreachable.\n";
            cout << "  Choose Cancel Assignment so the order can be re-planned.\n";
            return false;
        }
        here = trip->packingStation;
        trip = trip->next;
    }

    // Everything is reachable: rebuild each route and its battery cost
    trip = nextTrip(plan);
    here = robot->locationId;
    while (trip != nullptr) {
        trip->startLocation = here;
        TripStop* stop = trip->stops;
        while (stop != nullptr) {
            warehouse.shortestRoute(here, stop->locationId, *stop->routeIn);
            stop->batteryIn = robot->legConsumption(stop->routeIn->getTotalDistance(), stop->weightIn);
            here = stop->locationId;
            stop = stop->next;
        }
        warehouse.shortestRoute(here, trip->packingStation, *trip->routeToPacking);
        trip->batteryToPacking = robot->legConsumption(trip->routeToPacking->getTotalDistance(), trip->weight);
        here = trip->packingStation;
        trip = trip->next;
    }
    cout << "  Routes recalculated with the current warehouse layout.\n";
    return true;
}

// ---------------------------------------------------------------
// Cancel Assignment
// ---------------------------------------------------------------

void Executor::cancelAssignment(PickingPlan* plan) {
    string orderId = plan->orderId;
    Order* order = orders.findOrder(orderId);

    // Reservation records only hold what was not collected yet, so this frees exactly that
    int released = inventory.releaseOrder(orderId);
    fleet.releaseRobot(plan->robotId);
    order->robotId = "";

    // Back to Pending, at the FRONT of the queue so it is processed next
    orders.returnToFrontOfQueue(order);
    plans.remove(orderId);

    cout << "  Assignment cancelled. " << released << " reserved units released, robot freed.\n";
    cout << "  " << orderId << " is Pending again at the front of the queue (delivered items stay delivered).\n";
}

// ---------------------------------------------------------------
// Screens
// ---------------------------------------------------------------

void Executor::displayTaskScreen(const PickingPlan* plan, const Trip* trip) const {
    cout << "\n=====================================\n";
    cout << "   EXECUTE PICKING TASK\n";
    cout << "=====================================\n";
    cout << "\n  Trip " << trip->number << " of " << plan->tripCount;
    plan->displayTrip(trip);
}

void Executor::listAssignedOrders() const {
    PickingPlan* plan = plans.getHead();
    if (plan == nullptr) {
        cout << "  No orders are assigned right now. Generate a plan first (menu 5).\n";
        return;
    }
    cout << "\n  Assigned orders:\n";
    while (plan != nullptr) {
        int done = 0;
        Trip* trip = plan->trips;
        while (trip != nullptr) {
            if (trip->executed) {
                done++;
            }
            trip = trip->next;
        }
        cout << "  " << plan->orderId << "  robot " << plan->robotId << "  trips done "
             << done << "/" << plan->tripCount << "\n";
        plan = plan->next;
    }
}

void Executor::runOrder(const string& orderId) {
    while (true) {
        // Look the plan up again each time; it disappears once the order is finished
        PickingPlan* plan = plans.find(orderId);
        if (plan == nullptr) {
            cout << "  No assigned plan for " << orderId << ".\n";
            return;
        }
        Robot* robot = fleet.findRobot(plan->robotId);
        Trip* trip = nextTrip(plan);
        if (robot == nullptr || trip == nullptr) {
            cout << "  Plan for " << orderId << " has nothing left to execute.\n";
            return;
        }

        displayTaskScreen(plan, trip);
        cout << "\n  Robot " << robot->id << " is at " << robot->locationId
             << " with " << fixed << setprecision(1) << robot->battery << "% battery.\n";
        cout << "\n  1. Execute Trip\n";
        cout << "  2. Recalculate Route\n";
        cout << "  3. Cancel Assignment\n";
        cout << "  4. Assign but not execute now\n";

        int choice = readInt("  Choice: ", 1, 4);
        if (choice == 1) {
            if (validateTrip(plan, trip, robot)) {
                applyTrip(plan, trip, robot);
                if (plans.find(orderId) == nullptr) {
                    return; // order finished
                }
            } else {
                cout << "  Trip NOT executed. No records were changed.\n";
            }
        } else if (choice == 2) {
            recalculateRoutes(plan, robot);
        } else if (choice == 3) {
            cancelAssignment(plan);
            return;
        } else {
            cout << "  " << orderId << " stays Assigned. You can execute it later.\n";
            return;
        }
    }
}

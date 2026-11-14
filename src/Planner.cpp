#include "Planner.h"
#include "Utils.h"
#include <iostream>
#include <iomanip>

// ---------------------------------------------------------------
// Temporary helper used only while building a plan:
// all the products to be picked from one rack, grouped together.
// ---------------------------------------------------------------
class RackGroup {
public:
    string rackId;
    string locationId;
    Pickup* pickups;
    bool visited;
    RackGroup* next;

    RackGroup(const string& rack, const string& location) {
        rackId = rack;
        locationId = location;
        pickups = nullptr;
        visited = false;
        next = nullptr;
    }

    ~RackGroup() {
        Pickup* current = pickups;
        while (current != nullptr) {
            Pickup* temp = current;
            current = current->next;
            delete temp;
        }
    }

    void addPickup(const string& productId, int quantity, double unitWeight) {
        Pickup* pickup = new Pickup(productId, quantity, unitWeight);
        pickup->next = pickups;
        pickups = pickup;
    }
};

static void deleteGroups(RackGroup* head) {
    while (head != nullptr) {
        RackGroup* temp = head;
        head = head->next;
        delete temp;
    }
}

// ---------------------------------------------------------------

Planner::Planner(Graph& graph, Inventory& stock, RobotFleet& robots, OrderManager& orderManager, PlanManager& planManager)
    : warehouse(graph), inventory(stock), fleet(robots), orders(orderManager), plans(planManager) {
}

string Planner::nearestPackingStation(const string& from, Route& route) const {
    string bestId = "";
    int bestDistance = -1;

    Location* location = warehouse.getHead();
    while (location != nullptr) {
        if (location->type == "PACKING") {
            int d = warehouse.shortestDistance(from, location->id);
            if (d >= 0 && (bestId == "" || d < bestDistance
                           || (d == bestDistance && idLess(location->id, bestId)))) {
                bestId = location->id;
                bestDistance = d;
            }
        }
        location = location->next;
    }

    if (bestId != "") {
        warehouse.shortestRoute(from, bestId, route);
    }
    return bestId;
}

int Planner::unitsNeeded(const OrderItem* item) const {
    return item->quantity - item->delivered;
}

bool Planner::hasShortage(const Order* order, bool print) const {
    bool shortage = false;
    OrderItem* item = order->itemHead;
    while (item != nullptr) {
        int needed = unitsNeeded(item);
        Product* product = inventory.findProduct(item->productId);
        if (needed > 0 && product != nullptr && product->availableStock() < needed) {
            shortage = true;
            if (print) {
                cout << "  Shortage in " << order->orderId << ": " << item->productId << " needs " << needed
                     << ", only " << product->availableStock() << " available.\n";
            }
        }
        item = item->next;
    }
    return shortage;
}

void Planner::measureOrder(const Order* order, bool allowPartial, double& totalWeight, double& heaviestUnit) const {
    totalWeight = 0;
    heaviestUnit = 0;
    OrderItem* item = order->itemHead;
    while (item != nullptr) {
        Product* product = inventory.findProduct(item->productId);
        int units = unitsNeeded(item);
        if (allowPartial && product != nullptr && product->availableStock() < units) {
            units = product->availableStock();
        }
        if (product != nullptr && units > 0) {
            totalWeight += units * product->unitWeight;
            if (product->unitWeight > heaviestUnit) {
                heaviestUnit = product->unitWeight;
            }
        }
        item = item->next;
    }
}

// ---------------------------------------------------------------
// Building one plan for one robot
// ---------------------------------------------------------------
// Heuristic (from the PDF):
//  1. Allocate stock to racks with the Module 2 rule, for every product.
//  2. Visit order: from the robot, always go to the nearest rack still to visit
//     (Dijkstra distance, ties -> smaller RackID).
//  3. Trips: add pickups in visiting order until the next unit would exceed capacity.
//     Then go to the nearest packing station, unload, and start the next trip there.
//  4. After the last pickup, go to the nearest packing station.
//  5. Battery is calculated leg by leg with the weight carried on that leg.

PickingPlan* Planner::buildPlan(const Order* order, const Robot* robot, bool allowPartial, string& failReason) const {
    // ---- Step 1: allocation, grouped by rack ----
    RackGroup* groups = nullptr;
    int groupCount = 0;

    OrderItem* item = order->itemHead;
    while (item != nullptr) {
        int needed = unitsNeeded(item);
        if (needed <= 0) {
            item = item->next;
            continue;
        }

        Product* product = inventory.findProduct(item->productId);
        if (product->unitWeight > robot->capacity) {
            failReason = "cannot lift one unit of " + item->productId;
            deleteGroups(groups);
            return nullptr;
        }

        Allocation allocation;
        int shortage;
        if (!inventory.allocate(item->productId, needed, robot->locationId, warehouse, allocation, shortage, allowPartial)) {
            if (allowPartial) {
                // Nothing reachable for this product; partial mode just skips it
                item = item->next;
                continue;
            }
            failReason = "not enough reachable stock of " + item->productId;
            deleteGroups(groups);
            return nullptr;
        }

        // Put each allocated amount into its rack's group
        AllocationNode* node = allocation.getHead();
        while (node != nullptr) {
            RackGroup* group = groups;
            while (group != nullptr && group->rackId != node->rackId) {
                group = group->next;
            }
            if (group == nullptr) {
                group = new RackGroup(node->rackId, node->locationId);
                group->next = groups;
                groups = group;
                groupCount++;
            }
            group->addPickup(item->productId, node->quantity, product->unitWeight);
            node = node->next;
        }
        item = item->next;
    }

    if (groupCount == 0) {
        failReason = "nothing can be picked";
        return nullptr;
    }

    // ---- Step 2: visiting sequence (nearest rack first) ----
    RackGroup** visitOrder = new RackGroup*[groupCount];
    string current = robot->locationId;
    for (int k = 0; k < groupCount; k++) {
        RackGroup* best = nullptr;
        int bestDistance = -1;
        RackGroup* group = groups;
        while (group != nullptr) {
            if (!group->visited) {
                int d = warehouse.shortestDistance(current, group->locationId);
                if (d >= 0 && (best == nullptr || d < bestDistance
                               || (d == bestDistance && idLess(group->rackId, best->rackId)))) {
                    best = group;
                    bestDistance = d;
                }
            }
            group = group->next;
        }

        if (best == nullptr) {
            failReason = "a planned rack is unreachable";
            delete[] visitOrder;
            deleteGroups(groups);
            return nullptr;
        }
        best->visited = true;
        visitOrder[k] = best;
        current = best->locationId;
    }

    // ---- Steps 3 and 4: split into trips ----
    PickingPlan* plan = new PickingPlan(order->orderId, robot->id);
    Trip* trip = new Trip(1, robot->locationId);
    string here = robot->locationId;
    double load = 0;
    bool failed = false;

    for (int k = 0; k < groupCount && !failed; k++) {
        RackGroup* group = visitOrder[k];
        Pickup* pickup = group->pickups;
        while (pickup != nullptr && !failed) {
            int remaining = pickup->quantity;
            while (remaining > 0 && !failed) {
                // How many more whole units fit in the robot right now?
                int fit = (int)((robot->capacity - load) / pickup->unitWeight + 0.000001);

                if (fit <= 0) {
                    // Robot is full: finish this trip at the nearest packing station
                    trip->packingStation = nearestPackingStation(here, *trip->routeToPacking);
                    if (trip->packingStation == "") {
                        failReason = "no packing station is reachable";
                        failed = true;
                        break;
                    }
                    trip->batteryToPacking = robot->legConsumption(trip->routeToPacking->getTotalDistance(), load);
                    trip->weight = load;
                    plan->appendTrip(trip);

                    // The next trip starts empty, from that packing station
                    here = trip->packingStation;
                    trip = new Trip(plan->tripCount + 1, here);
                    load = 0;
                    continue;
                }

                int take = remaining;
                if (fit < take) {
                    take = fit;
                }

                // Moving to a new rack: add a stop and the route leg to reach it
                if (trip->lastStop == nullptr || trip->lastStop->rackId != group->rackId) {
                    TripStop* stop = new TripStop(group->rackId, group->locationId);
                    if (!warehouse.shortestRoute(here, group->locationId, *stop->routeIn)) {
                        delete stop;
                        failReason = "rack " + group->rackId + " is unreachable";
                        failed = true;
                        break;
                    }
                    stop->weightIn = load;
                    stop->batteryIn = robot->legConsumption(stop->routeIn->getTotalDistance(), load);
                    trip->appendStop(stop);
                    here = group->locationId;
                }

                trip->lastStop->addPickup(pickup->productId, take, pickup->unitWeight);
                load += take * pickup->unitWeight;
                remaining -= take;
            }
            pickup = pickup->next;
        }
    }

    // Final trip: from the last pickup to the nearest packing station
    if (!failed) {
        trip->packingStation = nearestPackingStation(here, *trip->routeToPacking);
        if (trip->packingStation == "") {
            failReason = "no packing station is reachable";
            failed = true;
        } else {
            trip->batteryToPacking = robot->legConsumption(trip->routeToPacking->getTotalDistance(), load);
            trip->weight = load;
            plan->appendTrip(trip);
            trip = nullptr;
        }
    }

    delete[] visitOrder;
    deleteGroups(groups);

    if (failed) {
        delete trip; // the unfinished trip was never added to the plan
        delete plan;
        return nullptr;
    }
    return plan;
}

// ---------------------------------------------------------------
// Robot Selection Rule
// ---------------------------------------------------------------

PickingPlan* Planner::chooseBestPlan(const Order* order, bool allowPartial, bool showComparison, string& failReason) const {
    double totalWeight, heaviestUnit;
    measureOrder(order, allowPartial, totalWeight, heaviestUnit);

    PickingPlan* best = nullptr;
    int notAvailable = 0;
    int notEligible = 0;
    int tooWeak = 0;
    int lowBattery = 0;
    int noPlan = 0;

    if (showComparison) {
        cout << "\n  Robot comparison for " << order->orderId << " (" << fixed << setprecision(1)
             << totalWeight << " kg, heaviest unit " << heaviestUnit << " kg):\n";
    }

    Robot* robot = fleet.getHead();
    while (robot != nullptr) {
        string result;
        PickingPlan* plan = nullptr;

        if (robot->status != RobotStatus::Available || robot->hasActiveTask()) {
            result = "skipped: " + robotStatusToString(robot->status);
            notAvailable++;
        } else if (!robot->typeAllowsOrder(totalWeight, heaviestUnit)) {
            result = "skipped: " + robot->typeName() + " type rule";
            notEligible++;
        } else if (!robot->canCarryUnit(heaviestUnit)) {
            result = "skipped: cannot lift heaviest unit";
            tooWeak++;
        } else {
            string reason;
            plan = buildPlan(order, robot, allowPartial, reason);
            if (plan == nullptr) {
                result = "no plan: " + reason;
                noPlan++;
            } else {
                // Safety reserve: battery must cover the whole plan plus 10
                double needed = plan->totalBattery() + 10;
                if (robot->battery < needed) {
                    result = "battery too low (has " + to_string((int)robot->battery) + "%, needs about "
                           + to_string((int)(needed + 0.999)) + "%)";
                    lowBattery++;
                    delete plan;
                    plan = nullptr;
                } else {
                    result = to_string(plan->tripCount) + " trip(s), " + to_string(plan->totalDistance())
                           + " units, battery ~" + to_string((int)(plan->totalBattery() + 0.5)) + "%";
                }
            }
        }

        if (showComparison) {
            cout << "    " << left << setw(6) << robot->id << setw(12) << robot->typeName() << result << "\n";
        }

        // Keep the better plan: fewer trips, then shorter distance, then smaller RobotID
        if (plan != nullptr) {
            bool better = false;
            if (best == nullptr) {
                better = true;
            } else if (plan->tripCount < best->tripCount) {
                better = true;
            } else if (plan->tripCount == best->tripCount && plan->totalDistance() < best->totalDistance()) {
                better = true;
            } else if (plan->tripCount == best->tripCount && plan->totalDistance() == best->totalDistance()
                       && idLess(plan->robotId, best->robotId)) {
                better = true;
            }

            if (better) {
                delete best;
                best = plan;
            } else {
                delete plan;
            }
        }
        robot = robot->next;
    }

    if (best == nullptr) {
        // Explain why no robot could take the order
        failReason = "";
        if (fleet.getHead() == nullptr || notAvailable == fleet.size()) {
            failReason = "no available robot";
        } else {
            if (notEligible > 0) failReason += "eligibility (" + to_string(notEligible) + ") ";
            if (tooWeak > 0) failReason += "capacity (" + to_string(tooWeak) + ") ";
            if (noPlan > 0) failReason += "route/stock (" + to_string(noPlan) + ") ";
            if (lowBattery > 0) failReason += "battery (" + to_string(lowBattery) + ") - consider charging robots ";
            if (notAvailable > 0) failReason += "not available (" + to_string(notAvailable) + ")";
        }
    }
    return best;
}

// ---------------------------------------------------------------
// Reservation and assignment
// ---------------------------------------------------------------

bool Planner::reservePlan(const PickingPlan* plan) {
    // Pass 1 - validate only. The same product and rack can appear in several trips,
    // so add up the plan's total demand per (product, rack) before comparing.
    Trip* trip = plan->trips;
    while (trip != nullptr) {
        TripStop* stop = trip->stops;
        while (stop != nullptr) {
            Pickup* pickup = stop->pickups;
            while (pickup != nullptr) {
                Product* product = inventory.findProduct(pickup->productId);
                if (product == nullptr) {
                    return false;
                }
                RackEntry* rack = product->findRack(stop->rackId);
                if (rack == nullptr) {
                    return false;
                }

                int demand = 0;
                Trip* t = plan->trips;
                while (t != nullptr) {
                    TripStop* s = t->stops;
                    while (s != nullptr) {
                        if (s->rackId == stop->rackId) {
                            Pickup* p = s->pickups;
                            while (p != nullptr) {
                                if (p->productId == pickup->productId) {
                                    demand += p->quantity;
                                }
                                p = p->next;
                            }
                        }
                        s = s->next;
                    }
                    t = t->next;
                }

                if (rack->available() < demand) {
                    return false;
                }
                pickup = pickup->next;
            }
            stop = stop->next;
        }
        trip = trip->next;
    }

    // Pass 2 - apply. Everything was checked, so these cannot fail.
    trip = plan->trips;
    while (trip != nullptr) {
        TripStop* stop = trip->stops;
        while (stop != nullptr) {
            Pickup* pickup = stop->pickups;
            while (pickup != nullptr) {
                RackEntry* rack = inventory.findProduct(pickup->productId)->findRack(stop->rackId);
                rack->addReservation(plan->orderId, pickup->quantity);
                pickup = pickup->next;
            }
            stop = stop->next;
        }
        trip = trip->next;
    }
    return true;
}

bool Planner::confirmAndAssign(Order* order, PickingPlan* plan) {
    if (!reservePlan(plan)) {
        cout << "  Reservation failed: stock changed. Nothing was reserved.\n";
        return false;
    }
    order->status = OrderStatus::Reserved;

    if (!fleet.assignTask(plan->robotId, order->orderId)) {
        // Undo the reservation so the system stays consistent
        inventory.releaseOrder(order->orderId);
        order->status = OrderStatus::Pending;
        return false;
    }

    order->status = OrderStatus::Assigned;
    order->robotId = plan->robotId;
    orders.removeFromQueue(order); // it is no longer pending
    plans.add(plan);
    return true;
}

// ---------------------------------------------------------------
// Menu actions
// ---------------------------------------------------------------

void Planner::processNextOrder() {
    Queue<Order*>& queue = orders.getQueue();
    if (queue.isEmpty()) {
        cout << "  The pending order queue is empty.\n";
        return;
    }

    // Walk the queue from the front. Orders that cannot be planned are skipped
    // but keep their position in the queue.
    QueueNode<Order*>* node = queue.getFront();
    while (node != nullptr) {
        Order* order = node->data;
        node = node->next;
        cout << "\n  ---- Checking order " << order->orderId << " ----\n";

        bool partial = false;
        if (hasShortage(order, true)) {
            cout << "  1. Skip (order stays Pending)\n";
            cout << "  2. Reserve only the available quantity\n";
            if (readInt("  Choice: ", 1, 2) == 1) {
                cout << "  " << order->orderId << " skipped, trying the next order.\n";
                continue;
            }
            partial = true;
        }

        string reason;
        PickingPlan* plan = chooseBestPlan(order, partial, true, reason);
        if (plan == nullptr) {
            cout << "  No feasible robot for " << order->orderId << ". Reason: " << reason << "\n";
            cout << "  Trying the next order.\n";
            continue;
        }

        cout << "\n=====================================\n";
        cout << "   PICKING PLAN\n";
        cout << "=====================================";
        plan->display();

        cout << "\n  1. Confirm plan (reserve stock and assign robot)\n";
        cout << "  2. Reject plan (order stays Pending)\n";
        if (readInt("  Choice: ", 1, 2) == 2) {
            delete plan;
            cout << "  Plan rejected. " << order->orderId << " stays in the queue.\n";
            return;
        }

        string robotId = plan->robotId;
        if (confirmAndAssign(order, plan)) {
            cout << "  Stock reserved. " << order->orderId << " is Assigned to " << robotId
                 << ". Execute it from the Order Execution menu.\n";
        } else {
            delete plan;
        }
        return;
    }
    cout << "\n  No order in the queue could be planned right now.\n";
}

void Planner::previewOrder(const string& orderId) const {
    Order* order = orders.findOrder(orderId);
    if (order == nullptr) {
        cout << "  Order " << orderId << " not found.\n";
        return;
    }
    if (order->status != OrderStatus::Pending) {
        cout << "  Only Pending orders can be planned (" << orderId << " is "
             << statusToString(order->status) << ").\n";
        return;
    }

    bool partial = hasShortage(order, true);
    if (partial) {
        cout << "  Preview uses only the available quantity.\n";
    }

    string reason;
    PickingPlan* plan = chooseBestPlan(order, partial, true, reason);
    if (plan == nullptr) {
        cout << "  No feasible robot. Reason: " << reason << "\n";
        return;
    }
    cout << "\n  Best plan (preview only, nothing reserved):";
    plan->display();
    delete plan;
}

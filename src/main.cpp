#include <iostream>
#include <string>
#include "Graph.h"
#include "Route.h"
#include "Inventory.h"
#include "Allocation.h"
#include "OrderManager.h"
#include "RobotFleet.h"
#include "PickingPlan.h"
#include "Planner.h"
#include "Executor.h"
#include "ChargingManager.h"
#include "History.h"
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

void handleBlockPathway(Graph& warehouse, HistoryManager& history) {
    string fromId = readId("  Enter first location ID: ");
    string toId = readId("  Enter second location ID: ");
    if (warehouse.blockPathway(fromId, toId)) {
        history.recordBlock(fromId, toId); // reversible, so it goes on the undo stack
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

void layoutMenu(Graph& warehouse, HistoryManager& history) {
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
            case 5: handleBlockPathway(warehouse, history); break;
            case 6: handleReopenPathway(warehouse); break;
            case 7: handleSearchLocation(warehouse); break;
            case 8: handleCheckConnected(warehouse); break;
            case 9: handleShortestRoute(warehouse); break;
            case 0: return;
        }
    }
}

// ---------------------------------------------------------------
// Module 2 menu: Product Inventory and Storage Racks
// ---------------------------------------------------------------

void handleInsertProduct(Inventory& inventory) {
    string id = readId("  Enter product ID (e.g. P111): ");
    string name = readLine("  Enter product name: ");
    double weight = readDouble("  Enter unit weight in kg (0.1 - 60): ", 0.1, 60);
    int reorder = readInt("  Enter reorder level (0 - 100000): ", 0, 100000);
    if (inventory.addProduct(id, name, weight, reorder)) {
        cout << "  Product " << id << " inserted into the AVL tree.\n";
    }
}

void handleDeleteProduct(Inventory& inventory) {
    string id = readId("  Enter product ID to delete: ");
    if (inventory.deleteProduct(id)) {
        cout << "  Product " << id << " deleted and tree rebalanced.\n";
    }
}

void handleAddRackEntry(Inventory& inventory) {
    string productId = readId("  Enter product ID: ");
    string rackId = readId("  Enter rack ID (e.g. RK05): ");
    int quantity = readInt("  Enter quantity (0 - 100000): ", 0, 100000);
    if (inventory.addRackEntry(productId, rackId, quantity)) {
        cout << "  Rack " << rackId << " added to " << productId << " with " << quantity << " units.\n";
    }
}

void handleUpdateQuantity(Inventory& inventory, HistoryManager& history) {
    string productId = readId("  Enter product ID: ");
    string rackId = readId("  Enter rack ID: ");
    int newQuantity = readInt("  Enter new total quantity (0 - 100000): ", 0, 100000);
    int oldQuantity;
    if (inventory.updateQuantity(productId, rackId, newQuantity, oldQuantity)) {
        history.recordQuantityUpdate(productId, rackId, oldQuantity, newQuantity);
        cout << "  " << productId << " in " << rackId << " updated: " << oldQuantity << " -> " << newQuantity << "\n";
    }
}

void handleRemoveRackEntry(Inventory& inventory) {
    string productId = readId("  Enter product ID: ");
    string rackId = readId("  Enter rack ID to remove: ");
    if (inventory.removeRackEntry(productId, rackId)) {
        cout << "  Rack " << rackId << " removed from " << productId << ".\n";
    }
}

void handleStockTotals(Inventory& inventory, bool availableOnly) {
    string productId = readId("  Enter product ID: ");
    if (inventory.findProduct(productId) == nullptr) {
        cout << "  Product " << productId << " not found.\n";
        return;
    }
    if (availableOnly) {
        cout << "  Available stock of " << productId << " (total - reserved): "
             << inventory.availableStock(productId) << " units\n";
    } else {
        cout << "  Total stock of " << productId << " across all racks: "
             << inventory.totalStock(productId) << " units\n";
    }
}

void handlePreviewAllocation(Inventory& inventory, Graph& warehouse) {
    string productId = readId("  Enter product ID: ");
    if (inventory.findProduct(productId) == nullptr) {
        cout << "  Product " << productId << " not found.\n";
        return;
    }
    int quantity = readInt("  Enter quantity needed (1 - 100000): ", 1, 100000);
    string reference = readId("  Enter robot's current location (e.g. L01): ");
    if (warehouse.findLocation(reference) == nullptr) {
        cout << "  Location " << reference << " does not exist.\n";
        return;
    }

    Allocation allocation;
    int shortage;
    if (inventory.allocate(productId, quantity, reference, warehouse, allocation, shortage, false)) {
        cout << "\n  Allocation for " << quantity << " x " << productId << " from " << reference << ":\n";
        allocation.display();
        cout << "  (Preview only. Nothing has been reserved.)\n";
    } else {
        cout << "  Shortage: only " << (quantity - shortage) << " reachable units available, "
             << shortage << " short. Order would stay Pending.\n";
    }
}

void inventoryMenu(Inventory& inventory, Graph& warehouse, HistoryManager& history) {
    while (true) {
        cout << "\n========== PRODUCT INVENTORY & RACKS ==========\n";
        cout << "  1. Display all products\n";
        cout << "  2. Display AVL tree structure\n";
        cout << "  3. Insert a new product\n";
        cout << "  4. Delete a product\n";
        cout << "  5. Search product by ID (with its racks)\n";
        cout << "  6. Add a rack entry to a product\n";
        cout << "  7. Update quantity in a rack\n";
        cout << "  8. Remove a rack entry\n";
        cout << "  9. Total stock of a product\n";
        cout << " 10. Available stock of a product\n";
        cout << " 11. Low stock report (below reorder level)\n";
        cout << " 12. Display all racks and locations\n";
        cout << " 13. Preview rack allocation for a quantity\n";
        cout << "  0. Back to main menu\n";

        int choice = readInt("Enter choice: ", 0, 13);
        switch (choice) {
            case 1: inventory.displayAllProducts(); break;
            case 2: inventory.displayTreeStructure(); break;
            case 3: handleInsertProduct(inventory); break;
            case 4: handleDeleteProduct(inventory); break;
            case 5: inventory.displayProduct(readId("  Enter product ID: ")); break;
            case 6: handleAddRackEntry(inventory); break;
            case 7: handleUpdateQuantity(inventory, history); break;
            case 8: handleRemoveRackEntry(inventory); break;
            case 9: handleStockTotals(inventory, false); break;
            case 10: handleStockTotals(inventory, true); break;
            case 11: inventory.displayLowStock(); break;
            case 12: inventory.displayRacks(); break;
            case 13: handlePreviewAllocation(inventory, warehouse); break;
            case 0: return;
        }
    }
}

// ---------------------------------------------------------------
// Module 3 menu: Customer Order Management
// ---------------------------------------------------------------

// Asks for products one by one and adds them to the given order.
// Used both when creating an order and when adding to an existing one.
int readOrderItems(Order* order, const Inventory& inventory) {
    int count = readInt("  How many different products to add? (1 - 20): ", 1, 20);
    int added = 0;
    for (int i = 1; i <= count; i++) {
        cout << "  Product " << i << ":\n";
        string productId = readId("    Product ID: ");
        if (inventory.findProduct(productId) == nullptr) {
            cout << "    Product " << productId << " does not exist, skipped.\n";
            continue;
        }
        int quantity = readInt("    Quantity (1 - 10000): ", 1, 10000);
        order->addItem(productId, quantity);
        added++;
    }
    return added;
}

void handleAddOrder(OrderManager& orders, const Inventory& inventory) {
    string orderId = readId("  Enter order ID (e.g. O112): ");
    if (orders.findOrder(orderId) != nullptr) {
        cout << "  Order " << orderId << " already exists. Order IDs must be unique.\n";
        return;
    }
    string customerId = readId("  Enter customer ID (e.g. CU12): ");

    // Build the order first, then hand it to the manager to validate and queue
    Order* order = new Order(orderId, customerId);
    readOrderItems(order, inventory);
    if (orders.addOrder(order, inventory)) {
        cout << "  Order " << orderId << " added and placed at the rear of the queue.\n";
    } else {
        delete order;
    }
}

void handleAddItemsToOrder(OrderManager& orders, const Inventory& inventory) {
    string orderId = readId("  Enter order ID: ");
    Order* order = orders.findOrder(orderId);
    if (order == nullptr) {
        cout << "  Order " << orderId << " not found.\n";
        return;
    }
    if (order->status != OrderStatus::Pending) {
        cout << "  Items can only be added to a Pending order (this one is "
             << statusToString(order->status) << ").\n";
        return;
    }

    int count = readInt("  How many different products to add? (1 - 20): ", 1, 20);
    for (int i = 1; i <= count; i++) {
        cout << "  Product " << i << ":\n";
        string productId = readId("    Product ID: ");
        int quantity = readInt("    Quantity (1 - 10000): ", 1, 10000);
        if (orders.addItemToOrder(orderId, productId, quantity, inventory)) {
            cout << "    Added (merged if already in the order).\n";
        }
    }
}

void handleCancelOrder(OrderManager& orders, Inventory& inventory, RobotFleet& fleet, PlanManager& plans) {
    string orderId = readId("  Enter order ID to cancel: ");
    if (orders.cancelOrder(orderId, inventory, fleet)) {
        // A cancelled order no longer needs its picking plan
        plans.remove(orderId);
        cout << "  Order " << orderId << " cancelled.\n";
    }
}

void ordersMenu(OrderManager& orders, Inventory& inventory, RobotFleet& fleet, PlanManager& plans) {
    while (true) {
        cout << "\n============ CUSTOMER ORDERS ============\n";
        cout << "  1. Display all orders\n";
        cout << "  2. Add a new order\n";
        cout << "  3. Add products to an order\n";
        cout << "  4. Search order by ID\n";
        cout << "  5. Enqueue pending orders into the queue\n";
        cout << "  6. Display pending queue (front to rear)\n";
        cout << "  7. Peek front order and queue size\n";
        cout << "  8. Cancel an order\n";
        cout << "  9. Remove cancelled orders from the queue\n";
        cout << " 10. Display pending and completed orders\n";
        cout << "  0. Back to main menu\n";

        int choice = readInt("Enter choice: ", 0, 10);
        switch (choice) {
            case 1: orders.displayAll(inventory); break;
            case 2: handleAddOrder(orders, inventory); break;
            case 3: handleAddItemsToOrder(orders, inventory); break;
            case 4: orders.displayOrder(readId("  Enter order ID: "), inventory); break;
            case 5:
                cout << "  " << orders.enqueuePendingOrders() << " pending orders added to the queue "
                     << "(orders already queued are not added twice).\n";
                break;
            case 6: orders.displayQueue(inventory); break;
            case 7: orders.displayFront(inventory); break;
            case 8: handleCancelOrder(orders, inventory, fleet, plans); break;
            case 9:
                cout << "  " << orders.removeCancelledFromQueue() << " cancelled orders removed from the queue.\n";
                break;
            case 10: orders.displayPendingAndCompleted(inventory); break;
            case 0: return;
        }
    }
}

// ---------------------------------------------------------------
// Module 4 menu: Robot Fleet Management
// ---------------------------------------------------------------

void handleRegisterRobot(RobotFleet& fleet, const Graph& warehouse) {
    string id = readId("  Enter robot ID (e.g. RB08): ");
    string type = readId("  Enter type (PICKER / CARRIER / HEAVYDUTY): ");
    string location = readId("  Enter starting location ID: ");
    double battery = readDouble("  Enter battery % (0 - 100): ", 0, 100);
    double capacity = readDouble("  Enter capacity in kg (Picker <=15, Carrier 15-30, Heavy-duty <=60): ", 0.1, 60);

    Robot* robot = createRobot(type, id, location, battery, capacity);
    if (robot == nullptr) {
        cout << "  Unknown robot type '" << type << "'.\n";
        return;
    }
    if (fleet.registerRobot(robot, warehouse)) {
        cout << "  Robot " << id << " registered as a " << robot->typeName() << ".\n";
    } else {
        delete robot;
    }
}

void handleUpdateBattery(RobotFleet& fleet) {
    string id = readId("  Enter robot ID: ");
    double battery = readDouble("  Enter new battery % (0 - 100): ", 0, 100);
    if (fleet.updateBattery(id, battery)) {
        cout << "  Battery of " << id << " set to " << battery << "%.\n";
    }
}

void handleAssignTask(RobotFleet& fleet) {
    string robotId = readId("  Enter robot ID: ");
    string taskId = readId("  Enter task ID (e.g. T01): ");
    // Orders are assigned through the picking plan (Module 5), which also reserves stock,
    // so this option only accepts general tasks
    if (!isValidId(taskId, "T")) {
        cout << "  Task ID must look like T01. Orders are assigned by generating a picking plan.\n";
        return;
    }
    if (fleet.assignTask(robotId, taskId)) {
        cout << "  Robot " << robotId << " assigned to task " << taskId << ".\n";
    }
}

void handleReleaseRobot(RobotFleet& fleet) {
    string robotId = readId("  Enter robot ID: ");
    Robot* robot = fleet.findRobot(robotId);
    if (robot != nullptr && isValidId(robot->assignedTask, "O")) {
        // An order is only finished when its last trip is executed (Module 6)
        cout << "  " << robotId << " is working on order " << robot->assignedTask
             << ". It is released automatically after the order's final trip, or when the order is cancelled.\n";
        return;
    }
    if (fleet.releaseRobot(robotId)) {
        cout << "  Robot " << robotId << " released and Available.\n";
    }
}

void handleMaintenance(RobotFleet& fleet, bool goingIn, HistoryManager& history) {
    string robotId = readId("  Enter robot ID: ");
    RobotStatus oldStatus;
    if (goingIn) {
        if (fleet.setUnavailable(robotId, oldStatus)) {
            history.recordMaintenance(robotId, oldStatus, RobotStatus::Unavailable);
            cout << "  Robot " << robotId << " is now Unavailable (maintenance).\n";
        }
    } else {
        if (fleet.returnFromMaintenance(robotId, oldStatus)) {
            history.recordMaintenance(robotId, oldStatus, RobotStatus::Available);
            cout << "  Robot " << robotId << " is back and Available.\n";
        }
    }
}

// Extra feature: shows how each robot type's rules apply to one order
void handleEligibilityCheck(RobotFleet& fleet, OrderManager& orders, const Inventory& inventory) {
    string orderId = readId("  Enter order ID: ");
    Order* order = orders.findOrder(orderId);
    if (order == nullptr) {
        cout << "  Order " << orderId << " not found.\n";
        return;
    }

    // Work out the order's total weight and heaviest single unit
    double totalWeight = order->totalWeight(inventory);
    double heaviestUnit = 0;
    OrderItem* item = order->itemHead;
    while (item != nullptr) {
        Product* product = inventory.findProduct(item->productId);
        if (product != nullptr && product->unitWeight > heaviestUnit) {
            heaviestUnit = product->unitWeight;
        }
        item = item->next;
    }

    cout << "\n  Order " << orderId << ": total " << totalWeight << " kg, heaviest unit "
         << heaviestUnit << " kg\n\n";
    Robot* robot = fleet.getHead();
    while (robot != nullptr) {
        cout << "  " << robot->id << " (" << robot->typeName() << ", " << robot->capacity << " kg): ";
        if (!robot->typeAllowsOrder(totalWeight, heaviestUnit)) {
            cout << "not eligible by type rule\n";
        } else if (!robot->canCarryUnit(heaviestUnit)) {
            cout << "cannot lift the heaviest unit\n";
        } else if (totalWeight <= robot->capacity) {
            cout << "eligible, single trip possible\n";
        } else {
            cout << "eligible, needs more than one trip\n";
        }
        robot = robot->next;
    }
}

void robotsMenu(RobotFleet& fleet, const Graph& warehouse, OrderManager& orders, const Inventory& inventory, HistoryManager& history) {
    while (true) {
        cout << "\n============ ROBOT FLEET ============\n";
        cout << "  1. Display all robots\n";
        cout << "  2. Display available robots\n";
        cout << "  3. Register a new robot\n";
        cout << "  4. Search robot by ID\n";
        cout << "  5. Update robot battery level\n";
        cout << "  6. Assign a robot to a task\n";
        cout << "  7. Release a robot after task completion\n";
        cout << "  8. Send robot to maintenance (Unavailable)\n";
        cout << "  9. Return robot from maintenance\n";
        cout << " 10. Check robot eligibility for an order\n";
        cout << "  0. Back to main menu\n";

        int choice = readInt("Enter choice: ", 0, 10);
        switch (choice) {
            case 1: fleet.displayAll(); break;
            case 2: fleet.displayAvailable(); break;
            case 3: handleRegisterRobot(fleet, warehouse); break;
            case 4: fleet.displayRobot(readId("  Enter robot ID: ")); break;
            case 5: handleUpdateBattery(fleet); break;
            case 6: handleAssignTask(fleet); break;
            case 7: handleReleaseRobot(fleet); break;
            case 8: handleMaintenance(fleet, true, history); break;
            case 9: handleMaintenance(fleet, false, history); break;
            case 10: handleEligibilityCheck(fleet, orders, inventory); break;
            case 0: return;
        }
    }
}

// ---------------------------------------------------------------
// Module 5 menu: Robot Assignment and Picking Plans
// ---------------------------------------------------------------

void planningMenu(Planner& planner, PlanManager& plans) {
    while (true) {
        cout << "\n========== ROBOT ASSIGNMENT & PICKING PLANS ==========\n";
        cout << "  1. Process next order from the queue\n";
        cout << "  2. Preview plan for a specific order (no reservation)\n";
        cout << "  3. Display all assigned picking plans\n";
        cout << "  0. Back to main menu\n";

        int choice = readInt("Enter choice: ", 0, 3);
        switch (choice) {
            case 1: planner.processNextOrder(); break;
            case 2: planner.previewOrder(readId("  Enter order ID: ")); break;
            case 3: plans.displayAll(); break;
            case 0: return;
        }
    }
}

// ---------------------------------------------------------------
// Module 6 menu: Order Picking and Delivery Execution
// ---------------------------------------------------------------

void executionMenu(Executor& executor, PlanManager& plans) {
    while (true) {
        cout << "\n========== ORDER PICKING & DELIVERY ==========\n";
        cout << "  1. Execute picking task for an assigned order\n";
        cout << "  2. Display all assigned picking plans\n";
        cout << "  0. Back to main menu\n";

        int choice = readInt("Enter choice: ", 0, 2);
        if (choice == 1) {
            executor.listAssignedOrders();
            if (plans.getHead() != nullptr) {
                executor.runOrder(readId("  Enter order ID: "));
            }
        } else if (choice == 2) {
            plans.displayAll();
        } else {
            return;
        }
    }
}

// ---------------------------------------------------------------
// Module 7 menu: Robot Battery and Charging
// ---------------------------------------------------------------

void handleFindStation(ChargingManager& charging, RobotFleet& fleet, const Graph& warehouse) {
    Robot* robot = fleet.findRobot(readId("  Enter robot ID: "));
    if (robot == nullptr) {
        cout << "  Robot not found.\n";
        return;
    }
    int distance;
    ChargingStation* station = charging.chooseStation(robot, warehouse, distance);
    if (station == nullptr) {
        cout << "  No charging station is reachable for " << robot->id << ".\n";
        return;
    }
    cout << "  Best station for " << robot->id << ": " << station->id << " at " << station->locationId
         << ", distance " << distance << ", uses about " << robot->legConsumption(distance, 0) << "% battery.\n";
    if (station->freeSlots() > 0) {
        cout << "  It has a free slot, so the robot would start charging immediately.\n";
    } else {
        cout << "  All slots are busy, so the robot would join the queue.\n";
    }
}

void chargingMenu(ChargingManager& charging, RobotFleet& fleet, const Graph& warehouse) {
    while (true) {
        cout << "\n========== BATTERY & CHARGING ==========\n";
        cout << "  1. Display all charging stations\n";
        cout << "  2. Identify robots requiring charging\n";
        cout << "  3. Find a reachable charging station for a robot\n";
        cout << "  4. Check slot availability at a station\n";
        cout << "  5. Send robot to charge (starts charging or joins the queue)\n";
        cout << "  6. Complete charging of a robot\n";
        cout << "  7. Remove a robot from a charging queue\n";
        cout << "  8. Update robot battery level\n";
        cout << "  0. Back to main menu\n";

        int choice = readInt("Enter choice: ", 0, 8);
        if (choice == 0) {
            return;
        }
        if (choice == 1) {
            charging.displayAll();
        } else if (choice == 2) {
            fleet.displayNeedingCharge();
        } else if (choice == 3) {
            handleFindStation(charging, fleet, warehouse);
        } else if (choice == 4) {
            charging.displayStation(readId("  Enter station ID (e.g. C01): "));
        } else if (choice == 8) {
            handleUpdateBattery(fleet);
        } else {
            Robot* robot = fleet.findRobot(readId("  Enter robot ID: "));
            if (robot == nullptr) {
                cout << "  Robot not found.\n";
            } else if (choice == 5) {
                charging.sendToCharge(robot, warehouse);
            } else if (choice == 6) {
                charging.completeCharging(robot);
            } else {
                charging.removeFromQueue(robot);
            }
        }
    }
}

// ---------------------------------------------------------------
// Module 8 menu: Operation History and Undo
// ---------------------------------------------------------------

void historyMenu(HistoryManager& history, Inventory& inventory, Graph& warehouse, RobotFleet& fleet) {
    while (true) {
        cout << "\n========== OPERATION HISTORY & UNDO ==========\n";
        cout << "  Reversible: manual quantity updates, blocked pathways, maintenance changes.\n";
        cout << "  1. Display most recent operation\n";
        cout << "  2. Undo most recent operation\n";
        cout << "  3. Discard the top entry (without undoing)\n";
        cout << "  4. Display full history\n";
        cout << "  0. Back to main menu\n";

        int choice = readInt("Enter choice: ", 0, 4);
        switch (choice) {
            case 1: history.displayMostRecent(); break;
            case 2: history.undo(inventory, warehouse, fleet); break;
            case 3: history.discardTop(); break;
            case 4: history.displayAll(); break;
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
    // Order matters: racks need locations, inventory needs products and racks.
    Graph warehouse;
    warehouse.loadLocations("data/locations.txt");
    warehouse.loadPathways("data/pathways.txt");

    Inventory inventory;
    inventory.loadRacks("data/racks.txt", warehouse);
    inventory.loadProducts("data/products.txt");
    inventory.loadInventory("data/inventory.txt");

    // Orders need products to exist, so they are loaded after the inventory
    OrderManager orders;
    orders.loadOrders("data/orders.txt", inventory);

    // Robots need valid locations
    RobotFleet fleet;
    fleet.loadRobots("data/robots.txt", warehouse);

    ChargingManager charging;
    charging.load("data/chargers.txt", warehouse);

    // Undo stack for reversible administrative operations
    HistoryManager history;

    // Picking plans connect all the modules above
    PlanManager plans;
    Planner planner(warehouse, inventory, fleet, orders, plans);
    Executor executor(warehouse, inventory, fleet, orders, plans);

    while (true) {
        cout << "\n================== MAIN MENU ==================\n";
        cout << "  1. Warehouse Layout & Pathways\n";
        cout << "  2. Product Inventory & Racks\n";
        cout << "  3. Customer Orders\n";
        cout << "  4. Robot Fleet\n";
        cout << "  5. Robot Assignment & Picking Plans\n";
        cout << "  6. Order Picking & Delivery Execution\n";
        cout << "  7. Battery & Charging\n";
        cout << "  8. Operation History & Undo\n";
        cout << "  0. Exit\n";

        int choice = readInt("Enter choice: ", 0, 8);
        if (choice == 1) {
            layoutMenu(warehouse, history);
        } else if (choice == 2) {
            inventoryMenu(inventory, warehouse, history);
        } else if (choice == 3) {
            ordersMenu(orders, inventory, fleet, plans);
        } else if (choice == 4) {
            robotsMenu(fleet, warehouse, orders, inventory, history);
        } else if (choice == 5) {
            planningMenu(planner, plans);
        } else if (choice == 6) {
            executionMenu(executor, plans);
        } else if (choice == 7) {
            chargingMenu(charging, fleet, warehouse);
        } else if (choice == 8) {
            historyMenu(history, inventory, warehouse, fleet);
        } else {
            cout << "Goodbye!\n";
            break;
        }
    }
    return 0;
}

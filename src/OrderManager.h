#ifndef ORDERMANAGER_H
#define ORDERMANAGER_H

#include <string>
#include "Order.h"
#include "Queue.h"
#include "Inventory.h"
#include "RobotFleet.h"
using namespace std;

// Module 3 manager.
// - Every order lives in one linked list (the "order records").
// - Pending orders are ALSO held in a FIFO queue, oldest at the front.
// The queue only stores pointers to the orders; the list owns them.
class OrderManager {
private:
    Order* head;
    Order* tail;
    int orderCount;
    Queue<Order*> pendingQueue;

    void appendOrder(Order* order);
    void displayOrderRow(const Order* order, const Inventory& inventory) const;

public:
    OrderManager();
    ~OrderManager();

    OrderManager(const OrderManager&) = delete;
    OrderManager& operator=(const OrderManager&) = delete;

    // Orders are enqueued in file order
    bool loadOrders(const string& fileName, const Inventory& inventory);

    // Adds a fully built order (with items) after validating it.
    // On success the manager takes ownership and the order joins the rear of the queue.
    bool addOrder(Order* order, const Inventory& inventory);

    // Adds a product to an existing order. Only allowed while the order is Pending.
    bool addItemToOrder(const string& orderId, const string& productId, int quantity, const Inventory& inventory);

    Order* findOrder(const string& orderId) const;

    // Puts any Pending order that is not already queued into the queue (in list order)
    int enqueuePendingOrders();

    // Cancels an order. Pending: leaves the queue.
    // Reserved/Assigned: all reservations are released and the robot is freed.
    bool cancelOrder(const string& orderId, Inventory& inventory, RobotFleet& fleet);

    // Removes any Cancelled orders still sitting in the queue
    int removeCancelledFromQueue();

    // Queue access for the picking module
    Queue<Order*>& getQueue();
    bool removeFromQueue(Order* order);
    void returnToFrontOfQueue(Order* order);

    // Displays
    void displayAll(const Inventory& inventory) const;
    void displayOrder(const string& orderId, const Inventory& inventory) const;
    void displayQueue(const Inventory& inventory) const;   // front to rear, does not remove
    void displayFront(const Inventory& inventory) const;   // peek + size
    void displayPendingAndCompleted(const Inventory& inventory) const;

    int size() const;
    Order* getHead() const;
};

#endif

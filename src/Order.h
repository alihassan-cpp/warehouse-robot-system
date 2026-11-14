#ifndef ORDER_H
#define ORDER_H

#include <string>
using namespace std;

class Inventory;

// All the states an order can be in
enum class OrderStatus {
    Pending,
    Reserved,
    Assigned,
    PartiallyFulfilled,
    Completed,
    Cancelled
};

string statusToString(OrderStatus status);

// One product line inside an order
class OrderItem {
public:
    string productId;
    int quantity;   // how many the customer ordered
    int delivered;  // how many have reached a packing station so far
    OrderItem* next;

    OrderItem(const string& product, int amount);
};

// A customer order. It owns its own linked list of items,
// and is itself a node in the OrderManager's list of all orders.
class Order {
public:
    string orderId;      // e.g. O101
    string customerId;   // e.g. CU12
    OrderStatus status;
    string robotId;      // robot assigned to this order ("" if none)
    OrderItem* itemHead;
    Order* next;         // next order in the main order list

    Order(const string& id, const string& customer);
    ~Order(); // frees the item list

    Order(const Order&) = delete;
    Order& operator=(const Order&) = delete;

    OrderItem* findItem(const string& productId) const;

    // Adds a product; if it is already in the order, the quantities are merged
    void addItem(const string& productId, int quantity);

    int itemCount() const;
    int totalUnits() const;

    // Total weight in kg, using the unit weights stored in the inventory
    double totalWeight(const Inventory& inventory) const;

    // Records units that reached a packing station, then updates the status:
    // Completed if every item is fully delivered, otherwise Partially Fulfilled
    bool recordDelivery(const string& productId, int amount);
    bool isFullyDelivered() const;

    // Only Pending, Reserved and Assigned orders (no trip started) can be cancelled
    bool canBeCancelled() const;

    void displayItems() const;  // P101 x5, P103 x2
};

#endif

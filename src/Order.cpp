#include "Order.h"
#include "Inventory.h"
#include <iostream>

string statusToString(OrderStatus status) {
    switch (status) {
        case OrderStatus::Pending: return "Pending";
        case OrderStatus::Reserved: return "Reserved";
        case OrderStatus::Assigned: return "Assigned";
        case OrderStatus::PartiallyFulfilled: return "Partially Fulfilled";
        case OrderStatus::Completed: return "Completed";
        case OrderStatus::Cancelled: return "Cancelled";
    }
    return "Unknown";
}

OrderItem::OrderItem(const string& product, int amount) {
    productId = product;
    quantity = amount;
    delivered = 0;
    next = nullptr;
}

Order::Order(const string& id, const string& customer) {
    orderId = id;
    customerId = customer;
    status = OrderStatus::Pending;
    robotId = "";
    itemHead = nullptr;
    next = nullptr;
}

Order::~Order() {
    OrderItem* current = itemHead;
    while (current != nullptr) {
        OrderItem* temp = current;
        current = current->next;
        delete temp;
    }
}

OrderItem* Order::findItem(const string& productId) const {
    OrderItem* current = itemHead;
    while (current != nullptr) {
        if (current->productId == productId) {
            return current;
        }
        current = current->next;
    }
    return nullptr;
}

void Order::addItem(const string& productId, int quantity) {
    // Same product added twice: merge into the existing line
    OrderItem* existing = findItem(productId);
    if (existing != nullptr) {
        existing->quantity += quantity;
        return;
    }

    // New product: add at the end so items stay in the order they were entered
    OrderItem* item = new OrderItem(productId, quantity);
    if (itemHead == nullptr) {
        itemHead = item;
        return;
    }
    OrderItem* current = itemHead;
    while (current->next != nullptr) {
        current = current->next;
    }
    current->next = item;
}

int Order::itemCount() const {
    int count = 0;
    OrderItem* current = itemHead;
    while (current != nullptr) {
        count++;
        current = current->next;
    }
    return count;
}

int Order::totalUnits() const {
    int units = 0;
    OrderItem* current = itemHead;
    while (current != nullptr) {
        units += current->quantity;
        current = current->next;
    }
    return units;
}

double Order::totalWeight(const Inventory& inventory) const {
    double weight = 0;
    OrderItem* current = itemHead;
    while (current != nullptr) {
        Product* product = inventory.findProduct(current->productId);
        if (product != nullptr) {
            weight += product->unitWeight * current->quantity;
        }
        current = current->next;
    }
    return weight;
}

bool Order::recordDelivery(const string& productId, int amount) {
    OrderItem* item = findItem(productId);
    if (item == nullptr || amount <= 0 || item->delivered + amount > item->quantity) {
        return false;
    }
    item->delivered += amount;

    if (isFullyDelivered()) {
        status = OrderStatus::Completed;
    } else {
        status = OrderStatus::PartiallyFulfilled;
    }
    return true;
}

bool Order::isFullyDelivered() const {
    OrderItem* current = itemHead;
    while (current != nullptr) {
        if (current->delivered < current->quantity) {
            return false;
        }
        current = current->next;
    }
    return true;
}

bool Order::canBeCancelled() const {
    return status == OrderStatus::Pending
        || status == OrderStatus::Reserved
        || status == OrderStatus::Assigned;
}

void Order::displayItems() const {
    OrderItem* current = itemHead;
    while (current != nullptr) {
        cout << current->productId << " x" << current->quantity;
        if (current->delivered > 0) {
            cout << " (" << current->delivered << " delivered)";
        }
        if (current->next != nullptr) {
            cout << ", ";
        }
        current = current->next;
    }
}

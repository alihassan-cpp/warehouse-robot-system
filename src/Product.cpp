#include "Product.h"

Product::Product(const string& productId, const string& productName, double weight, int reorder) {
    id = productId;
    name = productName;
    unitWeight = weight;
    reorderLevel = reorder;
    rackHead = nullptr;
}

Product::~Product() {
    RackEntry* current = rackHead;
    while (current != nullptr) {
        RackEntry* temp = current;
        current = current->next;
        delete temp;
    }
}

RackEntry* Product::findRack(const string& rackId) const {
    RackEntry* current = rackHead;
    while (current != nullptr) {
        if (current->rackId == rackId) {
            return current;
        }
        current = current->next;
    }
    return nullptr;
}

bool Product::addRack(const string& rackId, int quantity) {
    if (findRack(rackId) != nullptr) {
        return false;
    }

    RackEntry* entry = new RackEntry(rackId, quantity);
    if (rackHead == nullptr) {
        rackHead = entry;
        return true;
    }
    RackEntry* current = rackHead;
    while (current->next != nullptr) {
        current = current->next;
    }
    current->next = entry;
    return true;
}

bool Product::removeRack(const string& rackId) {
    RackEntry* current = rackHead;
    RackEntry* previous = nullptr;
    while (current != nullptr) {
        if (current->rackId == rackId) {
            if (previous == nullptr) {
                rackHead = current->next;
            } else {
                previous->next = current->next;
            }
            delete current;
            return true;
        }
        previous = current;
        current = current->next;
    }
    return false;
}

int Product::totalStock() const {
    int sum = 0;
    RackEntry* current = rackHead;
    while (current != nullptr) {
        sum += current->totalQuantity;
        current = current->next;
    }
    return sum;
}

int Product::reservedStock() const {
    int sum = 0;
    RackEntry* current = rackHead;
    while (current != nullptr) {
        sum += current->reservedQuantity;
        current = current->next;
    }
    return sum;
}

int Product::availableStock() const {
    return totalStock() - reservedStock();
}

bool Product::hasReservations() const {
    RackEntry* current = rackHead;
    while (current != nullptr) {
        if (current->hasReservations()) {
            return true;
        }
        current = current->next;
    }
    return false;
}

bool Product::isLowStock() const {
    return availableStock() < reorderLevel;
}

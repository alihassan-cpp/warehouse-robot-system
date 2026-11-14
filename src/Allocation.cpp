#include "Allocation.h"
#include <iostream>

AllocationNode::AllocationNode(const string& rack, const string& location, int amount, int dist) {
    rackId = rack;
    locationId = location;
    quantity = amount;
    distance = dist;
    next = nullptr;
}

Allocation::Allocation() {
    head = nullptr;
    tail = nullptr;
    totalQuantity = 0;
}

Allocation::~Allocation() {
    clear();
}

void Allocation::add(const string& rackId, const string& locationId, int quantity, int distance) {
    AllocationNode* node = new AllocationNode(rackId, locationId, quantity, distance);
    if (head == nullptr) {
        head = node;
        tail = node;
    } else {
        tail->next = node;
        tail = node;
    }
    totalQuantity += quantity;
}

void Allocation::clear() {
    while (head != nullptr) {
        AllocationNode* temp = head;
        head = head->next;
        delete temp;
    }
    tail = nullptr;
    totalQuantity = 0;
}

AllocationNode* Allocation::getHead() const {
    return head;
}

int Allocation::getTotalQuantity() const {
    return totalQuantity;
}

bool Allocation::isEmpty() const {
    return head == nullptr;
}

void Allocation::display() const {
    AllocationNode* current = head;
    while (current != nullptr) {
        cout << "    " << current->rackId << " (at " << current->locationId
             << ", distance " << current->distance << "): take " << current->quantity << " units\n";
        current = current->next;
    }
}

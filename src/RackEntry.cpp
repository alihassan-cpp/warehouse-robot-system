#include "RackEntry.h"
#include <iostream>

Reservation::Reservation(const string& order, int amount) {
    orderId = order;
    quantity = amount;
    next = nullptr;
}

RackEntry::RackEntry(const string& id, int quantity) {
    rackId = id;
    totalQuantity = quantity;
    reservedQuantity = 0;
    reservationHead = nullptr;
    next = nullptr;
}

RackEntry::~RackEntry() {
    Reservation* current = reservationHead;
    while (current != nullptr) {
        Reservation* temp = current;
        current = current->next;
        delete temp;
    }
}

int RackEntry::available() const {
    return totalQuantity - reservedQuantity;
}

bool RackEntry::hasReservations() const {
    return reservedQuantity > 0;
}

bool RackEntry::addReservation(const string& orderId, int amount) {
    if (amount <= 0 || amount > available()) {
        return false;
    }

    // If this order already reserved something here, just increase that record
    Reservation* current = reservationHead;
    while (current != nullptr) {
        if (current->orderId == orderId) {
            current->quantity += amount;
            reservedQuantity += amount;
            return true;
        }
        current = current->next;
    }

    // Otherwise add a new record at the front of the list
    Reservation* record = new Reservation(orderId, amount);
    record->next = reservationHead;
    reservationHead = record;
    reservedQuantity += amount;
    return true;
}

int RackEntry::releaseReservation(const string& orderId) {
    Reservation* current = reservationHead;
    Reservation* previous = nullptr;
    while (current != nullptr) {
        if (current->orderId == orderId) {
            int freed = current->quantity;
            if (previous == nullptr) {
                reservationHead = current->next;
            } else {
                previous->next = current->next;
            }
            delete current;
            reservedQuantity -= freed;
            return freed;
        }
        previous = current;
        current = current->next;
    }
    return 0;
}

bool RackEntry::collect(const string& orderId, int amount) {
    Reservation* current = reservationHead;
    Reservation* previous = nullptr;
    while (current != nullptr) {
        if (current->orderId == orderId) {
            if (amount <= 0 || amount > current->quantity) {
                return false;
            }
            current->quantity -= amount;
            reservedQuantity -= amount;
            totalQuantity -= amount;

            // A record that reaches 0 is no longer needed
            if (current->quantity == 0) {
                if (previous == nullptr) {
                    reservationHead = current->next;
                } else {
                    previous->next = current->next;
                }
                delete current;
            }
            return true;
        }
        previous = current;
        current = current->next;
    }
    return false;
}

int RackEntry::reservedBy(const string& orderId) const {
    Reservation* current = reservationHead;
    while (current != nullptr) {
        if (current->orderId == orderId) {
            return current->quantity;
        }
        current = current->next;
    }
    return 0;
}

void RackEntry::displayReservations() const {
    if (reservationHead == nullptr) {
        cout << "none";
        return;
    }
    Reservation* current = reservationHead;
    while (current != nullptr) {
        cout << current->orderId << ":" << current->quantity;
        if (current->next != nullptr) {
            cout << ", ";
        }
        current = current->next;
    }
}

#include "Route.h"
#include <iostream>

RouteNode::RouteNode(const string& id) {
    locationId = id;
    next = nullptr;
}

Route::Route() {
    head = nullptr;
    tail = nullptr;
    stopCount = 0;
    totalDistance = 0;
}

Route::~Route() {
    clear();
}

void Route::addStop(const string& locationId) {
    RouteNode* node = new RouteNode(locationId);
    if (head == nullptr) {
        head = node;
        tail = node;
    } else {
        tail->next = node;
        tail = node;
    }
    stopCount++;
}

void Route::addStopAtFront(const string& locationId) {
    RouteNode* node = new RouteNode(locationId);
    node->next = head;
    head = node;
    if (tail == nullptr) {
        tail = node;
    }
    stopCount++;
}

void Route::clear() {
    while (head != nullptr) {
        RouteNode* temp = head;
        head = head->next;
        delete temp;
    }
    tail = nullptr;
    stopCount = 0;
    totalDistance = 0;
}

void Route::setTotalDistance(int distance) {
    totalDistance = distance;
}

int Route::getTotalDistance() const {
    return totalDistance;
}

int Route::getStopCount() const {
    return stopCount;
}

bool Route::isEmpty() const {
    return head == nullptr;
}

RouteNode* Route::getHead() const {
    return head;
}

void Route::display() const {
    RouteNode* current = head;
    while (current != nullptr) {
        cout << current->locationId;
        if (current->next != nullptr) {
            cout << " -> ";
        }
        current = current->next;
    }
}

#include "OrderManager.h"
#include "Utils.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

OrderManager::OrderManager() {
    head = nullptr;
    tail = nullptr;
    orderCount = 0;
}

OrderManager::~OrderManager() {
    // The queue only holds pointers; the orders themselves are deleted here, once
    Order* current = head;
    while (current != nullptr) {
        Order* temp = current;
        current = current->next;
        delete temp;
    }
}

void OrderManager::appendOrder(Order* order) {
    if (head == nullptr) {
        head = order;
        tail = order;
    } else {
        tail->next = order;
        tail = order;
    }
    orderCount++;
}

// ---------------------------------------------------------------
// Loading and adding orders
// ---------------------------------------------------------------

bool OrderManager::loadOrders(const string& fileName, const Inventory& inventory) {
    ifstream file(fileName);
    if (!file.is_open()) {
        cout << "Error: could not open " << fileName << "\n";
        return false;
    }

    string line;
    int lineNumber = 0;
    int loaded = 0;
    while (getline(file, line)) {
        lineNumber++;
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Split "O102,CU02,P103:4;P107:2" into ID, customer, and the item list
        stringstream parts(line);
        string orderId, customerId, itemsText;
        getline(parts, orderId, ',');
        getline(parts, customerId, ',');
        getline(parts, itemsText);

        Order* order = new Order(toUpper(trim(orderId)), toUpper(trim(customerId)));

        // Split the item list at ';' and each item at ':'
        stringstream items(itemsText);
        string itemText;
        bool itemsValid = true;
        while (getline(items, itemText, ';')) {
            itemText = trim(itemText);
            if (itemText.empty()) {
                continue;
            }
            stringstream itemParts(itemText);
            string productId, quantityText;
            getline(itemParts, productId, ':');
            getline(itemParts, quantityText);
            productId = toUpper(trim(productId));

            int quantity;
            if (!parseInt(quantityText, quantity) || quantity <= 0) {
                cout << "  " << fileName << " line " << lineNumber << ": bad quantity for " << productId << ".\n";
                itemsValid = false;
                break;
            }
            if (inventory.findProduct(productId) == nullptr) {
                cout << "  " << fileName << " line " << lineNumber << ": product " << productId << " does not exist.\n";
                itemsValid = false;
                break;
            }
            order->addItem(productId, quantity);
        }

        if (itemsValid && addOrder(order, inventory)) {
            loaded++;
        } else {
            delete order; // rejected, so we still own it
            cout << "  (" << fileName << " line " << lineNumber << " skipped)\n";
        }
    }

    cout << "Loaded " << loaded << " orders from " << fileName << ".\n";
    return true;
}

bool OrderManager::addOrder(Order* order, const Inventory& inventory) {
    if (!isValidId(order->orderId, "O")) {
        cout << "  Invalid order ID '" << order->orderId << "'. It must look like O101.\n";
        return false;
    }
    if (!isValidId(order->customerId, "CU")) {
        cout << "  Invalid customer ID '" << order->customerId << "'. It must look like CU12.\n";
        return false;
    }
    if (findOrder(order->orderId) != nullptr) {
        cout << "  Order " << order->orderId << " already exists. Order IDs must be unique.\n";
        return false;
    }
    if (order->itemHead == nullptr) {
        cout << "  An order needs at least one product.\n";
        return false;
    }

    // Double-check every item (products must exist, quantities must be positive)
    OrderItem* item = order->itemHead;
    while (item != nullptr) {
        if (inventory.findProduct(item->productId) == nullptr || item->quantity <= 0) {
            cout << "  Invalid item " << item->productId << " in order " << order->orderId << ".\n";
            return false;
        }
        item = item->next;
    }

    // New orders always start Pending and join the REAR of the queue
    order->status = OrderStatus::Pending;
    appendOrder(order);
    pendingQueue.enqueue(order);
    return true;
}

bool OrderManager::addItemToOrder(const string& orderId, const string& productId, int quantity, const Inventory& inventory) {
    Order* order = findOrder(orderId);
    if (order == nullptr) {
        cout << "  Order " << orderId << " not found.\n";
        return false;
    }
    if (order->status != OrderStatus::Pending) {
        cout << "  Items can only be added to a Pending order (this one is "
             << statusToString(order->status) << ").\n";
        return false;
    }
    if (inventory.findProduct(productId) == nullptr) {
        cout << "  Product " << productId << " does not exist.\n";
        return false;
    }
    if (quantity <= 0) {
        cout << "  Quantity must be positive.\n";
        return false;
    }

    order->addItem(productId, quantity);
    return true;
}

Order* OrderManager::findOrder(const string& orderId) const {
    Order* current = head;
    while (current != nullptr) {
        if (current->orderId == orderId) {
            return current;
        }
        current = current->next;
    }
    return nullptr;
}

// ---------------------------------------------------------------
// Queue operations
// ---------------------------------------------------------------

int OrderManager::enqueuePendingOrders() {
    int added = 0;
    Order* current = head;
    while (current != nullptr) {
        if (current->status == OrderStatus::Pending && !pendingQueue.contains(current)) {
            pendingQueue.enqueue(current);
            added++;
        }
        current = current->next;
    }
    return added;
}

bool OrderManager::cancelOrder(const string& orderId, Inventory& inventory, RobotFleet& fleet) {
    Order* order = findOrder(orderId);
    if (order == nullptr) {
        cout << "  Order " << orderId << " not found.\n";
        return false;
    }
    if (!order->canBeCancelled()) {
        cout << "  Order " << orderId << " is " << statusToString(order->status)
             << " and cannot be cancelled.\n";
        return false;
    }

    if (order->status == OrderStatus::Pending) {
        // A pending order can be anywhere in the queue: linear scan, O(n)
        pendingQueue.remove(order);
    } else {
        // Reserved or Assigned: give back every unit this order reserved
        int released = inventory.releaseOrder(orderId);
        cout << "  Released " << released << " reserved units.\n";

        // If a robot was assigned, it becomes free again
        if (order->robotId != "") {
            if (fleet.releaseRobot(order->robotId)) {
                cout << "  Robot " << order->robotId << " released.\n";
            }
            order->robotId = "";
        }
    }

    order->status = OrderStatus::Cancelled;
    return true;
}

int OrderManager::removeCancelledFromQueue() {
    int removed = 0;
    QueueNode<Order*>* node = pendingQueue.getFront();
    while (node != nullptr) {
        Order* order = node->data;
        node = node->next; // move on before the node is deleted
        if (order->status == OrderStatus::Cancelled) {
            pendingQueue.remove(order);
            removed++;
        }
    }
    return removed;
}

Queue<Order*>& OrderManager::getQueue() {
    return pendingQueue;
}

bool OrderManager::removeFromQueue(Order* order) {
    return pendingQueue.remove(order);
}

void OrderManager::returnToFrontOfQueue(Order* order) {
    order->status = OrderStatus::Pending;
    if (!pendingQueue.contains(order)) {
        pendingQueue.enqueueFront(order);
    }
}

// ---------------------------------------------------------------
// Displays
// ---------------------------------------------------------------

void OrderManager::displayOrderRow(const Order* order, const Inventory& inventory) const {
    cout << "  " << left << setw(6) << order->orderId << setw(7) << order->customerId
         << setw(21) << statusToString(order->status)
         << right << setw(6) << fixed << setprecision(1) << order->totalWeight(inventory) << " kg   ";
    order->displayItems();
    cout << "\n";
}

void OrderManager::displayAll(const Inventory& inventory) const {
    if (head == nullptr) {
        cout << "  No orders.\n";
        return;
    }
    cout << "\n  " << left << setw(6) << "Order" << setw(7) << "Cust" << setw(21) << "Status"
         << right << setw(9) << "Weight" << "   Items\n";
    cout << "  ----------------------------------------------------------------------\n";
    Order* current = head;
    while (current != nullptr) {
        displayOrderRow(current, inventory);
        current = current->next;
    }
    cout << "  (" << orderCount << " orders)\n";
}

void OrderManager::displayOrder(const string& orderId, const Inventory& inventory) const {
    Order* order = findOrder(orderId);
    if (order == nullptr) {
        cout << "  Order " << orderId << " not found.\n";
        return;
    }
    cout << "\n  Order:     " << order->orderId << "\n";
    cout << "  Customer:  " << order->customerId << "\n";
    cout << "  Status:    " << statusToString(order->status) << "\n";
    if (order->robotId != "") {
        cout << "  Robot:     " << order->robotId << "\n";
    }
    cout << "  Weight:    " << fixed << setprecision(1) << order->totalWeight(inventory) << " kg\n";
    cout << "  Items:\n";
    OrderItem* item = order->itemHead;
    while (item != nullptr) {
        cout << "    " << item->productId << "  ordered " << item->quantity
             << ", delivered " << item->delivered << "\n";
        item = item->next;
    }
}

void OrderManager::displayQueue(const Inventory& inventory) const {
    if (pendingQueue.isEmpty()) {
        cout << "  The pending order queue is empty.\n";
        return;
    }
    cout << "\n  Pending queue (front = oldest, processed first):\n";
    int position = 1;
    QueueNode<Order*>* node = pendingQueue.getFront();
    while (node != nullptr) {
        cout << "  " << right << setw(2) << position << ".";
        displayOrderRow(node->data, inventory);
        node = node->next;
        position++;
    }

    // Short one-line view, matching the PDF's style
    cout << "\n  Front -> ";
    node = pendingQueue.getFront();
    while (node != nullptr) {
        cout << node->data->orderId << " -> ";
        node = node->next;
    }
    cout << "Rear\n";
}

void OrderManager::displayFront(const Inventory& inventory) const {
    Order* front;
    if (!pendingQueue.peek(front)) {
        cout << "  The pending order queue is empty.\n";
        return;
    }
    cout << "\n  Front of queue (next to be processed):\n ";
    displayOrderRow(front, inventory);
    cout << "  Queue size: " << pendingQueue.size() << "\n";
}

void OrderManager::displayPendingAndCompleted(const Inventory& inventory) const {
    int pending = 0;
    int completed = 0;

    cout << "\n  PENDING ORDERS:\n";
    Order* current = head;
    while (current != nullptr) {
        if (current->status == OrderStatus::Pending) {
            displayOrderRow(current, inventory);
            pending++;
        }
        current = current->next;
    }
    if (pending == 0) {
        cout << "  none\n";
    }

    cout << "\n  COMPLETED ORDERS:\n";
    current = head;
    while (current != nullptr) {
        if (current->status == OrderStatus::Completed) {
            displayOrderRow(current, inventory);
            completed++;
        }
        current = current->next;
    }
    if (completed == 0) {
        cout << "  none\n";
    }
}

int OrderManager::size() const {
    return orderCount;
}

Order* OrderManager::getHead() const {
    return head;
}

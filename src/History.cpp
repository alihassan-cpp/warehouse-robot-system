#include "History.h"
#include <iostream>

Operation::Operation(OperationType operationType) {
    type = operationType;
    oldQuantity = 0;
    newQuantity = 0;
    oldStatus = RobotStatus::Available;
    newStatus = RobotStatus::Available;
}

string Operation::describe() const {
    if (type == OperationType::QuantityUpdate) {
        return "Quantity update: " + productId + " in " + rackId + " changed "
             + to_string(oldQuantity) + " -> " + to_string(newQuantity);
    }
    if (type == OperationType::BlockPathway) {
        return "Block pathway " + fromId + " <-> " + toId + " (was OPEN)";
    }
    return "Maintenance change: " + robotId + " " + robotStatusToString(oldStatus)
         + " -> " + robotStatusToString(newStatus);
}

HistoryManager::HistoryManager() {
}

HistoryManager::~HistoryManager() {
    // Free every operation still on the stack
    Operation* op;
    while (history.pop(op)) {
        delete op;
    }
}

// ---------------------------------------------------------------
// Recording
// ---------------------------------------------------------------

void HistoryManager::recordQuantityUpdate(const string& productId, const string& rackId, int oldQuantity, int newQuantity) {
    Operation* op = new Operation(OperationType::QuantityUpdate);
    op->productId = productId;
    op->rackId = rackId;
    op->oldQuantity = oldQuantity;
    op->newQuantity = newQuantity;
    history.push(op);
}

void HistoryManager::recordBlock(const string& fromId, const string& toId) {
    Operation* op = new Operation(OperationType::BlockPathway);
    op->fromId = fromId;
    op->toId = toId;
    history.push(op);
}

void HistoryManager::recordMaintenance(const string& robotId, RobotStatus oldStatus, RobotStatus newStatus) {
    Operation* op = new Operation(OperationType::MaintenanceChange);
    op->robotId = robotId;
    op->oldStatus = oldStatus;
    op->newStatus = newStatus;
    history.push(op);
}

// ---------------------------------------------------------------
// Display
// ---------------------------------------------------------------

void HistoryManager::displayMostRecent() const {
    Operation* op;
    if (!history.peek(op)) {
        cout << "  No operations to undo.\n";
        return;
    }
    cout << "  Most recent: " << op->describe() << "\n";
}

void HistoryManager::displayAll() const {
    if (history.isEmpty()) {
        cout << "  History is empty.\n";
        return;
    }
    cout << "\n  Undo history (top = most recent):\n";
    StackNode<Operation*>* node = history.getTop();
    int position = 1;
    while (node != nullptr) {
        cout << "  " << position << ". " << node->data->describe() << "\n";
        node = node->next;
        position++;
    }
}

// ---------------------------------------------------------------
// Undo
// ---------------------------------------------------------------

bool HistoryManager::undo(Inventory& inventory, Graph& warehouse, RobotFleet& fleet) {
    Operation* op;
    if (!history.peek(op)) {
        cout << "  Nothing to undo.\n";
        return false;
    }

    // Each case checks first and only restores if it is safe.
    // On rejection we return before popping, so the entry stays on the stack.

    if (op->type == OperationType::QuantityUpdate) {
        Product* product = inventory.findProduct(op->productId);
        RackEntry* rack = nullptr;
        if (product != nullptr) {
            rack = product->findRack(op->rackId);
        }
        if (rack == nullptr) {
            cout << "  Undo rejected: the rack entry " << op->productId << "/" << op->rackId
                 << " no longer exists.\n";
            return false;
        }
        if (op->oldQuantity < rack->reservedQuantity) {
            cout << "  Undo rejected: restoring " << op->oldQuantity << " units would be below the "
                 << rack->reservedQuantity << " units currently reserved.\n";
            return false;
        }
        rack->totalQuantity = op->oldQuantity;
        cout << "  Undone: " << op->productId << " in " << op->rackId << " restored to "
             << op->oldQuantity << " units.\n";

    } else if (op->type == OperationType::BlockPathway) {
        // Never rejected: undoing a block simply reopens the pathway
        if (!warehouse.pathwayExists(op->fromId, op->toId)) {
            cout << "  Pathway " << op->fromId << " <-> " << op->toId
                 << " was removed since, so there is nothing to reopen.\n";
        } else if (warehouse.isPathwayBlocked(op->fromId, op->toId)) {
            warehouse.reopenPathway(op->fromId, op->toId);
            cout << "  Undone: pathway " << op->fromId << " <-> " << op->toId << " is OPEN again.\n";
        } else {
            cout << "  Pathway " << op->fromId << " <-> " << op->toId << " is already open.\n";
        }

    } else {
        Robot* robot = fleet.findRobot(op->robotId);
        if (robot == nullptr) {
            cout << "  Undo rejected: robot " << op->robotId << " no longer exists.\n";
            return false;
        }
        if (robot->hasActiveTask()) {
            cout << "  Undo rejected: " << op->robotId << " now has an active task ("
                 << robot->assignedTask << ").\n";
            return false;
        }
        if (robot->status == RobotStatus::Charging || robot->status == RobotStatus::WaitingForCharging) {
            cout << "  Undo rejected: " << op->robotId << " is in the charging system.\n";
            return false;
        }
        robot->status = op->oldStatus;
        cout << "  Undone: " << op->robotId << " status restored to "
             << robotStatusToString(op->oldStatus) << ".\n";
    }

    // Restored successfully: now remove the entry
    history.pop(op);
    delete op;
    return true;
}

bool HistoryManager::discardTop() {
    Operation* op;
    if (!history.pop(op)) {
        cout << "  Nothing to discard.\n";
        return false;
    }
    cout << "  Discarded without undoing: " << op->describe() << "\n";
    delete op;
    return true;
}

int HistoryManager::size() const {
    return history.size();
}

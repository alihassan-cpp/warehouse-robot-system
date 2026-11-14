#ifndef HISTORY_H
#define HISTORY_H

#include <string>
#include "Stack.h"
#include "Robot.h"
#include "Graph.h"
#include "Inventory.h"
#include "RobotFleet.h"
using namespace std;

// The three kinds of administrative operations that can be undone
enum class OperationType {
    QuantityUpdate,   // Module 2: manual rack quantity change
    BlockPathway,     // Module 1: a pathway was blocked
    MaintenanceChange // Module 4: robot sent to / returned from maintenance
};

// One entry on the undo stack: everything needed to put things back exactly
class Operation {
public:
    OperationType type;

    // QuantityUpdate
    string productId;
    string rackId;
    int oldQuantity;
    int newQuantity;

    // BlockPathway
    string fromId;
    string toId;

    // MaintenanceChange
    string robotId;
    RobotStatus oldStatus;
    RobotStatus newStatus;

    Operation(OperationType operationType);
    string describe() const; // one readable line, e.g. "Block pathway L02 <-> L06"
};

// Module 8: Operation history and undo management
class HistoryManager {
private:
    Stack<Operation*> history; // the stack stores pointers; this class deletes them

public:
    HistoryManager();
    ~HistoryManager();

    HistoryManager(const HistoryManager&) = delete;
    HistoryManager& operator=(const HistoryManager&) = delete;

    // Called right after each reversible operation succeeds
    void recordQuantityUpdate(const string& productId, const string& rackId, int oldQuantity, int newQuantity);
    void recordBlock(const string& fromId, const string& toId);
    void recordMaintenance(const string& robotId, RobotStatus oldStatus, RobotStatus newStatus);

    void displayMostRecent() const;
    void displayAll() const;

    // Pops and restores the top entry. If restoring would break consistency,
    // the entry stays on the stack, the reason is shown, and nothing changes.
    bool undo(Inventory& inventory, Graph& warehouse, RobotFleet& fleet);

    // Throws away the top entry without restoring it
    bool discardTop();

    int size() const;
};

#endif

#ifndef INVENTORY_H
#define INVENTORY_H

#include <string>
#include "Graph.h"
#include "ProductTree.h"
#include "RackDirectory.h"
#include "Allocation.h"
using namespace std;

// Module 2 manager: owns the product AVL tree and the rack directory,
// and enforces all the inventory rules from the PDF.
class Inventory {
private:
    ProductTree products;
    RackDirectory racks;

public:
    // File loading. Load racks first, then products, then inventory.
    bool loadRacks(const string& fileName, const Graph& warehouse);
    bool loadProducts(const string& fileName);
    bool loadInventory(const string& fileName);

    // Products
    bool addProduct(const string& id, const string& name, double weight, int reorderLevel);
    bool deleteProduct(const string& id);
    Product* findProduct(const string& id) const;
    void displayProduct(const string& id) const;
    void displayAllProducts() const;
    void displayTreeStructure() const;
    void displayLowStock() const;
    void displayRacks() const;

    // Rack entries inside a product's list
    bool addRackEntry(const string& productId, const string& rackId, int quantity);
    bool updateQuantity(const string& productId, const string& rackId, int newQuantity, int& oldQuantity);
    bool removeRackEntry(const string& productId, const string& rackId);

    // Stock totals; both return -1 if the product does not exist
    int totalStock(const string& productId) const;
    int availableStock(const string& productId) const;

    // Rack allocation rule (PDF Module 2). Fills 'result' with which racks to take from.
    // Distances are measured from referenceLocation using Dijkstra.
    // shortage = how many units could not be allocated (0 if fully allocated).
    // If allowPartial is false and there is a shortage, nothing is allocated.
    // Does NOT reserve anything; it only plans.
    bool allocate(const string& productId, int quantity, const string& referenceLocation,
                  const Graph& warehouse, Allocation& result, int& shortage, bool allowPartial) const;

    // Reserves the units listed in an allocation for an order.
    // All-or-nothing: checks every rack first, and changes nothing if any check fails.
    bool reserve(const string& productId, const Allocation& allocation, const string& orderId);

    // Releases all reservations of an order (used when an order is cancelled)
    int releaseOrder(const string& orderId);

    const RackDirectory& getRacks() const;
};

#endif

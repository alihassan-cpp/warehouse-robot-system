#ifndef PRODUCT_H
#define PRODUCT_H

#include <string>
#include "RackEntry.h"
using namespace std;

// One unique product. It owns a linked list of the racks that store it.
class Product {
public:
    string id;          // e.g. P101
    string name;
    double unitWeight;  // kg per unit
    int reorderLevel;   // warn when available stock falls below this
    RackEntry* rackHead;

    Product(const string& productId, const string& productName, double weight, int reorder);
    ~Product(); // frees the rack list

    Product(const Product&) = delete;
    Product& operator=(const Product&) = delete;

    RackEntry* findRack(const string& rackId) const;

    // Adds a rack at the end of the list. Fails if this rack is already listed.
    bool addRack(const string& rackId, int quantity);

    // Unlinks and deletes a rack entry. The caller checks it is empty first.
    bool removeRack(const string& rackId);

    int totalStock() const;     // sum of totals across all racks
    int reservedStock() const;  // sum of reserved across all racks
    int availableStock() const; // total - reserved
    bool hasReservations() const;
    bool isLowStock() const;
};

#endif

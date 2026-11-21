#include "Inventory.h"
#include "UI.h"
#include "Utils.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

// ---------------------------------------------------------------
// File loading
// ---------------------------------------------------------------

bool Inventory::loadRacks(const string& fileName, const Graph& warehouse) {
    return racks.load(fileName, warehouse);
}

bool Inventory::loadProducts(const string& fileName) {
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

        // Split "P101,Bluetooth Speaker,1.0,10"
        stringstream parts(line);
        string id, name, weightText, reorderText;
        getline(parts, id, ',');
        getline(parts, name, ',');
        getline(parts, weightText, ',');
        getline(parts, reorderText);

        double weight;
        int reorder;
        if (!parseDouble(weightText, weight) || !parseInt(reorderText, reorder)) {
            cout << "  " << fileName << " line " << lineNumber << ": weight or reorder level is not a number, skipped.\n";
            continue;
        }

        if (addProduct(toUpper(trim(id)), trim(name), weight, reorder)) {
            loaded++;
        } else {
            cout << "  (" << fileName << " line " << lineNumber << " skipped)\n";
        }
    }

    cout << "Loaded " << loaded << " products from " << fileName << ".\n";
    return true;
}

bool Inventory::loadInventory(const string& fileName) {
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

        // Split "P101,RK01,7"
        stringstream parts(line);
        string productId, rackId, quantityText;
        getline(parts, productId, ',');
        getline(parts, rackId, ',');
        getline(parts, quantityText);

        int quantity;
        if (!parseInt(quantityText, quantity)) {
            cout << "  " << fileName << " line " << lineNumber << ": quantity is not a number, skipped.\n";
            continue;
        }

        if (addRackEntry(toUpper(trim(productId)), toUpper(trim(rackId)), quantity)) {
            loaded++;
        } else {
            cout << "  (" << fileName << " line " << lineNumber << " skipped)\n";
        }
    }

    cout << "Loaded " << loaded << " inventory records from " << fileName << ".\n";
    return true;
}

// ---------------------------------------------------------------
// Products
// ---------------------------------------------------------------

bool Inventory::addProduct(const string& id, const string& name, double weight, int reorderLevel) {
    if (!isValidId(id, "P")) {
        cout << UI::err() << "Invalid product ID '" << id << "'. It must look like P101.\n";
        return false;
    }
    if (name.empty()) {
        cout << UI::err() << "Product name cannot be empty.\n";
        return false;
    }
    if (weight <= 0) {
        cout << UI::err() << "Unit weight must be greater than 0.\n";
        return false;
    }
    if (reorderLevel < 0) {
        cout << UI::err() << "Reorder level cannot be negative.\n";
        return false;
    }

    Product* product = new Product(id, name, weight, reorderLevel);
    if (!products.insert(product)) {
        // Tree rejected it, so we still own it and must free it
        delete product;
        cout << UI::err() << "Product " << id << " already exists.\n";
        return false;
    }
    return true;
}

bool Inventory::deleteProduct(const string& id) {
    Product* product = products.search(id);
    if (product == nullptr) {
        cout << UI::err() << "Product " << id << " not found.\n";
        return false;
    }

    // PDF rule 11: a product with active reservations cannot be deleted
    if (product->hasReservations()) {
        cout << UI::err() << "Cannot delete " << id << ": some racks hold reservations for it.\n";
        return false;
    }

    products.remove(id);
    return true;
}

Product* Inventory::findProduct(const string& id) const {
    return products.search(id);
}

void Inventory::displayProduct(const string& id) const {
    Product* p = products.search(id);
    if (p == nullptr) {
        cout << UI::err() << "Product " << id << " not found.\n";
        return;
    }

    cout << "\n  " << p->id << " - " << p->name << "\n";
    cout << "  Unit weight: " << fixed << setprecision(1) << p->unitWeight << " kg"
         << "   Reorder level: " << p->reorderLevel << "\n";
    cout << "  Total: " << p->totalStock() << "   Reserved: " << p->reservedStock()
         << "   Available: " << p->availableStock() << "\n";

    if (p->rackHead == nullptr) {
        cout << "  Not stored in any rack.\n";
        return;
    }

    cout << "\n  " << UI::start("1;36") << left << setw(7) << "Rack" << setw(10) << "Location"
         << right << setw(7) << "Total" << setw(10) << "Reserved" << setw(11) << "Available"
         << "   Reservations" << UI::reset() << "\n";
    cout << UI::start("90") << "  ------------------------------------------------------------------" << UI::reset() << "\n";
    RackEntry* rack = p->rackHead;
    while (rack != nullptr) {
        cout << "  " << left << setw(7) << rack->rackId << setw(10) << racks.locationOf(rack->rackId)
             << right << setw(7) << rack->totalQuantity << setw(10) << rack->reservedQuantity
             << setw(11) << rack->available() << "   ";
        rack->displayReservations();
        cout << "\n";
        rack = rack->next;
    }
}

void Inventory::displayAllProducts() const {
    products.displayAll();
}

void Inventory::displayTreeStructure() const {
    products.displayStructure();
}

void Inventory::displayLowStock() const {
    products.displayLowStock();
}

void Inventory::displayRacks() const {
    racks.display();
}

// ---------------------------------------------------------------
// Rack entries
// ---------------------------------------------------------------

bool Inventory::addRackEntry(const string& productId, const string& rackId, int quantity) {
    Product* product = products.search(productId);
    if (product == nullptr) {
        cout << UI::err() << "Product " << productId << " not found.\n";
        return false;
    }
    if (racks.find(rackId) == nullptr) {
        cout << UI::err() << "Rack " << rackId << " does not exist in racks.txt.\n";
        return false;
    }
    if (quantity < 0) {
        cout << UI::err() << "Quantity cannot be negative.\n";
        return false;
    }
    if (!product->addRack(rackId, quantity)) {
        cout << "  " << productId << " is already listed in rack " << rackId << ".\n";
        return false;
    }
    return true;
}

bool Inventory::updateQuantity(const string& productId, const string& rackId, int newQuantity, int& oldQuantity) {
    Product* product = products.search(productId);
    if (product == nullptr) {
        cout << UI::err() << "Product " << productId << " not found.\n";
        return false;
    }
    RackEntry* rack = product->findRack(rackId);
    if (rack == nullptr) {
        cout << "  " << productId << " is not stored in rack " << rackId << ".\n";
        return false;
    }

    // The new total cannot be negative or drop below what orders have already reserved
    if (newQuantity < 0) {
        cout << UI::err() << "Quantity cannot be negative.\n";
        return false;
    }
    if (newQuantity < rack->reservedQuantity) {
        cout << UI::err() << "Quantity cannot be lower than the reserved amount (" << rack->reservedQuantity << ").\n";
        return false;
    }

    oldQuantity = rack->totalQuantity;
    rack->totalQuantity = newQuantity;
    return true;
}

bool Inventory::removeRackEntry(const string& productId, const string& rackId) {
    Product* product = products.search(productId);
    if (product == nullptr) {
        cout << UI::err() << "Product " << productId << " not found.\n";
        return false;
    }
    RackEntry* rack = product->findRack(rackId);
    if (rack == nullptr) {
        cout << "  " << productId << " is not stored in rack " << rackId << ".\n";
        return false;
    }

    // Only an empty rack entry with no reservations may be removed
    if (rack->totalQuantity != 0 || rack->reservedQuantity != 0) {
        cout << UI::err() << "Rack entry can only be removed when total and reserved are both 0 "
             << "(total " << rack->totalQuantity << ", reserved " << rack->reservedQuantity << ").\n";
        return false;
    }

    product->removeRack(rackId);
    return true;
}

int Inventory::totalStock(const string& productId) const {
    Product* product = products.search(productId);
    if (product == nullptr) {
        return -1;
    }
    return product->totalStock();
}

int Inventory::availableStock(const string& productId) const {
    Product* product = products.search(productId);
    if (product == nullptr) {
        return -1;
    }
    return product->availableStock();
}

// ---------------------------------------------------------------
// Rack allocation rule
// ---------------------------------------------------------------
// 1. Only racks with available > 0 and a reachable location are considered.
// 2. If some rack can supply ALL the remaining quantity, take it from the nearest such rack.
// 3. Otherwise empty the nearest rack and repeat with what is left.
// Ties: smaller distance, then larger available quantity, then smaller RackID.
// All distances are measured from the same reference location.

bool Inventory::allocate(const string& productId, int quantity, const string& referenceLocation,
                         const Graph& warehouse, Allocation& result, int& shortage, bool allowPartial) const {
    result.clear();
    shortage = 0;

    Product* product = products.search(productId);
    if (product == nullptr || quantity <= 0) {
        return false;
    }

    // Count the rack entries so we can size our working arrays
    int rackCount = 0;
    RackEntry* rack = product->rackHead;
    while (rack != nullptr) {
        rackCount++;
        rack = rack->next;
    }
    if (rackCount == 0) {
        shortage = quantity;
        return false;
    }

    // Working arrays: one slot per candidate rack
    RackEntry** candidate = new RackEntry*[rackCount];
    string* location = new string[rackCount];
    int* distance = new int[rackCount];
    int* remainingInRack = new int[rackCount];
    int candidates = 0;
    int reachableTotal = 0;

    // Step 1: keep only racks with stock that a robot can actually reach
    rack = product->rackHead;
    while (rack != nullptr) {
        if (rack->available() > 0) {
            string loc = racks.locationOf(rack->rackId);
            int d = -1;
            if (loc != "") {
                d = warehouse.shortestDistance(referenceLocation, loc);
            }
            if (d >= 0) {
                candidate[candidates] = rack;
                location[candidates] = loc;
                distance[candidates] = d;
                remainingInRack[candidates] = rack->available();
                reachableTotal += rack->available();
                candidates++;
            }
        }
        rack = rack->next;
    }

    // Shortage check: by default nothing is allocated if we cannot cover everything
    int target = quantity;
    if (reachableTotal < quantity) {
        shortage = quantity - reachableTotal;
        if (!allowPartial || reachableTotal == 0) {
            delete[] candidate;
            delete[] location;
            delete[] distance;
            delete[] remainingInRack;
            return false;
        }
        target = reachableTotal;
    }

    int remaining = target;
    while (remaining > 0) {
        // Step 2: nearest rack that can supply everything that is left
        int pick = -1;
        for (int i = 0; i < candidates; i++) {
            if (remainingInRack[i] < remaining) {
                continue;
            }
            if (pick == -1 || distance[i] < distance[pick]
                || (distance[i] == distance[pick] && remainingInRack[i] > remainingInRack[pick])
                || (distance[i] == distance[pick] && remainingInRack[i] == remainingInRack[pick]
                    && idLess(candidate[i]->rackId, candidate[pick]->rackId))) {
                pick = i;
            }
        }

        int take;
        if (pick != -1) {
            take = remaining;
        } else {
            // Step 3: no single rack is enough, so empty the nearest one with stock
            for (int i = 0; i < candidates; i++) {
                if (remainingInRack[i] == 0) {
                    continue;
                }
                if (pick == -1 || distance[i] < distance[pick]
                    || (distance[i] == distance[pick] && remainingInRack[i] > remainingInRack[pick])
                    || (distance[i] == distance[pick] && remainingInRack[i] == remainingInRack[pick]
                        && idLess(candidate[i]->rackId, candidate[pick]->rackId))) {
                    pick = i;
                }
            }
            take = remainingInRack[pick];
        }

        result.add(candidate[pick]->rackId, location[pick], take, distance[pick]);
        remainingInRack[pick] -= take;
        remaining -= take;
    }

    delete[] candidate;
    delete[] location;
    delete[] distance;
    delete[] remainingInRack;
    return true;
}

// ---------------------------------------------------------------
// Reservations
// ---------------------------------------------------------------

bool Inventory::reserve(const string& productId, const Allocation& allocation, const string& orderId) {
    Product* product = products.search(productId);
    if (product == nullptr) {
        return false;
    }

    // Validate first: every rack must still exist and have enough available stock.
    // (An allocation lists each rack at most once, so checking one by one is enough.)
    AllocationNode* node = allocation.getHead();
    while (node != nullptr) {
        RackEntry* rack = product->findRack(node->rackId);
        if (rack == nullptr || rack->available() < node->quantity) {
            return false;
        }
        node = node->next;
    }

    // Then apply: nothing above failed, so every reservation will succeed
    node = allocation.getHead();
    while (node != nullptr) {
        product->findRack(node->rackId)->addReservation(orderId, node->quantity);
        node = node->next;
    }
    return true;
}

int Inventory::releaseOrder(const string& orderId) {
    return products.releaseOrder(orderId);
}

const RackDirectory& Inventory::getRacks() const {
    return racks;
}

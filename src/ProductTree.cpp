#include "ProductTree.h"
#include "Utils.h"
#include <iostream>
#include <iomanip>

AVLNode::AVLNode(Product* p) {
    product = p;
    left = nullptr;
    right = nullptr;
    height = 1;
}

ProductTree::ProductTree() {
    root = nullptr;
    count = 0;
}

ProductTree::~ProductTree() {
    destroy(root);
}

void ProductTree::destroy(AVLNode* node) {
    // Post-order: delete children before the parent
    if (node == nullptr) {
        return;
    }
    destroy(node->left);
    destroy(node->right);
    delete node->product;
    delete node;
}

// ---------------------------------------------------------------
// Height and balance helpers
// ---------------------------------------------------------------

int ProductTree::height(AVLNode* node) const {
    if (node == nullptr) {
        return 0;
    }
    return node->height;
}

int ProductTree::balanceFactor(AVLNode* node) const {
    // Positive = left side is taller, negative = right side is taller
    if (node == nullptr) {
        return 0;
    }
    return height(node->left) - height(node->right);
}

void ProductTree::updateHeight(AVLNode* node) {
    int leftHeight = height(node->left);
    int rightHeight = height(node->right);
    if (leftHeight > rightHeight) {
        node->height = leftHeight + 1;
    } else {
        node->height = rightHeight + 1;
    }
}

// ---------------------------------------------------------------
// Rotations
// ---------------------------------------------------------------
/*
    rotateRight(y):            rotateLeft(x):

          y                x          x                  y
         / \              / \        / \                / \
        x   C    -->     A   y      A   y      -->     x   C
       / \                  / \        / \            / \
      A   B                B   C      B   C          A   B
*/

AVLNode* ProductTree::rotateRight(AVLNode* y) {
    AVLNode* x = y->left;
    AVLNode* b = x->right;

    x->right = y;
    y->left = b;

    // y is now below x, so update y first
    updateHeight(y);
    updateHeight(x);
    return x;
}

AVLNode* ProductTree::rotateLeft(AVLNode* x) {
    AVLNode* y = x->right;
    AVLNode* b = y->left;

    y->left = x;
    x->right = b;

    updateHeight(x);
    updateHeight(y);
    return y;
}

AVLNode* ProductTree::rebalance(AVLNode* node) {
    updateHeight(node);
    int balance = balanceFactor(node);

    // Left side too tall
    if (balance > 1) {
        // Left-Right case: first rotate the left child left
        if (balanceFactor(node->left) < 0) {
            node->left = rotateLeft(node->left);
        }
        // Left-Left case (or LR after the first rotation)
        return rotateRight(node);
    }

    // Right side too tall
    if (balance < -1) {
        // Right-Left case: first rotate the right child right
        if (balanceFactor(node->right) > 0) {
            node->right = rotateRight(node->right);
        }
        // Right-Right case (or RL after the first rotation)
        return rotateLeft(node);
    }

    return node; // already balanced
}

// ---------------------------------------------------------------
// Insert
// ---------------------------------------------------------------

AVLNode* ProductTree::insertNode(AVLNode* node, Product* product, bool& inserted) {
    // Reached an empty spot: the new product goes here
    if (node == nullptr) {
        inserted = true;
        return new AVLNode(product);
    }

    if (idLess(product->id, node->product->id)) {
        node->left = insertNode(node->left, product, inserted);
    } else if (idLess(node->product->id, product->id)) {
        node->right = insertNode(node->right, product, inserted);
    } else {
        inserted = false; // duplicate ID
        return node;
    }

    // On the way back up, fix any imbalance the insert caused
    return rebalance(node);
}

bool ProductTree::insert(Product* product) {
    bool inserted = false;
    root = insertNode(root, product, inserted);
    if (inserted) {
        count++;
    }
    return inserted;
}

// ---------------------------------------------------------------
// Delete
// ---------------------------------------------------------------

AVLNode* ProductTree::minNode(AVLNode* node) const {
    while (node->left != nullptr) {
        node = node->left;
    }
    return node;
}

AVLNode* ProductTree::removeNode(AVLNode* node, const string& id, bool& removed) {
    if (node == nullptr) {
        return nullptr; // not found
    }

    if (idLess(id, node->product->id)) {
        node->left = removeNode(node->left, id, removed);
    } else if (idLess(node->product->id, id)) {
        node->right = removeNode(node->right, id, removed);
    } else {
        // Found the node to delete
        if (node->left == nullptr || node->right == nullptr) {
            // Zero or one child: replace the node with its child (or nothing)
            AVLNode* child;
            if (node->left != nullptr) {
                child = node->left;
            } else {
                child = node->right;
            }
            delete node->product;
            delete node;
            removed = true;
            return child;
        }

        // Two children: swap products with the in-order successor
        // (smallest node in the right subtree), then delete from the right subtree.
        // The target product now sits in the successor node, which has at most one child.
        AVLNode* successor = minNode(node->right);
        Product* temp = node->product;
        node->product = successor->product;
        successor->product = temp;
        node->right = removeNode(node->right, id, removed);
    }

    return rebalance(node);
}

bool ProductTree::remove(const string& id) {
    bool removed = false;
    root = removeNode(root, id, removed);
    if (removed) {
        count--;
    }
    return removed;
}

// ---------------------------------------------------------------
// Search
// ---------------------------------------------------------------

Product* ProductTree::search(const string& id) const {
    // Normal BST search: go left for smaller IDs, right for larger
    AVLNode* current = root;
    while (current != nullptr) {
        if (id == current->product->id) {
            return current->product;
        }
        if (idLess(id, current->product->id)) {
            current = current->left;
        } else {
            current = current->right;
        }
    }
    return nullptr;
}

// ---------------------------------------------------------------
// Display
// ---------------------------------------------------------------

void ProductTree::displayInOrder(AVLNode* node) const {
    if (node == nullptr) {
        return;
    }
    displayInOrder(node->left);

    Product* p = node->product;
    cout << "  " << left << setw(6) << p->id
         << setw(22) << p->name
         << right << setw(7) << fixed << setprecision(1) << p->unitWeight << " kg"
         << setw(8) << p->totalStock()
         << setw(10) << p->reservedStock()
         << setw(11) << p->availableStock()
         << setw(9) << p->reorderLevel << "\n";

    displayInOrder(node->right);
}

void ProductTree::displayAll() const {
    if (root == nullptr) {
        cout << "  No products in inventory.\n";
        return;
    }
    cout << "\n  " << left << setw(6) << "ID" << setw(22) << "Name"
         << right << setw(10) << "Weight" << setw(8) << "Total"
         << setw(10) << "Reserved" << setw(11) << "Available" << setw(9) << "Reorder" << "\n";
    cout << "  --------------------------------------------------------------------------\n";
    displayInOrder(root);
    cout << "  (" << count << " products, shown in ID order by in-order traversal)\n";
}

void ProductTree::displayLowStock(AVLNode* node, int& found) const {
    if (node == nullptr) {
        return;
    }
    displayLowStock(node->left, found);
    Product* p = node->product;
    if (p->isLowStock()) {
        cout << "  " << left << setw(6) << p->id << setw(22) << p->name << right
             << " available " << p->availableStock()
             << ", reorder level " << p->reorderLevel << "\n";
        found++;
    }
    displayLowStock(node->right, found);
}

void ProductTree::displayLowStock() const {
    int found = 0;
    cout << "\n  Products with available stock below reorder level:\n";
    displayLowStock(root, found);
    if (found == 0) {
        cout << "  None. All products are above their reorder level.\n";
    }
}

void ProductTree::displayStructure(AVLNode* node, int depth) const {
    // Right subtree is printed first so the tree reads correctly when tilted left
    if (node == nullptr) {
        return;
    }
    displayStructure(node->right, depth + 1);
    for (int i = 0; i < depth; i++) {
        cout << "        ";
    }
    cout << node->product->id << " (h=" << node->height << ")\n";
    displayStructure(node->left, depth + 1);
}

void ProductTree::displayStructure() const {
    if (root == nullptr) {
        cout << "  Tree is empty.\n";
        return;
    }
    cout << "\n  AVL tree (rotated 90 degrees: root on the left, right child above, left child below)\n\n";
    displayStructure(root, 0);
}

// ---------------------------------------------------------------
// Reservations across all products
// ---------------------------------------------------------------

int ProductTree::releaseOrder(AVLNode* node, const string& orderId) {
    if (node == nullptr) {
        return 0;
    }
    int freed = releaseOrder(node->left, orderId) + releaseOrder(node->right, orderId);
    RackEntry* rack = node->product->rackHead;
    while (rack != nullptr) {
        freed += rack->releaseReservation(orderId);
        rack = rack->next;
    }
    return freed;
}

int ProductTree::releaseOrder(const string& orderId) {
    return releaseOrder(root, orderId);
}

int ProductTree::size() const {
    return count;
}

bool ProductTree::isEmpty() const {
    return root == nullptr;
}

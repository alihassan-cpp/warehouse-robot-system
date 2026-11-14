#ifndef PRODUCTTREE_H
#define PRODUCTTREE_H

#include <string>
#include "Product.h"
using namespace std;

// One node of the AVL tree. It points to a Product instead of storing it directly,
// so during deletion we can swap pointers instead of copying whole products.
class AVLNode {
public:
    Product* product;
    AVLNode* left;
    AVLNode* right;
    int height; // a leaf has height 1, an empty tree has height 0

    AVLNode(Product* p);
};

// Self-balancing binary search tree of products, ordered by ProductID.
// After every insert or delete, each node's left and right heights differ by at most 1,
// so search, insert and delete are all O(log n).
class ProductTree {
private:
    AVLNode* root;
    int count;

    int height(AVLNode* node) const;
    int balanceFactor(AVLNode* node) const;
    void updateHeight(AVLNode* node);

    AVLNode* rotateRight(AVLNode* node);
    AVLNode* rotateLeft(AVLNode* node);
    AVLNode* rebalance(AVLNode* node);

    // Recursive helpers. They return the new root of the subtree after changes.
    AVLNode* insertNode(AVLNode* node, Product* product, bool& inserted);
    AVLNode* removeNode(AVLNode* node, const string& id, bool& removed);
    AVLNode* minNode(AVLNode* node) const;

    void destroy(AVLNode* node);
    void displayInOrder(AVLNode* node) const;
    void displayLowStock(AVLNode* node, int& found) const;
    void displayStructure(AVLNode* node, int depth) const;
    int releaseOrder(AVLNode* node, const string& orderId);

public:
    ProductTree();
    ~ProductTree();

    ProductTree(const ProductTree&) = delete;
    ProductTree& operator=(const ProductTree&) = delete;

    // Takes ownership of the product if inserted. Returns false for duplicate IDs.
    bool insert(Product* product);

    // Deletes the product and rebalances. Returns false if not found.
    bool remove(const string& id);

    Product* search(const string& id) const;

    void displayAll() const;       // in-order, so products appear sorted by ID
    void displayLowStock() const;  // available stock below reorder level
    void displayStructure() const; // draws the tree sideways to show balance

    // Releases every reservation belonging to an order, across all products
    int releaseOrder(const string& orderId);

    int size() const;
    bool isEmpty() const;
};

#endif

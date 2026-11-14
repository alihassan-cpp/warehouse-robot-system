#ifndef STACK_H
#define STACK_H

// A LIFO stack built on a singly linked list.
// push and pop both work at the top, so they are O(1).

template <typename T>
class StackNode {
public:
    T data;
    StackNode<T>* next; // the node below this one

    StackNode(const T& value) {
        data = value;
        next = nullptr;
    }
};

template <typename T>
class Stack {
private:
    StackNode<T>* top;
    int count;

public:
    Stack() {
        top = nullptr;
        count = 0;
    }

    ~Stack() {
        clear();
    }

    Stack(const Stack&) = delete;
    Stack& operator=(const Stack&) = delete;

    // New items always go on top
    void push(const T& value) {
        StackNode<T>* node = new StackNode<T>(value);
        node->next = top;
        top = node;
        count++;
    }

    // Removes the top item (the most recent one). Returns false if empty.
    bool pop(T& out) {
        if (top == nullptr) {
            return false;
        }
        StackNode<T>* old = top;
        out = old->data;
        top = top->next;
        delete old;
        count--;
        return true;
    }

    // Looks at the top item without removing it
    bool peek(T& out) const {
        if (top == nullptr) {
            return false;
        }
        out = top->data;
        return true;
    }

    bool isEmpty() const {
        return top == nullptr;
    }

    int size() const {
        return count;
    }

    // For displaying the history from newest to oldest
    StackNode<T>* getTop() const {
        return top;
    }

    void clear() {
        while (top != nullptr) {
            StackNode<T>* temp = top;
            top = top->next;
            delete temp;
        }
        count = 0;
    }
};

#endif

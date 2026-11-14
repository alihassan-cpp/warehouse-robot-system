#ifndef QUEUE_H
#define QUEUE_H

// A FIFO queue built on a singly linked list with front and rear pointers.
// It is a template so the same code can store Location pointers (BFS now),
// and later orders and robots (order queue, charging queues).

template <typename T>
class QueueNode {
public:
    T data;
    QueueNode<T>* next;

    QueueNode(const T& value) {
        data = value;
        next = nullptr;
    }
};

template <typename T>
class Queue {
private:
    QueueNode<T>* front;
    QueueNode<T>* rear;
    int count;

public:
    Queue() {
        front = nullptr;
        rear = nullptr;
        count = 0;
    }

    ~Queue() {
        clear();
    }

    // Copying would make two queues share the same nodes and delete them twice,
    // so copying is disabled on purpose.
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    // New items always join at the rear: O(1)
    void enqueue(const T& value) {
        QueueNode<T>* node = new QueueNode<T>(value);
        if (rear == nullptr) {
            front = node;
            rear = node;
        } else {
            rear->next = node;
            rear = node;
        }
        count++;
    }

    // Removes the oldest item from the front: O(1)
    // Returns false if the queue was empty.
    bool dequeue(T& out) {
        if (front == nullptr) {
            return false;
        }
        QueueNode<T>* old = front;
        out = old->data;
        front = front->next;
        if (front == nullptr) {
            rear = nullptr;
        }
        delete old;
        count--;
        return true;
    }

    // Looks at the front item without removing it
    bool peek(T& out) const {
        if (front == nullptr) {
            return false;
        }
        out = front->data;
        return true;
    }

    bool isEmpty() const {
        return front == nullptr;
    }

    int size() const {
        return count;
    }

    // Lets other code walk through the queue (for display) without removing anything
    QueueNode<T>* getFront() const {
        return front;
    }

    void clear() {
        while (front != nullptr) {
            QueueNode<T>* temp = front;
            front = front->next;
            delete temp;
        }
        rear = nullptr;
        count = 0;
    }
};

#endif

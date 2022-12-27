//
// Created by Martin Boros on 12/26/22.
//

#ifndef LOCKLESS_DATA_STRUCTURES_LOCK_FREE_QUEUE_H
#define LOCKLESS_DATA_STRUCTURES_LOCK_FREE_QUEUE_H

#include <atomic>

/// A lock free queue implementation based on the paper <a href="https://www.cs.rochester.edu/~scott/papers/1996_PODC_queues.pdf">Simple, Fast,
/// and Practical Non-Blocking and Blocking Concurrent Queue Algorithms<a> by Maged M. Michael and Michael L. Scott.
/// \tparam T type of the value stored in the queue
template<typename T>
class LockFreeQueue {
    /// Node object of the underlying linked list
    struct node_t {
        T value;
        std::atomic<node_t*> next;
        std::atomic<int> ref_count;

        node_t() {
            next.store(nullptr);
            ref_count = 0;
        }

        explicit node_t(T value) {
            this->value = value;
            next.store(nullptr);
            ref_count = 0;
        }
    };

    std::atomic<node_t*> m_head;
    std::atomic<node_t*> m_tail;

    /// gives us access to range based for loops over the linked list backing the queue.
    /// This is not thread safe and should only be used for inspecting the list when no operations are being done.
    class iterator {
    public:
        iterator& operator++() {
            current_ = current_->next.load();
            return *this;
        }

        bool operator!=(const iterator& other) const {
            return current_ != other.current_;
        }

        T& operator*() const {
            return current_->value;
        }

    private:
        friend LockFreeQueue;
        iterator(node_t* current) : current_(current) {}

        node_t* current_;
    };

public:
    LockFreeQueue() {
        auto node = new node_t();
        m_head.store(node);
        m_tail.store(node);
    }

    /// Pushes a value onto the queue
    /// \param value to be pushed
    void push(T value) {
        auto node = new node_t(value);
        node_t* tail;
        node_t* next;

        while (true) {
            tail = this->m_tail.load();
            next = tail->next.load();
            if (tail == this->m_tail.load()) {
                if (next == nullptr) {
                    if (this->m_tail.load()->next.compare_exchange_weak(next, node, std::memory_order_release, std::memory_order_acquire)) {
                        break;
                    }
                } else {
                    this->m_tail.compare_exchange_weak(tail, next, std::memory_order_release, std::memory_order_acquire);
                }
            }
        }
        this->m_tail.compare_exchange_weak(tail, node, std::memory_order_release, std::memory_order_acquire);
    }

    /// Tries to pop a value off the stack.
    /// \param value reference will be replaced by the popped value if the operation succeeds
    /// \return true if succeeds else false
    bool pop(T& value) {
        node_t* head;
        node_t* tail;
        node_t* next;
        while (true) {
            head = m_head.load();
            tail = m_tail.load();
            next = head->next.load();
            if (head == m_head.load()) {
                if (head == tail) {
                    if (next == nullptr) {
                        return false;
                    }
                    m_tail.compare_exchange_weak(tail, next, std::memory_order_release, std::memory_order_acquire);
                } else {
                    next->ref_count.fetch_add(1, std::memory_order_acquire);
                    value = next->value;
                    if (m_head.compare_exchange_weak(head, next, std::memory_order_release, std::memory_order_acquire)) {
                        break;
                    }
                }
            }
        }
        if (head->ref_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            free(head);
        }
        return true;
    }

    /// Used to iterate over the underlying linked list. This is not thread safe
    /// \return iterator to the first element in the list
    iterator begin() {
        return iterator(m_head.load()->next.load());
    }

    iterator end() {
        return iterator(nullptr);
    }
};

#endif //LOCKLESS_DATA_STRUCTURES_LOCK_FREE_QUEUE_H

/*
@file SPSCQueue.hpp
@brief Declaration of a lock-free single-producer single-consumer queue.
@ingroup common
Defines a lock-free single-producer single-consumer (SPSC) queue template class
for efficient inter-thread communication.
*/

#ifndef ENGINE_CORE_SPSCQUEUE_HPP
    #define ENGINE_CORE_SPSCQUEUE_HPP

    #include <atomic>
    #include <cstddef>

namespace Common {

    /// @brief Lock-free single-producer single-consumer queue.
    /// @tparam T The type of elements stored in the queue.
    /// @tparam Size The maximum size of the queue.
    template<typename T, std::size_t Size>
    class SPSCQueue {
public:
    
    /// @brief Enqueue an item into the queue.
    /// @param item The item to enqueue.
    /// @return True if the item was enqueued successfully, false if the queue is full.
    bool enqueue(T item) {
        std::size_t next_head = (head_ + 1) % Size;
        if (next_head == tail_.load(std::memory_order_acquire))
            return false;
        buffer_[head_] = std::move(item);
        head_.store(next_head, std::memory_order_release);
        return true;
    }

    /// @brief Dequeue an item from the queue.
    /// @param out Reference to store the dequeued item.
    /// @return True if an item was dequeued successfully, false if the queue is empty
    bool dequeue(T& out) {
        std::size_t tail = tail_.load(std::memory_order_relaxed);
        if (tail == head_.load(std::memory_order_acquire))
            return false;
        out = std::move(buffer_[tail]);
        tail_.store((tail + 1) % Size, std::memory_order_release);
        return true;
    }

private:
    T buffer_[Size];
    std::atomic<size_t> head_{0}; // written by producer, read by consumer
    std::atomic<size_t> tail_{0}; // written by consumer, read by producer
};

} // namespace Common

#endif // ENGINE_CORE_SPSCQUEUE_HPP
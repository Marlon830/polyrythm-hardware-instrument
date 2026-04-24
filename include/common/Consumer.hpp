/*
@file Consumer.hpp
@brief Declaration of the Consumer class for consuming items from a SPSCQueue.
@ingroup common
*/

#ifndef COMMON_CONSUMER_HPP
#define COMMON_CONSUMER_HPP

#include "common/SPSCQueue.hpp"

namespace Common {
    /// @brief Consumer class for consuming items from a SPSCQueue.
    /// @tparam T The type of items consumed.
    template<typename T>
    class Consumer {
    public:
        /// @brief Constructs a Consumer with the given SPSCQueue.
        /// @param q Reference to the SPSCQueue to consume items from.
        Consumer(SPSCQueue<T, 1024>& q) : queue(q) {}

    protected:
        /// @brief Processes all pending items from the queue.
        void processAll() {
            T item;
            while (queue.dequeue(item)) {
                handle(item);
            }
        }
        
        /// @brief Handles a consumed item.
        /// @param item The consumed item.
        virtual void handle(T& item) = 0;

    private:
        SPSCQueue<T, 1024>& queue;
    };
} // namespace Common
#endif // COMMON_CONSUMER_HPP
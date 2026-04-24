///@file Producer.hpp
///@brief Declaration of the Producer class for producing items into a SPSCQueue.
///@ingroup common

#ifndef COMMON_PRODUCER_HPP
#define COMMON_PRODUCER_HPP

#include "common/SPSCQueue.hpp"

namespace Common {
    /// @brief Producer class for producing items into a SPSCQueue.
    /// @tparam T The type of items produced.
    template<typename T>
    class Producer {
    public:
        /// @brief Constructs a Producer with the given SPSCQueue.
        /// @param q Reference to the SPSCQueue to produce items into.
        Producer(SPSCQueue<T, 1024>& q) : queue(q) {};

    protected:
        /// @brief Pushes an item into the queue.
        /// @param item The item to push into the queue.
        void push(T&& item) {
            queue.enqueue(std::move(item));
        }

    private:
        SPSCQueue<T, 1024>& queue;
    };

} // namespace Common
#endif // COMMON_PRODUCER_HPP
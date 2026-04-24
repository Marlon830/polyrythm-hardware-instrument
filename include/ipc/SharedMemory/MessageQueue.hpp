#ifndef IPC_MESSAGE_QUEUE_HPP
#define IPC_MESSAGE_QUEUE_HPP

#include <queue>
#include <mutex>
#include <string>
#include <optional>

namespace IPC {

/// @brief Simple thread-safe queue for string messages
class MessageQueue {
public:
    MessageQueue() = default;
    ~MessageQueue() = default;
    
    MessageQueue(const MessageQueue&) = delete;
    MessageQueue& operator=(const MessageQueue&) = delete;
    
    /// @brief Push a message to the queue
    void push(const std::string& message) {
        std::lock_guard<std::mutex> lock(_mutex);
        _queue.push(message);
    }
    
    /// @brief Try to pop a message (non-blocking)
    std::optional<std::string> tryPop() {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_queue.empty()) {
            return std::nullopt;
        }
        std::string msg = _queue.front();
        _queue.pop();
        return msg;
    }
    
    /// @brief Check if queue is empty
    bool empty() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _queue.empty();
    }

private:
    std::queue<std::string> _queue;
    mutable std::mutex _mutex;
};

} // namespace IPC

#endif // IPC_MESSAGE_QUEUE_HPP

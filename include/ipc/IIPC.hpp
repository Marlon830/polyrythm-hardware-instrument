#ifndef IIPC_HPP
#define IIPC_HPP

#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace IPC {

/**
 * @brief Interface for Inter-Process Communication mechanisms
 * 
 * This interface provides a common abstraction for different IPC implementations
 * such as shared memory, ZeroMQ, Unix sockets, etc.
 */
class IIPC {
public:
    virtual ~IIPC() = default;

    /**
     * @brief Initialize and connect the IPC mechanism
     * @param endpoint The endpoint/address to connect to (e.g., "tcp://localhost:5555" or "/dev/shm/myshm")
     * @return true if connection successful, false otherwise
     */
    virtual bool connect(const std::string& endpoint) = 0;

    /**
     * @brief Disconnect and cleanup the IPC mechanism
     */
    virtual bool disconnect() = 0;

    /**
     * @brief Check if the IPC is currently connected
     * @return true if connected, false otherwise
     */
    virtual bool isConnected() const = 0;

    /**
     * @brief Send a message through the IPC channel
     * @param data The data to send
     * @return true if send successful, false otherwise
     */
    virtual bool send(const std::vector<uint8_t>& data) = 0;

    /**
     * @brief Send a string message through the IPC channel
     * @param message The message to send
     * @return true if send successful, false otherwise
     */
    virtual bool send(const std::string& message) = 0;

    /**
     * @brief Receive a message from the IPC channel (blocking)
     * @param data Buffer to store received data
     * @return true if receive successful, false otherwise
     */
    virtual bool receive(std::vector<uint8_t>& data) = 0;

    /**
     * @brief Receive a string message from the IPC channel (blocking)
     * @param message Buffer to store received message
     * @return true if receive successful, false otherwise
     */
    virtual bool receive(std::string& message) = 0;
};

} // namespace ipc

#endif // IIPC_HPP
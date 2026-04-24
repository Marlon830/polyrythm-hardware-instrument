#ifndef SHARED_MEMORY_HPP
#define SHARED_MEMORY_HPP

#include "ipc/IIPC.hpp"
#include "ipc/SharedMemory/MessageQueue.hpp"
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <string>
#include <cstddef>
#include <memory>
#include <thread>
#include <atomic>

namespace IPC {

static constexpr size_t SHARED_BUFFER_SIZE = 1024;

struct SharedBuffer {
    boost::interprocess::interprocess_mutex mutex;
    boost::interprocess::interprocess_condition cond;
    bool data_ready = false;
    size_t data_size = 0;
    uint8_t data[SHARED_BUFFER_SIZE];
};

class SharedMemory : public IIPC {
public:
    SharedMemory(const std::string& endpoint, const std::string& name);
    ~SharedMemory() override;

    bool connect(const std::string& endpoint) override;
    bool disconnect() override;
    bool isConnected() const override;
    bool send(const std::vector<uint8_t>& data) override;
    bool send(const std::string& message) override;
    bool receive(std::vector<uint8_t>& data) override;
    bool receive(std::string& message) override;
    
    /// @brief Start the reader thread
    void startReaderThread();
    
    /// @brief Stop the reader thread
    void stopReaderThread();
    
    /// @brief Get the message queue
    MessageQueue& getMessageQueue() { return _message_queue; }
    
private:
    void readerThreadFunc();
    
    std::string _name;
    std::string _endpoint;
    std::unique_ptr<boost::interprocess::shared_memory_object> _shm;
    std::unique_ptr<boost::interprocess::mapped_region> _region;
    SharedBuffer* _buffer = nullptr;
    
    // Threading
    std::unique_ptr<std::thread> _reader_thread;
    std::atomic<bool> _running{false};
    MessageQueue _message_queue;
};

} // namespace IPC

#endif // SHARED_MEMORY_HPP
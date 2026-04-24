#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <cstring>
#include <iostream>
#include <chrono>

#include "ipc/SharedMemory/SharedMemory.hpp"

namespace IPC
{
    SharedMemory::SharedMemory(const std::string &endpoint, const std::string &name)
        : _name(name), _endpoint(endpoint)
    {
        if (name == "audio")
        {
            boost::interprocess::shared_memory_object::remove(endpoint.c_str());
            _shm = std::make_unique<boost::interprocess::shared_memory_object>(
                boost::interprocess::create_only,
                endpoint.c_str(),
                boost::interprocess::read_write);

            _shm->truncate(sizeof(SharedBuffer));

            _region = std::make_unique<boost::interprocess::mapped_region>(
                *_shm,
                boost::interprocess::read_write);

            // Construct SharedBuffer in shared memory using placement new
            _buffer = new (_region->get_address()) SharedBuffer();
        }

        if (name == "gui")
        {
            _shm = std::make_unique<boost::interprocess::shared_memory_object>(
                boost::interprocess::open_only,
                endpoint.c_str(),
                boost::interprocess::read_write);

            _region = std::make_unique<boost::interprocess::mapped_region>(
                *_shm,
                boost::interprocess::read_write);

            // Get pointer to existing SharedBuffer
            _buffer = static_cast<SharedBuffer*>(_region->get_address());
        }
    }

    SharedMemory::~SharedMemory()
    {
        stopReaderThread();
        disconnect();
    }

    bool SharedMemory::connect(const std::string &endpoint)
    {
        try
        {
            _endpoint = endpoint;
            _shm = std::make_unique<boost::interprocess::shared_memory_object>(
                boost::interprocess::open_only,
                endpoint.c_str(),
                boost::interprocess::read_write);

            _region = std::make_unique<boost::interprocess::mapped_region>(
                *_shm,
                boost::interprocess::read_write);

            _buffer = static_cast<SharedBuffer*>(_region->get_address());

            return true;
        }
        catch (const boost::interprocess::interprocess_exception &e)
        {
            std::cerr << "SharedMemory::connect failed: " << e.what() << std::endl;
            return false;
        }
    }

    bool SharedMemory::disconnect()
    {
        _buffer = nullptr;
        _region.reset();
        _shm.reset();
        return true;
    }

    bool SharedMemory::isConnected() const
    {
        return _shm != nullptr && _region != nullptr;
    }

    bool SharedMemory::send(const std::vector<uint8_t> &data)
    {
        if (!isConnected())
        {
            return false;
        }

        if (data.size() > SHARED_BUFFER_SIZE)
        {
            std::cerr << "SharedMemory::send: data size exceeds shared memory size" << std::endl;
            return false;
        }

        {
            boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(_buffer->mutex);
            
            // Wait until buffer is free (data_ready == false)
            // This prevents overwriting unread messages
            _buffer->cond.wait(lock, [this] { return !_buffer->data_ready; });
            
            std::memcpy(_buffer->data, data.data(), data.size());
            _buffer->data_size = data.size();
            _buffer->data_ready = true;
            _buffer->cond.notify_one();
        }
        return true;
    }

    bool SharedMemory::send(const std::string &message)
    {
        if (!isConnected())
        {
            return false;
        }

        if (message.size() + 1 > SHARED_BUFFER_SIZE)
        {
            std::cerr << "SharedMemory::send: message size exceeds shared memory size" << std::endl;
            return false;
        }

        {
            boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(_buffer->mutex);
            
            // Wait until buffer is free (data_ready == false)
            // This prevents overwriting unread messages
            _buffer->cond.wait(lock, [this] { return !_buffer->data_ready; });
            
            std::memcpy(_buffer->data, message.c_str(), message.size() + 1);
            _buffer->data_size = message.size() + 1;
            _buffer->data_ready = true;
            _buffer->cond.notify_one();
        }
        return true;
    }

    bool SharedMemory::receive(std::vector<uint8_t> &data)
    {
        if (!isConnected())
        {
            return false;
        }

        {
            boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(_buffer->mutex);
            _buffer->cond.wait(lock, [this] { return _buffer->data_ready; });

            data.resize(_buffer->data_size);
            std::memcpy(data.data(), _buffer->data, _buffer->data_size);
            _buffer->data_ready = false;
            
            // Notify sender that buffer is now free
            _buffer->cond.notify_one();
        }
        return true;
    }

    bool SharedMemory::receive(std::string &message)
    {
        if (!isConnected())
        {
            return false;
        }

        {
            boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(_buffer->mutex);
            _buffer->cond.wait(lock, [this] { return _buffer->data_ready; });

            message.assign(reinterpret_cast<const char *>(_buffer->data), _buffer->data_size - 1);
            _buffer->data_ready = false;
            
            // Notify sender that buffer is now free
            _buffer->cond.notify_one();
        }
        return true;
    }
    
    void SharedMemory::startReaderThread()
    {
        if (_running.load())
        {
            return;
        }
        
        _running.store(true);
        _reader_thread = std::make_unique<std::thread>(&SharedMemory::readerThreadFunc, this);
        std::cout << "[SharedMemory] Reader thread started for " << _name << std::endl;
    }
    
    void SharedMemory::stopReaderThread()
    {
        if (!_running.load())
        {
            return;
        }
        
        _running.store(false);
        
        if (_reader_thread && _reader_thread->joinable())
        {
            _reader_thread->join();
        }
        
        std::cout << "[SharedMemory] Reader thread stopped for " << _name << std::endl;
    }
    
    void SharedMemory::readerThreadFunc()
    {
        while (_running.load())
        {
            if (!isConnected())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            
            try
            {
                boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(_buffer->mutex);
                
                // Wait with timeout (100ms)
                auto timeout = boost::posix_time::microsec_clock::universal_time() + 
                               boost::posix_time::milliseconds(100);
                
                if (_buffer->cond.timed_wait(lock, timeout, [this] { return _buffer->data_ready; }))
                {
                    std::string message(reinterpret_cast<const char*>(_buffer->data), _buffer->data_size - 1);
                    _buffer->data_ready = false;
                    
                    // Notify sender that buffer is now free
                    _buffer->cond.notify_one();
                    
                    // Push to queue
                    _message_queue.push(message);
                }
            }
            catch (const std::exception& e)
            {
                std::cerr << "[SharedMemory] Reader thread error: " << e.what() << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }
}
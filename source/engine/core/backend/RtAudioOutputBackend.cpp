#include "engine/core/backend/RtAudioOutputBackend.hpp"
#include <cstring>
#include <algorithm>
#include <iostream>

namespace Engine {
    namespace Core {

    RtAudioOutputBackend::RtAudioOutputBackend() {
    }

    RtAudioOutputBackend::~RtAudioOutputBackend() {
        Engine::Core::RtAudioOutputBackend::close();
    }

    bool RtAudioOutputBackend::open() {
        if (_dac.getDeviceCount() < 1) {
            setError("open", -1); // No device
            return false;
        }

        // Internal buffer configuration (Ring Buffer)
        _ringBuffer.resize(_bufferSizeFrames * _channels, 0.0);
        _writeIndex = 0;
        _readIndex = 0;
        _availableData = 0;

        RtAudio::StreamParameters parameters;
        parameters.deviceId = _dac.getDefaultOutputDevice();
        parameters.nChannels = _channels;
        parameters.firstChannel = 0;

        unsigned int bufferFrames = 512; // Expected latency
        
        RtAudioErrorType result = _dac.openStream(
            &parameters, 
            nullptr, 
            RTAUDIO_FLOAT32,
            _sampleRate, 
            &bufferFrames, 
            &audioCallback, 
            (void*)this
        );

        if (result != RTAUDIO_NO_ERROR) {
            _lastErrMsg = _dac.getErrorText(); 
            setError("openStream", (int)result);
            return false;
        }

        result = _dac.startStream();
        if (result != RTAUDIO_NO_ERROR) {
            _lastErrMsg = _dac.getErrorText();
            setError("startStream", (int)result);
            return false;
        }

        _streamOpen = true;
        return true;
    }

    void RtAudioOutputBackend::close() {
        if (_dac.isStreamOpen()) {
            _dac.closeStream();
        }
        _streamOpen = false;
    }
    
    void RtAudioOutputBackend::write(const double* buffer, std::size_t size) {
         if (!_streamOpen) return;

        std::size_t framesToWrite = size / _channels;
        const double* srcPtr = buffer;

        while (framesToWrite > 0) {
            std::unique_lock<std::mutex> lock(_mutex);

            // ALSA-LIKE BLOCKING
            _cvBlockWrite.wait(lock, [this]() {
                return _availableData < _bufferSizeFrames;
            });

            std::size_t spaceUntilWrap = _bufferSizeFrames - _writeIndex;
            std::size_t spaceAvailable = _bufferSizeFrames - _availableData;
            
            std::size_t chunk = std::min(framesToWrite, spaceAvailable);
            
            std::size_t firstPart = std::min(chunk, spaceUntilWrap);
            
            std::memcpy(&_ringBuffer[_writeIndex * _channels], 
                        srcPtr, 
                        firstPart * _channels * sizeof(double));
            
            if (chunk > firstPart) {
                std::memcpy(&_ringBuffer[0], 
                            srcPtr + (firstPart * _channels), 
                            (chunk - firstPart) * _channels * sizeof(double));
            }

            _writeIndex = (_writeIndex + chunk) % _bufferSizeFrames;
            _availableData += chunk;
            
            framesToWrite -= chunk;
            srcPtr += chunk * _channels;

            lock.unlock();
        }
    }

    int RtAudioOutputBackend::audioCallback(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames,
                                            double streamTime, RtAudioStreamStatus status, void* userData) {
        RtAudioOutputBackend* backend = static_cast<RtAudioOutputBackend*>(userData);
        float* out = static_cast<float*>(outputBuffer);

        std::unique_lock<std::mutex> lock(backend->_mutex);

        if (status) std::cerr << "Stream underflow detected!" << std::endl;

        if (backend->_availableData < nBufferFrames) {
            std::memset(out, 0, nBufferFrames * backend->_channels * sizeof(float));
        } 
        else {
            std::size_t readIdx = backend->_readIndex;
            std::size_t untilWrap = backend->_bufferSizeFrames - readIdx;
            std::size_t firstPart = std::min((std::size_t)nBufferFrames, untilWrap);
            
            // Convert double to float for output
            for (std::size_t i = 0; i < firstPart * backend->_channels; ++i) {
                out[i] = static_cast<float>(backend->_ringBuffer[readIdx * backend->_channels + i]);
            }
            
            if (nBufferFrames > firstPart) {
                for (std::size_t i = 0; i < (nBufferFrames - firstPart) * backend->_channels; ++i) {
                    out[firstPart * backend->_channels + i] = static_cast<float>(backend->_ringBuffer[i]);
                }
            }

            backend->_readIndex = (backend->_readIndex + nBufferFrames) % backend->_bufferSizeFrames;
            backend->_availableData -= nBufferFrames;
        }

        backend->_cvBlockWrite.notify_one();
        return 0;
    }
    
    void RtAudioOutputBackend::clearError() noexcept {
        _lastErrno = 0;
        _lastErrMsg.clear();
    }

    void RtAudioOutputBackend::setError(const char *where, int errNo) noexcept {
        _lastErrno = errNo;
        _lastErrMsg = std::string(where) + " Error Code: " + std::to_string(errNo);
    }

    } // namespace Core
} // namespace Engine
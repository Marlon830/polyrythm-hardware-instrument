#include "engine/core/backend/AlsaOutputBackend.hpp"
#include <vector> 
namespace Engine {
        namespace Core {
        AlsaOutputBackend::AlsaOutputBackend()
            : _pcmHandle(nullptr) {
        }

        AlsaOutputBackend::~AlsaOutputBackend() {
            close();
        }

        bool AlsaOutputBackend::open() {
            // Open the ALSA PCM device
            int err = snd_pcm_open(&_pcmHandle, "default", SND_PCM_STREAM_PLAYBACK, 0);
            if (err < 0) {
                setError("snd_pcm_open", err);
                return false;
            }
            err = snd_pcm_set_params(_pcmHandle,
                                     SND_PCM_FORMAT_FLOAT_LE,
                                     SND_PCM_ACCESS_RW_INTERLEAVED,
                                     _channels,                // channels
                                     _sampleRate,            // sample rate
                                     1,                // allow resampling
                                     500000);          // latency in us
            if (err < 0) {
                // Handle error
                setError("snd_pcm_set_params", err);
                snd_pcm_close(_pcmHandle);
                _pcmHandle = nullptr;
                return false;
            }
            return true;
        }

        void AlsaOutputBackend::close() {
            // Close the ALSA PCM device
            if (!_pcmHandle) {
                return;
            }

            int err = snd_pcm_drop(_pcmHandle);
            if (err < 0 && err != -ENOTTY) {
                setError("snd_pcm_drop", err);
                // fallback
                err = snd_pcm_drain(_pcmHandle);
                if (err < 0) {
                    setError("snd_pcm_drain", err);
                }
            }
            snd_pcm_close(_pcmHandle);
            _pcmHandle = nullptr;
        }

        void AlsaOutputBackend::write(const double* buffer, std::size_t size) {
            // Write audio data to the ALSA PCM device
            std::vector<float> floatBuffer(size);
            for (std::size_t i = 0; i < size; ++i) {
                floatBuffer[i] = static_cast<float>(buffer[i]);
            }

            snd_pcm_sframes_t framesTotal = static_cast<snd_pcm_sframes_t>(size / _channels);
            const float* ptr = floatBuffer.data();

            if (!_pcmHandle || !buffer || size == 0) {
                setError("write", -EINVAL);
                return; // PCM device not open or invalid args
            }

            while (framesTotal > 0) {
                snd_pcm_sframes_t written = snd_pcm_writei(_pcmHandle, ptr, framesTotal);
                if (written < 0) {
                    if (written == -EPIPE) { // XRUN
                        setError("snd_pcm_writei: XRUN", written);
                        int prep = snd_pcm_prepare(_pcmHandle);
                        if (prep < 0) {
                            setError("snd_pcm_prepare after XRUN", prep);
                            return;
                        }
                        continue;
                    } else if (written == -ESTRPIPE) { // suspended
                        setError("snd_pcm_writei: ESTRPIPE", written);
                        int r = snd_pcm_resume(_pcmHandle);
                        if (r == -EAGAIN || r < 0) {
                            r = snd_pcm_prepare(_pcmHandle);
                        }
                        if (r < 0) {
                            setError("snd_pcm_resume/prepare", r);
                            return;
                        }
                        continue;
                    } else {
                        int rec = snd_pcm_recover(_pcmHandle, static_cast<int>(written), 0);
                        if (rec < 0) {
                            setError("snd_pcm_recover", rec);
                            return;
                        }
                        continue;
                    }
                } else if (written == 0) {
                    int prep = snd_pcm_prepare(_pcmHandle);
                    if (prep < 0) {
                        setError("snd_pcm_prepare after 0 write", prep);
                        return;
                    }
                    continue;
                } else {
                    ptr += static_cast<std::size_t>(written * _channels);
                    framesTotal -= written;
                }
            }
        }

        const std::string& AlsaOutputBackend::lastError() const noexcept {
            return _lastErrMsg;
        }

        int AlsaOutputBackend::lastErrorNo() const noexcept {
            return _lastErrno;
        }

        void AlsaOutputBackend::clearError() noexcept {
            _lastErrno = 0;
            _lastErrMsg.clear();
        }

        void AlsaOutputBackend::setError(const char *where, int errNo) noexcept {
            _lastErrno = errNo;
            _lastErrMsg = std::string(where) + ": " + snd_strerror(errNo);
        }

    } // namespace Core
} // namespace Engine
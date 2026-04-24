/**
 * @file WavLoader.hpp
 * @brief WAV file loader utility.
 *
 * @ingroup engine_util
 *
 * Provides functionality to load WAV audio files into memory.
 */

#ifndef WAV_LOADER_HPP
    #define WAV_LOADER_HPP

    #include <vector>
    #include <string>
    #include <cstdint>
    #include <sndfile.h>

namespace Engine {
    namespace Util {
        
        /// @brief Structure containing WAV file data
        struct WavData {
            std::vector<double> samples;            ///< Audio samples (normalized to [-1.0, 1.0])
            uint32_t sampleRate;          ///< Sample rate in Hz
            uint16_t numChannels;         ///< Number of channels (1=mono, 2=stereo)
            uint32_t numFrames;           ///< Total number of frames
            std::string name;             ///< Name of the WAV file (without path)
            
            WavData() : sampleRate(44100), numChannels(1), numFrames(0), name("") {}
        };

        /// @brief WAV file loader class
        class WavLoader {
        public:
            /// @brief Load a WAV file from disk
            /// @param filepath Path to the WAV file
            /// @return WavData structure containing the audio data
            /// @throws std::runtime_error if file cannot be loaded
            static WavData load(const std::string& filepath);

        private:
            /// @brief Read 16-bit PCM samples and convert to float
            static std::vector<float> readPCM16(std::ifstream& file, uint32_t numSamples);
            
            /// @brief Read 24-bit PCM samples and convert to float
            static std::vector<float> readPCM24(std::ifstream& file, uint32_t numSamples);

            /// @brief Read 32-bit float PCM samples
            static std::vector<float> readPCMFloat32(std::ifstream& file, uint32_t numSamples);

            static int32_t readInt24(const unsigned char b[3]);
        };
    } // namespace Util
} // namespace Engine

#endif // WAV_LOADER_HPP
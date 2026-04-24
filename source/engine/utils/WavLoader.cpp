#include "engine/utils/WavLoader.hpp"
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <cstring>

namespace Engine
{
    namespace Util
    {
#include "engine/core/backend/AudioParameters.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <stdexcept>

        WavData WavLoader::load(const std::string &filepath)
        {
            WavData data;
            // Extract filename from filepath
            size_t lastSlash = filepath.find_last_of("/\\");
            data.name = (lastSlash != std::string::npos) ? filepath.substr(lastSlash + 1) : filepath;
            
            SF_INFO sfinfo;
            SNDFILE *snd = sf_open(filepath.c_str(), SFM_READ, &sfinfo);
            if (!snd)
            {
                printf("Impossible d'ouvrir test.wav : %s\n", sf_strerror(NULL));
            }
            else
            {
                printf("Ouvert %s: %d canaux, %lld frames\n", filepath.c_str(), sfinfo.channels, (long long)sfinfo.frames);
                data.sampleRate = static_cast<uint32_t>(sfinfo.samplerate);
                data.numChannels = static_cast<uint16_t>(sfinfo.channels);
                data.numFrames = static_cast<uint32_t>(sfinfo.frames);
                data.samples.resize(data.numFrames * data.numChannels);
                if (NB_CHANNELS == 1 && sfinfo.channels == 2)
                {
                    std::cout << "Converting stereo to mono..." << std::endl;
                    std::vector<double> out(data.numFrames);
                    std::vector<double> temp(data.numFrames * 2);
                    sf_readf_double(snd, temp.data(), data.numFrames);
                    sf_close(snd);

                    for (uint32_t i = 0; i < data.numFrames; i++) {
                        double L = temp[i * 2 + 0];
                        double R = temp[i * 2 + 1];
                        out[i] = 0.5f * (L + R);  // moyenne
                    }

                    data.samples = std::move(out);
                    data.numChannels = 1;
                } else {
                    sf_readf_double(snd, data.samples.data(), data.numFrames);
                    sf_close(snd);
                }
            }
            return data;
        }
            // WavData WavLoader::load(const std::string &filepath)
            // {
            //     WavData data;
            //     std::ifstream file(filepath, std::ios::binary);

            //     if (!file.is_open())
            //     {
            //         throw std::runtime_error("Failed to open WAV file: " + filepath);
            //     }

            //     // Read RIFF header
            //     char riff[4];
            //     file.read(riff, 4);
            //     if (std::strncmp(riff, "RIFF", 4) != 0)
            //     {
            //         throw std::runtime_error("Invalid WAV file: missing RIFF header");
            //     }

            //     // Skip file size
            //     file.seekg(4, std::ios::cur);

            //     // Read WAVE format
            //     char wave[4];
            //     file.read(wave, 4);
            //     if (std::strncmp(wave, "WAVE", 4) != 0)
            //     {
            //         throw std::runtime_error("Invalid WAV file: missing WAVE format");
            //     }

            //     // Find and parse "fmt " chunk
            //     bool foundFmt = false;
            //     uint16_t audioFormat = 0;
            //     uint16_t bitsPerSample = 0;
            //     file.clear();
            //     file.seekg(12, std::ios::beg); // start after RIFF/WAVE header
            //     while (file.good())
            //     {
            //         char chunkId[4];
            //         if (!file.read(chunkId, 4))
            //             break;
            //         uint32_t chunkSize = 0;
            //         if (!file.read(reinterpret_cast<char *>(&chunkSize), 4))
            //             break;

            //         if (std::strncmp(chunkId, "fmt ", 4) == 0)
            //         {
            //             // read fmt chunk
            //             file.read(reinterpret_cast<char *>(&audioFormat), 2);
            //             file.read(reinterpret_cast<char *>(&data.numChannels), 2);
            //             file.read(reinterpret_cast<char *>(&data.sampleRate), 4);
            //             // byteRate and blockAlign
            //             file.seekg(6, std::ios::cur);
            //             file.read(reinterpret_cast<char *>(&bitsPerSample), 2);
            //             // skip any extra fmt bytes
            //             if (chunkSize > 16)
            //             {
            //                 file.seekg(static_cast<std::streamoff>(chunkSize - 16), std::ios::cur);
            //             }
            //             foundFmt = true;
            //             break;
            //         }
            //         else
            //         {
            //             // skip chunk (respect chunk size)
            //             file.seekg(static_cast<std::streamoff>(chunkSize), std::ios::cur);
            //         }
            //     }
            //     if (!foundFmt)
            //     {
            //         throw std::runtime_error("Invalid WAV file: missing fmt chunk");
            //     }

            //     // Validate supported formats
            //     // audioFormat == 1 -> PCM integer, audioFormat == 3 -> IEEE float
            //     if (!(audioFormat == 1 || audioFormat == 3))
            //     {
            //         throw std::runtime_error("Unsupported WAV audio format (only PCM integer and IEEE float supported)");
            //     }
            //     if (audioFormat == 1)
            //     { // PCM integer: accept 16 or 24 bits
            //         if (bitsPerSample != 16 && bitsPerSample != 24)
            //         {
            //             throw std::runtime_error("Unsupported bits per sample for PCM (only 16-bit and 24-bit supported)");
            //         }
            //     }
            //     else
            //     { // float
            //         if (bitsPerSample != 32)
            //         {
            //             throw std::runtime_error("Unsupported bits per sample for float WAV (expect 32-bit float)");
            //         }
            //     }

            //     // Find data chunk
            //     bool foundData = false;
            //     uint32_t dataSize = 0;
            //     file.clear();
            //     file.seekg(12, std::ios::beg);
            //     while (file.good())
            //     {
            //         char chunkId[4];
            //         if (!file.read(chunkId, 4))
            //             break;
            //         uint32_t chunkSize = 0;
            //         if (!file.read(reinterpret_cast<char *>(&chunkSize), 4))
            //             break;
            //         if (std::strncmp(chunkId, "data", 4) == 0)
            //         {
            //             dataSize = chunkSize;
            //             foundData = true;
            //             break;
            //         }
            //         else
            //         {
            //             file.seekg(static_cast<std::streamoff>(chunkSize), std::ios::cur);
            //         }
            //     }
            //     if (!foundData)
            //     {
            //         throw std::runtime_error("Invalid WAV file: missing data chunk");
            //     }

            //     uint32_t bytesPerSample = bitsPerSample / 8;
            //     uint32_t numSamples = dataSize / bytesPerSample;
            //     data.numFrames = numSamples / data.numChannels;

            //     // Move file cursor to start of data chunk (we scanned already, so compute offset)
            //     // Re-scan to locate data chunk start position precisely
            //     file.clear();
            //     file.seekg(12, std::ios::beg);
            //     std::streamoff dataChunkPos = -1;
            //     while (file.good())
            //     {
            //         char chunkId[4];
            //         if (!file.read(chunkId, 4))
            //             break;
            //         uint32_t chunkSize = 0;
            //         if (!file.read(reinterpret_cast<char *>(&chunkSize), 4))
            //             break;
            //         if (std::strncmp(chunkId, "data", 4) == 0)
            //         {
            //             dataChunkPos = file.tellg();
            //             break;
            //         }
            //         else
            //         {
            //             file.seekg(static_cast<std::streamoff>(chunkSize), std::ios::cur);
            //         }
            //     }
            //     if (dataChunkPos == -1)
            //     {
            //         throw std::runtime_error("Unable to locate data chunk start");
            //     }

            //     // file is currently positioned right after the data chunk size field, ready to read samples
            //     // Read PCM data based on format and bit depth
            //     if (audioFormat == 3)
            //     {
            //         std::cout << "Reading 32-bit float PCM samples" << std::endl;
            //         // 32-bit float
            //         data.samples = readPCMFloat32(file, numSamples);
            //     }
            //     else
            //     { // audioFormat == 1
            //         if (bitsPerSample == 16)
            //         {
            //             std::cout << "Reading 16-bit PCM samples" << std::endl;
            //             data.samples = readPCM16(file, numSamples);
            //         }
            //         else
            //         { // 24-bit
            //             std::cout << "Reading 24-bit PCM samples" << std::endl;
            //             data.samples = readPCM24(file, numSamples);
            //         }
            //     }

            //     file.close();

            //     std::cout << "Loaded WAV: " << filepath << std::endl;
            //     std::cout << "  Sample Rate: " << data.sampleRate << " Hz" << std::endl;
            //     std::cout << "  Channels: " << data.numChannels << std::endl;
            //     std::cout << "  Bit Depth: " << bitsPerSample << " bits" << std::endl;
            //     std::cout << "  Frames: " << data.numFrames << std::endl;

            //     return data;
            // }

            std::vector<float> WavLoader::readPCM16(std::ifstream & file, uint32_t numSamples)
            {
                std::vector<float> samples(numSamples);

                for (uint32_t i = 0; i < numSamples; ++i)
                {
                    int16_t sample;
                    file.read(reinterpret_cast<char *>(&sample), 2);
                    // Normalize to [-1.0, 1.0]
                    samples[i] = static_cast<float>(sample) / 32768.0f;
                }

                return samples;
            }

            std::vector<float> WavLoader::readPCM24(std::ifstream & file, uint32_t numSamples)
            {
                std::vector<float> samples(numSamples);

                for (uint32_t i = 0; i < numSamples; ++i)
                {
                    // Read 3 bytes for 24-bit sample
                    uint8_t bytes[3];
                    file.read(reinterpret_cast<char *>(bytes), 3);

                    // Convert 3 bytes to 32-bit signed integer
                    // WAV is little-endian, so: bytes[0] = LSB, bytes[2] = MSB
                    int32_t sample = (bytes[0]) | (bytes[1] << 8) | (bytes[2] << 16);

                    // Sign-extend if negative (check bit 23)
                    if (sample & 0x800000)
                    {
                        sample |= 0xFF000000;
                    }

                    // Normalize to [-1.0, 1.0]
                    // 24-bit range: -8388608 to 8388607
                    samples[i] = static_cast<float>(sample) / 8388608.0f;
                }

                return samples;
            }

            int32_t WavLoader::readInt24(const unsigned char b[3])
            {
                int32_t value = (b[0] | (b[1] << 8) | (b[2] << 16));
                // Sign extend if needed
                if (value & 0x800000)
                    value |= 0xFF000000;
                return value;
            }

            std::vector<float> WavLoader::readPCMFloat32(std::ifstream & file, uint32_t numSamples)
            {
                std::vector<float> samples(numSamples);

                for (uint32_t i = 0; i < numSamples; ++i)
                {
                    float sample;
                    file.read(reinterpret_cast<char *>(&sample), 4);
                    samples[i] = sample; // Already in [-1.0, 1.0]
                }

                return samples;
            }

        } // namespace Util
    } // namespace Engine
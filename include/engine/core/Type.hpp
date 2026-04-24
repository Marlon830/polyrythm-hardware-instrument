/// @file Type.hpp
/// @brief Core type definitions for the TinyMB engine.
/// @ingroup engine_core

namespace Engine {
    namespace Core {
        /// @brief Audio context structure containing buffer and sample rate information.
        /// @ingroup engine_core
        /// @details This structure is passed to modules during processing to provide
        /// necessary audio parameters.
        
        struct AudioContext {
            int bufferSize;
            int sampleRate;
            double frequency;
            bool isNoteOn;
        };
    }
}
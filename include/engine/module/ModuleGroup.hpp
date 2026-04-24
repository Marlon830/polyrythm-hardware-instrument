/// @file ModuleGroup.hpp
/// @brief ModuleGroup module definition.
/// @ingroup engine_module
/// @details Defines the ModuleGroup class which is a composite module
/// that contains an AudioGraph for processing audio data through its sub-modules.

#ifndef MODULEGROUP_HPP
    #define MODULEGROUP_HPP

    #include "engine/module/BaseModule.hpp"
    #include "engine/signal/AudioSignal.hpp"
    #include "engine/signal/EventSignal.hpp"
    #include "engine/AudioGraph/AudioGraph.hpp"
    #include <vector>
    #include <memory>

namespace Engine {
    namespace Module {
        /// @brief ModuleGroup class representing a composite module.
        /// @details The ModuleGroup contains an AudioGraph that manages
        /// multiple sub-modules. It processes audio data by delegating to its
        /// internal AudioGraph.
        class ModuleGroup : public BaseModule {
        public:
            /// @brief Constructs a ModuleGroup.
            ModuleGroup(): BaseModule(), _graph() {}

            /// @brief Destroys the ModuleGroup.
            virtual ~ModuleGroup();

            virtual IModule* clone() const override;

            /// @brief Process audio data for the provided audio context.
            /// @param context The audio context containing buffers and parameters.
            /// @note Overrides BaseModule process method to delegate processing
            /// to the internal AudioGraph.
            void process(Core::AudioContext& context) override;

            /// @brief Get the internal AudioGraph.
            /// @return Reference to the internal AudioGraph.
            AudioGraph& getAudioGraph();
            
            /// @brief Set the internal AudioGraph.
            /// @param graph The AudioGraph to set.
            void setAudioGraph(const AudioGraph& graph);
        protected:
            /// @brief Internal AudioGraph managing sub-modules.
            AudioGraph _graph;
        };
    }
}

#endif // MODULEGROUP_HPP
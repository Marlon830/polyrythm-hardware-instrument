/// @file IModule.hpp
/// @brief Interface for engine modules.
/// @ingroup engine_module
/// @details IModule defines the essential interface that all engine modules
/// must implement to interact with the TinyMB firmware.

#ifndef IMODULE_HPP
    #define IMODULE_HPP

    #include "engine/core/Type.hpp"
    #include "engine/port/InputPort.hpp"
    #include "engine/port/OutputPort.hpp"
    #include "engine/core/IEventEmitter.hpp"
    #include <vector>
    #include <memory>

namespace Engine {
    namespace Module {
        /// @brief Interface class for all engine modules.
        /// @details IModule defines the essential interface that all engine modules
        /// must implement to interact with the TinyMB firmware.
        class IModule {
        public:
            /// @brief Virtual destructor for IModule.
            virtual ~IModule() = default;

            virtual IModule* clone() const = 0;

            /// @brief Process audio data for the provided audio context.
            /// @param context The audio context containing buffers and parameters.
            /// @note Must be implemented by derived classes.
            virtual void process(Core::AudioContext& context) = 0;

            /// @brief Get the input ports for this module.
            /// @return A vector of shared pointers to the input ports.
            /// @note Implemented by BaseModule.
            /// @see BaseModule
            virtual std::vector<std::shared_ptr<Port::InputPort>>& getInputPorts() = 0;
            
            /// @brief Get the output ports for this module.
            /// @return A vector of shared pointers to the output ports.
            /// @note Implemented by BaseModule.
            /// @see BaseModule
            virtual std::vector<std::shared_ptr<Port::OutputPort>>& getOutputPorts() = 0;

            /// @brief Get an input port by its name.
            /// @param name The name of the input port.
            /// @return A shared pointer to the input port, or nullptr if not found.
            virtual std::shared_ptr<Port::InputPort> getInputPortByName(const std::string& name) = 0;

            /// @brief Get an output port by its name.
            /// @param name The name of the output port.
            /// @return A shared pointer to the output port, or nullptr if not found.
            virtual std::shared_ptr<Port::OutputPort> getOutputPortByName(const std::string& name) = 0;

            virtual void setEventEmitter(Core::IEventEmitter* emitter) = 0;

            virtual std::string getName() = 0;

            virtual void setName(const std::string& name) = 0;

            virtual void setType(const std::string& type) = 0;

            virtual std::string getType() const = 0;

            virtual void setId(int id) = 0;
            virtual int getId() const = 0;
        };
    }
}

#endif // IMODULE_HPP
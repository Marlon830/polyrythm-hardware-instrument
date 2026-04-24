/**
 * @file BaseModule.hpp
 * @brief Base class for all engine modules.
 *
 * @ingroup engine_module
 *
 * BaseModule provides common lifecycle methods and helpers used by
 * all concrete modules in TinyMB firmware.
 */
 
#ifndef BASEMODULE_HPP
    #define BASEMODULE_HPP

    #include "engine/module/IModule.hpp"
    #include "engine/port/InputPort.hpp"
    #include "engine/port/OutputPort.hpp"

    #include <vector>
    #include <memory>
    #include <string>


namespace Engine {
    namespace Module {
        /// @brief Base class representing a processing module.
        /// @details Implementations should override process() to handle audio buffers.
        /// @see IModule
        class BaseModule : public IModule {
        public:
            /// @brief Construct a new BaseModule.
            BaseModule();

            BaseModule(std::string name);

            /// @brief Destroy the BaseModule.
            virtual ~BaseModule();

            virtual IModule* clone() const override;

            /// @brief Process audio data for the provided audio context.
            /// @param context The audio context containing buffers and parameters.
            /// @note Default implementation does nothing. Override in derived classes.
            virtual void process(Core::AudioContext& context) override;

            /// @brief Get the input ports for this module.
            /// @return A vector of shared pointers to the input ports.
            std::vector<std::shared_ptr<Port::InputPort>>& getInputPorts() override;

            /// @brief Get the output ports for this module.
            /// @return A vector of shared pointers to the output ports.
            std::vector<std::shared_ptr<Port::OutputPort>>& getOutputPorts() override;

            /// @brief Get an input port by its name.
            /// @param name The name of the input port.
            /// @return A shared pointer to the input port, or nullptr if not found.
            virtual std::shared_ptr<Port::InputPort> getInputPortByName(const std::string& name) override;

            /// @brief Get an output port by its name.
            /// @param name The name of the output port.
            /// @return A shared pointer to the output port, or nullptr if not found.
            virtual std::shared_ptr<Port::OutputPort> getOutputPortByName(const std::string& name) override;


            void setEventEmitter(Core::IEventEmitter* emitter) override;

            /// @brief Get the name of the module.
            /// @return The name of the module as a string.
            std::string getName() override { return _name; }

            /// @brief Set the name of the module.
            /// @param name The new name for the module.
            void setName(const std::string& name) override { _name = std::string(name + "_" + std::to_string(_id)); }

            /// @brief Set the ID of the module.
            /// @param id The ID to set.
            void setId(int id) override { _id = id; }

            void setType(const std::string& type) override { _type = type; }

            std::string getType() const override { return _type; }

            /// @brief Get the ID of the module.
            /// @return The ID of the module.
            int getId() const override { return _id; }
        protected:
            /// @brief Owned input ports for this module.
            /// @details Each element is a shared pointer to an InputPort owned by this module.
            /// Use getInputPorts() to read the list. Modifying the returned vector does not change
            /// ownership inside the module.
            std::vector<std::shared_ptr<Port::InputPort>> _inputPorts;

            /// @brief Owned output ports for this module.
            /// @details Each element is a shared pointer to an OutputPort owned by this module.
            /// Use getOutputPorts() to read the list. Modifying the returned vector does not change
            /// ownership inside the module.
            std::vector<std::shared_ptr<Port::OutputPort>> _outputPorts;

            std::string _name; ///< Name of the module

            std::string _type; ///< Type of the module

            Core::IEventEmitter* _eventEmitter = nullptr; ///< Event emitter for this module
            int _id = -1; ///< ID of the module
        };
    }
}

#endif // BASEMODULE_HPP
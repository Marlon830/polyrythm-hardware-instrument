#include "engine/config/InstrumentBuilder.hpp"

#include <stdexcept>
#include <iostream>

#include "engine/parameter/Param.hpp"
#include "engine/parameter/Parameters.hpp"
#include "engine/module/OscModule.hpp"

namespace Engine {
    namespace Config {

        InstrumentBuilder::InstrumentBuilder(Module::ModuleFactory& factory)
            : moduleFactory(factory) {
        }

std::shared_ptr<Module::IModule> InstrumentBuilder::build(const InstrumentConfig& config) {
    auto instrument = Module::ModuleFactory::instance().create("instrument");
    auto& instrument_graph = std::static_pointer_cast<Module::Instrument>(instrument)->getAudioGraph();

    instrument->setName(config.name);

    // Create and add modules
    for (const auto& moduleConfig : config.modules) {
        auto modulePtr = moduleFactory.create(moduleConfig.type);
        std::cerr << "Creating module of type: " << moduleConfig.type << " with initial name: " << modulePtr->getName() << " and new name: " << moduleConfig.name << std::endl;
        modulePtr->setName(moduleConfig.name);
        if (!modulePtr) {
            throw std::runtime_error("Unknown module type: " + moduleConfig.type);
        }
        applyParams(modulePtr.get(), moduleConfig.params);
        instrument_graph.addModule(modulePtr);
    }
    createConnections(instrument_graph, config.connections);
    return instrument;
}

        void InstrumentBuilder::applyParams(Module::IModule* module, const std::unordered_map<std::string, ParamValue>& params) {
            for (const auto& [paramName, paramValue] : params) {
                // module should herit of Parameters to have getParameterByName
                auto paramBase = dynamic_cast<Module::Parameters*>(module)->getParameterByName(paramName);
                if (!paramBase) {
                    std::cerr << "Warning: Parameter " << paramName << " not found in module " << module->getName() << std::endl;
                    continue;
                }

                std::visit([&paramBase, &paramName, &module](auto&& arg) {
                    using T = std::decay_t<decltype(arg)>;
                    auto param = dynamic_cast<Module::Param<T>*>(paramBase.get());
                    if (param) {
                        param->set(arg);
                    } else {
                        if constexpr (std::is_same_v<T, std::string>) {
                            auto oscModule = dynamic_cast<Module::OscillatorModule*>(module);
                            if (oscModule && paramName == "waveform") {
                                if (arg == "sine") {
                                    dynamic_cast<Module::Param<Module::WaveformType>*>(paramBase.get())->set(Module::WaveformType::SINE);
                                } else if (arg == "square") {
                                    dynamic_cast<Module::Param<Module::WaveformType>*>(paramBase.get())->set(Module::WaveformType::SQUARE);
                                } else if (arg == "triangle") {
                                    dynamic_cast<Module::Param<Module::WaveformType>*>(paramBase.get())->set(Module::WaveformType::TRIANGLE);
                                } else if (arg == "sawtooth") {
                                    dynamic_cast<Module::Param<Module::WaveformType>*>(paramBase.get())->set(Module::WaveformType::SAWTOOTH);
                                } else {
                                    std::cerr << "Warning: Unknown waveform type " << arg << " for parameter " << paramName << " in module " << module->getName() << std::endl;
                                }
                            } else {
                                std::cerr << "Warning: Type mismatch for parameter " << paramName << " in module " << module->getName() << std::endl;
                            }
                        }
                    }
                }, paramValue);
            }
        }

        void InstrumentBuilder::createConnections(AudioGraph& instrument_graph, const std::vector<ConnectionConfig>& connections) {
            // Implementation for creating connections between modules
            for (const auto& connection : connections) {
                auto fromModule = instrument_graph.getModuleByName(connection.from.moduleName);
                auto toModule = instrument_graph.getModuleByName(connection.to.moduleName);
                if (!fromModule || !toModule) {
                    throw std::runtime_error("Invalid module name in connection");
                }

                auto fromPort = fromModule->getOutputPortByName(connection.from.portName);
                auto toPort = toModule->getInputPortByName(connection.to.portName);
                if (!fromPort || !toPort) {
                    throw std::runtime_error("Invalid port name in connection");
                }
                fromPort->connect(toPort);
            }
        }
    } // namespace Config
} // namespace Engine
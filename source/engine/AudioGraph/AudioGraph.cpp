#include "engine/AudioGraph/AudioGraph.hpp"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <stdexcept>
#include <iostream>
#include <map>
#include <vector>
#include <memory>
#include <string>

#include "engine/parameter/Parameters.hpp"
#include "engine/module/Instrument.hpp"
#include "engine/core/backend/AudioParameters.hpp"
#include "common/event/EmitModuleList.hpp"

namespace Engine
{
    AudioGraph::AudioGraph() {
        // Initialize empty audio context with default values
        _audioContext.bufferSize = BUFFER_SIZE;
        _audioContext.sampleRate = SAMPLE_RATE;
        _audioContext.frequency = 440.0f;
    }

    AudioGraph::~AudioGraph() = default;

    AudioGraph* AudioGraph::clone() const
    {
        AudioGraph* newGraph = new AudioGraph();

        for (const auto& module : this->_modules) {
            std::shared_ptr<Module::IModule> clonedModule(module->clone());
            newGraph->addModule(clonedModule);
        }

        for (const auto& connection : this->_connections) {
            auto src = connection->getSource();
            auto dst = connection->getTarget();

            std::shared_ptr<Port::OutputPort> newSrc = nullptr;
            std::shared_ptr<Port::InputPort> newDst = nullptr;

            for (const auto& module : newGraph->getModules()) {
                auto outPorts = module->getOutputPorts();
                for (const auto& outPort : outPorts) {
                    if (outPort->getName() == src->getName()) {
                        newSrc = outPort;
                        break;
                    }
                }

                auto inPorts = module->getInputPorts();
                for (const auto& inPort : inPorts) {
                    if (inPort->getName() == dst->getName()) {
                        newDst = inPort;
                        break;
                    }
                }
            }

            if (newSrc && newDst) {
                newGraph->connect(newSrc, newDst);
            } else {
                std::cerr << "AudioGraph::clone: Could not find matching ports for connection from " << src->getName() << " to " << dst->getName() << std::endl;
            }
        }

        return newGraph;
    }

    void AudioGraph::addModule(std::shared_ptr<Module::IModule> module)
    {
        if (!module) {
            throw std::invalid_argument("Cannot add null module to AudioGraph");
        }

        // Check if module already exists
        auto it = std::find(this->_modules.begin(), this->_modules.end(), module);
        if (it == this->_modules.end()) {
            this->_modules.push_back(module);
            this->updateExecutionOrder();
        }
    }

    void AudioGraph::removeModule(int moduleId)
    {
            auto it = std::find_if(this->_modules.begin(), this->_modules.end(),
                                [&moduleId](const std::shared_ptr<Module::IModule>& mod) {
                                    return mod->getName().find(std::to_string(moduleId)) != std::string::npos;
                                });
            if (it == this->_modules.end()) {
                throw std::invalid_argument("Module ID " + std::to_string(moduleId) + " not found in AudioGraph");
            }

        // Remove associated connections
        auto moduleToRemove = *it;
        this->_connections.erase(
        std::remove_if(this->_connections.begin(), this->_connections.end(),
                        [&moduleToRemove](const std::shared_ptr<Port::Connection>& conn) {
                            auto src = conn->getSource();
                            auto dst = conn->getTarget();

                            bool isConnectedToModule = false;

                            // Check if source port belongs to the module
                            for (const auto& outPort : moduleToRemove->getOutputPorts()) {
                                if (outPort == src) {
                                    isConnectedToModule = true;
                                    break;
                                }
                            }

                            // Check if target port belongs to the module
                            if (!isConnectedToModule) {
                                for (const auto& inPort : moduleToRemove->getInputPorts()) {
                                    if (inPort == dst) {
                                        isConnectedToModule = true;
                                        break;
                                    }
                                }
                            }

                            return isConnectedToModule;
                        }),
            this->_connections.end());

        // Remove the module
        this->_modules.erase(it);

        // Update execution order
        this->updateExecutionOrder();
    }

    void AudioGraph::connect(std::shared_ptr<Port::OutputPort> src, std::shared_ptr<Port::InputPort> dst)
    {
        if (!src || !dst) {
            throw std::invalid_argument("Cannot connect null ports");
        }

        // Create connection
        auto connection = std::make_shared<Port::Connection>(src, dst);
        this->_connections.push_back(connection);

        // Establish actual port connection
        src->connect(dst);

        // Update execution order
        this->updateExecutionOrder();
    }

    void AudioGraph::disconnect(std::shared_ptr<Port::OutputPort> src, std::shared_ptr<Port::InputPort> dst)
    {
        if (!src || !dst) {
            throw std::invalid_argument("Cannot disconnect null ports");
        }

        auto it = std::find_if(this->_connections.begin(), this->_connections.end(),
                               [&src, &dst](const std::shared_ptr<Port::Connection>& conn) {
                                   return conn->getSource() == src && conn->getTarget() == dst;
                               });

        if (it == this->_connections.end()) {
            throw std::invalid_argument("Connection does not exist between the specified ports");
        }

        // Remove connection
        this->_connections.erase(it);

        src->disconnect(dst);

        // Update execution order
        this->updateExecutionOrder();
    }

    void AudioGraph::updateExecutionOrder()
    {
        try {
            this->_executionOrder = topologicalSort();
        } catch (const std::runtime_error& e) {
            std::cerr << "AudioGraph::updateExecutionOrder error: " << e.what() << std::endl;
            this->_executionOrder.clear();
        }
    }

    std::vector<std::shared_ptr<Module::IModule>> AudioGraph::topologicalSort()
    {
        // Build adjacency list and in-degree map
        std::unordered_map<Module::IModule*, std::vector<Module::IModule*>> adjList;
        std::unordered_map<Module::IModule*, int> inDegree;

        // Initialize in-degree for all modules
        for (const auto& module : this->_modules) {
            inDegree[module.get()] = 0;
            adjList[module.get()] = {};
        }

        // Build graph from connections
        for (const auto& connection : this->_connections) {
            auto src = connection->getSource();
            auto dst = connection->getTarget();

            // Find source and target modules
            Module::IModule* srcModule = nullptr;
            Module::IModule* dstModule = nullptr;

            for (const auto& module : this->_modules) {
                const auto& outputs = module->getOutputPorts();
                const auto& inputs = module->getInputPorts();

                if (std::find(outputs.begin(), outputs.end(), src) != outputs.end()) {
                    srcModule = module.get();
                }
                if (std::find(inputs.begin(), inputs.end(), dst) != inputs.end()) {
                    dstModule = module.get();
                }
            }

            if (srcModule && dstModule && srcModule != dstModule) {
                adjList[srcModule].push_back(dstModule);
                inDegree[dstModule]++;
            }
        }

        // Kahn's algorithm for topological sort
        std::vector<std::shared_ptr<Module::IModule>> sortedModules;
        std::vector<Module::IModule*> queue;

        // Add all modules with in-degree 0
        for (const auto& module : this->_modules) {
            if (inDegree[module.get()] == 0) {
                queue.push_back(module.get());
            }
        }

        while (!queue.empty()) {
            Module::IModule* current = queue.back();
            queue.pop_back();

            // Find shared_ptr for current module
            for (const auto& module : this->_modules) {
                if (module.get() == current) {
                    sortedModules.push_back(module);
                    break;
                }
            }

            // Decrease in-degree for neighbors
            for (Module::IModule* neighbor : adjList[current]) {
                inDegree[neighbor]--;
                if (inDegree[neighbor] == 0) {
                    queue.push_back(neighbor);
                }
            }
        }

        // Check for cycles
        if (sortedModules.size() != this->_modules.size()) {
            throw std::runtime_error("Cycle detected in AudioGraph - cannot process");
        }

        return sortedModules;
    }

    void AudioGraph::process()
    {
        // Process each module in cached topological order
        for (auto& module : this->_executionOrder) {
            module->process(_audioContext);
        }
    }

    std::map<std::string, Common::ModuleInfo> AudioGraph::getScopedParams(unsigned int scope) const
    {
        std::map<std::string, Common::ModuleInfo> allParams;
        std::vector<std::shared_ptr<Module::IModule>> modulesToCheck;

        if (scope != 0) {
            for (const auto& module : this->_modules) {
                if (static_cast<Module::Instrument*>(module.get()) && module->getName().find(std::to_string(scope)) != std::string::npos) {
                    std::vector<std::shared_ptr<Module::IModule>> instrumentModules = static_cast<Module::Instrument*>(module.get())->getAudioGraph().getModules();
                    modulesToCheck.insert(modulesToCheck.end(), instrumentModules.begin(), instrumentModules.end());
                }
            }
        } else if (scope == 0 || modulesToCheck.empty()) {
            modulesToCheck = this->_modules;
        }

        for (const auto& module : modulesToCheck) {
            allParams[module->getName()];
            allParams[module->getName()].type = module->getType();
            allParams[module->getName()].id   = module->getId();
            allParams[module->getName()].name = module->getName();
            if (auto* paramOwner = dynamic_cast<Module::Parameters*>(module.get())) {
                for (auto& p : paramOwner->getParameters()) {
                    if (p) {
                        allParams[module->getName()].parameters.push_back(p);
                    }
                }
            }
        }
        return allParams;
    }

    std::shared_ptr<Module::IModule> AudioGraph::getModuleByName(const std::string& name) const
    {
        for (const auto& module : this->_modules) {
            if (module->getName().find(name) != std::string::npos) {
                return module;
            }
        }
        return nullptr;
    }

    std::map<std::string, Common::ModuleInfo> AudioGraph::getAllParams() const
    {
        std::map<std::string, Common::ModuleInfo> allParams;

        for (const auto& module : this->_modules) {
            if (auto* paramOwner = dynamic_cast<Module::Parameters*>(module.get())) {
                allParams[module->getName()].type = module->getType();
                allParams[module->getName()].id   = module->getId();
                allParams[module->getName()].name = module->getName();
                for (auto& p : paramOwner->getParameters()) {
                    if (p) {
                        allParams[module->getName()].parameters.push_back(p);
                    }
                }
            } else if (auto* instrument = dynamic_cast<Module::Instrument*>(module.get())) {
                // Special handling for Instrument modules
                auto instrumentParams = instrument->getAllParams();
                for (const auto& kv : instrumentParams) {
                    const std::string& name = kv.first;
                    const auto& params = kv.second;
                    allParams[name].name = name;
                    allParams[name].type = module->getType();
                    allParams[name].id   = module->getId();
                    allParams[name].parameters.insert(allParams[name].parameters.end(),
                                                     params.parameters.begin(),
                                                     params.parameters.end());
                }
            }
        }
        return allParams;
    }

    std::vector<std::shared_ptr<Engine::ConnectionInfo>> AudioGraph::getConnectionInfos(unsigned int scope) const
    {
        std::vector<std::shared_ptr<Engine::ConnectionInfo>> infos;
        const Engine::AudioGraph* graphToUse = this;

        // Resolve scoped graph if needed (instrument_<scope>)
        if (scope != 0) {
            const std::string wanted = "instrument_" + std::to_string(scope);
            for (const auto& module : this->_modules) {
                if (module->getName().find(wanted) != std::string::npos) {
                    auto instr = std::dynamic_pointer_cast<Engine::Module::Instrument>(module);
                    if (instr) {
                        graphToUse = &instr->getAudioGraph();
                    }
                    break;
                }
            }
        }

        // Helper to stringify type
        auto typeToString = [](Engine::Signal::SignalType t) -> std::string {
            switch (t) {
                case Engine::Signal::SignalType::AUDIO:  return "Audio";
                case Engine::Signal::SignalType::EVENT:  return "Event";
                case Engine::Signal::SignalType::CONTROL:return "Control";
                default:                                  return "Unknown";
            }
        };

        // Build quick lookup maps for connections per port
        std::unordered_map<Engine::Port::OutputPort*, std::vector<std::shared_ptr<Engine::Port::Connection>>> outsMap;
        std::unordered_map<Engine::Port::InputPort*,  std::vector<std::shared_ptr<Engine::Port::Connection>>> insMap;

        for (const auto& c : graphToUse->getConnections()) {
            outsMap[c->getSource().get()].push_back(c);
            insMap[c->getTarget().get()].push_back(c);
        }

        // Emit:
        // - For each output port: one entry per connection; if none, one "none" entry.
        // - For each input port with no incoming connections: one "none" entry.
        for (const auto& mod : graphToUse->_modules) {
            // Outputs
            for (const auto& outPort : mod->getOutputPorts()) {
                auto it = outsMap.find(outPort.get());
                if (it == outsMap.end() || it->second.empty()) {
                    // Unconnected output: dest = none
                    auto info = std::make_shared<Engine::ConnectionInfo>();
                    info->sourceModule  = mod->getName();
                    info->sourcePort    = outPort->getName();
                    info->destModule    = "none";
                    info->destPort      = "";
                    info->connectionType= typeToString(outPort->getType());
                    infos.push_back(std::move(info));
                } else {
                    // Real connections for this output
                    for (const auto& conn : it->second) {
                        // Find destination module name and port name
                        std::string dstModuleName;
                        std::string dstPortName;
                        auto dst = conn->getTarget();

                        for (const auto& m2 : graphToUse->_modules) {
                            for (const auto& ip : m2->getInputPorts()) {
                                if (ip == dst) {
                                    dstModuleName = m2->getName();
                                    dstPortName   = ip->getName();
                                    break;
                                }
                            }
                            if (!dstModuleName.empty()) break;
                        }

                        auto info = std::make_shared<Engine::ConnectionInfo>();
                        info->sourceModule   = mod->getName();
                        info->sourcePort     = outPort->getName();
                        info->destModule     = dstModuleName;
                        info->destPort       = dstPortName;
                        info->connectionType = typeToString(outPort->getType());
                        infos.push_back(std::move(info));
                    }
                }
            }

            // Inputs that have no incoming connection: source = none
            for (const auto& inPort : mod->getInputPorts()) {
                auto itIn = insMap.find(inPort.get());
                if (itIn == insMap.end() || itIn->second.empty()) {
                    auto info = std::make_shared<Engine::ConnectionInfo>();
                    info->sourceModule   = "none";
                    info->sourcePort     = "";
                    info->destModule     = mod->getName();
                    info->destPort       = inPort->getName();
                    info->connectionType = typeToString(inPort->getType());
                    infos.push_back(std::move(info));
                }
            }
        }

        return infos;
    }
}

#ifndef INSTRUMENT_BUILDER_HPP
    #define INSTRUMENT_BUILDER_HPP
    #include "engine/config/InstrumentConfig.hpp"
    #include "engine/module/Instrument.hpp"
    #include "engine/module/IModule.hpp"
    #include "engine/module/ModuleFactory.hpp"
    #include <memory>

namespace Engine {
    namespace Config {

        class InstrumentBuilder {
        public:
            explicit InstrumentBuilder(Module::ModuleFactory& factory);
            
            // Construit un instrument depuis une configuration
            std::shared_ptr<Module::IModule> build(const InstrumentConfig& config);
            
        private:
            void applyParams(Module::IModule* module, const std::unordered_map<std::string, ParamValue>& params);
            void createConnections(AudioGraph& instrument_graph, const std::vector<ConnectionConfig>& connections);

            Module::ModuleFactory& moduleFactory;
        };
    } // namespace Config
} // namespace Engine

#endif // INSTRUMENT_BUILDER_HPP
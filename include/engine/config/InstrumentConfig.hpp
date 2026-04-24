#ifndef INSTRUMENT_CONFIG_HPP
    #define INSTRUMENT_CONFIG_HPP

    #include <string>
    #include <vector>
    #include <memory>
    #include <unordered_map>
    #include <variant>
    #include <optional>

namespace Engine {
    namespace Config {

        using ParamValue = std::variant<int, double, std::string, bool>;

        struct ModuleConfig {
            std::string type;
            std::string name;
            std::unordered_map<std::string, ParamValue> params;
        };
        
        struct PortReference {
            std::string moduleName;
            std::string portName;
            
            static std::optional<PortReference> parse(const std::string& str);
        };

        struct ConnectionConfig {
            PortReference from;  // format: "module_name:port_name"
            PortReference to;    // format: "module_name:port_name"
        };

        struct InstrumentConfig {
            std::string name;
            std::vector<ModuleConfig> modules;
            std::vector<ConnectionConfig> connections;
        };


    } // namespace Config
} // namespace Engine
#endif // INSTRUMENT_CONFIG_HPP
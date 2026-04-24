#ifndef INSTRUMENT_PARSER_HPP
    #define INSTRUMENT_PARSER_HPP

    #include "engine/config/InstrumentConfig.hpp"
    #include <nlohmann/json.hpp>
    #include <filesystem>
    #include <optional>

namespace Engine {
    namespace Config {

        class ParseError : public std::runtime_error {
        public:
            explicit ParseError(const std::string& message)
                : std::runtime_error("ParseError: " + message) {}
        };

        class InstrumentParser {
        public:
            static InstrumentConfig parseFromFile(const std::filesystem::path& filePath);
            static InstrumentConfig parseFromString(const std::string& jsonString);

            static std::string serialize(const InstrumentConfig& config);

            static ModuleConfig parseModule(const nlohmann::json& j);
            static ConnectionConfig parseConnection(const nlohmann::json& j);
            static ParamValue parseParamValue(const nlohmann::json& j);

        };

        void from_json(const nlohmann::json& j, ModuleConfig& m);
        void from_json(const nlohmann::json& j, ConnectionConfig& c);
        void from_json(const nlohmann::json& j, InstrumentConfig& i);

        void to_json(nlohmann::json& j, const ModuleConfig& m);
        void to_json(nlohmann::json& j, const ConnectionConfig& c);
        void to_json(nlohmann::json& j, const InstrumentConfig& i);
    } // namespace Config
} // namespace Engine
#endif // INSTRUMENT_PARSER_HPP
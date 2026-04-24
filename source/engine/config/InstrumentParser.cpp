#include "engine/config/InstrumentParser.hpp"
#include <fstream>
#include <sstream>

namespace Engine {
    namespace Config {

        InstrumentConfig InstrumentParser::parseFromFile(const std::filesystem::path& filePath) {
            std::ifstream file(filePath);
            if (!file.is_open()) {
                throw ParseError("Could not open file: " + filePath.string());
            }

            std::stringstream buffer;
            buffer << file.rdbuf();
            return parseFromString(buffer.str());
        }

        InstrumentConfig InstrumentParser::parseFromString(const std::string& jsonString) {
            nlohmann::json j = nlohmann::json::parse(jsonString);
            InstrumentConfig config;
            from_json(j, config);
            return config;
        }

        std::string InstrumentParser::serialize(const InstrumentConfig& config) {
            nlohmann::json j;
            to_json(j, config);
            return j.dump(4); // Pretty print with 4 spaces indentation
        }

        // JSON -> ModuleConfig
        ModuleConfig InstrumentParser::parseModule(const nlohmann::json& j) {
            ModuleConfig m;
            from_json(j, m);
            return m;
        }

        // JSON -> ConnectionConfig
        ConnectionConfig InstrumentParser::parseConnection(const nlohmann::json& j) {
            ConnectionConfig c;
            from_json(j, c);
            return c;
        }

        // JSON -> ParamValue
        ParamValue InstrumentParser::parseParamValue(const nlohmann::json& j) {
            if (j.is_number_integer()) {
                return j.get<int>();
            } else if (j.is_number_float()) {
                return j.get<double>();
            } else if (j.is_string()) {
                return j.get<std::string>();
            } else if (j.is_boolean()) {
                return j.get<bool>();
            } else {
                throw ParseError("Unsupported ParamValue type");
            }
        }

        //parse helper
        std::optional<PortReference> PortReference::parse(const std::string& str) {
            auto delimiterPos = str.find(':');
            if (delimiterPos == std::string::npos) {
                return std::nullopt;
            }
            PortReference ref;
            ref.moduleName = str.substr(0, delimiterPos);
            ref.portName = str.substr(delimiterPos + 1);
            return ref;
        }

        void from_json(const nlohmann::json& j, ConnectionConfig& c) {
            c.from = PortReference::parse(j.at("from").get<std::string>()).value();
            c.to = PortReference::parse(j.at("to").get<std::string>()).value();
        }

        void from_json(const nlohmann::json& j, ModuleConfig& m) {
            m.type = j.at("type").get<std::string>();
            m.name = j.at("name").get<std::string>();
            if (j.find("params") == j.end() || !j.at("params").is_object()) {
                return;
            }
            for (auto& [key, value] : j.at("params").items()) {
                m.params[key] = InstrumentParser::parseParamValue(value);
            }
        }

        void from_json(const nlohmann::json& j, InstrumentConfig& i) {
            i.name = j.at("name").get<std::string>();
            for (const auto& moduleJson : j.at("modules")) {
                ModuleConfig module;
                from_json(moduleJson, module);
                i.modules.push_back(module);
            }
            for (const auto& connectionJson : j.at("connections")) {
                ConnectionConfig connection;
                from_json(connectionJson, connection);
                i.connections.push_back(connection);
            }
        }

        void to_json(nlohmann::json& j, const ModuleConfig& m) {
            j = nlohmann::json{
                {"type", m.type},
                {"name", m.name},
                {"params", nlohmann::json::object()}
            };
            for (const auto& [key, value] : m.params) {
                std::visit([&j, &key](auto&& arg) {
                    j["params"][key] = arg;
                }, value);
            }
        }

        void to_json(nlohmann::json& j, const ConnectionConfig& c) {
            j = nlohmann::json{
                {"from", c.from.moduleName + ":" + c.from.portName},
                {"to", c.to.moduleName + ":" + c.to.portName}
            };
        }

        void to_json(nlohmann::json& j, const InstrumentConfig& i) {
            j = nlohmann::json{
                {"name", i.name},
                {"modules", nlohmann::json::array()},
                {"connections", nlohmann::json::array()}
            };
            for (const auto& module : i.modules) {
                nlohmann::json moduleJson;
                to_json(moduleJson, module);
                j["modules"].push_back(moduleJson);
            }
            for (const auto& connection : i.connections) {
                nlohmann::json connectionJson;
                to_json(connectionJson, connection);
                j["connections"].push_back(connectionJson);
            }
        }
    } // namespace Config
} // namespace Engine
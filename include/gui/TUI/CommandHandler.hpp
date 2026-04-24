/*
 @file CommandHandler.hpp
 @brief Declaration of the CommandHandler class for parsing and dispatching commands entered by the user.
 @ingroup gui
*/

#ifndef GUI_TUI_COMMANDHANDLER_HPP
    #define GUI_TUI_COMMANDHANDLER_HPP

    #include <string>
    #include <vector>
    #include <sstream>
    #include <functional>
    #include <algorithm>
    #include <map>

namespace GUI {

    /// @brief Structure representing a parsed command.
    struct ParsedCommand {
        std::string name;
        std::vector<std::string> args;
    };

    /// @brief Parses a command line string into a ParsedCommand structure.
    /// @param line The command line string to parse.
    /// @return A ParsedCommand containing the command name and arguments.
    static ParsedCommand ParseCommandLine(const std::string& line) {
        ParsedCommand out;
        auto trimmed = line;
        auto trim = [](std::string& s) {
            const auto first = s.find_first_not_of(' ');
            const auto last  = s.find_last_not_of(' ');
            if (first == std::string::npos) { s.clear(); return; }
            s = s.substr(first, last - first + 1);
        };
        trim(trimmed);

        auto space_pos = trimmed.find(' ');
        if (space_pos == std::string::npos) {
            out.name = trimmed;
            return out;
        }
        out.name = trimmed.substr(0, space_pos);
        std::istringstream args_stream(trimmed.substr(space_pos + 1));
        std::string arg;
        while (std::getline(args_stream, arg, ' ')) {
            if (!arg.empty()) {
                // remove CR/LF
                arg.erase(std::remove(arg.begin(), arg.end(), '\n'), arg.end());
                arg.erase(std::remove(arg.begin(), arg.end(), '\r'), arg.end());
                out.args.push_back(arg);
            }
        }
        return out;
    }
    /// @brief Class for handling command registration and dispatching.
    class CommandHandler {
        public:
            /// @brief Type definition for command handler functions.
            using Handler = std::function<void(const ParsedCommand&)>;

            /// @brief Registers a command handler for a specific command name.
            /// @param name The name of the command.
            /// @param handler The handler function to associate with the command.
            void registerCommand(const std::string& name, Handler handler);

            /// @brief Dispatches a command to the appropriate handler.
            /// @param cmd The parsed command to dispatch.
            /// @return True if the command was handled, false otherwise.
            bool dispatchCommand(const ParsedCommand& cmd) const;

            /// @brief Retrieves a list of all registered command names.
            /// @return A vector of registered command names.
            std::vector<std::string> getRegisteredCommands() const {
                std::vector<std::string> commands;
                for (const auto& [name, _] : _handlers) {
                    commands.push_back(name);
                }
                return commands;
            }

        private:
            std::map<std::string, Handler> _handlers;
    };
};
#endif // GUI_TUI_COMMANDHANDLER_HPP
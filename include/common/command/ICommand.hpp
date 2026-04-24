#ifndef COMMON_COMMAND_ICOMMAND_HPP
#define COMMON_COMMAND_ICOMMAND_HPP

namespace Engine {
    class AudioEngine; // used for forward declaration
}

namespace Common {
    /// @brief Interface for command pattern implementations.
    /// @details This interface defines the contract for command objects
    /// that encapsulate actions and their execution logic.
    class ICommand {
    public:
        /// @brief Virtual destructor for ICommand.
        virtual ~ICommand() = default;

        /// @brief Executes the command action.
        virtual void apply(Engine::AudioEngine& engine) = 0;
    };
} // namespace Common
#endif // COMMON_COMMAND_ICOMMAND_HPP
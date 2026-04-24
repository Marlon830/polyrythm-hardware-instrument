#ifndef ENGINE_PARAMETER_PARAMBASE_HPP
    #define ENGINE_PARAMETER_PARAMBASE_HPP

#include <string>

namespace Engine {
    namespace Module {
        /// @brief Abstract base class for parameters.
        /// @details This class serves as a base for different types of parameters
        /// used in modules. It provides a common interface for parameter handling.
        class ParamBase {
        public:
            /// @brief Virtual destructor for ParamBase.
            virtual ~ParamBase() = default;

            /// @brief Gets the name of the parameter.
            /// @return The name of the parameter as a string.
            virtual std::string getName() const = 0;
        };
    }
}
#endif // ENGINE_PARAMETER_PARAMBASE_HPP
#ifndef ENGINE_PARAMETER_PARAMETERS_HPP
#define ENGINE_PARAMETER_PARAMETERS_HPP

#include "engine/parameter/ParamBase.hpp"
#include <vector>
#include <memory>

namespace Engine {
    namespace Module {
        /// @brief Type alias for a vector of shared pointers to ParamBase.
        class Parameters {
        public:
            /// @brief get all parameters.
            /// @return vector of shared pointers to ParamBase.
            std::vector<std::shared_ptr<ParamBase>>& getParameters() {
                return _parameters;
            }

            /// @brief get parameters by name.
            /// @param name The name of the parameter to search for.
            virtual std::shared_ptr<ParamBase> getParameterByName(std::string name) {
                for (const auto& param : _parameters) {
                    if (param->getName() == name) {
                        return param;
                    }
                }
                static std::shared_ptr<ParamBase> nullParam = nullptr;
                return nullParam;
            }

        protected:
            std::vector<std::shared_ptr<ParamBase>> _parameters;
        };
    }
}
#endif // ENGINE_PARAMETER_PARAMETERS_HPP
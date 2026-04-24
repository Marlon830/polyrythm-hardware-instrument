#ifndef COMMON_EVENT_MODULE_EVENT_HPP
#define COMMON_EVENT_MODULE_EVENT_HPP

#include "string"
#include <vector>
#include <memory>
#include "engine/module/IModule.hpp"
#include "engine/parameter/ParamBase.hpp"
namespace Common {
    struct ModuleInfo {
        std::string name;
        std::string type;
        int id;
        std::vector<std::shared_ptr<Engine::Module::ParamBase>> parameters;
    };
} // namespace Common
#endif // COMMON_EVENT_MODULE_EVENT_HPP
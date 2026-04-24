#include "engine/module/ModuleFactory.hpp"

#include <stdexcept>
#include <utility>
#include <cctype>
#include <iostream>
#include <algorithm>



// note : this file implements a factory pattern,
// in the future we might want to replace it with a builder pattern for composite modules
namespace Engine::Module {

    namespace {
        std::unordered_map<std::string, Creator>& registryStorage() {
            static std::unordered_map<std::string, Creator> map;
            return map;
        }
    }

    ModuleFactory& ModuleFactory::instance() {
        static ModuleFactory factory;
        return factory;
    }

    std::unordered_map<std::string, Creator>& ModuleFactory::registry() {
        return registryStorage();
    }

    bool ModuleFactory::registerModule(std::string key, Creator fn) {
        return registryStorage().emplace(std::move(key), std::move(fn)).second;
    }

    void ModuleFactory::unregisterModule(std::string_view key) {
        registryStorage().erase(std::string{key});
    }

    std::shared_ptr<IModule> ModuleFactory::create(std::string_view name) const {
        return this->create(name, nullptr);
    }

    std::shared_ptr<IModule> ModuleFactory::create(std::string_view name, Core::IEventEmitter* emitter) const {
        if (auto it = registryStorage().find(std::string{name}); it != registryStorage().end()) {
            // method is const but we need to increment the instance id; cast away const to update the counter
            const_cast<ModuleFactory*>(this)->_id_counter++;
            auto module = it->second(std::string{name} + "_" + std::to_string(const_cast<ModuleFactory*>(this)->_id_counter));
            module->setType(std::string{name});
            module->setId(const_cast<ModuleFactory*>(this)->_id_counter);
            // If the module supports event emission, set the emitter
            module->setEventEmitter(emitter);
            return module;
        }
        throw std::runtime_error("ModuleFactory::create - unknown module: " + std::string{name});
    }

    AutoRegister::AutoRegister(std::string key, Creator fn) {
        ModuleFactory::registerModule(std::move(key), std::move(fn));
    }

} // namespace Engine::Module
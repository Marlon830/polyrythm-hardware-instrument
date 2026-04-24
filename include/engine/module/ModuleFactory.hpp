#ifndef MODULE_FACTORY_HPP
    #define MODULE_FACTORY_HPP

    #include <memory>
    #include <functional>
    #include <unordered_map>
    #include "engine/module/IModule.hpp"
    #include "engine/core/IEventEmitter.hpp"
    #include <string>

namespace Engine::Module {

    using Creator = std::function<std::shared_ptr<IModule>(std::string)>;

    class ModuleFactory final {
    public:
        static ModuleFactory& instance();

        std::shared_ptr<IModule> create(std::string_view name) const;
        std::shared_ptr<IModule> create(std::string_view name, Core::IEventEmitter* emitter) const;

        static bool registerModule(std::string key, Creator fn);
        static void unregisterModule(std::string_view key);
        static std::unordered_map<std::string, Creator>& registry();

    private:
        ModuleFactory() : _id_counter(0) {}
        int _id_counter;
    };

    struct AutoRegister final {
        AutoRegister(std::string key, Creator fn);
    };
} // namespace Engine::Module

#endif // MODULE_FACTORY_HPP
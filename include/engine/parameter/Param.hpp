
#ifndef ENGINE_CORE_PARAM_HPP
#define ENGINE_CORE_PARAM_HPP

#include <string>

#include "engine/parameter/ParamBase.hpp"

namespace Engine {
    namespace Module {
        
template <typename T>
class Param : public ParamBase {
public:
    Param(const std::string& name, const T& defaultValue) : _name(name), _value(defaultValue) {}
    ~Param() = default;
    
    T get() const {
        return _value;
    }

    void set(const T& newValue) {
        _value = newValue;
    }

    std::string getName() const override {
        return _name;
    }

private:
    std::string _name;
    T _value;
};

    } // namespace Module
} // namespace Engine
#endif // ENGINE_CORE_PARAM_HPP
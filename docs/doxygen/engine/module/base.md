/**
@file base.md
@brief Base documentation for Module module
@defgroup module Module
@ingroup engine_modules
@{
*/

# Module

## Overview

The Module module provides the building blocks for creating audio processing units within the TinyMB firmware. Modules can represent various audio components such as oscillators, filters, effects, and more. Each module is designed to process various types of signals, including audio, control, and MIDI signals.

## Key Components

- **Module Base Class**: The foundational class for all modules, providing common functionality and interfaces for signal processing.

- **Oscillator Module**: A specific implementation of a module that generates audio signals based on different waveforms (sine, square, sawtooth, triangle).

## Usage

The first rule of the module is the independent operation of each module. Each module processes its input signals and produces output signals without relying on the internal state of other modules.

Modules can be connected together to form complex audio processing chains. Each module can have multiple input and output ports, allowing for flexible signal routing.

## Creating Custom Modules

To create a custom module, inherit from the Module base class and implement the required methods for signal processing. Define the input and output ports as needed, and implement the processing logic in the `process` method.

```cpp
class CustomModule : public Engine::Module::Module {
public:
    CustomModule() {
        // Initialize input and output ports
    }

    void process(Core::AudioContext& context) override {
        // Implement signal processing logic
    }
};
```

## Testing Modules
Unit tests are essential to ensure the correct functionality of modules. The provided test framework allows for the creation of tests that validate the behavior of modules under various conditions.

```cpp
TEST(CustomModuleTest, ProcessSignal) {
    Module::CustomModule module;
    Core::AudioContext context{bufferSize, sampleRate};

    // Set up input signals and expected output
    module.process(context);

    // Validate output signals
}
```

/** @} */

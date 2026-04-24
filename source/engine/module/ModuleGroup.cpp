#include "engine/module/ModuleGroup.hpp"

namespace Engine {
    namespace Module {
        ModuleGroup::~ModuleGroup() = default;

        IModule* ModuleGroup::clone() const {
            return new ModuleGroup(*this);
        }

        void ModuleGroup::process(Core::AudioContext& context) {
            _graph.process();
        }

        AudioGraph& ModuleGroup::getAudioGraph() {
            return _graph;
        }

        void ModuleGroup::setAudioGraph(const AudioGraph& graph) {
            _graph = graph;
        }
    }
}
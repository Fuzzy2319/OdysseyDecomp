#include "Library/Event/EventFlowNodeActorBaseMovementEnd.h"

#include "Library/Event/EventFlowFunction.h"

namespace al {
EventFlowNodeActorBaseMovementEnd::EventFlowNodeActorBaseMovementEnd(const char* name)
    : EventFlowNode(name) {}

void EventFlowNodeActorBaseMovementEnd::init(const EventFlowNodeInitInfo& info) {
    initEventFlowNode(this, info);
}

void EventFlowNodeActorBaseMovementEnd::start() {
    EventFlowNode::start();
    stopEventMovement(this);
    end();
}
}  // namespace al

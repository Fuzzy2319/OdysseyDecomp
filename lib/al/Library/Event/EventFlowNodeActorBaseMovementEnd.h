#pragma once

#include "Library/Event/EventFlowNode.h"

namespace al {
class EventFlowNodeActorBaseMovementEnd : public EventFlowNode {
public:
    EventFlowNodeActorBaseMovementEnd(const char* name);

    void init(const EventFlowNodeInitInfo& info) override;
    void start() override;
};

static_assert(sizeof(EventFlowNodeActorBaseMovementEnd) == 0x68);
}  // namespace al

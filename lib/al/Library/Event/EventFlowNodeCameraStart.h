#pragma once

#include "Library/Event/EventFlowNode.h"

namespace al {
class EventFlowNodeCameraStart : public EventFlowNode {
public:
    EventFlowNodeCameraStart(const char* name);

    void init(const EventFlowNodeInitInfo& info) override;
    void start() override;
    void control() override;

private:
    const char* mCameraName = nullptr;
    s32 mInterpoleStep = -1;
    bool mIsFirstStep = false;
};

static_assert(sizeof(EventFlowNodeCameraStart) == 0x78);
}  // namespace al

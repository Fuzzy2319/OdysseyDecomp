#pragma once

#include "Library/Event/EventFlowNode.h"

namespace al {
class EventFlowNodeAnimCameraStart : public EventFlowNode {
public:
    EventFlowNodeAnimCameraStart(const char* name);

    void init(const EventFlowNodeInitInfo& info) override;
    void start() override;
    void control() override;

private:
    const char* mCameraName = nullptr;
    const char* mAnimName = nullptr;
    s32 mInterpoleStep = -1;
    bool mIsOneTime = true;
    bool _7d = false;
};

static_assert(sizeof(EventFlowNodeAnimCameraStart) == 0x80);
}  // namespace al

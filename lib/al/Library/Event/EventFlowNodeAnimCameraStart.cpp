#include "Library/Event/EventFlowNodeAnimCameraStart.h"

#include "Library/Event/EventFlowFunction.h"
#include "Library/Event/EventFlowUtil.h"

namespace al {
EventFlowNodeAnimCameraStart::EventFlowNodeAnimCameraStart(const char* name)
    : EventFlowNode(name) {}

void EventFlowNodeAnimCameraStart::init(const EventFlowNodeInitInfo& info) {
    initEventFlowNode(this, info);
    mCameraName = getParamIterKeyString(info, "CameraName");
    mAnimName = getParamIterKeyString(info, "AnimName");
    mIsOneTime = getParamIterKeyBool(info, "IsOneTime");
    tryGetParamIterKeyInt(&mInterpoleStep, info, "InterpoleStep");
    registerEventCamera(this, mCameraName);
}

void EventFlowNodeAnimCameraStart::start() {
    EventFlowNode::start();
    startEventAnimCamera(this, mCameraName, mAnimName, mInterpoleStep);

    if (mIsOneTime) {
        end();

        return;
    }

    _7d = true;
}

void EventFlowNodeAnimCameraStart::control() {
    if (isPlayingEventAnimCamera(this, mCameraName))
        return;

    end();
}
}  // namespace al

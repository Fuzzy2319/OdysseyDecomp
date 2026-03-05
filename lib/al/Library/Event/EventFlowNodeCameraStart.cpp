#include "Library/Event/EventFlowNodeCameraStart.h"

#include "Library/Event/EventFlowFunction.h"
#include "Library/Event/EventFlowUtil.h"

namespace al {
EventFlowNodeCameraStart::EventFlowNodeCameraStart(const char* name) : EventFlowNode(name) {}

void EventFlowNodeCameraStart::init(const EventFlowNodeInitInfo& info) {
    initEventFlowNode(this, info);
    mCameraName = getParamIterKeyString(info, "CameraName");
    tryGetParamIterKeyInt(&mInterpoleStep, info, "InterpoleStep");
    registerEventCamera(this, mCameraName);
}

void EventFlowNodeCameraStart::start() {
    EventFlowNode::start();
    startEventCamera(this, mCameraName, mInterpoleStep);
    mIsFirstStep = true;
}

void EventFlowNodeCameraStart::control() {
    if (mIsFirstStep) {
        mIsFirstStep = false;

        return;
    }

    if (isEndInterpoleEventCamera(this, mCameraName))
        end();
}
}  // namespace al

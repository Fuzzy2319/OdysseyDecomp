#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

namespace al {
class LiveActor;

class JointRippleGenerator {
public:
    JointRippleGenerator(const LiveActor* parentActor);
    void reset();
    void updateAndGenerate();

    void set(const sead::Vector3f& offset, const char* joint, f32 param_2c, f32 param_30,
             f32 rateAmplitude, f32 maxDistance) {
        mOffset.set(offset);
        mJoint = joint;
        _2c = param_2c;
        _30 = param_30;
        mRateAmplitude = rateAmplitude;
        mMaxDistance = maxDistance;
    }

private:
    const LiveActor* mParent = nullptr;
    sead::Vector3f mLastJointOffset = sead::Vector3f::zero;
    const char* mJoint = nullptr;
    sead::Vector3f mOffset = sead::Vector3f::zero;
    f32 _2c = 0.0f;
    f32 _30 = 0.0f;
    f32 mRateAmplitude = 1.0f;
    f32 mMaxDistance = 1.0f;
};

}  // namespace al

#pragma once

#include <math/seadMatrix.h>
#include <math/seadVector.h>

#include "Library/Camera/ActorCameraTarget.h"

namespace al {
class CameraPoser;
struct CameraStartInfo;
class LiveActor;

class ActorMatrixCameraTarget : public ActorCameraTarget {
public:
    ActorMatrixCameraTarget(const LiveActor*, const sead::Matrix34f*);

    void calcTrans(sead::Vector3f*) const override;
    void calcSide(sead::Vector3f*) const override;
    void calcUp(sead::Vector3f*) const override;
    void calcFront(sead::Vector3f*) const override;
    void calcVelocity(sead::Vector3f*) const override;

private:
    const sead::Matrix34f* _28;
    f32* _30;
};

static_assert(sizeof(ActorMatrixCameraTarget) == 0x38);

}  // namespace al

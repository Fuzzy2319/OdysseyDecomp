#pragma once

#include <math/seadVector.h>

#include "Library/Nerve/NerveStateBase.h"

namespace al {
struct ActorInitInfo;
}

class EnemyCap;

class EnemyStateReset : public al::ActorStateBase {
public:
    EnemyStateReset(al::LiveActor* actor, const al::ActorInitInfo& info, EnemyCap* cap);

    void appear() override;
    void kill() override;

    void exeWait();

    const sead::Vector3f& getPos() const { return mPos; }

    void setPos(const sead::Vector3f& value) { mPos = value; }

    const sead::Vector3f& getRot() const { return mRot; }

    void setRot(const sead::Vector3f& value) { mRot = value; }

    bool isRevive() const { return mIsRevive; }

private:
    sead::Vector3f mPos = sead::Vector3f::zero;
    sead::Vector3f mRot = sead::Vector3f::zero;
    f32 mValidDistance = 4000.0f;
    bool mIsRevive = true;
    bool mIsInvalidateSensors = false;
    EnemyCap* mEnemyCap = nullptr;
};

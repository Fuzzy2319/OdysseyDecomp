#pragma once

#include <math/seadMatrix.h>

#include "Library/Nerve/NerveStateBase.h"

namespace al {
class ActorMatrixCameraTarget;
class LiveActor;
}  // namespace al

class ActorStateSandGeyser;
class HackerStateNormalJump;
class HackerStateNormalMove;
class IUsePlayerHack;
class KuriboHack;
class PlayerHackStartShaderCtrl;

class KuriboStateHack : public al::HostStateBase<KuriboHack> {
public:
    KuriboStateHack(KuriboHack* host);

    void calcAnim();
    void startHack(al::HitSensor*, al::HitSensor*, al::LiveActor*, bool);
    bool receiveMsg(const al::SensorMsg* message, al::HitSensor* other, al::HitSensor* self);

    bool isSandGeyser() const;
    bool isDemoHackStart() const;

    IUsePlayerHack* get_20() const { return _20; }

    bool get_c0() const { return _c0; }

    void set_e4(bool value) { _e4 = value; }

private:
    IUsePlayerHack* _20;
    HackerStateNormalMove* mHackerStateNormalMove;
    HackerStateNormalMove* mHackerStateNormalMoveDash;
    HackerStateNormalMove* mHackerStateNormalMoveSink;
    HackerStateNormalJump* mHackerStateNormalJump;
    HackerStateNormalJump* mHackerStateNormalJumpHigh;
    HackerStateNormalJump* mHackerStateNormalJumpTurn;
    HackerStateNormalJump* mHackerStateNormalJumpBurn;
    ActorStateSandGeyser* mActorStateSandGeyser;
    al::ActorMatrixCameraTarget* mActorMatrixCameraTarget;
    sead::Matrix34f _70;
    f32 _a0;
    unsigned char padding[0x1c];
    bool _c0;
    f32 _c4;
    s32 _c8;
    f32 _cc;
    al::LiveActor* _d0;
    u32 _d8;
    bool _dc;
    s32 _e0;
    bool _e4;
    PlayerHackStartShaderCtrl* mPlayerHackStartShaderCtrl;
    sead::Vector3f _f0;
};

static_assert(sizeof(KuriboStateHack) == 0x100);

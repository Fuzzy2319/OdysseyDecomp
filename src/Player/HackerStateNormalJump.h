#pragma once

#include <math/seadVector.h>

#include "Library/Nerve/NerveStateBase.h"

class IUsePlayerHack;
class IUsePlayerCollision;
class PlayerActionTurnControl;

class HackerStateNormalJump : public al::ActorStateBase {
public:
    HackerStateNormalJump(al::LiveActor*, IUsePlayerHack**, const char*, const char*);

    void set_38(const sead::Vector3f& value) { _38.set(value); }

    void set_48(sead::Vector2f value) { _48.set(value); }

private:
    IUsePlayerHack** _20;
    IUsePlayerCollision* _28;
    PlayerActionTurnControl* _30;
    sead::Vector3f _38;
    s32 _44;
    sead::Vector2f _48;
    f32 _50;
    f32 _54;
    f32 _58;
    s32 _5c;
    bool _60;
    bool _61;
    bool _62;
    sead::Vector3f _64;
    sead::Vector3f _70;
    f32 _7c;
    bool _80;
    const char* _88;
    const char* _90;
};

static_assert(sizeof(HackerStateNormalJump) == 0x98);

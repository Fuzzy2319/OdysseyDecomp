#pragma once

#include "Library/Nerve/NerveStateBase.h"

class ActorStateSandGeyser : public al::ActorStateBase {
public:
    ActorStateSandGeyser(al::LiveActor* actor);

    void appear() override;
    void kill() override;

    bool receiveMsgSandGeyser(const al::SensorMsg* message, const al::HitSensor* other);

    void tryEndPadRumble();
    void tryStartPadRumble();

private:
    const al::HitSensor* _20 = nullptr;
    f32 _28 = 0.0f;
    f32 _2c = 0.0f;
    bool _30 = false;
};

static_assert(sizeof(ActorStateSandGeyser) == 0x38);

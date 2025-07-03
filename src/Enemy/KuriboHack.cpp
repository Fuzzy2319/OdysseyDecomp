#include "Enemy/KuriboHack.h"

#include "Library/Collision/KCollisionServer.h"
#include "Library/Joint/JointSpringControllerHolder.h"
#include "Library/Nerve/NerveSetupUtil.h"

#include "Enemy/EnemyStateReset.h"
#include "Enemy/EnemyStateSwoon.h"
#include "Enemy/EnemyStateWander.h"
#include "Player/CapTargetInfo.h"
#include "Player/CollisionShapeKeeper.h"
#include "Player/PlayerCeilingCheck.h"
#include "Player/PlayerPushReceiver.h"

// TODO: create these headers.

// #include "Enemy/DisregardReceiver.h"
// #include "Enemy/EnemyStateBlowDown.h"
// #include "Enemy/KuriboStateHack.h"
// #include "Util/ActorStateSandGeyser.h"
// #include "Player/CollisionMultiShape.h"

namespace {
NERVE_IMPL(KuriboHack, Wait)
NERVE_IMPL(KuriboHack, Turn)
NERVE_IMPL(KuriboHack, Find)
NERVE_IMPL(KuriboHack, Chase)
NERVE_IMPL(KuriboHack, Stop)
NERVE_IMPL(KuriboHack, Attack)
NERVE_IMPL(KuriboHack, PressDown)
NERVE_IMPL(KuriboHack, BlowDown)
NERVE_IMPL(KuriboHack, DamageCap)
NERVE_IMPL(KuriboHack, Fall)
NERVE_IMPL(KuriboHack, Land)
NERVE_IMPL(KuriboHack, Sink)
NERVE_IMPL(KuriboHack, Slide)
NERVE_IMPL(KuriboHack, Reset)
NERVE_IMPL(KuriboHack, SandGeyser)
NERVE_IMPL(KuriboHack, WaitHack)
NERVE_IMPL(KuriboHack, TowerHackEnd)
NERVE_IMPL(KuriboHack, Hack)
NERVE_IMPL(KuriboHack, RideOn)
NERVE_IMPL(KuriboHack, Drown)
NERVE_IMPL(KuriboHack, EatBind)

NERVES_MAKE_NOSTRUCT(KuriboHack, Wait, Turn, Find, Chase, Stop, Attack, PressDown, BlowDown,
                     DamageCap, Fall, Land, Sink, Slide, Reset, SandGeyser, WaitHack, TowerHackEnd,
                     Hack, RideOn, Drown, EatBind)
}  // namespace

KuriboHack::KuriboHack(const char* name) : LiveActor(name) {}

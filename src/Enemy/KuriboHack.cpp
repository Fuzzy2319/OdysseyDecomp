#include "Enemy/KuriboHack.h"

#include "Library/Base/StringUtil.h"
#include "Library/Collision/Collider.h"
#include "Library/Collision/KCollisionServer.h"
#include "Library/Effect/EffectSystemInfo.h"
#include "Library/Item/ItemUtil.h"
#include "Library/Joint/JointControllerKeeper.h"
#include "Library/Joint/JointSpringControllerHolder.h"
#include "Library/LiveActor/ActorActionFunction.h"
#include "Library/LiveActor/ActorAnimFunction.h"
#include "Library/LiveActor/ActorClippingFunction.h"
#include "Library/LiveActor/ActorCollisionFunction.h"
#include "Library/LiveActor/ActorInitUtil.h"
#include "Library/LiveActor/ActorModelFunction.h"
#include "Library/LiveActor/ActorPoseUtil.h"
#include "Library/LiveActor/ActorResourceFunction.h"
#include "Library/LiveActor/ActorSensorUtil.h"
#include "Library/Math/MathUtil.h"
#include "Library/Movement/EnemyStateBlowDown.h"
#include "Library/Nature/WaterSurfaceFinder.h"
#include "Library/Nerve/NerveSetupUtil.h"
#include "Library/Nerve/NerveUtil.h"
#include "Library/Placement/PlacementFunction.h"
#include "Library/Thread/FunctorV0M.h"
#include "Library/Yaml/ByamlIter.h"
#include "Library/Yaml/ByamlUtil.h"

#include "Enemy/DisregardReceiver.h"
#include "Enemy/EnemyCap.h"
#include "Enemy/EnemyStateReset.h"
#include "Enemy/EnemyStateSwoon.h"
#include "Enemy/EnemyStateWander.h"
#include "Enemy/KuriboStateHack.h"
#include "Player/CapTargetInfo.h"
#include "Player/CollisionMultiShape.h"
#include "Player/CollisionShapeKeeper.h"
#include "Player/PlayerCeilingCheck.h"
#include "Player/PlayerPushReceiver.h"
#include "Scene/SceneEventNotifier.h"
#include "Util/ActorStateSandGeyser.h"
#include "Util/Hack.h"

namespace {
NERVE_IMPL(KuriboHack, Wait)
NERVE_IMPL(KuriboHack, Wander)
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
                     Hack, RideOn, Drown, EatBind, Wander)
}  // namespace

static al::EnemyStateBlowDownParam gEnemyStateBlowDownParam = {"BlowDown", 18.0f, 35.0f, 2.0f,
                                                               0.97f,      120,   true};

// TODO: Move this
f32 KuriboHack::getRideOnRowSize() {
    return 110.0f;
}

void KuriboHack::resetRideOnPosBottomWithDefaultParam() {
    resetRideOnPosBottom(getRideOnRowSize());
}

KuriboHack::KuriboHack(const char* name) : LiveActor(name) {}

void KuriboHack::init(const al::ActorInitInfo& info) {
    using KuriboHackFunctor = al::FunctorV0M<KuriboHack*, void (KuriboHack::*)()>;

    al::tryGetArg(&mIsGold, info, "IsGold");
    mIsGold = mIsGold || al::isEqualString("クリボーゴールド", getName());
    if (mIsGold)
        al::initActorWithArchiveName(this, info, "KuriboGold", nullptr);
    else
        al::initActorWithArchiveName(this, info, "Kuribo", nullptr);

    mCapTargetInfo = rs::createCapTargetInfo(this, nullptr);
    mCapTargetInfo->set_79(true);
    tryCreateEnemyCap(info);
    if (!mEnemyCap || !mIsEyebrowOff)
        al::startVisAnim(this, "HackOffCapOff");
    else
        al::startVisAnim(this, "HackOffCapOn");

    al::ByamlIter modelResourceYamlIter = {al::getModelResourceYaml(this, "InitHackCap", nullptr)};
    mJointName = al::tryGetByamlKeyStringOrNULL(modelResourceYamlIter, "JointName");
    sead::Vector3f localTrans = {0.0f, 0.0f, 0.0f};
    al::tryGetByamlV3f(&localTrans, modelResourceYamlIter, "LocalTrans");
    sead::Vector3f localRotate = {0.0f, 0.0f, 0.0f};
    al::tryGetByamlV3f(&localRotate, modelResourceYamlIter, "LocalRotate");

    sead::Matrix34f r;
    r.makeR(localRotate * (sead::Mathf::pi() / 180.0f));
    sead::Matrix34f srt;
    srt.makeSRT({1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, localTrans * al::getScaleY(this));
    _218.setMul(r, srt);

    mCapTargetInfo->setPoseMatrix(&_1e8);

    al::initNerve(this, &Wander, 6);
    mEnemyStateSwoon = new EnemyStateSwoon(this, "SwoonStart", "Swoon", "SwoonEnd", false, true);
    mEnemyStateSwoon->initParams(150, nullptr);
    al::initNerveState(this, mEnemyStateSwoon, &WaitHack, "気絶");
    mEnemyStateReset = new EnemyStateReset(this, info, nullptr);
    al::initNerveState(this, mEnemyStateReset, &Reset, "リセット");
    mEnemyStateWander = new EnemyStateWander(this, "Walk");
    al::initNerveState(this, mEnemyStateWander, &Wander, "さんぽ");
    mEnemyStateBlowDown =
        new al::EnemyStateBlowDown(this, &gEnemyStateBlowDownParam, "吹き飛び状態");
    al::initNerveState(this, mEnemyStateBlowDown, &BlowDown, "吹き飛び");
    mActorStateSandGeyser = new ActorStateSandGeyser(this);
    al::initNerveState(this, mActorStateSandGeyser, &SandGeyser, "砂の間欠泉");
    mKuriboStateHack = new KuriboStateHack(this);
    al::initNerveState(this, mKuriboStateHack, &Hack, "憑依");

    al::initJointControllerKeeper(this, 2);
    mJointSpringControllerHolder = new al::JointSpringControllerHolder();
    mJointSpringControllerHolder->init(this, "InitJointSpringCtrl");

    s32 rideOnNum = 0;
    al::tryGetArg(&rideOnNum, info, "RideOnNum");
    _2e8.initOffset(0x300);
    if (rideOnNum > 0) {
        if (mEnemyCap)
            mEnemyCap->kill();

        for (s32 i = 0; i < rideOnNum; i++) {
            KuriboHack* kuriboHack = new KuriboHack("クリボー");
            al::initCreateActorNoPlacementInfo(kuriboHack, info);

            sead::Vector3f offset;
            al::getRandomVector(&offset, 20.0f);
            offset.y = 0.0f;

            kuriboHack->mEnemyStateReset->setPos(al::getTrans(this) + offset);
            kuriboHack->mEnemyStateReset->setRot(mEnemyStateReset->getRot());

            _2e8.pushBack(kuriboHack);
            kuriboHack->_160 = this;
            al::copyPose(kuriboHack, this);
            al::setNerve(kuriboHack, &RideOn);
            al::invalidateClipping(kuriboHack);
            al::stopDitherAnimAutoCtrl(kuriboHack);
            al::startAction(kuriboHack, "WaitTower");
            al::startMtsAnim(kuriboHack, "EyeReset");
        }

        resetRideOnPosBottomWithDefaultParam();
    }

    al::startMtsAnim(this, "EyeReset");
    al::startAction(this, "Wait");
    mEnemyStateSwoon->enableLockOnDelay(true);

    mCollisionPartsFilterSpecialPurpose = new al::CollisionPartsFilterSpecialPurpose("MoveLimit");
    al::setColliderFilterCollisionParts(this, mCollisionPartsFilterSpecialPurpose);

    mClippingRadius = al::getClippingRadius(this);

    mPlayerPushReceiver = new PlayerPushReceiver(this);
    al::invalidateHitSensor(this, "SpecialPush");

    mCollisionMultiShape = new CollisionMultiShape(this, 128);
    mCollisionShapeKeeper = new CollisionShapeKeeper(4, 16, 0);
    f32 colliderRadius = al::getColliderRadius(this);
    sead::Mathf::cos(sead::Mathf::pi() / 4.0f);
    colliderRadius *= 0.7071068f /* sead::Mathf::cos(sead::Mathf::pi() / 4.0f) */;
    sead::Vector3f local_d0 = (colliderRadius * -1.5f) * sead::Vector3f::ey;
    mCollisionShapeKeeper->createShapeArrow(
        "FL", colliderRadius * (sead::Vector3f::ex + sead::Vector3f::ez), local_d0, 0.0f, 0);
    mCollisionShapeKeeper->createShapeArrow(
        "FR", colliderRadius * (sead::Vector3f::ex - sead::Vector3f::ez), local_d0, 0.0f, 0);
    mCollisionShapeKeeper->createShapeArrow(
        "BL", colliderRadius * (-sead::Vector3f::ex + sead::Vector3f::ez), local_d0, 0.0f, 0);
    mCollisionShapeKeeper->createShapeArrow(
        "BR", colliderRadius * (-sead::Vector3f::ex - sead::Vector3f::ez), local_d0, 0.0f, 0);

    al::hideSilhouetteModelIfShow(this);

    mWaterSurfaceFinder = new al::WaterSurfaceFinder(this);
    al::setEffectNamedMtxPtr(this, "WaterSurface", &mWaterSurfaceEffectMtx);
    al::setEffectNamedMtxPtr(this, "SandSurface", &mSandSurfaceEffectMtx);

    mDisregardReceiver = new DisregardReceiver(this, nullptr);

    al::setAppearItemFactor(this, "間接攻撃", al::getHitSensor(this, "Body"));

    mPlayerCeilingCheck = new PlayerCeilingCheck(getCollider()->getCollisionDirector());

    al::invalidateHitSensor(this, "HipDropProbe");
    al::setHitSensorPosPtr(this, "HipDropProbe", &mHipDropProbeSensorPos);
    mHipDropProbeSensor = al::getHitSensor(this, "HipDropProbe");

    rs::listenSnapShotModeOnOff(this, KuriboHackFunctor(this, &KuriboHack::onSnapshotMode),
                                KuriboHackFunctor(this, &KuriboHack::offSnapshotMode));

    if (!al::trySyncStageSwitchAppearAndKill(this))
        makeActorAlive();
}

void KuriboHack::tryCreateEnemyCap(const al::ActorInitInfo& info) {
    const char* capName = nullptr;
    if (!al::tryGetStringArg(&capName, info, "CapName"))
        return;

    if (al::isEqualString(capName, ""))
        return;

    if (!al::isExistModelResourceYaml(this, "EnemyCap", nullptr))
        return;

    al::ByamlIter modelResourceYamlIter = {al::getModelResourceYaml(this, "EnemyCap", nullptr)};
    if (!modelResourceYamlIter.isTypeArray())
        return;

    s32 size = modelResourceYamlIter.getSize();
    for (s32 i = 0; i < size; i++) {
        al::ByamlIter subModelResourceIter;
        if (!modelResourceYamlIter.tryGetIterByIndex(&subModelResourceIter, i))
            continue;

        const char* name = nullptr;
        if (!subModelResourceIter.tryGetStringByKey(&name, "Name"))
            continue;

        if (!al::isEqualString(name, capName))
            continue;

        const char* res = nullptr;
        if (!subModelResourceIter.tryGetStringByKey(&res, "Res"))
            return;

        subModelResourceIter.tryGetBoolByKey(&mIsEyebrowOff, "EyebrowOff");
        mEnemyCap = rs::tryCreateEnemyCap(this, info, res);

        return;
    }
}

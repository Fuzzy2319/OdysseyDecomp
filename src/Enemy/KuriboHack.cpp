#include "Enemy/KuriboHack.h"

#include "Library/Base/StringUtil.h"
#include "Library/Collision/Collider.h"
#include "Library/Collision/CollisionPartsKeeperUtil.h"
#include "Library/Collision/CollisionPartsTriangle.h"
#include "Library/Collision/KCollisionServer.h"
#include "Library/Effect/EffectSystemInfo.h"
#include "Library/Item/ItemUtil.h"
#include "Library/Joint/JointControllerKeeper.h"
#include "Library/Joint/JointSpringControllerHolder.h"
#include "Library/LiveActor/ActorActionFunction.h"
#include "Library/LiveActor/ActorAnimFunction.h"
#include "Library/LiveActor/ActorAreaFunction.h"
#include "Library/LiveActor/ActorClippingFunction.h"
#include "Library/LiveActor/ActorCollisionFunction.h"
#include "Library/LiveActor/ActorFlagFunction.h"
#include "Library/LiveActor/ActorInitUtil.h"
#include "Library/LiveActor/ActorModelFunction.h"
#include "Library/LiveActor/ActorMovementFunction.h"
#include "Library/LiveActor/ActorPoseUtil.h"
#include "Library/LiveActor/ActorResourceFunction.h"
#include "Library/LiveActor/ActorSensorUtil.h"
#include "Library/Math/MathUtil.h"
#include "Library/Movement/EnemyStateBlowDown.h"
#include "Library/Nature/WaterSurfaceFinder.h"
#include "Library/Nerve/NerveSetupUtil.h"
#include "Library/Nerve/NerveUtil.h"
#include "Library/Placement/PlacementFunction.h"
#include "Library/Player/PlayerUtil.h"
#include "Library/Thread/FunctorV0M.h"
#include "Library/Yaml/ByamlIter.h"
#include "Library/Yaml/ByamlUtil.h"

#include "Enemy/DisregardReceiver.h"
#include "Enemy/EnemyCap.h"
#include "Enemy/EnemyStateReset.h"
#include "Enemy/EnemyStateSwoon.h"
#include "Enemy/EnemyStateWander.h"
#include "Enemy/KuriboStateHack.h"
#include "Npc/SphinxQuizRouteKillExecutor.h"
#include "Player/CapTargetInfo.h"
#include "Player/CollidedShapeResult.h"
#include "Player/CollisionMultiShape.h"
#include "Player/CollisionShapeKeeper.h"
#include "Player/PlayerCeilingCheck.h"
#include "Player/PlayerPushReceiver.h"
#include "Scene/SceneEventNotifier.h"
#include "Util/ActorStateSandGeyser.h"
#include "Util/CollisionUtil.h"
#include "Util/DemoUtil.h"
#include "Util/Hack.h"
#include "Util/ItemUtil.h"
#include "Util/PlayerUtil.h"
#include "Util/SensorMsgFunction.h"

namespace {
NERVE_IMPL(KuriboHack, Wander)
NERVE_IMPL(KuriboHack, WaitHack)
NERVE_IMPL(KuriboHack, Reset)
NERVE_IMPL(KuriboHack, BlowDown)
NERVE_IMPL(KuriboHack, SandGeyser)
NERVE_END_IMPL(KuriboHack, Hack)
NERVE_END_IMPL(KuriboHack, RideOn)
NERVE_IMPL(KuriboHack, PressDown)
NERVE_IMPL(KuriboHack, EatBind)
NERVE_IMPL(KuriboHack, Turn)
NERVE_IMPL(KuriboHack, Find)
NERVE_IMPL(KuriboHack, Chase)
NERVE_IMPL(KuriboHack, Stop)
NERVE_IMPL(KuriboHack, Attack)
NERVE_IMPL(KuriboHack, Wait)
NERVE_IMPL(KuriboHack, Land)
NERVE_IMPL(KuriboHack, TowerHackEnd)
NERVE_IMPL(KuriboHack, Sink)
NERVE_IMPL(KuriboHack, Fall)
NERVE_IMPL(KuriboHack, Slide)
NERVE_IMPL(KuriboHack, DamageCap)
NERVE_IMPL(KuriboHack, Drown)

NERVES_MAKE_STRUCT(KuriboHack, Wander, WaitHack, Reset, BlowDown, SandGeyser, Hack, RideOn,
                   PressDown, EatBind, Turn, Find, Chase, Stop, Attack, Wait, Land, TowerHackEnd,
                   Sink, Fall, Slide, DamageCap, Drown)
}  // namespace

const sead::Vector3f gDitherAnimClippingJudgeLocalOffset = {0.0f, 70.0f, 0.0f};

const al::EnemyStateBlowDownParam gEnemyStateBlowDownParam = {"BlowDown", 18.0f, 35.0f, 2.0f,
                                                              0.97f,      120,   true};

// TODO: find a proper name
inline f32 test(f32 a) {
    f32 b;
    if (a > 90.0f)
        b = 180.0f;
    else if (a < -90.0f)
        b = -180.0f;
    else
        return a;

    return b - a;
}

inline void updateColliderOffsetYAll(sead::OffsetList<KuriboHack>* towerList, KuriboHack* kuribo,
                                     f32 offsetY) {
    al::setColliderOffsetY(kuribo, offsetY);
    for (auto kuriboHack = towerList->begin(); kuriboHack != towerList->end(); ++kuriboHack)
        al::setColliderOffsetY(kuriboHack, offsetY);
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

    al::initNerve(this, &NrvKuriboHack.Wander, 6);
    mEnemyStateSwoon = new EnemyStateSwoon(this, "SwoonStart", "Swoon", "SwoonEnd", false, true);
    mEnemyStateSwoon->initParams(150, nullptr);
    al::initNerveState(this, mEnemyStateSwoon, &NrvKuriboHack.WaitHack, "気絶");
    mEnemyStateReset = new EnemyStateReset(this, info, nullptr);
    al::initNerveState(this, mEnemyStateReset, &NrvKuriboHack.Reset, "リセット");
    mEnemyStateWander = new EnemyStateWander(this, "Walk");
    al::initNerveState(this, mEnemyStateWander, &NrvKuriboHack.Wander, "さんぽ");
    mEnemyStateBlowDown =
        new al::EnemyStateBlowDown(this, &gEnemyStateBlowDownParam, "吹き飛び状態");
    al::initNerveState(this, mEnemyStateBlowDown, &NrvKuriboHack.BlowDown, "吹き飛び");
    mActorStateSandGeyser = new ActorStateSandGeyser(this);
    al::initNerveState(this, mActorStateSandGeyser, &NrvKuriboHack.SandGeyser, "砂の間欠泉");
    mKuriboStateHack = new KuriboStateHack(this);
    al::initNerveState(this, mKuriboStateHack, &NrvKuriboHack.Hack, "憑依");

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
            kuriboHack->mHost = this;
            al::copyPose(kuriboHack, this);
            al::setNerve(kuriboHack, &NrvKuriboHack.RideOn);
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

    rs::listenSnapShotModeOnOff(this, KuriboHackFunctor(this, &KuriboHack::onSnapShotMode),
                                KuriboHackFunctor(this, &KuriboHack::offSnapShotMode));

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

void KuriboHack::setNerveRideOnCommon() {
    al::setNerve(this, &NrvKuriboHack.RideOn);
    al::invalidateClipping(this);
    al::stopDitherAnimAutoCtrl(this);
}

void KuriboHack::resetRideOnPosBottomWithDefaultParam() {
    resetRideOnPosBottom(getRideOnRowSize());
}

void KuriboHack::onSnapShotMode() {
    if (!al::isNerve(this, &NrvKuriboHack.RideOn))
        return;

    al::restartDitherAnimAutoCtrl(this);
}

void KuriboHack::offSnapShotMode() {
    if (!al::isNerve(this, &NrvKuriboHack.RideOn))
        return;

    al::stopDitherAnimAutoCtrl(this);
}

void KuriboHack::initAfterPlacement() {
    rs::tryRegisterSphinxQuizRouteKillSensorAfterPlacement(al::getHitSensor(this, "Body"));
}

void KuriboHack::makeActorAlive() {
    LiveActor::makeActorAlive();
    onDynamics();
    resetRideOnPosBottomWithDefaultParam();
    for (auto kuriboHack = _2e8.begin(); kuriboHack != _2e8.end(); ++kuriboHack)
        kuriboHack->makeActorAlive();
}

void KuriboHack::onDynamics() {
    if (!mJointSpringControllerHolder || _148)
        return;

    mJointSpringControllerHolder->resetControlAll();
    mJointSpringControllerHolder->onControlAll();
    _148 = true;
}

void KuriboHack::makeActorDead() {
    for (auto kuriboHack = _2e8.begin(); kuriboHack != _2e8.end(); ++kuriboHack)
        kuriboHack->makeActorDead();
    LiveActor::makeActorDead();
}

void KuriboHack::appear() {
    LiveActor::appear();
    al::validateHitSensors(this);
    al::invalidateHitSensor(this, "SpecialPush");
    al::invalidateHitSensor(this, "HipDropProbe");
    al::setNerve(this, &NrvKuriboHack.Wander);
}

void KuriboHack::kill() {
    if (al::isNerve(this, &NrvKuriboHack.Reset) && mEnemyStateReset->isRevive())
        return;

    LiveActor::kill();
}

void KuriboHack::movement() {
    if (al::isNerve(this, &NrvKuriboHack.RideOn) && _2e0)
        return;

    LiveActor::movement();
    for (auto kuriboHack = _2e8.robustBegin(); kuriboHack != _2e8.robustEnd(); ++kuriboHack) {
        if (!al::isNerve(kuriboHack, &NrvKuriboHack.RideOn))
            continue;

        kuriboHack->_2e0 = false;
        kuriboHack->movement();
        kuriboHack->_2e0 = true;
        if (_2e8.isEmpty())
            break;
    }

    if (!al::isOnGround(this, 0) && !al::isNerve(this, &NrvKuriboHack.SandGeyser) &&
        (!al::isNerve(this, &NrvKuriboHack.Hack) || !mKuriboStateHack->isSandGeyser())) {
        for (auto kuriboHack = _2e8.begin(); kuriboHack != _2e8.end(); ++kuriboHack) {
            if (!al::isOnGround(kuriboHack, 0))
                continue;

            sead::Vector3f colliderPos;
            al::calcColliderPos(&colliderPos, kuriboHack);
            sead::Vector3f diff = al::getCollidedGroundPos(kuriboHack) - colliderPos;
            if (sead::Mathf::abs(diff.y) > sead::Mathf::abs(diff.x) &&
                sead::Mathf::abs(diff.y) > sead::Mathf::abs(diff.z)) {
                if (kuriboHack)
                    detach(kuriboHack);

                break;
            }
        }
    }

    if (!al::isNerve(this, &NrvKuriboHack.Hack))
        return;

    _1cc = _1e8.getTranslation();
}

void KuriboHack::detach(KuriboHack* kuribo) {
    f32 angle = 0.0f;
    for (auto kuriboHack = _2e8.robustBegin(); kuriboHack != _2e8.robustEnd(); ++kuriboHack) {
        if (kuriboHack->mHost) {
            kuriboHack->mHost->_2e8.erase(kuriboHack);
            al::showModelIfHide(kuriboHack);
        }

        kuriboHack->mHost = nullptr;
        kuriboHack->_1ac = 30;

        sead::Vector3f velocity;
        velocity.set({0.0f, 0.0f, 10.0f});
        sead::Quatf quat;
        al::makeQuatYDegree(&quat, angle);
        velocity.setRotated(quat, velocity);
        al::setVelocity(kuriboHack, velocity);

        kuriboHack->_1bc = al::getTrans(this).y;
        al::setNerve(kuriboHack, &NrvKuriboHack.WaitHack);

        if (kuriboHack != kuribo) {
            angle += 50.0f;

            continue;
        }

        sead::Vector3f t;
        t.set(al::getTrans(kuriboHack));
        sead::Quatf q;
        q.set(al::getQuat(kuriboHack));
        al::copyPose(kuriboHack, this);
        al::resetPosition(kuriboHack);
        al::setQuat(this, q);
        al::resetPosition(this, t);

        return;
    }
}

// NON_MATCHING
void KuriboHack::control() {
    _1c8 = false;
    mHackDamageFrame = sead::Mathi::clampMin(mHackDamageFrame - 1, 0);
    _1ac = sead::Mathi::clampMin(_1ac - 1, 0);
    _1b8 = sead::Mathi::clampMin(_1b8 - 1, 0);
    _25c = sead::Mathi::clampMin(_25c - 1, 0);

    if (_25c == 0)
        al::invalidateHitSensor(this, "HipDropProbe");

    al::updateMaterialCodeWet(this, al::isInAreaObj(this, "WetArea"));
    mWaterSurfaceFinder->update(al::getTrans(this), sead::Vector3f::ey, 80.0f);
    if (mWaterSurfaceFinder->isFoundSurface()) {
        al::makeMtxR(&mWaterSurfaceEffectMtx, this);
        sead::Vector3f trans = al::getTrans(this);
        mWaterSurfaceEffectMtx.setTranslation(
            {trans.x, mWaterSurfaceFinder->getSurfacePosition().y, trans.z});
    }
    if (al::isCollidedGroundFloorCode(this, "SandSink")) {
        al::makeMtxR(&mSandSurfaceEffectMtx, this);
        sead::Vector3f trans = al::getTrans(this);
        mSandSurfaceEffectMtx.setTranslation(
            {trans.x, trans.y + (al::getColliderOffsetY(this) - al::getColliderRadius(this)),
             trans.z});
    }
    if (al::isNerve(this, &NrvKuriboHack.RideOn) && mHost)
        al::setModelAlphaMask(this, al::getModelAlphaMask(mHost));
}

bool KuriboHack::checkSandSinkPrecisely() const {
    if (!al::isCollidedGroundFloorCode(this, "SandSink"))
        return false;

    mCollisionShapeKeeper->clearResult();
    sead::Vector3f colliderPos;
    al::calcColliderPos(&colliderPos, this);
    sead::Matrix34f local_70;
    local_70 = *getBaseMtx();
    local_70.setTranslation(colliderPos);
    mCollisionMultiShape->check(mCollisionShapeKeeper, &local_70, 1.0f, al::getVelocity(this),
                                nullptr);

    s32 numCollideResult = mCollisionShapeKeeper->getNumCollideResult();
    for (s32 i = 0; i < numCollideResult; i++)
        if (!al::isFloorCode(
                mCollisionShapeKeeper->getCollidedShapeResult(i)->getArrowHitInfo().hitInfo.data(),
                "SandSink"))
            return false;

    return true;
}

void handleKill(KuriboHack* kuribo, KuriboHack** host, sead::OffsetList<KuriboHack>* towerList,
                bool param_4) {
    if (*host) {
        rs::sendMsgRideOnRelease(al::getHitSensor(*host, "Body"), al::getHitSensor(kuribo, "Body"));
        *host = nullptr;
    } else if (towerList->size() > 0) {
        KuriboHack* k = towerList->front();
        k->transferGroup(towerList);
        al::setNerve(k, &NrvKuriboHack.Wander);
    }

    if (!param_4)
        al::startHitReaction(kuribo, "死亡");

    al::setNerve(kuribo, &NrvKuriboHack.Reset);
}

void endRideAll(sead::OffsetList<KuriboHack>* towerList, al::HitSensor* other) {
    for (auto kuriboHack = towerList->robustBegin(); kuriboHack != towerList->robustEnd();
         ++kuriboHack) {
        kuriboHack->eraseFromHost();
        rs::sendMsgRideOnEnd(al::getHitSensor(kuriboHack, "Body"), other);
    }
}

void handleNeedle(KuriboHack* kuribo, al::EnemyStateBlowDown* state, KuriboHack** host,
                  sead::OffsetList<KuriboHack>* towerList) {
    if (*host) {
        rs::sendMsgRideOnRelease(al::getHitSensor(*host, "Body"), al::getHitSensor(kuribo, "Body"));
        *host = nullptr;
    } else if (towerList->size() > 0) {
        KuriboHack* k = towerList->front();
        k->transferGroup(towerList);
        al::setNerve(k, &NrvKuriboHack.Wander);
    }

    sead::Vector3f dir;
    dir.set(al::getCollidedGroundNormal(kuribo));
    dir.y = 0;
    if (!al::tryNormalizeOrZero(&dir)) {
        al::calcFrontDir(&dir, kuribo);
        dir.x = -dir.x;
        dir.y = -dir.y;
        dir.z = -dir.z;
    }
    state->start(dir);

    al::setNerve(kuribo, &NrvKuriboHack.BlowDown);
}

// void KuriboHack::updateCollider() {}

// NON_MATCHING
void KuriboHack::solveCollisionInHacking(const sead::Vector3f& param_1) {
    if (!al::isNerve(this, &NrvKuriboHack.Hack) &&
        (!al::isNerve(this, &NrvKuriboHack.RideOn) || !al::isNerve(mHost, &NrvKuriboHack.Hack)))
        return;

    f32 colliderOffsetY = al::getColliderOffsetY(this);
    f32 colliderRadius = al::getColliderRadius(this);
    sead::Vector3f local_80 = sead::Vector3f::ey * colliderOffsetY + param_1;
    const sead::Vector3f& trans = al::getTrans(this);
    sead::Vector3f local_90 = sead::Vector3f::ey * colliderOffsetY + trans - local_80;

    if (!al::tryNormalizeOrZero(&local_90))
        return;

    colliderRadius += local_90.length();
    s32 strikeArrowCount = alCollisionUtil::checkStrikeArrow(
        this, local_80, local_90 * colliderRadius, nullptr, nullptr);
    f32 fVar7 = colliderRadius;
    f32 fVar9;
    for (s32 i = 0; i != strikeArrowCount; i++) {
        fVar9 = fVar7;

        al::HitInfo* hitInfo = **alCollisionUtil::getStrikeArrowInfo(this, i);
        if (!hitInfo->triangle.isHostMoved() &&
            al::isWallPolygon(hitInfo->triangle.getNormal(0), al::getGravity(this)))
            fVar9 = sead::Mathf::clampMax(fVar7, hitInfo->_70);

        fVar7 = fVar9;
    }

    if (fVar7 < colliderRadius) {
        sead::Vector3f sub = {local_90.x, 0.0f, local_90.z};
        sub.setScale(sub, colliderRadius - fVar7);
        sead::Vector3f* transPtr = al::getTransPtr(this);
        transPtr->x -= sub.x;
        transPtr->y += 0.0f;
        transPtr->z -= sub.z;
    }
}

void KuriboHack::pushFrom(KuriboHack* kuribo, const sead::Vector3f& up) {
    sead::Vector3f gravity = -up;
    if (!al::tryNormalizeOrZero(&gravity))
        return;

    for (auto kuriboHack = --_2e8.begin(kuribo); kuriboHack != _2e8.end(); --kuriboHack)
        kuriboHack->mPlayerPushReceiver->receiveForceDirect(up);

    mPlayerPushReceiver->receiveForceDirect(up);
    al::limitVelocityDirSign(this, gravity, 0.0f);
}

void KuriboHack::calcAnim() {
    if (al::isNerve(this, &NrvKuriboHack.RideOn) && _2e0)
        return;

    if ((!al::isNerve(this, &NrvKuriboHack.Hack) &&
         (!al::isNerve(this, &NrvKuriboHack.RideOn) || !al::isNerve(mHost, &NrvKuriboHack.Hack))) ||
        al::isHideModel(this))
        al::hideModelIfShow(this);
    else
        al::showModelIfHide(this);

    LiveActor::calcAnim();
    updateCapLockOnMtx();

    if (!mKuriboStateHack->isDead())
        mKuriboStateHack->calcAnim();

    for (auto kuriboHack = _2e8.begin(); kuriboHack != _2e8.end(); ++kuriboHack) {
        kuriboHack->_2e0 = false;
        kuriboHack->calcAnim();
        kuriboHack->_2e0 = true;
    }
}

void KuriboHack::updateCapLockOnMtx() {
    KuriboHack* kuriboHack;
    if (al::isNerve(this, &NrvKuriboHack.Hack) && _2e8.isEmpty()) {
        kuriboHack = this;
    } else {
        if (!al::isNerve(this, &NrvKuriboHack.RideOn))
            return;

        if (!al::isNerve(mHost, &NrvKuriboHack.Hack))
            return;

        if ((mHost->_2e8.isEmpty() ? mHost : mHost->_2e8.back()) != this)
            return;

        kuriboHack = mHost;
    }

    const sead::Matrix34f* mtx = mJointName ? al::getJointMtxPtr(this, mJointName) : getBaseMtx();
    sead::Matrix34f local_60;
    local_60 = *mtx;
    al::normalize(&local_60);
    kuriboHack->_1e8.setMul(local_60, _218);
}

void KuriboHack::startClipped() {
    for (auto kuriboHack = _2e8.begin(); kuriboHack != _2e8.end(); ++kuriboHack)
        kuriboHack->forceStartClipped();

    forceStartClipped();
}

void KuriboHack::forceStartClipped() {
    LiveActor::startClipped();
}

void KuriboHack::endClipped() {
    for (auto kuriboHack = _2e8.begin(); kuriboHack != _2e8.end(); ++kuriboHack)
        kuriboHack->endClipped();  // wtf

    LiveActor::endClipped();
}

void KuriboHack::appearFall() {
    LiveActor::appear();
    al::validateHitSensors(this);
    al::invalidateHitSensor(this, "SpecialPush");
    al::invalidateHitSensor(this, "HipDropProbe");
    al::startVisAnim(this, "HackOffCapOff");
    al::startMtpAnim(this, "Kuribo");
    al::startMtsAnim(this, "EyeReset");
    al::setNerve(this, &NrvKuriboHack.Fall);
}

void KuriboHack::noRevive() {
    mEnemyStateReset->setRevive(false);
}

void startActionAll(KuriboHack* host, const sead::OffsetList<KuriboHack>& towerList,
                    const char* hostActionName, const char* towerActionName = nullptr);

void KuriboHack::exeWait() {
    if (al::isFirstStep(this)) {
        startActionAll(this, _2e8, "Wait");
        setShiftTypeOnGround(1);

        offDynamics();
    }

    syncRideOnPosBottomWithDefaultParam();
    if (al::isOnGround(this, 3))
        al::scaleVelocity(this, 0.7f);
    else
        al::scaleVelocity(this, 0.97f);

    if (al::isCollidedGround(this))
        al::addVelocityToGravityFittedGround(this, 2.0f, 0);
    else
        al::addVelocityToGravity(this, 2.0f);

    if (al::isGreaterEqualStep(this, 40)) {
        if (al::isInAreaObj(al::getPlayerActor(this, 0), "StealthArea") ||
            rs::isPlayerHackJugemFishing(this) || !al::isNearPlayer(this, 1200.0f)) {
            al::setNerve(this, &NrvKuriboHack.Wander);
        } else {
            al::setNerve(this, &NrvKuriboHack.Turn);
        }
    } else if (al::isCollidedGroundFloorCode(this, "Slide")) {
        al::setNerve(this, &NrvKuriboHack.Slide);
    }
}

void startActionAll(KuriboHack* host, const sead::OffsetList<KuriboHack>& towerList,
                    const char* hostActionName, const char* towerActionName) {
    al::startAction(host, hostActionName);
    al::StringTmp<128> towerActionStrTmp;
    if (towerActionName) {
        towerActionStrTmp.format("%s", towerActionName);
    } else {
        towerActionStrTmp.format("%sTower", hostActionName);
        if (!al::isExistAction(host, towerActionStrTmp.cstr()))
            towerActionStrTmp.format("%s", hostActionName);
    }

    const char* towerActionStr = towerActionStrTmp.cstr();
    for (auto kuriboHack = towerList.begin(); kuriboHack != towerList.end(); ++kuriboHack)
        al::startAction(kuriboHack, towerActionStr);
}

void KuriboHack::setShiftTypeOnGround(s32 shiftTypeOnGround) {
    mTypeOnGround = shiftTypeOnGround;
    for (auto kuriboHack = _2e8.begin(); kuriboHack != _2e8.end(); ++kuriboHack)
        kuriboHack->mTypeOnGround = shiftTypeOnGround;
}

void KuriboHack::offDynamics() {
    if (!mJointSpringControllerHolder || !_148)
        return;

    mJointSpringControllerHolder->offControlAll();
    _148 = false;
}

void KuriboHack::syncRideOnPosBottomWithDefaultParam() {
    if (_1a4 != 0)
        _1a4--;
    syncRideOnPosBottom((f32)_1a4 / -10.0f + 1.0f, getRideOnRowSize());
}

void KuriboHack::exeWander() {
    if (al::isFirstStep(this)) {
        if (_2e8.size() > 0)
            mEnemyStateWander->changeWalkAnim("WalkTowerBottom");
        else
            mEnemyStateWander->changeWalkAnim("Walk");
    }

    bool isFirstWanderStep = al::isFirstStep(mEnemyStateWander);
    syncRideOnPosBottomWithDefaultParam();
    al::updateNerveState(this);
    if (al::isFirstStep(this)) {
        onDynamics();
        al::validateClipping(this);
        al::startMtsAnim(this, "EyeReset");
        setShiftTypeOnGround(1);
    }

    if (isFirstWanderStep && _2e8.size() > 0) {
        if (mEnemyStateWander->isWait())
            for (auto kuriboHack = _2e8.begin(); kuriboHack != _2e8.end(); ++kuriboHack)
                al::startAction(kuriboHack, "WaitTower");
        else if (mEnemyStateWander->isWalk())
            for (auto kuriboHack = _2e8.begin(); kuriboHack != _2e8.end(); ++kuriboHack)
                al::startAction(kuriboHack, "WalkTower");
        else
            for (auto kuriboHack = _2e8.begin(); kuriboHack != _2e8.end(); ++kuriboHack)
                al::startAction(kuriboHack, "FallTower");
    }

    if (!mEnemyStateWander->isWait() || !tryShiftDrown()) {
        if (al::isCollidedGroundFloorCode(this, "Slide")) {
            al::setNerve(this, &NrvKuriboHack.Slide);
        } else if (!al::isInAreaObj(al::getPlayerActor(this, 0), "StealthArea") &&
                   !rs::isPlayerHackJugemFishing(this) && al::isNearPlayer(this, 1200.0f) &&
                   al::isGreaterEqualStep(this, 60)) {
            al::setNerve(this, &NrvKuriboHack.Turn);
        }
    }
}

bool KuriboHack::tryShiftDrown() {
    if (!al::isOnGround(this, 0) || !al::isFallNextMove(this, sead::Vector3f::zero, 50.0f, 200.0f))
        return false;

    al::setVelocityZeroH(this);
    al::setNerve(this, &NrvKuriboHack.Drown);

    return true;
}

void KuriboHack::exeTurn() {
    syncRideOnPosBottomWithDefaultParam();

    if (al::isFirstStep(this)) {
        al::startAction(this, "Turn");
        al::invalidateClipping(this);
        onDynamics();
    }

    f32 angleToPlayer = al::calcAngleToTargetH(this, rs::getPlayerPos(this));
    angleToPlayer = al::lerpValue(test(angleToPlayer), -90.0f, 90.0f, 0.0f,
                                  al::getMtsAnimFrameMax(this, "EyeMove"));
    al::startMtsAnimAndSetFrameAndStop(this, "EyeMove", angleToPlayer);

    sead::Vector3f frontDir = sead::Vector3f::zero;
    al::calcFrontDir(&frontDir, this);

    al::LiveActor* player = al::tryFindNearestPlayerActor(this);
    if (player) {
        if (al::isFaceToTargetDegreeH(this, al::getTrans(player), frontDir, 8.0f)) {
            al::setNerve(this, &NrvKuriboHack.Find);

            return;
        }

        al::turnToTarget(this, al::getTrans(player), 8.0f);
    }

    if (al::isOnGround(this, 3))
        al::scaleVelocity(this, 0.7f);
    else
        al::scaleVelocity(this, 0.97f);

    if (al::isCollidedGround(this))
        al::addVelocityToGravityFittedGround(this, 2.0f, 0);
    else
        al::addVelocityToGravity(this, 2.0f);
}

void KuriboHack::exeFind() {
    syncRideOnPosBottomWithDefaultParam();

    if (al::isFirstStep(this)) {
        startActionAll(this, _2e8, "Find");
        al::invalidateClipping(this);
        onDynamics();
        setShiftTypeOnGround(0);
    }

    f32 angleToPlayer = al::calcAngleToTargetH(this, rs::getPlayerPos(this));
    angleToPlayer = al::lerpValue(test(angleToPlayer), -90.0f, 90.0f, 0.0f,
                                  al::getMtsAnimFrameMax(this, "EyeMove"));
    al::startMtsAnimAndSetFrameAndStop(this, "EyeMove", angleToPlayer);

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvKuriboHack.Chase);

        return;
    }

    if (al::isOnGround(this, 3))
        al::scaleVelocity(this, 0.7f);
    else
        al::scaleVelocity(this, 0.97f);

    if (al::isCollidedGround(this))
        al::addVelocityToGravityFittedGround(this, 2.0f, 0);
    else
        al::addVelocityToGravity(this, 2.0f);
}

// NON_MATCHING: vector code gen https://decomp.me/scratch/p7QEp
void KuriboHack::exeChase() {
    syncRideOnPosBottomWithDefaultParam();

    if (al::isFirstStep(this)) {
        if (_2e8.size() >= 1)
            startActionAll(this, _2e8, "RunTowerBottom", "RunTower");
        else
            startActionAll(this, _2e8, "Run");

        _16c = 0;
        al::invalidateClipping(this);
        onDynamics();
    }

    f32 angleToPlayer = al::calcAngleToTargetH(this, rs::getPlayerPos(this));
    angleToPlayer = al::lerpValue(test(angleToPlayer), -90.0f, 90.0f, 0.0f,
                                  al::getMtsAnimFrameMax(this, "EyeMove"));
    al::startMtsAnimAndSetFrameAndStop(this, "EyeMove", angleToPlayer);

    if (al::isCollidedGroundFloorCode(this, "Slide")) {
        al::setNerve(this, &NrvKuriboHack.Slide);

        return;
    }

    if (al::isGreaterEqualStep(this, 400)) {
        al::setNerve(this, &NrvKuriboHack.Stop);

        return;
    }

    if (!al::isOnGround(this, 0)) {
        if (al::isOnGround(this, 3))
            al::scaleVelocity(this, 0.7f);
        else
            al::scaleVelocity(this, 0.97f);

        if (al::isCollidedGround(this))
            al::addVelocityToGravityFittedGround(this, 2.0f, 0);
        else
            al::addVelocityToGravity(this, 2.0f);

        return;
    }

    al::LiveActor* player = al::tryFindNearestPlayerActor(this);
    sead::Vector3f frontDir = sead::Vector3f::zero;
    al::calcQuatFront(&frontDir, this);
    if (player) {
        if (al::isInAreaObj(al::getPlayerActor(this, 0), "StealthArea") ||
            rs::isPlayerHackJugemFishing(this)) {
            al::setNerve(this, &NrvKuriboHack.Stop);

            return;
        }

        if (al::isInSightFan(this, al::getTrans(player), frontDir, 2000.0f, 80.0f, 40.0f)) {
            al::turnToTarget(this, al::getTrans(player), 2.0f);
            _16c = 0;
        } else {
            _16c++;
        }
    } else {
        _16c++;
    }

    if (_16c >= 21) {
        al::setNerve(this, &NrvKuriboHack.Stop);

        return;
    }

    if (al::isOnGround(this, 0)) {
        sead::Quatf quat;
        al::makeQuatRotationRate(&quat, sead::Vector3f::ey, al::getCollidedGroundNormal(this),
                                 1.0f);
        frontDir.setRotated(quat, frontDir);
    }

    if (_16c == 0)
        al::addVelocityToDirection(this, frontDir, 4.0f);
    else
        al::addVelocityToDirection(this, frontDir, 3.3f);

    if (al::isFallNextMove(this, sead::Vector3f::zero, 50.0f, 200.0f)) {
        sead::Vector3f local_50 = al::getCollidedGroundPos(this) - al::getTrans(this);
        local_50.y = 0.0f;

        mPlayerPushReceiver->receiveForceDirect(local_50 * 0.5f);
        if (al::tryNormalizeOrZero(&local_50)) {
            f32 fVar11 = local_50.dot(al::getVelocity(this));
            if (fVar11 < 0.0f)
                *al::getVelocityPtr(this) -= fVar11 * local_50;  // mismatch here
        }
    }

    if (al::isOnGround(this, 3))
        al::scaleVelocity(this, 0.7f);
    else
        al::scaleVelocity(this, 0.97f);

    if (al::isCollidedGround(this))
        al::addVelocityToGravityFittedGround(this, 2.0f, 0);
    else
        al::addVelocityToGravity(this, 2.0f);
}

void KuriboHack::exeStop() {
    syncRideOnPosBottomWithDefaultParam();

    if (al::isFirstStep(this)) {
        al::startMtsAnim(this, "EyeReset");
        startActionAll(this, _2e8, "Miss");
        al::invalidateClipping(this);
        offDynamics();
        setShiftTypeOnGround(2);
    }

    if (al::isActionEnd(this)) {
        if (!tryShiftDrown())
            al::setNerve(this, &NrvKuriboHack.Wander);

        return;
    }

    if (al::isOnGround(this, 3))
        al::scaleVelocity(this, 0.7f);
    else
        al::scaleVelocity(this, 0.97f);

    if (al::isCollidedGround(this))
        al::addVelocityToGravityFittedGround(this, 2.0f, 0);
    else
        al::addVelocityToGravity(this, 2.0f);
}

void KuriboHack::exeAttack() {
    syncRideOnPosBottomWithDefaultParam();

    if (al::isFirstStep(this)) {
        startActionAll(this, _2e8, "Attack", "AttackTower");
        al::invalidateClipping(this);
        onDynamics();
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvKuriboHack.Wait);

        return;
    }

    if (al::isOnGround(this, 3))
        al::scaleVelocity(this, 0.7f);
    else
        al::scaleVelocity(this, 0.97f);

    if (al::isCollidedGround(this))
        al::addVelocityToGravityFittedGround(this, 2.0f, 0);
    else
        al::addVelocityToGravity(this, 2.0f);
}

void KuriboHack::exePressDown() {
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
        al::invalidateHitSensors(this);
        al::setVelocityZero(this);
        offDynamics();
    }

    if (mHipDropActor) {
        sead::Vector3f local_1 = (al::getTrans(mHipDropActor) + _250) - al::getTrans(this);
        sead::Vector3f local_2 = {local_1.x, 0.0f, local_1.z};

        if (local_1.y < 0.0f && local_2.length() < al::getSensorRadius(this, "Body") * 2.0f)
            al::setVelocity(this, {0.0f, local_1.y, 0.0f});
        else
            al::setVelocityZero(this);
    }

    if (al::isActionEnd(this)) {
        al::setVelocityZero(this);
        al::appearItem(this);
        al::startHitReaction(this, "死亡");
        al::setNerve(this, &NrvKuriboHack.Reset);
    }
}

void KuriboHack::exeBlowDown() {
    if (al::updateNerveState(this)) {
        al::startHitReaction(this, "死亡");
        al::invalidateHitSensors(this);
        al::appearItem(this);
        al::setNerve(this, &NrvKuriboHack.Reset);
        onDynamics();
    }
}

void KuriboHack::exeDamageCap() {
    syncRideOnPosBottomWithDefaultParam();

    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
        al::startAction(this, "DamageCap");
        onDynamics();
    }

    if (al::isOnGround(this, 3))
        al::scaleVelocity(this, 0.7f);
    else
        al::scaleVelocity(this, 0.97f);

    if (al::isCollidedGround(this))
        al::addVelocityToGravityFittedGround(this, 2.0f, 0);
    else
        al::addVelocityToGravity(this, 2.0f);

    if (al::isActionEnd(this)) {
        if (!tryShiftDrown()) {
            if (al::isOnGround(this, 0))
                al::setNerve(this, &NrvKuriboHack.Land);
            else
                al::setNerve(this, &NrvKuriboHack.Fall);
        }
    }
}

void KuriboHack::exeFall() {
    syncRideOnPosBottomWithDefaultParam();

    if (al::isFirstStep(this)) {
        startActionAll(this, _2e8, "Fall");
        onDynamics();
    }

    if (al::isOnGround(this, 3))
        al::scaleVelocity(this, 0.7f);
    else
        al::scaleVelocity(this, 0.97f);

    if (al::isCollidedGround(this))
        al::addVelocityToGravityFittedGround(this, 2.0f, 0);
    else
        al::addVelocityToGravity(this, 2.0f);

    if (!tryShiftDrown() && al::isOnGround(this, 0))
        al::setNerve(this, &NrvKuriboHack.Land);
}

void KuriboHack::exeLand() {
    syncRideOnPosBottomWithDefaultParam();

    if (al::isFirstStep(this)) {
        startActionAll(this, _2e8, "Land");
        onDynamics();
    }

    if (al::isOnGround(this, 3))
        al::scaleVelocity(this, 0.7f);
    else
        al::scaleVelocity(this, 0.97f);

    if (al::isCollidedGround(this))
        al::addVelocityToGravityFittedGround(this, 2.0f, 0);
    else
        al::addVelocityToGravity(this, 2.0f);

    if (al::isActionEnd(this) && !tryShiftDrown() && !tryShiftChaseOrWander())
        al::setNerve(this, &NrvKuriboHack.Fall);
}

bool KuriboHack::tryShiftChaseOrWander() {
    if (!al::isOnGround(this, 0))
        return false;

    if (mTypeOnGround == 2 || !al::isNearPlayer(this, 1200.0f))
        al::setNerve(this, &NrvKuriboHack.Wander);
    else if (mTypeOnGround == 0)
        al::setNerve(this, &NrvKuriboHack.Chase);
    else
        al::setNerve(this, &NrvKuriboHack.Turn);

    return true;
}

void KuriboHack::exeSink() {
    syncRideOnPosBottomWithDefaultParam();

    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
        onDynamics();
        startActionAll(this, _2e8, "SandWait");
    }

    al::setVelocityZero(this);
    al::addVelocityToGravity(this, 2.0f);

    if (updateSink()) {
        clearSink();
        endRideAll(&_2e8, al::getHitSensor(this, "Body"));
        handleKill(this, &mHost, &_2e8, true);
        al::setNerve(this, &NrvKuriboHack.Reset);
    }
}

bool KuriboHack::updateSink() {
    f32 radius = al::getColliderRadius(this);
    f32 offsetY = al::getColliderOffsetY(this) + 1.0f;
    if (offsetY > radius * 3.0f)
        return true;

    updateColliderOffsetYAll(&_2e8, this, offsetY);

    return false;
}

void KuriboHack::exeSlide() {
    syncRideOnPosBottomWithDefaultParam();

    if (al::isFirstStep(this))
        startActionAll(this, _2e8, "Slide");

    if (al::isOnGround(this, 3))
        al::scaleVelocity(this, 0.7f);
    else
        al::scaleVelocity(this, 0.97f);

    if (al::isCollidedGround(this))
        al::addVelocityToGravityFittedGround(this, 2.0f, 0);
    else
        al::addVelocityToGravity(this, 2.0f);

    if (!rs::trySlideIfOnFloorSlide(this, 10.0f, 1.0f)) {
        if (tryShiftDrown()) {
        } else if (al::isOnGround(this, 0)) {
            al::setNerve(this, &NrvKuriboHack.Land);
        } else {
            al::setNerve(this, &NrvKuriboHack.Fall);
        }
    }
}

void KuriboHack::exeReset() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Reset");
        al::stopAction(this);
        al::startMtpAnim(this, "Kuribo");
        al::startMtsAnim(this, "EyeReset");
        clearSink();
        al::setAppearItemFactor(this, "間接攻撃", al::getHitSensor(this, "Body"));
        offDynamics();
        onDynamics();
        mRideOnRotationFrame = 0;
        al::invalidateHitSensor(this, "SpecialPush");
        al::invalidateHitSensor(this, "HipDropProbe");
    }

    if (al::updateNerveStateAndNextNerve(this, &NrvKuriboHack.Wander)) {
        al::invalidateHitSensor(this, "SpecialPush");
        al::invalidateHitSensor(this, "HipDropProbe");

        if (mEnemyCap) {
            mEnemyCap->appear();

            if (mIsEyebrowOff)
                al::startVisAnim(this, "HackOffCapOn");
            else
                al::startVisAnim(this, "HackOffCapOff");
        } else {
            al::startVisAnim(this, "HackOffCapOff");
        }

        al::startAction(this, "Wait");
    }
}

void KuriboHack::exeSandGeyser() {
    if (al::isFirstStep(this)) {
        startActionAll(this, _2e8, "SandGeyser");
        al::invalidateClipping(this);
        onDynamics();
        clearSink();
    }

    al::updateNerveStateAndNextNerve(this, &NrvKuriboHack.Wander);
}

void KuriboHack::clearSink() {
    updateColliderOffsetYAll(&_2e8, this, al::getColliderRadius(this));
}

void KuriboHack::exeWaitHack() {
    syncRideOnPosBottomWithDefaultParam();

    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
        al::startMtpAnim(this, "Kuribo");
        offDynamics();
        setShiftTypeOnGround(1);
    }

    if (checkSandSinkPrecisely() && !al::isActionPlaying(this, "SwoonStart") &&
        !al::isActionPlaying(this, "SwoonLand")) {
        al::setNerve(this, &NrvKuriboHack.Sink);
    } else if (!al::updateNerveState(this)) {
        if (al::isOnGround(this, 3))
            al::scaleVelocity(this, 0.7f);
        else
            al::scaleVelocity(this, 0.97f);

        if (al::isCollidedGround(this))
            al::addVelocityToGravityFittedGround(this, 2.0f, 0);
        else
            al::addVelocityToGravity(this, 2.0f);
    } else if (!tryShiftDrown()) {
        al::setNerve(this, &NrvKuriboHack.Wander);
    }
}

void KuriboHack::exeTowerHackEnd() {
    syncRideOnPosBottomWithDefaultParam();

    if (al::isFirstStep(this)) {
        startActionAll(this, _2e8, "HackEnd");
        al::setVelocityZero(this);
        setShiftTypeOnGround(2);
        offDynamics();
    }

    if (al::isOnGround(this, 3))
        al::scaleVelocity(this, 0.7f);
    else
        al::scaleVelocity(this, 0.97f);

    if (al::isCollidedGround(this))
        al::addVelocityToGravityFittedGround(this, 2.0f, 0);
    else
        al::addVelocityToGravity(this, 2.0f);

    if (al::isCollidedGroundFloorCode(this, "Slide"))
        al::setNerve(this, &NrvKuriboHack.Slide);
    else if (al::isActionEnd(this) && !tryShiftDrown() && !tryShiftChaseOrWander())
        al::setNerve(this, &NrvKuriboHack.Fall);
}

void KuriboHack::exeHack() {
    if (al::isFirstStep(this)) {
        al::setColliderFilterCollisionParts(this, nullptr);
        notifyJumpSink(0.0f);
        invalidateHackDamage();
        onDynamics();
    }

    if (al::updateNerveState(this)) {
        if (!mKuriboStateHack->get_c0()) {
            endRideAll(&_2e8, al::getHitSensor(this, "Body"));
            shiftWaitHack();
        } else {
            al::startHitReaction(this, "死亡");
            endRideAll(&_2e8, al::getHitSensor(this, "Body"));
            al::setNerve(this, &NrvKuriboHack.Reset);
        }
    }
}

void KuriboHack::invalidateHackDamage() {
    mHackDamageFrame = 30;
}

void KuriboHack::shiftWaitHack() {
    _1bc = al::getTrans(this).y;
    al::setNerve(this, &NrvKuriboHack.WaitHack);
}

void KuriboHack::endHack() {
    al::setColliderFilterCollisionParts(this, mCollisionPartsFilterSpecialPurpose);
}

void KuriboHack::exeRideOn() {
    if (al::isFirstStep(this)) {
        al::startMtsAnim(this, "EyeReset");
        al::invalidateShadow(this);
        al::setColliderFilterCollisionParts(this, nullptr);
        onDynamics();
    }

    if (al::isActionPlaying(this, "MissTower") || al::isNerve(mHost, &NrvKuriboHack.WaitHack) ||
        al::isNerve(mHost, &NrvKuriboHack.TowerHackEnd))
        offDynamics();
    else
        onDynamics();

    al::StringTmp<128> animName = {"%sTower", al::getPlayingMtsAnimName(mHost)};
    if (al::isMtsAnimExist(this, animName.cstr()))
        al::tryStartMtsAnimIfNotPlaying(this, animName.cstr());

    if (al::isMtsAnimPlaying(this, "EyeMoveTower")) {
        f32 angleToPlayer = al::calcAngleToTargetH(this, rs::getPlayerPos(this));
        angleToPlayer = al::lerpValue(test(angleToPlayer), -90.0f, 90.0f, 0.0f,
                                      al::getMtsAnimFrameMax(this, "EyeMove"));
        al::startMtsAnimAndSetFrameAndStop(this, "EyeMove", angleToPlayer);
    }
}

void KuriboHack::endRideOn() {
    al::validateShadow(this);
    endHack();
    al::restartDitherAnimAutoCtrl(this);
}

void KuriboHack::exeDrown() {
    syncRideOnPosBottomWithDefaultParam();

    if (al::isFirstStep(this)) {
        startActionAll(this, _2e8, "Wait");
        onDynamics();
    }

    bool isFallNextMove = al::isFallNextMove(this, sead::Vector3f::zero, 50.0f, 200.0f);
    bool isOnGround = al::isOnGround(this, 4);
    if (isFallNextMove && isOnGround) {
        if (al::isOnGround(this, 0)) {
            sead::Vector3f dir = al::getTrans(this) - al::getCollidedGroundPos(this);
            f32 dirX = dir.x;
            f32 dirZ = dir.z;
            if (al::tryNormalizeOrZero(&dir)) {
                f32 flatSquaredLength = dirX * dirX + dirZ * dirZ;
                if (al::isLessEqualStep(this, 30) && flatSquaredLength < 400.0f) {
                    dir.x = -dir.x;
                    dir.y = -dir.y;
                    dir.z = -dir.z;
                }
                al::addVelocityToDirection(this, dir, 4.0f);
            }
        }

        if (al::isOnGround(this, 3))
            al::scaleVelocity(this, 0.7f);
        else
            al::scaleVelocity(this, 0.97f);

        if (al::isCollidedGround(this))
            al::addVelocityToGravityFittedGround(this, 2.0f, 0);
        else
            al::addVelocityToGravity(this, 2.0f);
    } else if (!tryShiftChaseOrWander()) {
        al::setNerve(this, &NrvKuriboHack.Fall);
    }
}

void KuriboHack::exeEatBind() {}

// void KuriboHack::attackSensor(al::HitSensor* self, al::HitSensor* other) {}
// bool KuriboHack::receiveMsg(const al::SensorMsg* message, al::HitSensor* other,
//                             al::HitSensor* self) {}

void KuriboHack::prepareKillByShineGet() {
    al::deleteEffectAll(this);
    al::resetPosition(this, mEnemyStateReset->getPos());
    al::hideModelIfShow(this);

    if (mEnemyCap)
        mEnemyCap->kill();

    al::setNerve(this, &NrvKuriboHack.Reset);
}

// NON_MATCHING
bool KuriboHack::tryReceiveMsgHack(const al::SensorMsg* message, al::HitSensor* other,
                                   al::HitSensor* self) {
    if (mHipDropProbeSensor == self || !mKuriboStateHack->receiveMsg(message, other, self))
        return false;

    if (!mKuriboStateHack->isDead()) {
        if (rs::isMsgKuriboTowerNum(message) && !mHost)
            rs::setKuriboTowerNum(message, _2e8.size() + 1);

        return true;
    }

    if (rs::isMsgHackMarioDemo(message)) {
        for (auto kuriboHack = _2e8.robustBegin(); kuriboHack != _2e8.robustEnd(); ++kuriboHack) {
            kuriboHack->eraseFromHost();
            rs::sendMsgRideOnEnd(al::getHitSensor(kuriboHack, "Body"), other);
            kuriboHack->prepareKillByShineGet();
        }

        prepareKillByShineGet();

        if (rs::isMsgKuriboTowerNum(message) && !mHost)
            rs::setKuriboTowerNum(message, _2e8.size() + 1);

        return true;
    }

    if (rs::isMsgCancelHackByDokan(message)) {
        for (auto kuriboHack = _2e8.robustBegin(); kuriboHack != _2e8.robustEnd(); ++kuriboHack) {
            kuriboHack->eraseFromHost();
            rs::sendMsgRideOnEnd(al::getHitSensor(kuriboHack, "Body"), other);
            al::startHitReaction(kuriboHack, "死亡");
            al::setNerve(kuriboHack, &NrvKuriboHack.Reset);
        }

        al::startHitReaction(this, "死亡");
        al::setNerve(this, &NrvKuriboHack.Reset);

        if (rs::isMsgKuriboTowerNum(message) && !mHost)
            rs::setKuriboTowerNum(message, _2e8.size() + 1);

        return true;
    }

    if (mKuriboStateHack->get_c0()) {
        al::startHitReaction(this, "死亡");
        endRideAll(&_2e8, other);
        al::setNerve(this, &NrvKuriboHack.Reset);

        if (rs::isMsgKuriboTowerNum(message) && !mHost)
            rs::setKuriboTowerNum(message, _2e8.size() + 1);

        return true;
    }

    if (_2e8.size() >= 1) {
        al::setNerve(this, &NrvKuriboHack.TowerHackEnd);

        if (rs::isMsgKuriboTowerNum(message) && !mHost)
            rs::setKuriboTowerNum(message, _2e8.size() + 1);

        return true;
    }

    if (al::isOnGround(this, 0))
        al::setVelocityJump(this, 35.0f);
    else
        al::setVelocityJump(this, 30.0f);

    al::addVelocityToFront(this, -5.0f);
    endRideAll(&_2e8, other);
    _1bc = al::getTrans(this).y;
    al::setNerve(this, &NrvKuriboHack.WaitHack);

    if (rs::isMsgKuriboTowerNum(message) && !mHost)
        rs::setKuriboTowerNum(message, _2e8.size() + 1);

    return true;
}

void handlePressDown(KuriboHack* kuribo, const al::SensorMsg* message, al::HitSensor* other,
                     al::HitSensor* self, KuriboHack** host,
                     sead::OffsetList<KuriboHack>* towerList);
bool checkMessageCommon(const al::SensorMsg* message);
void handleBlowDown(KuriboHack* kuribo, al::EnemyStateBlowDown* state, const al::SensorMsg* message,
                    al::HitSensor* other, al::HitSensor* self, KuriboHack** host,
                    sead::OffsetList<KuriboHack>* towerList);

void handleEatBind(KuriboHack* kuribo, const al::SensorMsg* message, al::HitSensor* other,
                   al::HitSensor* self, KuriboHack** host, sead::OffsetList<KuriboHack>* towerList);

// NON_MATCHING
bool KuriboHack::tryReceiveMsgWaitHack(const al::SensorMsg* message, al::HitSensor* other,
                                       al::HitSensor* self) {
    if (isEnableHack() && mEnemyStateSwoon->tryReceiveMsgEnableLockOn(message))
        return true;

    if (_1ac == 0) {
        if (mEnemyStateSwoon->tryReceiveMsgPressDown(message)) {
            trySetHipDropActor(message, other);
            handlePressDown(this, message, other, self, &mHost, &_2e8);

            return true;
        }

        if (rs::isMsgSeedAttackBig(message)) {
            sead::Vector3f local_60 = al::getSensorPos(other) - al::getSensorPos(self);
            if (al::tryNormalizeOrZero(&local_60) &&
                local_60.y >= 6.123032e-17f /* sead::Mathf::cos(sead::Mathf::piHalf()) */) {
                trySetHipDropActor(message, other);
                handlePressDown(this, message, other, self, &mHost, &_2e8);

                return true;
            }
        }
    }

    if (checkMessageCommon(message)) {
        handleBlowDown(this, mEnemyStateBlowDown, message, other, self, &mHost, &_2e8);

        return true;
    }

    if (rs::isMsgFireDamageAll(message)) {
        handleKill(this, &mHost, &_2e8, false);

        return true;
    }

    if (mEnemyStateSwoon->tryReceiveMsgStartHack(message)) {
        mKuriboStateHack->startHack(self, other, this, checkSandSinkPrecisely());
        addCapToHackDemo();
        al::setNerve(this, &NrvKuriboHack.Hack);

        return true;
    }

    if (mEnemyStateSwoon->tryReceiveMsgEndSwoon(message))
        return true;

    if (tryReceiveMsgPush(message, other, self))
        return true;

    if (rs::isMsgRideOnStart(message)) {
        if (_1ac != 0)
            return false;

        KuriboHack* kuriboHack = (KuriboHack*)al::getSensorHost(other);
        al::startMtsAnim(this, "EyeReset");
        al::startMtpAnim(this, "Kuribo");
        kuriboHack->_2e8.pushBack(this);

        if (isSinking()) {
            updateColliderOffsetYAll(&kuriboHack->_2e8, kuriboHack, al::getColliderOffsetY(this));
            kuriboHack->mKuriboStateHack->set_e4(true);
        } else {
            kuriboHack->mKuriboStateHack->set_e4(false);
        }

        mHost = kuriboHack;
        setNerveRideOnCommon();

        return true;
    }

    if (rs::isMsgKillByShineGet(message) || rs::isMsgKillByHomeDemo(message) ||
        rs::isMsgSphinxQuizRouteKill(message) || rs::isMsgKillByMoonRockDemo(message)) {
        prepareKillByShineGet();

        return true;
    }

    if (rs::isMsgYoshiTongueEatBind(message)) {
        EnemyCap* enemyCap = mEnemyCap;
        if (enemyCap && !enemyCap->isBlowDown())
            enemyCap->startBlowDown(other);

        handleEatBind(this, message, other, self, &mHost, &_2e8);

        return true;
    }

    return false;
}

// NON_MATCHING
bool KuriboHack::tryReceiveMsgRideOn(const al::SensorMsg* message, al::HitSensor* other,
                                     al::HitSensor* self) {
    if (al::isNerve(this, &NrvKuriboHack.RideOn)) {
        if (!al::isNerve(mHost, &NrvKuriboHack.Hack) && mHipDropProbeSensor == self &&
            !rs::isMsgPlayerAndCapObjHipDropAll(message))
            return false;

        if (mHipDropProbeSensor == self)
            return false;

        if (rs::isMsgKuriboGirlLove(message))
            return true;

        if (rs::isMsgKuriboTop(message) &&
            (mHost->_2e8.isEmpty() ? mHost : mHost->_2e8.back()) == this)
            return true;

        if (checkMessageCommon(message) || rs::isMsgPlayerDamage(message) ||
            rs::isMsgPushToPlayer(message))
            return mHost->receiveMsg(message, other, self);

        if (rs::isMsgRideOnEnd(message)) {
            mHost = nullptr;
            sead::Vector3f local_50;
            al::addRandomVector(&local_50, sead::Vector3f::zero, 20.0f);
            local_50.y = 0;
            if (al::isNearZero(local_50))
                local_50.set({10.0f, 0.0f, 0.0f});

            al::setVelocityBlowAttackAndTurnToTarget(this, al::getTrans(this) + local_50, 7.0f,
                                                     30.0f);
            al::setColliderOffsetY(this, al::getColliderRadius(this));
            _1bc = al::getTrans(this).y;
            al::setNerve(this, &NrvKuriboHack.WaitHack);

            return true;
        }

        return false;
    }

    if (mHipDropProbeSensor == self && !rs::isMsgPlayerAndCapObjHipDropAll(message))
        return false;

    if (tryRideOnHack(message, other, self))
        return true;

    if (rs::isMsgPressDown(message)) {
        if ((mHost->_2e8.isEmpty() ? mHost : mHost->_2e8.back()) != this)
            return false;

        trySetHipDropActor(message, other);
        handlePressDown(this, message, other, self, &mHost, &_2e8);

        return true;
    }

    if (al::isMsgPushAll(message))
        return mHost->receiveMsg(message, other, self);

    if (rs::isMsgSeedAttackBig(message)) {
        sead::Vector3f local_60 = al::getSensorPos(other) - al::getSensorPos(self);
        if (al::tryNormalizeOrZero(&local_60) &&
            local_60.y >= 6.123032e-17f /* sead::Mathf::cos(sead::Mathf::piHalf()) */ &&
            (mHost->_2e8.isEmpty() ? mHost : mHost->_2e8.back()) == this)
            return true;
    }

    if (rs::isMsgYoshiTongueEatBind(message)) {
        mHost->_1a4 = 10;
        handleEatBind(this, message, other, self, &mHost, &_2e8);

        return true;
    }

    if (checkMessageCommon(message)) {
        mHost->_1a4 = 10;
        handleBlowDown(this, mEnemyStateBlowDown, message, other, self, &mHost, &_2e8);

        return true;
    }

    if (rs::isMsgRideOnStart(message)) {
        if (_1a4 != 0)
            return false;

        if (!al::isSensorEnemyBody(self))
            return false;

        if ((mHost->_2e8.isEmpty() ? mHost : mHost->_2e8.back()) != this)
            return false;

        mHost->_1a4 = 10;
        handleBlowDown(this, mEnemyStateBlowDown, message, other, self, &mHost, &_2e8);

        return true;
    }

    if (!rs::isMsgKillByShineGet(message) && !rs::isMsgKillByHomeDemo(message) &&
        !rs::isMsgSphinxQuizRouteKill(message) && !rs::isMsgKillByMoonRockDemo(message)) {
        if (rs::isMsgRideOnEnd(message)) {
            mHost = nullptr;
            sead::Vector3f local_50;
            al::addRandomVector(&local_50, sead::Vector3f::zero, 20.0f);
            local_50.y = 0;
            if (al::isNearZero(local_50))
                local_50.set({10.0f, 0.0f, 0.0f});

            al::setVelocityBlowAttackAndTurnToTarget(this, al::getTrans(this) + local_50, 7.0f,
                                                     30.0f);
            al::setColliderOffsetY(this, al::getColliderRadius(this));
            _1bc = al::getTrans(this).y;
            al::setNerve(this, &NrvKuriboHack.WaitHack);

            return true;
        }

        return false;
    }

    if (!al::isNerve(mHost, &NrvKuriboHack.Hack))
        return mHost->receiveMsg(message, other, self);

    return false;
}

bool KuriboHack::tryReceiveMsgEatBind(const al::SensorMsg* message, al::HitSensor* other,
                                      al::HitSensor* self) {
    if (mHipDropProbeSensor == self)
        return false;

    if (rs::isMsgYoshiTongueEatBindCancel(message)) {
        al::setNerve(this, &NrvKuriboHack.Wander);

        return true;
    }

    if (rs::isMsgYoshiTongueEatBindFinish(message)) {
        rs::setAppearItemFactorAndOffsetByMsg(this, message, other);
        al::appearItem(this);
        rs::requestHitReactionToAttacker(message, self, other);
        al::setNerve(this, &NrvKuriboHack.Reset);

        return true;
    }

    return false;
}

bool KuriboHack::tryReceiveMsgNormal(const al::SensorMsg* message, al::HitSensor* other,
                                     al::HitSensor* self) {
    if (mHipDropProbeSensor == self && !rs::isMsgPlayerAndCapObjHipDropAll(message))
        return false;

    if (rs::isMsgRideOnRelease(message)) {
        ((KuriboHack*)al::getSensorHost(other))->eraseFromHost();

        return true;
    }

    if (rs::isMsgRideOnStart(message)) {
        if (_1ac != 0)
            return false;

        if (!_2e8.isEmpty() && _2e8.back() != this && !al::isMySensor(self, _2e8.back()))
            return false;

        EnemyCap* enemyCap = mEnemyCap;
        if (enemyCap && !enemyCap->isBlowDown())
            enemyCap->startBlowDown(other);

        KuriboHack* kuriboHack = (KuriboHack*)al::getSensorHost(other);
        kuriboHack->transferGroup(&_2e8);
        kuriboHack->_2e8.pushBack(this);

        if (isSinking()) {
            updateColliderOffsetYAll(&kuriboHack->_2e8, kuriboHack, al::getColliderOffsetY(this));
            kuriboHack->mKuriboStateHack->set_e4(true);
        } else {
            kuriboHack->mKuriboStateHack->set_e4(false);
        }

        mHost = kuriboHack;
        setNerveRideOnCommon();

        return true;
    }

    if (rs::isMsgCapAttack(message)) {
        EnemyCap* enemyCap = mEnemyCap;
        if (!enemyCap || enemyCap->isBlowDown())
            return false;

        enemyCap->startBlowDown(other);

        if (mHost) {
            rs::sendMsgRideOnRelease(al::getHitSensor(mHost, "Body"), self);
            mHost = nullptr;
        } else if (_2e8.size() > 0) {
            KuriboHack* k = _2e8.front();
            k->transferGroup(&_2e8);
            al::setNerve(k, &NrvKuriboHack.Fall);
        }

        rs::requestHitReactionToAttacker(message, self, other);

        if (al::isNerve(this, &NrvKuriboHack.Sink))
            return true;

        al::setVelocityBlowAttackAndTurnToTarget(this, al::getActorTrans(other), 12.0f, 25.0f);
        al::setNerve(this, &NrvKuriboHack.DamageCap);

        return true;
    }

    if (rs::isMsgCapEnableLockOn(message))
        return isEnableHack() && !isCapWorn() && !mHost;

    if (rs::isMsgCapCancelLockOn(message)) {
        _1b8 = 10;

        return true;
    }

    if (rs::isMsgStartHack(message)) {
        mKuriboStateHack->startHack(self, other, this, al::isNerve(this, &NrvKuriboHack.Sink));
        addCapToHackDemo();
        startActionAll(this, _2e8, "Wait");
        al::setNerve(this, &NrvKuriboHack.Hack);

        return true;
    }

    if (rs::isMsgSeedAttackBig(message)) {
        sead::Vector3f local_60 = al::getSensorPos(other) - al::getSensorPos(self);
        if (al::tryNormalizeOrZero(&local_60) &&
            local_60.y >= 6.123032e-17f /* sead::Mathf::cos(sead::Mathf::piHalf()) */ &&
            _2e8.isEmpty()) {
            EnemyCap* enemyCap = mEnemyCap;
            if (enemyCap && !enemyCap->isBlowDown())
                enemyCap->startBlowDown(other);

            trySetHipDropActor(message, other);
            handlePressDown(this, message, other, self, &mHost, &_2e8);

            return true;
        }
    }

    if (rs::isMsgPressDown(message) && _2e8.isEmpty()) {
        EnemyCap* enemyCap = mEnemyCap;
        if (enemyCap && !enemyCap->isBlowDown())
            enemyCap->startBlowDown(other);

        trySetHipDropActor(message, other);
        handlePressDown(this, message, other, self, &mHost, &_2e8);

        return true;
    }

    if (checkMessageCommon(message)) {
        handleBlowDown(this, mEnemyStateBlowDown, message, other, self, &mHost, &_2e8);

        return true;
    }

    if (rs::isMsgFireDamageAll(message)) {
        handleKill(this, &mHost, &_2e8, false);

        return true;
    }

    if (mActorStateSandGeyser->receiveMsgSandGeyser(message, other)) {
        if (!al::isNerve(this, &NrvKuriboHack.SandGeyser))
            al::setNerve(this, &NrvKuriboHack.SandGeyser);

        return true;
    }

    if (tryReceiveMsgPush(message, other, self))
        return true;

    if (rs::isMsgKuriboFlick(message)) {
        al::pushAndAddVelocityH(this, other, self, 5.0f);
        al::setNerve(this, &NrvKuriboHack.Wander);

        return true;
    }

    if (rs::isMsgKillByShineGet(message) || rs::isMsgKillByHomeDemo(message) ||
        rs::isMsgSphinxQuizRouteKill(message) || rs::isMsgKillByMoonRockDemo(message)) {
        notifyKillByShineGetToGroup(message, other, self);
        prepareKillByShineGet();

        return true;
    }

    if (rs::isMsgYoshiTongueEatBind(message)) {
        EnemyCap* enemyCap = mEnemyCap;
        if (enemyCap && !enemyCap->isBlowDown())
            enemyCap->startBlowDown(other);

        handleEatBind(this, message, other, self, &mHost, &_2e8);

        return true;
    }

    return false;
}

void KuriboHack::transferGroup(sead::OffsetList<KuriboHack>* dst) {
    if (dst->size() <= 0)
        return;

    for (auto kuriboHack = dst->robustBegin(); kuriboHack != dst->robustEnd(); ++kuriboHack) {
        kuriboHack->eraseFromHost();
        if (kuriboHack != this) {
            _2e8.pushBack(kuriboHack);
            kuriboHack->mHost = this;
        }
    }

    mHost = nullptr;
}

void KuriboHack::eraseFromHost() {
    if (!mHost)
        return;

    mHost->_2e8.erase(this);
    al::showModelIfHide(this);
}

void KuriboHack::notifyJumpSink(f32 param_1) {
    _1a0 = param_1;
}

bool KuriboHack::isSinking() const {
    // This is crazy that `return al::isNerve(this, &NrvKuriboHack.Sink) || (al::isNerve(this,
    // &NrvKuriboHack.WaitHack) && checkSandSinkPrecisely())` doesn't match.
    if (al::isNerve(this, &NrvKuriboHack.Sink) ||
        (al::isNerve(this, &NrvKuriboHack.WaitHack) && checkSandSinkPrecisely()))
        return true;
    else
        return false;
}

void syncRideOnPos(f32, f32, KuriboHack*, KuriboHack*);

// NON_MATCHING
void KuriboHack::syncRideOnPosBottom(f32 param_1, f32 param_2) {
    if (_2e8.isEmpty())
        return;

    KuriboHack* k = _2e8.back();
    if (al::isCollidedCeiling(k))
        param_2 = sead::Mathf::clampMax(
            (al::getTrans(k).y - al::getTrans(this).y) / (f32)_2e8.size(), param_2);

    syncRideOnPos(param_1, param_2, this, _2e8.front());
    auto kuribo = ++_2e8.begin();
    for (auto kuriboHack = _2e8.begin(); kuribo != _2e8.end(); ++kuriboHack)
        syncRideOnPos(param_1, param_2, kuriboHack, kuribo);
}

void syncRideOnPos(f32 param_1, f32 param_2, KuriboHack* param_3, KuriboHack* param_4) {
    sead::Vector3f up;
    al::calcUpDir(&up, param_3);
    sead::Quatf quat;
    al::calcQuat(&quat, param_3);
    param_4->applyRideOnQuat(quat);

    sead::Vector3f velocity;
    velocity.setSub(al::getTrans(param_3) + up * param_2, al::getTrans(param_4));

    al::setVelocity(param_4, velocity * param_1);
}

// NON_MATCHING
void KuriboHack::resetRideOnPosBottom(f32 param_1) {
    if (_2e8.isEmpty())
        return;

    KuriboHack* k = _2e8.back();
    if (al::isCollidedCeiling(k))
        param_1 = sead::Mathf::clampMax(
            (al::getTrans(k).y - al::getTrans(this).y) / (f32)_2e8.size(), param_1);

    KuriboHack* kuribo = _2e8.front();
    sead::Vector3f up;
    al::calcUpDir(&up, this);
    sead::Quatf quat;
    al::calcQuat(&quat, this);
    al::setQuat(kuribo, quat);
    al::resetPosition(kuribo, al::getTrans(this) + param_1 * up);

    auto kur = ++_2e8.begin();
    for (auto kuriboHack = _2e8.begin(); kuribo != _2e8.end(); ++kuriboHack) {
        al::calcUpDir(&up, kuriboHack);
        al::calcQuat(&quat, kuriboHack);
        al::setQuat(kur, quat);
        al::resetPosition(kur, al::getTrans(kuriboHack) + param_1 * up);
    }
}

void KuriboHack::validateSpecialPush(u32 param_1) {
    if (_1b0 == 0)
        al::validateHitSensor(this, "SpecialPush");

    if (_1b0 < param_1)
        _1b0 = param_1;
}

void KuriboHack::startRideOnRotation() {
    mRideOnRotationFrame = 30;
}

void KuriboHack::applyRideOnQuat(const sead::Quatf& quat) {
    f32 rate = sead::Mathf::clampMax((f32)(30 - mRideOnRotationFrame) / 30.0f, 1.0f);
    sead::Quatf q;
    al::slerpQuat(&q, al::getQuat(this), quat, rate);
    al::setQuat(this, q);
}

bool KuriboHack::isInvalidHackDamage() const {
    return mHackDamageFrame != 0;
}

void KuriboHack::validateHipDropProbe(al::HitSensor* hitSensor) {
    _25c = 5;
    al::validateHitSensor(this, "HipDropProbe");
    al::getSensorPos(hitSensor);
    al::getSensorPos(this, "Body");
    mHipDropProbeSensorPos.set(al::getSensorPos(hitSensor));
    mHipDropProbeSensorPos.y = al::getSensorPos(this, "Body").y;
}

s32 KuriboHack::indexInHostList() const {
    return mHost ? mHost->_2e8.indexOf(this) : -1;
}

f32 KuriboHack::getRideOnRowSize() {
    return 110.0f;
}

bool KuriboHack::isCapWorn() const {
    return mEnemyCap && !mEnemyCap->isBlowDown();
}

bool KuriboHack::isEnableHack() const {
    const KuriboHack* kuriboHack = this;
    while (kuriboHack->mHost)
        kuriboHack = kuriboHack->mHost;

    if (al::isNerve(kuriboHack, &NrvKuriboHack.WaitHack) && !al::isOnGround(kuriboHack, 0) &&
        kuriboHack->_1bc < al::getTrans(kuriboHack).y)
        return false;

    if (al::isNerve(kuriboHack, &NrvKuriboHack.Hack))
        return false;

    return kuriboHack->_1b8 == 0;
}

// NON_MATCHING
void KuriboHack::trySetHipDropActor(const al::SensorMsg* message, al::HitSensor* other) {
    if (!rs::isMsgPlayerAndCapObjHipDropAll(message)) {
        mHipDropActor = nullptr;

        return;
    }

    mHipDropActor = al::getSensorHost(other);
    _250.set(al::getTrans(this) - al::getTrans(mHipDropActor));

    if (!mHost)
        return;

    mHost->_1d8 = 30;
    if (al::isNerve(this, &NrvKuriboHack.Wander) || al::isNerve(this, &NrvKuriboHack.Chase))
        al::setNerve(mHost, &NrvKuriboHack.Stop);

    mHost->_2e8.prev(this)->validateHipDropProbe(other);
}

void handlePressDown(KuriboHack* kuribo, const al::SensorMsg* message, al::HitSensor* other,
                     al::HitSensor* self, KuriboHack** host,
                     sead::OffsetList<KuriboHack>* towerList) {
    if (!*host) {
        al::startAction(kuribo, "PressDown");
    } else {
        rs::sendMsgRideOnRelease(al::getHitSensor(*host, "Body"), self);
        *host = nullptr;

        if (!rs::isMsgPlayerAndCapObjHipDropAll(message))
            al::startAction(kuribo, "PressDownTower");
        else
            al::startAction(kuribo, "PressDown");
    }

    endRideAll(towerList, other);
    rs::setAppearItemFactorAndOffsetByMsg(kuribo, message, other);
    rs::requestHitReactionToAttacker(message, self, other);
    al::setNerve(kuribo, &NrvKuriboHack.PressDown);
}

bool checkMessageCommon(const al::SensorMsg* message) {
    return rs::isMsgBlowDown(message) || rs::isMsgGrowerAttack(message) ||
           rs::isMsgBubbleAttack(message) || rs::isMsgIcicleAttack(message) ||
           rs::isMsgSeedAttack(message) || rs::isMsgSeedAttackHold(message) ||
           rs::isMsgWaterRoadIn(message) || rs::isMsgFireDamageAll(message) ||
           rs::isMsgHackAttackFire(message) || rs::isMsgGunetterBodyTouch(message) ||
           rs::isMsgGunetterAttack(message);
}

void handleBlowDown(KuriboHack* kuribo, al::EnemyStateBlowDown* state, const al::SensorMsg* message,
                    al::HitSensor* other, al::HitSensor* self, KuriboHack** host,
                    sead::OffsetList<KuriboHack>* towerList) {
    if (*host) {
        rs::sendMsgRideOnRelease(al::getHitSensor(*host, "Body"), self);
        *host = nullptr;
    } else if (towerList->size() > 0) {
        KuriboHack* k = towerList->front();
        k->transferGroup(towerList);
        al::setNerve(k, &NrvKuriboHack.Wander);
    }

    rs::setAppearItemFactorAndOffsetByMsg(kuribo, message, other);
    sead::Vector3f dir;
    rs::calcBlowDownDir(&dir, message, other, self);
    state->start(dir);
    rs::requestHitReactionToAttacker(message, self, other);
    al::setNerve(kuribo, &NrvKuriboHack.BlowDown);
}

void KuriboHack::addCapToHackDemo() {
    if (!mEnemyCap || al::isDead(mEnemyCap))
        return;

    rs::addDemoActor(mEnemyCap, false);
}

// NON_MATCHING
bool KuriboHack::tryReceiveMsgPush(const al::SensorMsg* message, al::HitSensor* other,
                                   al::HitSensor* self) {
    if (!al::isMsgPushAll(message))
        return false;

    sead::Vector3f pushForce = al::getSensorPos(self) - al::getSensorPos(other);
    f32 selfRadius = al::getSensorRadius(self);
    f32 otherRadius = al::getSensorRadius(other);

    if (selfRadius + otherRadius < pushForce.length())
        return false;

    f32 pushForceLength = pushForce.length();
    if (pushForceLength > 0.0f)
        pushForce *= (selfRadius + otherRadius - pushForceLength) / pushForceLength;

    if (al::isNearZero(pushForce.x * pushForce.x + pushForce.z * pushForce.z))
        pushForce.z += 1.0f;

    mPlayerPushReceiver->receiveForceDirect(pushForce);

    return true;
}

void handleEatBind(KuriboHack* kuribo, const al::SensorMsg* message, al::HitSensor* other,
                   al::HitSensor* self, KuriboHack** host,
                   sead::OffsetList<KuriboHack>* towerList) {
    if (*host) {
        rs::sendMsgRideOnRelease(al::getHitSensor(*host, "Body"), self);
        *host = nullptr;
    } else if (towerList->size() > 0) {
        KuriboHack* k = towerList->front();
        k->transferGroup(towerList);
        al::setNerve(k, &NrvKuriboHack.Wander);
    }

    al::setNerve(kuribo, &NrvKuriboHack.EatBind);
    rs::requestHitReactionToAttacker(message, self, other);
}

bool KuriboHack::tryRideOnHack(const al::SensorMsg* message, al::HitSensor* other,
                               al::HitSensor* self) {
    if (!isEnableHack())
        return false;

    if (mEnemyStateSwoon->tryReceiveMsgStartLockOn(message) ||
        mEnemyStateSwoon->tryReceiveMsgEnableLockOn(message))
        return true;

    if (mEnemyStateSwoon->tryReceiveMsgStartHack(message)) {
        bool isHostSinking = mHost->isSinking();
        f32 colliderOffsetY = al::getColliderOffsetY(mHost);
        KuriboHack* host = mHost;
        s32 hostIndex = indexInHostList();
        eraseFromHost();
        mHost = nullptr;
        host->mHost = this;
        host->setNerveRideOnCommon();
        s32 towerIndex = 0;
        if (towerIndex == hostIndex) {
            _2e8.pushBack(host);
            towerIndex++;
        }

        for (auto kuriboHack = host->_2e8.robustBegin(); kuriboHack != host->_2e8.robustEnd();
             ++kuriboHack) {
            kuriboHack->eraseFromHost();
            kuriboHack->mHost = this;
            kuriboHack->setNerveRideOnCommon();
            _2e8.pushBack(kuriboHack);
            towerIndex++;
            if (towerIndex == hostIndex) {
                _2e8.pushBack(host);
                towerIndex++;
            }
        }

        sead::Vector3f trans = al::getTrans(this);
        al::copyPose(this, host);
        al::setNerve(this, &NrvKuriboHack.Hack);
        al::resetPosition(this);
        al::resetPosition(host, trans);
        syncRideOnPosBottomWithDefaultParam();
        updateColliderOffsetYAll(&_2e8, this, colliderOffsetY);
        mKuriboStateHack->startHack(al::getHitSensor(this, "Body"), other, this, isHostSinking);
        addCapToHackDemo();
        startActionAll(this, _2e8, "Wait");

        return true;
    }

    if (mEnemyStateSwoon->tryReceiveMsgEndSwoon(message)) {
        mHost->_1b8 = 10;

        return true;
    }

    return false;
}

void KuriboHack::notifyKillByShineGetToGroup(const al::SensorMsg* message, al::HitSensor* other,
                                             al::HitSensor* self) {
    for (auto kuriboHack = _2e8.robustBegin(); kuriboHack != _2e8.robustEnd(); ++kuriboHack) {
        kuriboHack->eraseFromHost();
        rs::sendMsgRideOnEnd(al::getHitSensor(kuriboHack, "Body"), other);
        kuriboHack->receiveMsg(message, other, self);
    }
}

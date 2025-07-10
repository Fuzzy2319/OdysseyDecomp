#include "Enemy/KuriboHack.h"

#include "Library/Base/StringUtil.h"
#include "Library/Collision/Collider.h"
#include "Library/Collision/CollisionPartsKeeperUtil.h"
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
#include "Util/Hack.h"
#include "Util/PlayerUtil.h"

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

static al::EnemyStateBlowDownParam gEnemyStateBlowDownParam = {"BlowDown", 18.0f, 35.0f, 2.0f,
                                                               0.97f,      120,   true};

// TODO: Move this
f32 KuriboHack::getRideOnRowSize() {
    return 110.0f;
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
            kuriboHack->_160 = this;
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
        if (kuriboHack->_160) {
            kuriboHack->_160->_2e8.erase(kuriboHack);
            al::showModelIfHide(kuriboHack);
        }

        kuriboHack->_160 = nullptr;
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
    if (al::isNerve(this, &NrvKuriboHack.RideOn) && _160)
        al::setModelAlphaMask(this, al::getModelAlphaMask(_160));
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

// void FUN_710014d2ac(KuriboHack *param_1,KuriboHack **param_2,OffsetList *param_3,bool param_4) {}
// void FUN_710014d390(OffsetList *param_1,HitSensor *param_2) {}
// void FUN_710014d440(KuriboHack *param_1,EnemyStateBlowDown *param_2,KuriboHack **param_3,
//                     OffsetList *param_4) {}
// void KuriboHack::updateCollider() {}
// void KuriboHack::solveCollisionInHacking(const sead::Vector3f&) {}

// NON_MATCHING regswap https://decomp.me/scratch/GObFz
void KuriboHack::pushFrom(KuriboHack* kuribo, const sead::Vector3f& up) {
    sead::Vector3f gravity = -up;
    if (!al::tryNormalizeOrZero(&gravity))
        return;

    for (auto kuriboHack = _2e8.begin(kuribo); kuriboHack != _2e8.end(); --kuriboHack)
        kuriboHack->mPlayerPushReceiver->receiveForceDirect(up);

    mPlayerPushReceiver->receiveForceDirect(up);
    al::limitVelocityDirSign(this, gravity, 0.0f);
}

void KuriboHack::calcAnim() {
    if (al::isNerve(this, &NrvKuriboHack.RideOn) && _2e0)
        return;

    if ((!al::isNerve(this, &NrvKuriboHack.Hack) &&
         (!al::isNerve(this, &NrvKuriboHack.RideOn) || !al::isNerve(_160, &NrvKuriboHack.Hack))) ||
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

        if (!al::isNerve(_160, &NrvKuriboHack.Hack))
            return;

        if ((_160->_2e8.isEmpty() ? _160 : _160->_2e8.back()) != this)
            return;

        kuriboHack = _160;
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

void FUN_710014e3f0(KuriboHack*, const sead::OffsetList<KuriboHack>&, const char*, const char*);

void KuriboHack::exeWait() {
    if (al::isFirstStep(this)) {
        FUN_710014e3f0(this, _2e8, "Wait", nullptr);
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

// void FUN_710014e3f0(KuriboHack* param_1, const sead::OffsetList<KuriboHack>& param_2,
//                     const char* param_3, const char* param_4) {}

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

    if (mEnemyStateWander->isWait() && tryShiftDrown()) {
    } else if (al::isCollidedGroundFloorCode(this, "Slide")) {
        al::setNerve(this, &NrvKuriboHack.Slide);
    } else if (!al::isInAreaObj(al::getPlayerActor(this, 0), "StealthArea") &&
               !rs::isPlayerHackJugemFishing(this) && al::isNearPlayer(this, 1200.0f) &&
               al::isGreaterEqualStep(this, 60)) {
        al::setNerve(this, &NrvKuriboHack.Turn);
    }
}

bool KuriboHack::tryShiftDrown() {
    if (!al::isOnGround(this, 0) || !al::isFallNextMove(this, sead::Vector3f::zero, 50.0f, 200.0f))
        return false;

    al::setVelocityZeroH(this);
    al::setNerve(this, &NrvKuriboHack.Drown);

    return true;
}

// void KuriboHack::exeTurn() {}
// void KuriboHack::exeFind() {}
// void KuriboHack::exeChase() {}

void KuriboHack::exeStop() {
    syncRideOnPosBottomWithDefaultParam();

    if (al::isFirstStep(this)) {
        al::startMtsAnim(this, "EyeReset");
        FUN_710014e3f0(this, _2e8, "Miss", nullptr);
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
        FUN_710014e3f0(this, _2e8, "Attack", "AttackTower");
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

// void KuriboHack::exePressDown() {}

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
        if (tryShiftDrown()) {
        } else if (al::isOnGround(this, 0)) {
            al::setNerve(this, &NrvKuriboHack.Land);
        } else {
            al::setNerve(this, &NrvKuriboHack.Fall);
        }
    }
}

void KuriboHack::exeFall() {
    syncRideOnPosBottomWithDefaultParam();

    if (al::isFirstStep(this)) {
        FUN_710014e3f0(this, _2e8, "Fall", nullptr);
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
        FUN_710014e3f0(this, _2e8, "Land", nullptr);
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

// void KuriboHack::exeSink() {}

// NON_MATCHING
bool KuriboHack::updateSink() {
    f32 radius = al::getColliderRadius(this);
    f32 offsetY = al::getColliderOffsetY(this) + 1.0f;
    if (offsetY > radius * 3.0f)
        return true;

    al::setColliderOffsetY(this, offsetY);
    for (auto kuriboHack = _2e8.begin(); kuriboHack != _2e8.end(); ++kuriboHack)
        al::setColliderOffsetY(kuriboHack, offsetY);

    return false;
}

void KuriboHack::exeSlide() {
    syncRideOnPosBottomWithDefaultParam();

    if (al::isFirstStep(this))
        FUN_710014e3f0(this, _2e8, "Slide", nullptr);

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

// void KuriboHack::exeReset() {}

void KuriboHack::exeSandGeyser() {
    if (al::isFirstStep(this)) {
        FUN_710014e3f0(this, _2e8, "SandGeyser", nullptr);
        al::invalidateClipping(this);
        onDynamics();
        clearSink();
    }

    al::updateNerveStateAndNextNerve(this, &NrvKuriboHack.Wander);
}

// NON_MATCHING
void KuriboHack::clearSink() {
    f32 radius = al::getColliderRadius(this);
    al::setColliderOffsetY(this, radius);
    for (auto kuriboHack = _2e8.begin(); kuriboHack != _2e8.end(); ++kuriboHack)
        al::setColliderOffsetY(kuriboHack, radius);
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

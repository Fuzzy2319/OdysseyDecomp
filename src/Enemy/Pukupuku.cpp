#include "Enemy/Pukupuku.h"

#include "Library/Area/AreaObjUtil.h"
#include "Library/Base/StringUtil.h"
#include "Library/Effect/EffectSystemInfo.h"
#include "Library/Fluid/JointRippleGenerator.h"
#include "Library/Joint/JointControllerKeeper.h"
#include "Library/Joint/JointSpringController.h"
#include "Library/LiveActor/ActorActionFunction.h"
#include "Library/LiveActor/ActorAnimFunction.h"
#include "Library/LiveActor/ActorClippingFunction.h"
#include "Library/LiveActor/ActorFlagFunction.h"
#include "Library/LiveActor/ActorInitUtil.h"
#include "Library/LiveActor/ActorModelFunction.h"
#include "Library/LiveActor/ActorPoseUtil.h"
#include "Library/LiveActor/ActorSceneFunction.h"
#include "Library/LiveActor/ActorSensorUtil.h"
#include "Library/LiveActor/LiveActorFunction.h"
#include "Library/Math/MathUtil.h"
#include "Library/Matrix/MatrixUtil.h"
#include "Library/Movement/EnemyStateBlowDown.h"
#include "Library/Nature/NatureUtil.h"
#include "Library/Nature/WaterSurfaceFinder.h"
#include "Library/Nerve/NerveSetupUtil.h"
#include "Library/Nerve/NerveUtil.h"
#include "Library/Placement/PlacementFunction.h"
#include "Library/Rail/RailUtil.h"
#include "Library/Shadow/ActorShadowUtil.h"

#include "Enemy/EnemyStateReviveInsideScreen.h"
#include "Enemy/EnemyStateSwoon.h"
#include "Enemy/HackerDepthShadowMapCtrl.h"
#include "Npc/SphinxQuizRouteKillExecutor.h"
#include "Player/HackerStateNormalJump.h"
#include "Player/PlayerHackStartShaderCtrl.h"
#include "Util/Hack.h"

namespace {
NERVE_IMPL(Pukupuku, Wait)
NERVE_IMPL(Pukupuku, Swoon)
NERVE_IMPL(Pukupuku, Revive)
NERVE_IMPL(Pukupuku, BlowDown)
NERVE_IMPL_(Pukupuku, BlowDownFromCapture, BlowDown)
NERVE_IMPL_(Pukupuku, BlowDownWithoutMsg, BlowDown)
NERVE_IMPL(Pukupuku, CaptureJumpGround)
NERVE_IMPL(Pukupuku, CaptureWait)
NERVE_IMPL_(Pukupuku, CaptureWaitTurnStart, CaptureWait)
NERVE_IMPL_(Pukupuku, CaptureWaitTurn, CaptureWait)
NERVE_IMPL(Pukupuku, CaptureSwimStart)
NERVE_IMPL(Pukupuku, CaptureSwim)
NERVE_IMPL_(Pukupuku, CaptureSwimDash, CaptureSwim)
NERVE_IMPL(Pukupuku, CaptureReactionWall)
NERVE_IMPL(Pukupuku, CaptureAttack)
NERVE_IMPL_(Pukupuku, CaptureRollingL, CaptureRolling)
NERVE_IMPL_(Pukupuku, CaptureRollingR, CaptureRolling)
NERVE_IMPL(Pukupuku, WaitTurnToRailDir)
NERVE_IMPL(Pukupuku, WaitRollingRail)
NERVE_IMPL(Pukupuku, Reaction)
NERVE_IMPL(Pukupuku, Trample)
NERVE_IMPL(Pukupuku, CaptureStart)
NERVE_IMPL(Pukupuku, CaptureStartEnd)
NERVE_IMPL(Pukupuku, DemoWaitToRevive)
NERVE_IMPL_(Pukupuku, CaptureWaitAir, CaptureWait)
NERVE_IMPL_(Pukupuku, CaptureJumpOut, CaptureWait)
NERVE_IMPL(Pukupuku, CaptureWaitGround)
NERVE_IMPL(Pukupuku, CaptureLandGround)

NERVES_MAKE_STRUCT(Pukupuku, Wait, Swoon, Revive, BlowDown, BlowDownFromCapture, BlowDownWithoutMsg,
                   CaptureJumpGround, CaptureWait, CaptureWaitTurnStart, CaptureWaitTurn,
                   CaptureSwimStart, CaptureSwim, CaptureSwimDash, CaptureReactionWall,
                   CaptureAttack, CaptureRollingL, CaptureRollingR, WaitTurnToRailDir,
                   WaitRollingRail, Reaction, Trample, CaptureStart, CaptureStartEnd,
                   DemoWaitToRevive, CaptureWaitAir, CaptureJumpOut, CaptureWaitGround,
                   CaptureLandGround)

class AreaObjFilterWaterIgnore : public al::AreaObjFilterBase {
    bool isValidArea(al::AreaObj* areaObj) const override { return al::isWaterAreaIgnore(areaObj); }
};

class AreaObjFilterWater : public al::AreaObjFilterBase {
    bool isValidArea(al::AreaObj* areaObj) const override {
        return !al::isWaterAreaIgnore(areaObj);
    }
};
}  // namespace

static sead::Vector3f g_7101e62d50 = {0.0f, 50.0f, 50.0f};
static al::EnemyStateBlowDownParam g_7101e62d10 = {"BlowDown", 8.0f, 13.0f, 0.5f, 1.0f, 120, true};
static al::EnemyStateBlowDownParam g_7101e62d30 = {"BlowDown", 16.0f, 32.0f, 1.0f,
                                                   0.95f,      120,   true};
static PlayerHackStartShaderParam g_7101e62d5c = {true, -1.0f, 10, 20};
const f32 g_71018a3dac[] = {0.5f, 0.1f, 45.0f, 0.5f, 0.5f, 45.0f,
                            0.3f, 0.2f, 15.0f, 0.3f, 0.2f, 15.0f};
const char* g_7101ca5700[] = {"Tail1", "Tail2", "WingLeft", "WingRight"};

Pukupuku::Pukupuku(const char* name) : LiveActor(name) {}

void Pukupuku::init(const al::ActorInitInfo& info) {
    s32 lightType = 1;
    al::tryGetArg(&lightType, info, "LightType");
    const char* suffix;
    switch (lightType) {
    case 0:
        suffix = "LightLow";
        break;
    case 2:
        suffix = "LightMiddle";
        break;
    default:
        suffix = "LightHigh";
        break;
    }
    al::initActorWithArchiveName(this, info, "Pukupuku", suffix);

    bool isPukupukuSnow = al::isObjectName(info, "PukupukuSnow");
    mIsPukupukuSnow = isPukupukuSnow;

    al::startVisAnimAndSetFrameAndStop(this, "CapOnOff", 0.0f);
    const char* mtpAnimName = "CapOnOffSnow";
    if (!isPukupukuSnow)
        mtpAnimName = "CapOnOff";
    al::startMtpAnimAndSetFrameAndStop(this, mtpAnimName, 0.0f);

    const char* capName = "Snow";
    if (!mIsPukupukuSnow)
        capName = nullptr;
    mCapTargetInfo = rs::createCapTargetInfo(this, capName);
    mWaterSurfaceFinder = new al::WaterSurfaceFinder(this);

    al::initNerve(this, &NrvPukupuku.Wait, 6);

    al::tryGetArg(&mMoveType, info, "MoveType");

    if (al::isExistRail(info, "Rail")) {
        al::setSyncRailToNearestPos(this);
        mRailPointNo = al::getRailPointNo(this);
    }

    EnemyStateSwoonInitParam enemyStateSwoonInitParam = {
        "SwoonStart", "SwoonLoop", "SwoonEnd", nullptr, "SwoonStartFall", "SwoonStartLand"};
    mEnemyStateSwoon = new EnemyStateSwoon(this, "SwoonStart", "Swoon", "SwoonEnd", false, true);
    mEnemyStateSwoon->initParams(enemyStateSwoonInitParam);
    al::initNerveState(this, mEnemyStateSwoon, &NrvPukupuku.Swoon, "気絶");
    mEnemyStateReviveInsideScreen = new EnemyStateReviveInsideScreen(this);
    al::initNerveState(this, mEnemyStateReviveInsideScreen, &NrvPukupuku.Revive, "画面内復活");
    mEnemyStateBlowDown = new al::EnemyStateBlowDown(this, &g_7101e62d10, "吹き飛び状態");
    al::initNerveState(this, mEnemyStateBlowDown, &NrvPukupuku.BlowDown, "吹き飛び");
    al::addNerveState(this, mEnemyStateBlowDown, &NrvPukupuku.BlowDownFromCapture,
                      "吹き飛び[憑依中]");
    al::addNerveState(this, mEnemyStateBlowDown, &NrvPukupuku.BlowDownWithoutMsg,
                      "吹き飛び[メッセージなし死亡]");
    mHackerStateNormalJump = new HackerStateNormalJump(this, &_118, "JumpGround", nullptr);
    mHackerStateNormalJump->set_38({15.0f, 30.0f, 2.0f});
    mHackerStateNormalJump->set_48({8.0f, 8.0f});
    al::initNerveState(this, mHackerStateNormalJump, &NrvPukupuku.CaptureJumpGround,
                       "地上飛び跳ね");

    al::initJointControllerKeeper(this, 7);
    al::initJointLocalRotator(this, &_168, "AllRoot");
    mJointSpringControllers.allocBuffer(4, nullptr);
    for (s32 i = 0; i < 4; i++) {
        al::JointSpringController* jointSpringController =
            al::initJointSpringController(this, g_7101ca5700[i]);
        jointSpringController->setStability(g_71018a3dac[i * 3]);
        jointSpringController->setFriction(g_71018a3dac[i * 3 + 1]);
        jointSpringController->setLimitDegree(g_71018a3dac[i * 3 + 2]);
        al::StringTmp<128> _ = {"ダイナミクス[%s]", g_7101ca5700[i]};
        _.cstr();
        mJointSpringControllers.pushBack(jointSpringController);
    }

    al::setEffectFollowMtxPtr(this, "SwimSurfaceTrace", &mSwimSurfaceTraceEffectFollowMtx);
    al::setEffectFollowMtxPtr(this, "WaterAreaIn", &mWaterAreaInEffectFollowMtx);
    al::setEffectFollowMtxPtr(this, "WaterAreaOut", &mWaterAreaOutEffectFollowMtx);
    al::setEffectNamedMtxPtr(this, "WaterSurface", &mWaterSurfaceEffectFollowMtx);

    al::offCollide(this);

    al::makeMtxQuatPos(&_1a0, al::getQuat(this), al::getTrans(this));

    al::invalidateDepthShadowMap(this);
    _2b4 = al::getShadowMaskDropLength(this, "シャドウマスク");
    al::setShadowMaskDropLength(this, 100.0f, "シャドウマスク");

    mJointRippleGenerator = new al::JointRippleGenerator(this);
    mJointRippleGenerator->set({120.0f, 0.0f, 0.0f}, "Tail2", 0.15f, 110.0f, 2.0f, 250.0f);

    mPlayerHackStartShaderCtrl = new PlayerHackStartShaderCtrl(this, &g_7101e62d5c);
    mHackerDepthShadowMapCtrl = new HackerDepthShadowMapCtrl(this, "Ground", 100.0f, 0.3f, 0.5f);

    makeActorAlive();
}

void Pukupuku::initAfterPlacement() {
    rs::tryRegisterSphinxQuizRouteKillSensorAfterPlacement(al::getHitSensor(this, "Body"));
}

bool Pukupuku::isNerveInWater() const {
    return al::isNerve(this, &NrvPukupuku.CaptureWait) ||
           al::isNerve(this, &NrvPukupuku.CaptureWaitTurnStart) ||
           al::isNerve(this, &NrvPukupuku.CaptureWaitTurn) ||
           al::isNerve(this, &NrvPukupuku.CaptureSwimStart) ||
           al::isNerve(this, &NrvPukupuku.CaptureSwim) ||
           al::isNerve(this, &NrvPukupuku.CaptureSwimDash) ||
           al::isNerve(this, &NrvPukupuku.CaptureReactionWall) ||
           al::isNerve(this, &NrvPukupuku.CaptureAttack) ||
           al::isNerve(this, &NrvPukupuku.CaptureRollingL) ||
           al::isNerve(this, &NrvPukupuku.CaptureRollingR) ||
           (al::isNerve(this, &NrvPukupuku.Swoon) && _19d) ||
           al::isNerve(this, &NrvPukupuku.Wait) ||
           al::isNerve(this, &NrvPukupuku.WaitTurnToRailDir) ||
           al::isNerve(this, &NrvPukupuku.WaitRollingRail) ||
           al::isNerve(this, &NrvPukupuku.Reaction) || al::isNerve(this, &NrvPukupuku.Trample) ||
           al::isNerve(this, &NrvPukupuku.BlowDown) ||
           al::isNerve(this, &NrvPukupuku.BlowDownFromCapture) ||
           al::isNerve(this, &NrvPukupuku.BlowDownWithoutMsg);
}

// void Pukupuku::attackSensor(al::HitSensor* self, al::HitSensor* other) {}

bool FUN_7100175f24(Pukupuku* pukupuku) {
    return al::isNerve(pukupuku, &NrvPukupuku.CaptureStart) ||
           al::isNerve(pukupuku, &NrvPukupuku.CaptureStartEnd) ||
           al::isNerve(pukupuku, &NrvPukupuku.CaptureWait) ||
           al::isNerve(pukupuku, &NrvPukupuku.CaptureWaitTurnStart) ||
           al::isNerve(pukupuku, &NrvPukupuku.CaptureWaitTurn) ||
           al::isNerve(pukupuku, &NrvPukupuku.CaptureSwimStart) ||
           al::isNerve(pukupuku, &NrvPukupuku.CaptureSwim) ||
           al::isNerve(pukupuku, &NrvPukupuku.CaptureSwimDash) ||
           al::isNerve(pukupuku, &NrvPukupuku.CaptureReactionWall) ||
           al::isNerve(pukupuku, &NrvPukupuku.CaptureWaitAir) ||
           al::isNerve(pukupuku, &NrvPukupuku.CaptureJumpOut) ||
           al::isNerve(pukupuku, &NrvPukupuku.CaptureWaitGround) ||
           al::isNerve(pukupuku, &NrvPukupuku.CaptureJumpGround) ||
           al::isNerve(pukupuku, &NrvPukupuku.CaptureLandGround) ||
           al::isNerve(pukupuku, &NrvPukupuku.CaptureAttack) ||
           al::isNerve(pukupuku, &NrvPukupuku.CaptureRollingL) ||
           al::isNerve(pukupuku, &NrvPukupuku.CaptureRollingR);
}

void FUN_710017605c(f32 rate, Pukupuku* pukupuku) {
    sead::Vector3f frontDir;
    al::calcFrontDir(&frontDir, pukupuku);

    sead::Quatf quat;
    if (al::isParallelDirection(sead::Vector3f::ey, frontDir))
        al::makeQuatUpNoSupport(&quat, sead::Vector3f::ey);
    else
        al::makeQuatUpFront(&quat, sead::Vector3f::ey, frontDir);

    al::slerpQuat(al::getQuatPtr(pukupuku), al::getQuat(pukupuku), quat, rate);
}

// bool Pukupuku::receiveMsg(const al::SensorMsg* message, al::HitSensor* other, al::HitSensor*
// self) {}

void Pukupuku::endCapture() {
    FUN_710017605c(1.0f, this);

    sead::Vector3f frontDir;
    al::calcFrontDir(&frontDir, this);

    sead::Vector3f pos =
        al::getTrans(this) + g_7101e62d50.y * sead::Vector3f::ey + g_7101e62d50.z * frontDir;

    rs::endHackFromTargetPos(&_118, pos, frontDir);
    al::validateClipping(this);
    al::offCollide(this);
    al::onGroupClipping(this);
    al::showModelIfHide(this);

    _168.set(sead::Vector3f::zero);
    _150 = 0;
}

void Pukupuku::revive(s32 hitType) {
    if (al::isNerve(this, &NrvPukupuku.Revive))
        return;

    al::setNerve(this, &NrvPukupuku.Revive);

    switch (hitType) {
    case 2:
        al::startHitReaction(this, "死亡");
        break;
    case 1:
        al::startHitReaction(this, "消滅");
        break;
    default:
        break;
    }
}

void Pukupuku::startCapture() {
    al::invalidateClipping(this);
    al::onCollide(this);
    al::offGroupClipping(this);
    al::addDemoActor(al::tryGetSubActor(this, "ライト"));
    rs::startHackStartDemo(_118, this);
}

void Pukupuku::updateEffectWaterSurface() {
    if (mWaterSurfaceFinder->isFoundSurface()) {
        sead::Vector3f frontDir;
        al::calcFrontDir(&frontDir, this);
        al::makeMtxUpFrontPos(&mSwimSurfaceTraceEffectFollowMtx, sead::Vector3f::ey, frontDir,
                              mWaterSurfaceFinder->getSurfacePosition());
    }

    if ((al::isNerve(this, &NrvPukupuku.CaptureSwimStart) ||
         al::isNerve(this, &NrvPukupuku.CaptureSwim) ||
         al::isNerve(this, &NrvPukupuku.CaptureSwimDash)) &&
        mWaterSurfaceFinder->isNearSurface(100.0f)) {
        if (!al::isEffectEmitting(this, "SwimSurfaceTrace"))
            al::emitEffect(this, "SwimSurfaceTrace", nullptr);
    } else if (al::isEffectEmitting(this, "SwimSurfaceTrace")) {
        al::deleteEffect(this, "SwimSurfaceTrace");
    }
}

#pragma once

#include <container/seadOffsetList.h>

#include "Library/LiveActor/LiveActor.h"

namespace al {
class CollisionPartsFilterSpecialPurpose;
class EnemyStateBlowDown;
class JointSpringControllerHolder;
class WaterSurfaceFinder;
}  // namespace al

class ActorStateSandGeyser;
class CapTargetInfo;
class CollisionMultiShape;
class CollisionShapeKeeper;
class DisregardReceiver;
class EnemyCap;
class EnemyStateReset;
class EnemyStateSwoon;
class EnemyStateWander;
class KuriboStateHack;
class PlayerCeilingCheck;
class PlayerPushReceiver;

class KuriboHack : public al::LiveActor {
public:
    static f32 getRideOnRowSize();

    KuriboHack(const char* name);

    void init(const al::ActorInitInfo& info) override;
    void initAfterPlacement() override;
    void appear() override;
    void makeActorAlive() override;
    void kill() override;
    void makeActorDead() override;
    void movement() override;
    void calcAnim() override;
    void startClipped() override;
    void endClipped() override;
    void attackSensor(al::HitSensor* self, al::HitSensor* other) override;
    bool receiveMsg(const al::SensorMsg* message, al::HitSensor* other,
                    al::HitSensor* self) override;
    void control() override;
    void updateCollider() override;

    bool checkSandSinkPrecisely() const;
    s32 indexInHostList() const;
    bool isCapWorn() const;
    bool isEnableHack() const;
    bool isSinking() const;

    void addCapToHackDemo();
    void appearFall();
    void applyRideOnQuat(const sead::Quatf& quat);
    void clearSink();
    void detach(KuriboHack* kuribo);
    void eraseFromHost();
    void forceStartClipped();
    void invalidateHackDamage();
    bool isInvalidHackDamage() const;
    void noRevive();
    void notifyJumpSink(f32);
    void notifyKillByShineGetToGroup(const al::SensorMsg* message, al::HitSensor* other,
                                     al::HitSensor* self);
    void offDynamics();
    void offSnapShotMode();
    void onDynamics();
    void onSnapShotMode();
    void prepareKillByShineGet();
    void pushFrom(KuriboHack* kuribo, const sead::Vector3f& up);
    void resetRideOnPosBottom(f32);
    void resetRideOnPosBottomWithDefaultParam();
    void setNerveRideOnCommon();
    void setShiftTypeOnGround(s32);
    void shiftWaitHack();
    void solveCollisionInHacking(const sead::Vector3f&);
    void startRideOnRotation();
    void syncRideOnPosBottom(f32, f32);
    void syncRideOnPosBottomWithDefaultParam();
    void transferGroup(sead::OffsetList<KuriboHack>* dst);
    void tryCreateEnemyCap(const al::ActorInitInfo& info);
    bool tryReceiveMsgEatBind(const al::SensorMsg* message, al::HitSensor* other,
                              al::HitSensor* self);
    bool tryReceiveMsgHack(const al::SensorMsg* message, al::HitSensor* other, al::HitSensor* self);
    bool tryReceiveMsgNormal(const al::SensorMsg* message, al::HitSensor* other,
                             al::HitSensor* self);
    bool tryReceiveMsgPush(const al::SensorMsg* message, al::HitSensor* other, al::HitSensor* self);
    bool tryReceiveMsgRideOn(const al::SensorMsg* message, al::HitSensor* other,
                             al::HitSensor* self);
    bool tryReceiveMsgWaitHack(const al::SensorMsg* message, al::HitSensor* other,
                               al::HitSensor* self);
    bool tryRideOnHack(const al::SensorMsg* message, al::HitSensor* other, al::HitSensor* self);
    void trySetHipDropActor(const al::SensorMsg* message, al::HitSensor* other);
    __attribute__((noinline)) bool tryShiftChaseOrWander();
    bool tryShiftDrown();
    void updateCapLockOnMtx();
    bool updateSink();
    void validateHipDropProbe(al::HitSensor* hitSensor);
    void validateSpecialPush(u32);

    void exeWander();
    void exeWaitHack();
    void exeReset();
    void exeBlowDown();
    void exeSandGeyser();
    void exeHack();
    void endHack();
    void exeRideOn();
    void endRideOn();
    void exePressDown();
    void exeEatBind();
    void exeTurn();
    void exeFind();
    void exeChase();
    void exeStop();
    void exeAttack();
    void exeWait();
    void exeLand();
    void exeTowerHackEnd();
    void exeSink();
    void exeFall();
    void exeSlide();
    void exeDamageCap();
    void exeDrown();

    // private:
    CapTargetInfo* mCapTargetInfo = nullptr;
    EnemyStateSwoon* mEnemyStateSwoon = nullptr;
    EnemyStateReset* mEnemyStateReset = nullptr;
    EnemyStateWander* mEnemyStateWander = nullptr;
    ActorStateSandGeyser* mActorStateSandGeyser = nullptr;
    KuriboStateHack* mKuriboStateHack = nullptr;
    al::EnemyStateBlowDown* mEnemyStateBlowDown = nullptr;
    al::JointSpringControllerHolder* mJointSpringControllerHolder = nullptr;
    bool _148 = false;
    EnemyCap* mEnemyCap = nullptr;
    bool mIsEyebrowOff = false;
    KuriboHack* mHost = nullptr;
    bool mIsGold = false;
    s32 _16c = 0;
    al::CollisionPartsFilterSpecialPurpose* mCollisionPartsFilterSpecialPurpose = nullptr;
    unsigned char padding1[0xc] = {0};
    f32 mClippingRadius = 0.0f;
    PlayerPushReceiver* mPlayerPushReceiver = nullptr;
    CollisionMultiShape* mCollisionMultiShape = nullptr;
    CollisionShapeKeeper* mCollisionShapeKeeper = nullptr;
    f32 _1a0 = 0.0f;
    u32 _1a4 = 0;
    u32 mRideOnRotationFrame = 0;
    s32 _1ac = 0;
    u32 _1b0 = 0;
    s32 mTypeOnGround = 1;
    s32 _1b8 = 0;
    f32 _1bc = 0.0f;
    PlayerCeilingCheck* mPlayerCeilingCheck = nullptr;
    bool _1c8 = false;
    sead::Vector3f _1cc = {0.0f, 0.0f, 0.0f};
    s32 _1d8 = 0;
    const char* mJointName = nullptr;
    sead::Matrix34f _1e8 = sead::Matrix34f::ident;
    sead::Matrix34f _218 = sead::Matrix34f::ident;
    al::LiveActor* mHipDropActor = nullptr;
    sead::Vector3f _250 = {0.0f, 0.0f, 0.0f};
    s32 _25c = 0;
    al::HitSensor* mHipDropProbeSensor = nullptr;
    sead::Vector3f mHipDropProbeSensorPos = {0.0f, 0.0f, 0.0f};
    s32 mHackDamageFrame = 0;
    al::WaterSurfaceFinder* mWaterSurfaceFinder = nullptr;
    sead::Matrix34f mWaterSurfaceEffectMtx = sead::Matrix34f::ident;
    sead::Matrix34f mSandSurfaceEffectMtx = sead::Matrix34f::ident;
    bool _2e0 = true;
    sead::OffsetList<KuriboHack> _2e8 = {};
    void* padding2 = nullptr;
    void* padding3 = nullptr;
    DisregardReceiver* mDisregardReceiver = nullptr;
    u32 _318 = 0;
};

static_assert(sizeof(KuriboHack) == 0x320);

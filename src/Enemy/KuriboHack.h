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
    void makeActorAlive() override;
    void makeActorDead() override;
    void updateCollider() override;
    void appear() override;
    void kill() override;
    void calcAnim() override;
    void movement() override;
    void startClipped() override;
    void control() override;
    void endClipped() override;
    void attackSensor(al::HitSensor* self, al::HitSensor* other) override;
    bool receiveMsg(const al::SensorMsg* message, al::HitSensor* other,
                    al::HitSensor* self) override;

    bool checkSandSinkPrecisely() const;
    bool isSinking() const;
    s32 indexInHostList() const;
    bool isCapWorn() const;
    bool isEnableHack() const;

    void tryCreateEnemyCap(const al::ActorInitInfo& info);
    void setNerveRideOnCommon();
    void resetRideOnPosBottomWithDefaultParam();
    void onSnapShotMode();
    void offSnapShotMode();
    void onDynamics();
    void detach(KuriboHack* kuribo);
    void solveCollisionInHacking(const sead::Vector3f&);
    void pushFrom(KuriboHack* kuribo, const sead::Vector3f& up);
    void updateCapLockOnMtx();
    void forceStartClipped();
    void appearFall();
    void noRevive();
    void setShiftTypeOnGround(s32);
    void offDynamics();
    void syncRideOnPosBottomWithDefaultParam();
    bool tryShiftDrown();
    bool tryShiftChaseOrWander();
    bool updateSink();
    void clearSink();
    void invalidateHackDamage();
    void shiftWaitHack();
    void endHack();
    void endRideOn();
    void prepareKillByShineGet();
    bool receiveMsgHack(const al::SensorMsg* message, al::HitSensor* other, al::HitSensor* self);
    bool receiveMsgWaitHack(const al::SensorMsg* message, al::HitSensor* other,
                            al::HitSensor* self);
    bool receiveMsgRideOn(const al::SensorMsg* message, al::HitSensor* other, al::HitSensor* self);
    bool receiveMsgEatBind(const al::SensorMsg* message, al::HitSensor* other, al::HitSensor* self);
    bool receiveMsgNormal(const al::SensorMsg* message, al::HitSensor* other, al::HitSensor* self);
    void transferGroup(sead::OffsetList<KuriboHack>* dst);
    void eraseFromHost();
    void notifyJumpSink(f32);
    void syncRideOnPosBottom(f32, f32);
    void resetRideOnPosBottom(f32);
    void validateSpecialPush(u32);
    void startRideOnRotation();
    void applyRideOnQuat(const sead::Quatf& quat);
    bool isInvalidHackDamage() const;
    void validateHipDropProbe(al::HitSensor*);
    void trySetHipDropActor(const al::SensorMsg* message, al::HitSensor* other);
    void addCapToHackDemo();
    bool tryReceiveMsgPush(const al::SensorMsg* message, al::HitSensor* other, al::HitSensor* self);
    bool tryRideOnHack(const al::SensorMsg* message, al::HitSensor* other, al::HitSensor* self);
    bool notifyKillByShineGetToGroup(const al::SensorMsg* message, al::HitSensor* other,
                                     al::HitSensor* self);

    void exeWander();
    void exeWait();
    void exeTurn();
    void exeFind();
    void exeChase();
    void exeStop();
    void exeAttack();
    void exePressDown();
    void exeBlowDown();
    void exeDamageCap();
    void exeFall();
    void exeLand();
    void exeSink();
    void exeSlide();
    void exeReset();
    void exeSandGeyser();
    void exeWaitHack();
    void exeTowerHackEnd();
    void exeHack();
    void exeRideOn();
    void exeDrown();
    void exeEatBind();

private:
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
    KuriboHack* _160 = nullptr;
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
    s32 mRideOnRotationFrame = 0;
    s32 _1ac = 0;
    u32 _1b0 = 0;
    s32 _1b4 = 1;
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
    void* padding7 = nullptr;
    void* padding8 = nullptr;
    DisregardReceiver* mDisregardReceiver = nullptr;
    u32 _318 = 0;
};

static_assert(sizeof(KuriboHack) == 0x320);

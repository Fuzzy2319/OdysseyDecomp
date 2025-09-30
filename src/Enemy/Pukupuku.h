#pragma once

#include <container/seadPtrArray.h>

#include "Library/LiveActor/LiveActor.h"

namespace al {
class WaterSurfaceFinder;
class EnemyStateBlowDown;
class JointRippleGenerator;
class JointSpringController;
}  // namespace al

class CapTargetInfo;
class IUsePlayerHack;
class EnemyStateReviveInsideScreen;
class EnemyStateSwoon;
class HackerStateNormalJump;
class PlayerHackStartShaderCtrl;
class HackerDepthShadowMapCtrl;

class Pukupuku : public al::LiveActor {
public:
    Pukupuku(const char* name);

    void init(const al::ActorInitInfo& info) override;
    void initAfterPlacement() override;
    void attackSensor(al::HitSensor* self, al::HitSensor* other) override;
    bool receiveMsg(const al::SensorMsg* message, al::HitSensor* other,
                    al::HitSensor* self) override;
    void control() override;

    f32 getAccel(IUsePlayerHack*) const;
    bool isNerveInWater() const;
    bool isSwimTypeA() const;
    bool isTriggerSwimDash() const;

    void endCapture();
    void revive(s32 hitType);
    void startCapture();
    void updateEffectWaterSurface();
    void updateWaterCondition();
    void updateInputRolling();
    void updateInputKiss();
    void updateInputUpDown();
    void updateVelocity();
    bool checkCollidedFloorDamageAndNextNerve();
    void onWaterOut();
    bool tryAddVelocityWaterSurfaceJumpOut();
    void approachSurface();
    bool updatePoseSwim();
    void onWaterIn();
    bool checkJumpOutCondition();
    void updateCameraCaptureWait();
    bool updateGroundTimeLimit();

    void exeReaction();
    void exeWaitRollingRail();
    void exeWait();
    void exeWaitTurnToRailDir();
    void exeSwoon();
    void exeCaptureStart();
    void exeCaptureStartEnd();
    void exeCaptureSwimStart();
    void exeCaptureSwim();
    void exeCaptureReactionWall();
    void exeCaptureWait();
    void exeCaptureAttack();
    void exeCaptureRolling();
    void exeCaptureWaitGround();
    void exeCaptureJumpGround();
    void exeCaptureLandGround();
    void exeBlowDown();
    void exeTrample();
    void exeRevive();
    void exeDemoWaitToRevive();

private:
    al::WaterSurfaceFinder* mWaterSurfaceFinder = nullptr;
    CapTargetInfo* mCapTargetInfo = nullptr;
    IUsePlayerHack* _118 = nullptr;
    EnemyStateReviveInsideScreen* mEnemyStateReviveInsideScreen = nullptr;
    EnemyStateSwoon* mEnemyStateSwoon = nullptr;
    HackerStateNormalJump* mHackerStateNormalJump = nullptr;
    al::EnemyStateBlowDown* mEnemyStateBlowDown = nullptr;
    sead::Vector3f _140 = sead::Vector3f::ez;
    s32 mMoveType = 0;  // enum?
    s32 _150 = 0;
    s32 _154 = 0;
    s32 _158 = 0;
    al::JointRippleGenerator* mJointRippleGenerator = nullptr;
    sead::Vector3f _168 = sead::Vector3f::zero;
    unsigned char unkType[4] = {0};
    sead::Vector3f _178 = sead::Vector3f::ez;
    f32 _184 = 0.0f;
    sead::PtrArray<al::JointSpringController> mJointSpringControllers;
    s32 mRailPointNo = 0;
    bool _19c = true;
    bool _19d = false;
    sead::Matrix34f _1a0 = sead::Matrix34f::ident;
    sead::Matrix34f mSwimSurfaceTraceEffectFollowMtx = sead::Matrix34f::ident;
    sead::Matrix34f mWaterAreaInEffectFollowMtx = sead::Matrix34f::ident;
    sead::Matrix34f mWaterAreaOutEffectFollowMtx = sead::Matrix34f::ident;
    sead::Matrix34f mWaterSurfaceEffectFollowMtx = sead::Matrix34f::ident;
    sead::Vector3f _290 = sead::Vector3f::zero;
    sead::Vector3f _29c = sead::Vector3f::zero;
    sead::Vector3f _2a8 = sead::Vector3f::zero;
    f32 _2b4 = 0.0f;
    f32 _2b8 = 100.0f;
    bool _2bc = false;
    f32 _2c0 = 0.0f;
    bool _2c4 = false;
    bool _2c5 = false;
    s32 _2c8 = 0;
    PlayerHackStartShaderCtrl* mPlayerHackStartShaderCtrl = nullptr;
    bool mIsTriggerSwimDash = false;
    bool _2d9 = false;
    s32 _2dc = 0;
    s32 _2e0 = 0;
    bool mIsPukupukuSnow = false;
    sead::Vector3f _2e8 = sead::Vector3f::zero;
    bool _2f4 = true;
    s32 _2f8 = 0;
    HackerDepthShadowMapCtrl* mHackerDepthShadowMapCtrl = nullptr;
};

static_assert(sizeof(Pukupuku) == 0x308);

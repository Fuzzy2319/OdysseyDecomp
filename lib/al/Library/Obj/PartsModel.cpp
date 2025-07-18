#include "Library/Obj/PartsModel.h"

#include "Library/Base/StringUtil.h"
#include "Library/LiveActor/ActorClippingFunction.h"
#include "Library/LiveActor/ActorFlagFunction.h"
#include "Library/LiveActor/ActorInitFunction.h"
#include "Library/LiveActor/ActorInitUtil.h"
#include "Library/LiveActor/ActorModelFunction.h"
#include "Library/LiveActor/ActorPoseUtil.h"
#include "Library/LiveActor/ActorResourceFunction.h"
#include "Library/LiveActor/LiveActorFunction.h"
#include "Library/Math/MathUtil.h"
#include "Library/Matrix/MatrixUtil.h"
#include "Library/Yaml/ByamlUtil.h"

namespace al {
PartsModel::PartsModel(const char* name) : LiveActor(name) {}

void PartsModel::endClipped() {
    LiveActor::endClipped();
    updatePose();
}

void PartsModel::calcAnim() {
    updatePose();
    LiveActor::calcAnim();
}

void PartsModel::attackSensor(HitSensor* self, HitSensor* other) {
    mParentModel->attackSensor(self, other);
}

bool PartsModel::receiveMsg(const SensorMsg* message, HitSensor* other, HitSensor* self) {
    return mParentModel->receiveMsg(message, other, self);
}

void PartsModel::initPartsDirect(LiveActor* parent, const ActorInitInfo& initInfo,
                                 const char* arcName, const sead::Matrix34f* jointMtx,
                                 const sead::Vector3f& localTrans,
                                 const sead::Vector3f& localRotate,
                                 const sead::Vector3f& localScale, bool useFollowMtxScale) {
    mParentModel = parent;
    mJointMtx = jointMtx;
    mIsUseFollowMtxScale = useFollowMtxScale;
    initChildActorWithArchiveNameNoPlacementInfo(this, initInfo, arcName, nullptr);
    invalidateClipping(this);
    registerSubActor(parent, this);
    makeActorAlive();
    mIsUseLocalPos = true;
    mLocalTrans = localTrans;
    mLocalRotate = localRotate;
    mLocalScale = localScale;
}

void PartsModel::initPartsSuffix(LiveActor* parent, const ActorInitInfo& initInfo,
                                 const char* arcName, const char* suffix,
                                 const sead::Matrix34f* jointMtx, bool useFollowMtxScale) {
    mParentModel = parent;
    mJointMtx = jointMtx;
    mIsUseFollowMtxScale = useFollowMtxScale;
    initChildActorWithArchiveNameNoPlacementInfo(this, initInfo, arcName, suffix);
    invalidateClipping(this);
    registerSubActor(parent, this);
    makeActorAlive();
}

void PartsModel::initPartsMtx(LiveActor* parent, const ActorInitInfo& initInfo, const char* arcName,
                              const sead::Matrix34f* jointMtx, bool useFollowMtxScale) {
    mParentModel = parent;
    mJointMtx = jointMtx;
    mIsUseFollowMtxScale = useFollowMtxScale;
    initChildActorWithArchiveNameNoPlacementInfo(this, initInfo, arcName, nullptr);
    invalidateClipping(this);
    registerSubActor(parent, this);
    makeActorAlive();
}

void PartsModel::initPartsFixFile(LiveActor* parent, const ActorInitInfo& initInfo,
                                  const char* arcName, const char* arcSuffix, const char* suffix) {
    initPartsFixFileNoRegister(parent, initInfo, arcName, arcSuffix, suffix);
    registerSubActor(parent, this);
}

void PartsModel::initPartsFixFileNoRegister(LiveActor* parent, const ActorInitInfo& initInfo,
                                            const char* arcName, const char* arcSuffix,
                                            const char* suffix) {
    mParentModel = parent;
    mJointMtx = parent->getBaseMtx();

    initChildActorWithArchiveNameNoPlacementInfo(this, initInfo, arcName, arcSuffix);
    invalidateClipping(this);

    sead::FixedSafeString<0x80> initArcName;
    createFileNameBySuffix(&initArcName, "InitPartsFixInfo", suffix);

    if (!isExistModelResourceYaml(mParentModel, initArcName.cstr(), nullptr))
        return makeActorAlive();
    const u8* modelResByml = getModelResourceYaml(mParentModel, initArcName.cstr(), nullptr);
    ByamlIter modelResIter(modelResByml);

    const char* jointName = nullptr;
    modelResIter.tryGetStringByKey(&jointName, "JointName");

    if (jointName)
        mJointMtx = getJointMtxPtr(mParentModel, jointName);

    tryGetByamlV3f(&mLocalTrans, modelResIter, "LocalTrans");
    tryGetByamlV3f(&mLocalRotate, modelResIter, "LocalRotate");
    tryGetByamlV3f(&mLocalScale, modelResIter, "LocalScale");

    mIsUseLocalScale = tryGetByamlKeyBoolOrFalse(modelResIter, "UseLocalScale");

    if (!isNearZero(mLocalTrans) || !isNearZero(mLocalRotate) || mIsUseLocalScale)
        mIsUseLocalPos = true;

    mIsUseFollowMtxScale = tryGetByamlKeyBoolOrFalse(modelResIter, "UseFollowMtxScale");

    makeActorAlive();
}

void PartsModel::updatePose() {
    if (!mIsUpdate)
        return;

    if (!mIsUseLocalPos) {
        sead::Matrix34f baseMtx = *mJointMtx;
        if (mIsUseFollowMtxScale) {
            sead::Vector3f mtxScale;
            calcMtxScale(&mtxScale, baseMtx);
            const sead::Vector3f& scale = sead::Vector3f::ones;
            mtxScale.x *= scale.x;
            mtxScale.y *= scale.y;
            mtxScale.z *= scale.z;
            setScale(this, mtxScale);
        }
        normalize(&baseMtx);
        updatePoseMtx(this, &baseMtx);
        return;
    }

    sead::Matrix34f rotationMatrix;
    sead::Vector3f rotate(sead::Mathf::deg2rad(mLocalRotate.x),
                          sead::Mathf::deg2rad(mLocalRotate.y),
                          sead::Mathf::deg2rad(mLocalRotate.z));
    rotationMatrix.makeR(rotate);

    sead::Matrix34f translationMatrix;
    translationMatrix.makeRT({0.0f, 0.0f, 0.0f}, mLocalTrans);

    sead::Matrix34f poseMatrix = rotationMatrix * translationMatrix;

    sead::Matrix34f baseMtx = *mJointMtx;

    if (mIsUseFollowMtxScale) {
        sead::Matrix34f rotBaseMtx = baseMtx * rotationMatrix;

        sead::Vector3f mtxScale = {0.0f, 0.0f, 0.0f};
        calcMtxScale(&mtxScale, rotBaseMtx);

        poseMatrix.m[0][3] *= mtxScale.x;
        poseMatrix.m[1][3] *= mtxScale.y;
        poseMatrix.m[2][3] *= mtxScale.z;

        if (mIsUseLocalScale) {
            mtxScale.x *= mLocalScale.x;
            mtxScale.y *= mLocalScale.y;
            mtxScale.z *= mLocalScale.z;
        }

        setScale(this, mtxScale);
    } else if (mIsUseLocalScale) {
        setScale(this, mLocalScale);
    }

    normalize(&baseMtx);
    baseMtx = baseMtx * poseMatrix;
    updatePoseMtx(this, &baseMtx);
}

void PartsModel::offSyncAppearAndHide() {
    offSyncAppearSubActor(mParentModel, this);
    offSyncHideSubActor(mParentModel, this);
}

void PartsModel::onSyncAppearAndHide() {
    onSyncHideSubActor(mParentModel, this);

    if (isHideModel(mParentModel))
        hideModelIfShow(this);
    else
        showModelIfHide(this);

    onSyncAppearSubActor(mParentModel, this);

    if (isDead(mParentModel) && isAlive(this))
        makeActorDead();
    else if (isAlive(mParentModel) && isDead(this))
        makeActorAlive();
}
}  // namespace al

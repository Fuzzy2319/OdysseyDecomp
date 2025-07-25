#include "Library/Placement/PlacementFunction.h"

#include "Library/Area/AreaInitInfo.h"
#include "Library/Base/StringUtil.h"
#include "Library/LiveActor/ActorInitInfo.h"
#include "Library/Math/Axis.h"
#include "Library/Math/MathUtil.h"
#include "Library/Placement/PlacementId.h"
#include "Library/Placement/PlacementInfo.h"
#include "Library/Yaml/ByamlIter.h"
#include "Library/Yaml/ByamlUtil.h"

namespace al {

bool isValidInfo(const PlacementInfo& placementInfo) {
    return placementInfo.getPlacementIter().isValid();
}

bool isPlaced(const ActorInitInfo& initInfo) {
    return isValidInfo(*initInfo.placementInfo);
}

void getObjectName(const char** name, const ActorInitInfo& initInfo) {
    getObjectName(name, *initInfo.placementInfo);
}

void getObjectName(const char** name, const PlacementInfo& placementInfo) {
    tryGetObjectName(name, placementInfo);
}

bool tryGetObjectName(const char** name, const ActorInitInfo& initInfo) {
    return tryGetObjectName(name, *initInfo.placementInfo);
}

bool tryGetObjectName(const char** name, const PlacementInfo& placementInfo) {
    const char* obj = "";
    if (tryGetStringArg(&obj, placementInfo, "UnitConfigName")) {
        *name = obj;
        return true;
    }
    return false;
}

bool isObjectName(const ActorInitInfo& initInfo, const char* name) {
    return isObjectName(*initInfo.placementInfo, name);
}

bool isObjectName(const PlacementInfo& placementInfo, const char* name) {
    const char* obj;
    return tryGetObjectName(&obj, placementInfo) && isEqualString(obj, name);
}

bool isObjectNameSubStr(const ActorInitInfo& initInfo, const char* name) {
    return isObjectNameSubStr(*initInfo.placementInfo, name);
}

bool isObjectNameSubStr(const PlacementInfo& placementInfo, const char* name) {
    const char* obj;
    return tryGetObjectName(&obj, placementInfo) && isEqualSubString(obj, name);
}

void getClassName(const char** name, const ActorInitInfo& initInfo) {
    getClassName(name, *initInfo.placementInfo);
}

void getClassName(const char** name, const PlacementInfo& placementInfo) {
    tryGetClassName(name, placementInfo);
}

bool tryGetClassName(const char** name, const ActorInitInfo& initInfo) {
    return tryGetClassName(name, *initInfo.placementInfo);
}

bool tryGetClassName(const char** name, const PlacementInfo& placementInfo) {
    PlacementInfo unitConfig;
    if (!tryGetPlacementInfoByKey(&unitConfig, placementInfo, "UnitConfig"))
        return false;
    return tryGetStringArg(name, unitConfig, "ParameterConfigName");
}

bool isClassName(const ActorInitInfo& initInfo, const char* name) {
    return isClassName(*initInfo.placementInfo, name);
}

bool isClassName(const PlacementInfo& placementInfo, const char* name) {
    const char* className = nullptr;
    return tryGetClassName(&className, placementInfo) && isEqualString(className, name);
}

void getDisplayName(const char** name, const ActorInitInfo& initInfo) {
    getDisplayName(name, *initInfo.placementInfo);
}

void getDisplayName(const char** name, const PlacementInfo& placementInfo) {
    tryGetDisplayName(name, placementInfo);
}

bool tryGetDisplayName(const char** name, const ActorInitInfo& initInfo) {
    return tryGetDisplayName(name, *initInfo.placementInfo);
}

bool tryGetDisplayName(const char** name, const PlacementInfo& placementInfo) {
    PlacementInfo unitConfig;
    if (!tryGetPlacementInfoByKey(&unitConfig, placementInfo, "UnitConfig"))
        return false;
    return tryGetStringArg(name, unitConfig, "DisplayName");
}

void getPlacementTargetFile(const char** targetFile, const PlacementInfo& placementInfo) {
    PlacementInfo unitConfig;
    getPlacementInfoByKey(&unitConfig, placementInfo, "UnitConfig");
    tryGetStringArg(targetFile, unitConfig, "PlacementTargetFile");
}

void getTrans(sead::Vector3f* trans, const ActorInitInfo& initInfo) {
    getTrans(trans, *initInfo.placementInfo);
}

void getTrans(sead::Vector3f* trans, const PlacementInfo& placementInfo) {
    tryGetTrans(trans, placementInfo);
}

void multZoneMtx(sead::Vector3f* trans, const PlacementInfo& placementInfo) {
    sead::Matrix34f mtx;
    if (tryGetZoneMatrixTR(&mtx, placementInfo))
        trans->mul(mtx);
}

bool tryGetTrans(sead::Vector3f* trans, const ActorInitInfo& initInfo) {
    return tryGetTrans(trans, *initInfo.placementInfo);
}

bool tryGetTrans(sead::Vector3f* trans, const PlacementInfo& placementInfo) {
    if (!tryGetArgV3f(trans, placementInfo, "Translate"))
        return false;
    multZoneMtx(trans, placementInfo);
    return true;
}

void getRotate(sead::Vector3f* rotate, const PlacementInfo& placementInfo) {
    tryGetRotate(rotate, placementInfo);
}

bool tryGetRotate(sead::Vector3f* rotate, const ActorInitInfo& initInfo) {
    return tryGetRotate(rotate, *initInfo.placementInfo);
}

template <typename T>
inline void getRotation(sead::Vector3f* v, const sead::Matrix34f& n) {
    T abs = sead::MathCalcCommon<T>::abs(n.m[2][0]);

    // making sure pitch stays within bounds, setting roll to 0 otherwise
    if ((1.0f - abs) < sead::MathCalcCommon<T>::epsilon() * 10) {
        const T a12 = n.m[0][1];
        const T a13 = n.m[0][2];
        const T a31 = n.m[2][0];

        v->x = 0.0f;
        v->y = (a31 / abs) * (-sead::numbers::pi_v<T> / 2);
        v->z = std::atan2(-a12, -(a31 * a13));
    } else {
        const T a11 = n.m[0][0];
        const T a21 = n.m[1][0];
        const T a31 = n.m[2][0];
        const T a32 = n.m[2][1];
        const T a33 = n.m[2][2];

        v->x = std::atan2(a32, a33);
        v->y = std::asin(-a31);
        v->z = std::atan2(a21, a11);
    }
}

bool tryGetRotate(sead::Vector3f* rotate, const PlacementInfo& placementInfo) {
    if (!tryGetArgV3f(rotate, placementInfo, "Rotate"))
        return false;

    sead::Matrix34f mtx;
    if (tryGetZoneMatrixTR(&mtx, placementInfo)) {
        sead::Matrix34f rot, rot2;
        sead::Vector3f vec1 = {sead::Mathf::deg2rad(rotate->x), sead::Mathf::deg2rad(rotate->y),
                               sead::Mathf::deg2rad(rotate->z)};
        rot.makeRT(vec1, sead::Vector3f::zero);
        rot2 = mtx * rot;

        getRotation<f32>(rotate, rot2);
        rotate->set(sead::Mathf::rad2deg(rotate->x), sead::Mathf::rad2deg(rotate->y),
                    sead::Mathf::rad2deg(rotate->z));
    }
    return true;
}

bool tryGetZoneMatrixTR(sead::Matrix34f* matrix, const PlacementInfo& placementInfo) {
    ByamlIter zone = placementInfo.getZoneIter();
    if (!zone.isValid())
        return false;

    sead::Vector3f translate = sead::Vector3f::zero;
    if (!tryGetByamlV3f(&translate, zone, "Translate"))
        return false;

    sead::Vector3f rotate = sead::Vector3f::zero;
    if (!tryGetByamlV3f(&rotate, zone, "Rotate"))
        return false;

    matrix->makeRT({sead::Mathf::rad2deg(rotate.x), sead::Mathf::rad2deg(rotate.y),
                    sead::Mathf::rad2deg(rotate.z)},
                   translate);
    return true;
}

void getQuat(sead::Quatf* quat, const ActorInitInfo& initInfo) {
    tryGetQuat(quat, initInfo);
}

void getQuat(sead::Quatf* quat, const PlacementInfo& placementInfo) {
    tryGetQuat(quat, placementInfo);
}

bool tryGetQuat(sead::Quatf* quat, const ActorInitInfo& initInfo) {
    return tryGetQuat(quat, *initInfo.placementInfo);
}

bool tryGetQuat(sead::Quatf* quat, const PlacementInfo& placementInfo) {
    sead::Vector3f rotate = sead::Vector3f::zero;
    if (!tryGetRotate(&rotate, placementInfo)) {
        *quat = sead::Quatf::unit;
        return false;
    }
    quat->setRPY(sead::Mathf::deg2rad(rotate.x), sead::Mathf::deg2rad(rotate.y),
                 sead::Mathf::deg2rad(rotate.z));
    return true;
}

void getScale(sead::Vector3f* scale, const PlacementInfo& placementInfo) {
    tryGetScale(scale, placementInfo);
}

void getScale(f32* x, f32* y, f32* z, const PlacementInfo& placementInfo) {
    sead::Vector3f scale = {0.0f, 0.0f, 0.0f};
    tryGetScale(&scale, placementInfo);

    if (x)
        *x = scale.x;
    if (y)
        *y = scale.y;
    if (z)
        *z = scale.z;
}

bool tryGetScale(sead::Vector3f* scale, const ActorInitInfo& initInfo) {
    return tryGetScale(scale, *initInfo.placementInfo);
}

bool tryGetScale(sead::Vector3f* scale, const PlacementInfo& placementInfo) {
    return tryGetArgV3f(scale, placementInfo, "Scale");
}

void getSide(sead::Vector3f* side, const ActorInitInfo& initInfo) {
    tryGetSide(side, initInfo);
}

void getSide(sead::Vector3f* side, const PlacementInfo& placementInfo) {
    tryGetSide(side, placementInfo);
}

bool tryGetSide(sead::Vector3f* side, const ActorInitInfo& initInfo) {
    return tryGetSide(side, *initInfo.placementInfo);
}

bool tryGetSide(sead::Vector3f* side, const PlacementInfo& placementInfo) {
    sead::Quatf quat = sead::Quatf::unit;
    if (!tryGetQuat(&quat, placementInfo))
        return false;
    calcQuatSide(side, quat);
    return true;
}

void getUp(sead::Vector3f* up, const ActorInitInfo& initInfo) {
    tryGetUp(up, initInfo);
}

void getUp(sead::Vector3f* up, const PlacementInfo& placementInfo) {
    tryGetUp(up, placementInfo);
}

bool tryGetUp(sead::Vector3f* up, const ActorInitInfo& initInfo) {
    return tryGetUp(up, *initInfo.placementInfo);
}

bool tryGetUp(sead::Vector3f* up, const PlacementInfo& placementInfo) {
    sead::Quatf quat = sead::Quatf::unit;
    if (!tryGetQuat(&quat, placementInfo))
        return false;
    calcQuatUp(up, quat);
    return true;
}

void getFront(sead::Vector3f* front, const ActorInitInfo& initInfo) {
    tryGetFront(front, initInfo);
}

void getFront(sead::Vector3f* front, const PlacementInfo& placementInfo) {
    tryGetFront(front, placementInfo);
}

bool tryGetFront(sead::Vector3f* front, const ActorInitInfo& initInfo) {
    return tryGetFront(front, *initInfo.placementInfo);
}

bool tryGetFront(sead::Vector3f* front, const PlacementInfo& placementInfo) {
    sead::Quatf quat = sead::Quatf::unit;
    if (!tryGetQuat(&quat, placementInfo))
        return false;
    calcQuatFront(front, quat);
    return true;
}

bool tryGetLocalAxis(sead::Vector3f* dir, const ActorInitInfo& initInfo, s32 axis) {
    return tryGetLocalAxis(dir, *initInfo.placementInfo, axis);
}

bool tryGetLocalAxis(sead::Vector3f* dir, const PlacementInfo& placementInfo, s32 axis) {
    switch ((Axis)(axis + 1)) {
    case Axis::X:
        return tryGetSide(dir, placementInfo);
    case Axis::Y:
        return tryGetUp(dir, placementInfo);
    case Axis::Z:
        return tryGetFront(dir, placementInfo);
    default:
        return false;
    }
}

bool tryGetLocalSignAxis(sead::Vector3f* dir, const ActorInitInfo& initInfo, s32 axis) {
    return tryGetLocalSignAxis(dir, *initInfo.placementInfo, axis);
}

bool tryGetLocalSignAxis(sead::Vector3f* dir, const PlacementInfo& placementInfo, s32 axis) {
    switch ((Axis)sead::Mathi::abs(axis)) {
    case Axis::X:
        tryGetSide(dir, placementInfo);
        break;
    case Axis::Y:
        tryGetUp(dir, placementInfo);
        break;
    case Axis::Z:
        tryGetFront(dir, placementInfo);
        break;
    default:
        return false;
    }
    if (axis < 0)
        *dir *= -1;
    return true;
}

bool tryGetMatrixTR(sead::Matrix34f* matrix, const ActorInitInfo& initInfo) {
    return tryGetMatrixTR(matrix, *initInfo.placementInfo);
}

bool tryGetMatrixTR(sead::Matrix34f* matrix, const PlacementInfo& placementInfo) {
    sead::Vector3f trans = sead::Vector3f::zero;
    sead::Vector3f rotate = sead::Vector3f::zero;
    if (!tryGetTrans(&trans, placementInfo))
        return false;
    if (!tryGetRotate(&rotate, placementInfo))
        return false;
    matrix->makeRT({sead::Mathf::deg2rad(rotate.x), sead::Mathf::deg2rad(rotate.y),
                    sead::Mathf::deg2rad(rotate.z)},
                   trans);
    return true;
}

bool tryGetMatrixTRS(sead::Matrix34f* matrix, const ActorInitInfo& initInfo) {
    return tryGetMatrixTRS(matrix, *initInfo.placementInfo);
}

bool tryGetMatrixTRS(sead::Matrix34f* matrix, const PlacementInfo& placementInfo) {
    sead::Vector3f trans = sead::Vector3f::zero;
    sead::Vector3f rotate = sead::Vector3f::zero;
    sead::Vector3f scale = sead::Vector3f::ones;
    if (!tryGetTrans(&trans, placementInfo))
        return false;
    if (!tryGetRotate(&rotate, placementInfo))
        return false;
    if (!tryGetScale(&scale, placementInfo))
        return false;
    matrix->makeSRT(scale,
                    {sead::Mathf::deg2rad(rotate.x), sead::Mathf::deg2rad(rotate.y),
                     sead::Mathf::deg2rad(rotate.z)},
                    trans);
    return true;
}

bool tryGetInvertMatrixTR(sead::Matrix34f* matrix, const ActorInitInfo& initInfo) {
    return tryGetInvertMatrixTR(matrix, *initInfo.placementInfo);
}

bool tryGetInvertMatrixTR(sead::Matrix34f* matrix, const PlacementInfo& placementInfo) {
    sead::Matrix34f mtx;
    if (!tryGetMatrixTR(&mtx, placementInfo))
        return false;
    matrix->setInverse(mtx);
    return true;
}

void calcMatrixMultParent(sead::Matrix34f* matrix, const ActorInitInfo& initInfo1,
                          const ActorInitInfo& initInfo2) {
    calcMatrixMultParent(matrix, *initInfo1.placementInfo, *initInfo2.placementInfo);
}

void calcMatrixMultParent(sead::Matrix34f* matrix, const PlacementInfo& placementInfo1,
                          const PlacementInfo& placementInfo2) {
    sead::Matrix34f mtx1;
    mtx1.makeIdentity();
    tryGetMatrixTR(&mtx1, placementInfo1);
    sead::Matrix34f mtx2;
    mtx2.makeIdentity();
    tryGetMatrixTR(&mtx2, placementInfo2);
    matrix->setMul(mtx2, mtx1);
}

void getArg(s32* arg, const ActorInitInfo& initInfo, const char* key) {
    getArg(arg, *initInfo.placementInfo, key);
}

void getArg(s32* arg, const PlacementInfo& placementInfo, const char* key) {
    tryGetArg(arg, placementInfo, key);
}

bool tryGetArg(s32* arg, const ActorInitInfo& initInfo, const char* key) {
    return tryGetArg(arg, *initInfo.placementInfo, key);
}

bool tryGetArg(s32* arg, const PlacementInfo& placementInfo, const char* key) {
    return placementInfo.getPlacementIter().tryGetIntByKey(arg, key);
}

void getArg(f32* arg, const ActorInitInfo& initInfo, const char* key) {
    getArg(arg, *initInfo.placementInfo, key);
}

void getArg(f32* arg, const PlacementInfo& placementInfo, const char* key) {
    tryGetArg(arg, placementInfo, key);
}

bool tryGetArg(f32* arg, const ActorInitInfo& initInfo, const char* key) {
    return tryGetArg(arg, *initInfo.placementInfo, key);
}

bool tryGetArg(f32* arg, const PlacementInfo& placementInfo, const char* key) {
    return placementInfo.getPlacementIter().tryGetFloatByKey(arg, key);
}

void getArg(bool* arg, const ActorInitInfo& initInfo, const char* key) {
    getArg(arg, *initInfo.placementInfo, key);
}

void getArg(bool* arg, const PlacementInfo& placementInfo, const char* key) {
    tryGetArg(arg, placementInfo, key);
}

bool tryGetArg(bool* arg, const ActorInitInfo& initInfo, const char* key) {
    return tryGetArg(arg, *initInfo.placementInfo, key);
}

bool tryGetArg(bool* arg, const PlacementInfo& placementInfo, const char* key) {
    return placementInfo.getPlacementIter().tryGetBoolByKey(arg, key);
}

s32 getArgS32(const ActorInitInfo& actorInitInfo, const char* key) {
    s32 arg = 0;
    getArg(&arg, actorInitInfo, key);
    return arg;
}

f32 getArgF32(const ActorInitInfo& actorInitInfo, const char* key) {
    f32 arg = 0.0f;
    getArg(&arg, actorInitInfo, key);
    return arg;
}

void getArgV3f(sead::Vector3f* arg, const ActorInitInfo& actorInitInfo, const char* key) {
    tryGetArgV3f(arg, actorInitInfo, key);
}

void getArgV3f(sead::Vector3f* arg, const PlacementInfo& placementInfo, const char* key) {
    tryGetArgV3f(arg, placementInfo, key);
}

bool tryGetArgV3f(sead::Vector3f* arg, const ActorInitInfo& actorInitInfo, const char* key) {
    return tryGetArgV3f(arg, *actorInitInfo.placementInfo, key);
}

bool tryGetArgV3f(sead::Vector3f* arg, const PlacementInfo& placementInfo, const char* key) {
    return tryGetByamlV3f(arg, placementInfo.getPlacementIter(), key);
}

bool isArgBool(const ActorInitInfo& initInfo, const char* key) {
    bool arg = false;
    getArg(&arg, initInfo, key);
    return arg;
}

bool isArgBool(const PlacementInfo& placementInfo, const char* key) {
    bool arg = false;
    getArg(&arg, placementInfo, key);
    return arg;
}

bool isArgString(const ActorInitInfo& initInfo, const char* key, const char* arg) {
    return isArgString(*initInfo.placementInfo, key, arg);
}

bool isArgString(const PlacementInfo& placementInfo, const char* key, const char* arg) {
    return isEqualString(getStringArg(placementInfo, key), arg);
}

void getStringArg(const char** arg, const ActorInitInfo& initInfo, const char* key) {
    getStringArg(arg, *initInfo.placementInfo, key);
}

void getStringArg(const char** arg, const PlacementInfo& placementInfo, const char* key) {
    tryGetStringArg(arg, placementInfo, key);
}

void getStringArg(const char** arg, const AreaInitInfo& initInfo, const char* key) {
    getStringArg(arg, (const PlacementInfo&)initInfo, key);
}

const char* getStringArg(const ActorInitInfo& initInfo, const char* key) {
    return getStringArg(*initInfo.placementInfo, key);
}

const char* getStringArg(const PlacementInfo& placementInfo, const char* key) {
    const char* str = "";
    if (!placementInfo.getPlacementIter().tryGetStringByKey(&str, key) || isEqualString("", str))
        return nullptr;
    return str;
}

const char* getStringArg(const AreaInitInfo& initInfo, const char* key) {
    return getStringArg((const PlacementInfo&)initInfo, key);
}

bool tryGetStringArg(const char** arg, const ActorInitInfo& initInfo, const char* key) {
    return tryGetStringArg(arg, *initInfo.placementInfo, key);
}

bool tryGetStringArg(const char** arg, const PlacementInfo& initInfo, const char* key) {
    const char* str = "";
    if (!initInfo.getPlacementIter().tryGetStringByKey(&str, key) || isEqualString("", str))
        return false;
    *arg = str;
    return true;
}

bool tryGetStringArg(const char** arg, const AreaInitInfo& initInfo, const char* key) {
    return tryGetStringArg(arg, (const PlacementInfo&)initInfo, key);
}

bool tryGetArgV2f(sead::Vector2f* arg, const ActorInitInfo& initInfo, const char* key) {
    return tryGetArgV2f(arg, *initInfo.placementInfo, key);
}

bool tryGetArgV2f(sead::Vector2f* arg, const PlacementInfo& initInfo, const char* key) {
    return tryGetByamlV2f(arg, initInfo.getPlacementIter(), key);
}

bool tryGetArgColor(sead::Color4f* arg, const ActorInitInfo& initInfo, const char* key) {
    return tryGetArgColor(arg, *initInfo.placementInfo, key);
}

bool tryGetArgColor(sead::Color4f* arg, const PlacementInfo& initInfo, const char* key) {
    return tryGetByamlColor(arg, initInfo.getPlacementIter(), key);
}

void getLayerConfigName(const char** name, const ActorInitInfo& initInfo) {
    getLayerConfigName(name, *initInfo.placementInfo);
}

void getLayerConfigName(const char** name, const PlacementInfo& initInfo) {
    return getStringArg(name, initInfo, "LayerConfigName");
}

bool tryGetZoneNameIfExist(const char** name, const PlacementInfo& placementInfo) {
    PlacementId id;
    getPlacementId(&id, placementInfo);
    if (!id.getUnitConfigName())
        return false;
    *name = id.getUnitConfigName();
    return true;
}

void getPlacementId(PlacementId* placementId, const PlacementInfo& placementInfo) {
    tryGetPlacementId(placementId, placementInfo);
}

bool tryGetBoolArgOrFalse(const ActorInitInfo& initInfo, const char* key) {
    bool val = false;
    if (!tryGetArg(&val, initInfo, key))
        return false;
    return val;
}

s32 getCountPlacementInfo(const PlacementInfo& placementInfo) {
    return placementInfo.getPlacementIter().getSize();
}

void getPlacementInfoByKey(PlacementInfo* outPlacementInfo, const PlacementInfo& placementInfo,
                           const char* key) {
    tryGetPlacementInfoByKey(outPlacementInfo, placementInfo, key);
}

bool tryGetPlacementInfoByKey(PlacementInfo* outPlacementInfo, const PlacementInfo& placementInfo,
                              const char* key) {
    ByamlIter iter;
    if (!placementInfo.getPlacementIter().tryGetIterByKey(&iter, key))
        return false;
    outPlacementInfo->set(iter, placementInfo.getZoneIter());
    return true;
}

void getPlacementInfoByIndex(PlacementInfo* outPlacementInfo, const PlacementInfo& placementInfo,
                             s32 index) {
    tryGetPlacementInfoByIndex(outPlacementInfo, placementInfo, index);
}

bool tryGetPlacementInfoByIndex(PlacementInfo* outPlacementInfo, const PlacementInfo& placementInfo,
                                s32 index) {
    ByamlIter iter;
    if (!placementInfo.getPlacementIter().tryGetIterByIndex(&iter, index))
        return false;
    outPlacementInfo->set(iter, placementInfo.getZoneIter());
    return true;
}

void getPlacementInfoAndKeyNameByIndex(PlacementInfo* outPlacementInfo, const char** outKey,
                                       const PlacementInfo& placementInfo, s32 index) {
    tryGetPlacementInfoAndKeyNameByIndex(outPlacementInfo, outKey, placementInfo, index);
}

bool tryGetPlacementInfoAndKeyNameByIndex(PlacementInfo* outPlacementInfo, const char** outKey,
                                          const PlacementInfo& placementInfo, s32 index) {
    ByamlIter iter;
    if (!placementInfo.getPlacementIter().tryGetIterAndKeyNameByIndex(&iter, outKey, index))
        return false;
    outPlacementInfo->set(iter, placementInfo.getZoneIter());
    return true;
}

PlacementId* createPlacementId(const ActorInitInfo& initInfo) {
    return createPlacementId(*initInfo.placementInfo);
}

PlacementId* createPlacementId(const PlacementInfo& placementInfo) {
    PlacementId* id = new PlacementId();
    id->init(placementInfo);
    return id;
}

bool tryGetPlacementId(PlacementId* placementId, const ActorInitInfo& initInfo) {
    return tryGetPlacementId(placementId, *initInfo.placementInfo);
}

bool tryGetPlacementId(PlacementId* placementId, const PlacementInfo& placementInfo) {
    return placementId->init(placementInfo);
}

void getPlacementId(PlacementId* placementId, const ActorInitInfo& initInfo) {
    getPlacementId(placementId, *initInfo.placementInfo);
}

bool isEqualPlacementId(const PlacementId& placementId, const PlacementId& otherPlacementId) {
    return PlacementId::isEqual(placementId, otherPlacementId);
}

bool isEqualPlacementId(const PlacementInfo& placementInfo,
                        const PlacementInfo& otherPlacementInfo) {
    PlacementId id1;
    if (!tryGetPlacementId(&id1, placementInfo))
        return false;
    PlacementId id2;
    if (!tryGetPlacementId(&id2, otherPlacementInfo))
        return false;
    return isEqualPlacementId(id1, id2);
}

bool isExistRail(const ActorInitInfo& initInfo, const char* linkName) {
    PlacementInfo info;
    return tryGetRailIter(&info, *initInfo.placementInfo, linkName);
}

bool tryGetRailIter(PlacementInfo* railPlacementInfo, const PlacementInfo& placementInfo,
                    const char* linkName) {
    if (!tryGetLinksInfo(railPlacementInfo, placementInfo, linkName))
        return false;
    return railPlacementInfo->getPlacementIter().isTypeContainer();
}

bool tryGetLinksInfo(PlacementInfo* railPlacementInfo, const PlacementInfo& placementInfo,
                     const char* linkName) {
    PlacementInfo links;
    if (!tryGetPlacementInfoByKey(&links, placementInfo, "Links"))
        return false;
    PlacementInfo link;
    if (!tryGetPlacementInfoByKey(&link, links, linkName))
        return false;
    if (!tryGetPlacementInfoByIndex(railPlacementInfo, link, 0))
        return false;
    return true;
}

bool tryGetMoveParameterRailIter(PlacementInfo* railPlacementInfo,
                                 const PlacementInfo& placementInfo) {
    return tryGetRailIter(railPlacementInfo, placementInfo, "RailWithMoveParameter");
}

bool tryGetRailPointPos(sead::Vector3f* railPoint, const PlacementInfo& placementInfo) {
    return tryGetTrans(railPoint, placementInfo);
}

void getRailPointHandlePrev(sead::Vector3f* railPoint, const PlacementInfo& placementInfo) {
    tryGetRailPointHandlePrev(railPoint, placementInfo);
}

bool tryGetRailPointHandlePrev(sead::Vector3f* railPoint, const PlacementInfo& placementInfo) {
    PlacementInfo controlPoints;
    if (!tryGetPlacementInfoByKey(&controlPoints, placementInfo, "ControlPoints"))
        return false;
    PlacementInfo controlPoint;
    if (!tryGetPlacementInfoByIndex(&controlPoint, controlPoints, 0))
        return false;
    if (!tryGetByamlV3f(railPoint, controlPoint.getPlacementIter()))
        return false;
    multZoneMtx(railPoint, placementInfo);
    return true;
}

void getRailPointHandleNext(sead::Vector3f* railPoint, const PlacementInfo& placementInfo) {
    tryGetRailPointHandleNext(railPoint, placementInfo);
}

bool tryGetRailPointHandleNext(sead::Vector3f* railPoint, const PlacementInfo& placementInfo) {
    PlacementInfo controlPoints;
    if (!tryGetPlacementInfoByKey(&controlPoints, placementInfo, "ControlPoints"))
        return false;
    PlacementInfo controlPoint;
    if (!tryGetPlacementInfoByIndex(&controlPoint, controlPoints, 1))
        return false;
    if (!tryGetByamlV3f(railPoint, controlPoint.getPlacementIter()))
        return false;
    multZoneMtx(railPoint, placementInfo);
    return true;
}

bool isExistGraphRider(const ActorInitInfo& initInfo) {
    PlacementInfo info;
    return tryGetRailIter(&info, *initInfo.placementInfo, "Rail");
}

s32 calcLinkChildNum(const ActorInitInfo& initInfo, const char* linkName) {
    return calcLinkChildNum(*initInfo.placementInfo, linkName);
}

s32 calcLinkChildNum(const PlacementInfo& placementInfo, const char* linkName) {
    PlacementInfo links;
    PlacementInfo link;
    if (!tryGetPlacementInfoByKey(&links, placementInfo, "Links"))
        return false;
    if (!tryGetPlacementInfoByKey(&link, links, linkName))
        return false;
    return link.getPlacementIter().getSize();
}

bool isExistLinkChild(const ActorInitInfo& initInfo, const char* linkName, s32 index) {
    return isExistLinkChild(*initInfo.placementInfo, linkName, index);
}

bool isExistLinkChild(const PlacementInfo& placementInfo, const char* linkName, s32 index) {
    return calcLinkChildNum(placementInfo, linkName) > index;
}

bool isExistLinkChild(const AreaInitInfo& initInfo, const char* linkName, s32 index) {
    return isExistLinkChild((const PlacementInfo&)initInfo, linkName, index);
}

s32 calcLinkNestNum(const ActorInitInfo& initInfo, const char* linkName) {
    return calcLinkNestNum(*initInfo.placementInfo, linkName);
}

s32 calcLinkNestNum(const PlacementInfo& placementInfo, const char* linkName) {
    PlacementInfo links;
    if (!tryGetPlacementInfoByKey(&links, placementInfo, "Links"))
        return 0;
    PlacementInfo link = links;
    s32 depth = 0;
    while (tryGetPlacementInfoByKey(&link, links, linkName) &&
           link.getPlacementIter().getSize() != 0) {
        PlacementInfo item;
        getPlacementInfoByIndex(&item, link, 0);
        getPlacementInfoByKey(&links, item, "Links");
        depth++;
    }
    return depth;
}

void getLinksInfo(PlacementInfo* linkPlacementInfo, const PlacementInfo& placementInfo,
                  const char* linkName) {
    getLinksInfoByIndex(linkPlacementInfo, placementInfo, linkName, 0);
}

void getLinksInfoByIndex(PlacementInfo* linkPlacementInfo, const PlacementInfo& placementInfo,
                         const char* linkName, s32 index) {
    PlacementInfo links;
    if (!tryGetPlacementInfoByKey(&links, placementInfo, "Links"))
        return;
    PlacementInfo link;
    if (!tryGetPlacementInfoByKey(&link, links, linkName))
        return;
    getPlacementInfoByIndex(linkPlacementInfo, link, index);
}

void getLinksInfo(PlacementInfo* linkPlacementInfo, const ActorInitInfo& initInfo,
                  const char* linkName) {
    getLinksInfo(linkPlacementInfo, *initInfo.placementInfo, linkName);
}

void getLinksInfoByIndex(PlacementInfo* linkPlacementInfo, const ActorInitInfo& initInfo,
                         const char* linkName, s32 index) {
    getLinksInfoByIndex(linkPlacementInfo, *initInfo.placementInfo, linkName, index);
}

bool tryGetLinksInfo(PlacementInfo* linkPlacementInfo, const ActorInitInfo& initInfo,
                     const char* linkName) {
    return tryGetLinksInfo(linkPlacementInfo, *initInfo.placementInfo, linkName);
}

void getLinksMatrix(sead::Matrix34f* matrix, const ActorInitInfo& initInfo, const char* linkName) {
    getLinksMatrixByIndex(matrix, initInfo, linkName, 0);
}

void getLinksMatrixByIndex(sead::Matrix34f* matrix, const ActorInitInfo& initInfo,
                           const char* linkName, s32 index) {
    PlacementInfo info;
    getLinksInfoByIndex(&info, initInfo, linkName, index);
    tryGetMatrixTR(matrix, info);
}

void getLinkTR(sead::Vector3f* trans, sead::Vector3f* rotate, const PlacementInfo& placementInfo,
               const char* linkName) {
    PlacementInfo info;
    getLinksInfo(&info, placementInfo, linkName);
    getTrans(trans, info);
    getRotate(rotate, info);
}

void getLinkTR(sead::Vector3f* trans, sead::Vector3f* rotate, const ActorInitInfo& initInfo,
               const char* linkName) {
    getLinkTR(trans, rotate, *initInfo.placementInfo, linkName);
}

void getLinkTR(sead::Vector3f* trans, sead::Vector3f* rotate, const AreaInitInfo& initInfo,
               const char* linkName) {
    getLinkTR(trans, rotate, (const PlacementInfo&)initInfo, linkName);
}

void getLinksQT(sead::Quatf* quat, sead::Vector3f* trans, const ActorInitInfo& initInfo,
                const char* linkName) {
    getLinksQT(quat, trans, *initInfo.placementInfo, linkName);
}

void getLinksQT(sead::Quatf* quat, sead::Vector3f* trans, const PlacementInfo& placementInfo,
                const char* linkName) {
    PlacementInfo info;
    getLinksInfo(&info, placementInfo, linkName);
    if (quat)
        getQuat(quat, info);
    if (trans)
        getTrans(trans, info);
}

bool tryGetLinksQT(sead::Quatf* quat, sead::Vector3f* trans, const ActorInitInfo& initInfo,
                   const char* linkName) {
    PlacementInfo info;
    if (!tryGetLinksInfo(&info, initInfo, linkName))
        return false;
    bool success = true;
    if (quat)
        success &= tryGetQuat(quat, info);
    if (trans)
        success &= tryGetTrans(trans, info);
    return success;
}

bool tryGetLinksQTS(sead::Quatf* quat, sead::Vector3f* trans, sead::Vector3f* scale,
                    const ActorInitInfo& initInfo, const char* linkName) {
    PlacementInfo info;
    if (!tryGetLinksInfo(&info, initInfo, linkName))
        return false;
    bool success = true;
    if (quat)
        success &= tryGetQuat(quat, info);
    if (trans)
        success &= tryGetTrans(trans, info);
    if (scale)
        success &= tryGetScale(scale, info);
    return success;
}

bool tryGetLinksMatrixTR(sead::Matrix34f* matrix, const ActorInitInfo& initInfo,
                         const char* linkName) {
    PlacementInfo info;
    if (!tryGetLinksInfo(&info, *initInfo.placementInfo, linkName))
        return false;
    return tryGetMatrixTR(matrix, info);
}

bool tryGetLinksMatrixTR(sead::Matrix34f* matrix, const AreaInitInfo& initInfo,
                         const char* linkName) {
    PlacementInfo info;
    if (!tryGetLinksInfo(&info, (const PlacementInfo&)initInfo, linkName))
        return false;
    return tryGetMatrixTR(matrix, info);
}

bool tryGetLinksMatrixTRS(sead::Matrix34f* matrix, const ActorInitInfo& initInfo,
                          const char* linkName) {
    PlacementInfo info;
    if (!tryGetLinksInfo(&info, *initInfo.placementInfo, linkName))
        return false;
    return tryGetMatrixTRS(matrix, info);
}

bool tryGetLinksTrans(sead::Vector3f* trans, const ActorInitInfo& initInfo, const char* linkName) {
    return tryGetLinksQT(nullptr, trans, initInfo, linkName);
}

bool tryGetLinksTrans(sead::Vector3f* trans, const PlacementInfo& placementInfo,
                      const char* linkName) {
    PlacementInfo info;
    if (!tryGetLinksInfo(&info, placementInfo, linkName))
        return false;
    if (!trans)
        return false;
    return tryGetTrans(trans, info);
}

bool tryGetLinksQuat(sead::Quatf* quat, const ActorInitInfo& initInfo, const char* linkName) {
    return tryGetLinksQT(quat, nullptr, initInfo, linkName);
}

bool tryGetLinksTR(sead::Vector3f* trans, sead::Vector3f* rotate, const ActorInitInfo& initInfo,
                   const char* linkName) {
    PlacementInfo info;
    if (!tryGetLinksInfo(&info, initInfo, linkName))
        return false;
    getTrans(trans, info);
    getRotate(rotate, info);
    return true;
}

void getChildTrans(sead::Vector3f* trans, const PlacementInfo& placementInfo,
                   const char* linkName) {
    PlacementInfo info;
    getLinksInfo(&info, placementInfo, linkName);
    getTrans(trans, info);
}

void getChildTrans(sead::Vector3f* trans, const ActorInitInfo& initInfo, const char* linkName) {
    getChildTrans(trans, *initInfo.placementInfo, linkName);
}

void getChildTrans(sead::Vector3f* trans, const AreaInitInfo& initInfo, const char* linkName) {
    getChildTrans(trans, (const PlacementInfo&)initInfo, linkName);
}

void getChildLinkT(sead::Vector3f* trans, const ActorInitInfo& initInfo, const char* linkName,
                   s32 index) {
    PlacementInfo info;
    getLinksInfoByIndex(&info, initInfo, linkName, index);
    getTrans(trans, info);
}

void getChildLinkTR(sead::Vector3f* trans, sead::Vector3f* rotate, const ActorInitInfo& initInfo,
                    const char* linkName, s32 index) {
    PlacementInfo info;
    getLinksInfoByIndex(&info, initInfo, linkName, index);
    getTrans(trans, info);
    getRotate(rotate, info);
}

s32 calcMatchNameLinkCount(const PlacementInfo& placementInfo, const char* match) {
    PlacementInfo links;
    if (!tryGetPlacementInfoByKey(&links, placementInfo, "Links"))
        return 0;
    s32 numItems = links.getPlacementIter().getSize();
    s32 count = 0;
    for (s32 i = 0; i < numItems; ++i) {
        PlacementInfo item;
        const char* key = nullptr;
        getPlacementInfoAndKeyNameByIndex(&item, &key, links, i);
        if (isMatchString(key, {match}))
            count++;
    }
    return count;
}

s32 calcLinkCountClassName(const PlacementInfo& placementInfo, const char* className) {
    PlacementInfo links;
    if (!tryGetPlacementInfoByKey(&links, placementInfo, "Links"))
        return false;
    s32 numItems = links.getPlacementIter().getSize();
    s32 count = 0;
    for (s32 i = 0; i < numItems; ++i) {
        PlacementInfo item;
        getPlacementInfoByIndex(&item, links, i);
        PlacementInfo first;
        getPlacementInfoByIndex(&first, item, 0);

        const char* classNameFirst = nullptr;
        if (tryGetClassName(&classNameFirst, first) && isEqualString(classNameFirst, className))
            count++;
    }
    return count;
}

bool tryGetZoneMatrixTR(sead::Matrix34f* matrix, const ActorInitInfo& initInfo) {
    return tryGetZoneMatrixTR(matrix, *initInfo.placementInfo);
}

bool tryGetDisplayOffset(sead::Vector3f* offset, const ActorInitInfo& initInfo) {
    return tryGetDisplayOffset(offset, *initInfo.placementInfo);
}

bool tryGetDisplayOffset(sead::Vector3f* offset, const PlacementInfo& placementInfo) {
    PlacementInfo info;
    if (!tryGetPlacementInfoByKey(&info, placementInfo, "UnitConfig"))
        return false;

    if (!tryGetArgV3f(offset, info, "DisplayTranslate"))
        return false;

    sead::Matrix34f tr = sead::Matrix34f::ident;
    if (!tryGetMatrixTR(&tr, placementInfo))
        return false;
    offset->rotate(tr);

    sead::Matrix34f mtx = sead::Matrix34f::ident;
    if (tryGetZoneMatrixTR(&mtx, placementInfo))
        offset->rotate(mtx);
    return true;
}

bool tryGetChildDisplayOffset(sead::Vector3f* offset, const ActorInitInfo& initInfo,
                              const char* linkName) {
    PlacementInfo info;
    return tryGetLinksInfo(&info, initInfo, linkName) && tryGetDisplayOffset(offset, info);
}

bool tryGetDisplayRotate(sead::Vector3f* rotate, const ActorInitInfo& initInfo) {
    PlacementInfo info;
    getPlacementInfoByKey(&info, *initInfo.placementInfo, "UnitConfig");
    return tryGetArgV3f(rotate, info, "DisplayRotate");
}

bool tryGetDisplayScale(sead::Vector3f* scale, const ActorInitInfo& initInfo) {
    PlacementInfo info;
    getPlacementInfoByKey(&info, *initInfo.placementInfo, "UnitConfig");
    return tryGetArgV3f(scale, info, "DisplayScale");
}

}  // namespace al

namespace alPlacementFunction {

s32 getCameraId(const al::ActorInitInfo& initInfo) {
    s32 id = -1;
    if (!al::tryGetArg(&id, initInfo, "CameraId"))
        return -1;
    return id;
}

bool getLinkGroupId(al::PlacementId* groupId, const al::ActorInitInfo& initInfo,
                    const char* linkName) {
    al::PlacementInfo info;
    if (al::tryGetLinksInfo(&info, initInfo, linkName) && al::tryGetPlacementId(groupId, info))
        return true;
    return false;
}

bool isEnableLinkGroupId(const al::ActorInitInfo& initInfo, const char* linkName) {
    al::PlacementId id;
    return getLinkGroupId(&id, initInfo, linkName);
}

bool isEnableGroupClipping(const al::ActorInitInfo& initInfo) {
    return isEnableLinkGroupId(initInfo, "GroupClipping");
}

bool getClippingGroupId(al::PlacementId* groupId, const al::ActorInitInfo& initInfo) {
    return getLinkGroupId(groupId, initInfo, "GroupClipping");
}

al::PlacementId* createClippingViewId(const al::PlacementInfo& placementInfo) {
    al::PlacementId* id = new al::PlacementId();
    al::PlacementInfo info;
    if (al::tryGetLinksInfo(&info, placementInfo, "ViewGroup"))
        id->init(info);
    return id;
}

bool getClippingViewId(al::PlacementId* viewId, const al::PlacementInfo& placementInfo) {
    al::PlacementInfo info;
    if (al::tryGetLinksInfo(&info, placementInfo, "ViewGroup") &&
        al::tryGetPlacementId(viewId, info))
        return true;
    return false;
}

bool getClippingViewId(al::PlacementId* viewId, const al::ActorInitInfo& initInfo) {
    return getClippingViewId(viewId, *initInfo.placementInfo);
}

void getModelName(const char** modelName, const al::ActorInitInfo& initInfo) {
    getModelName(modelName, *initInfo.placementInfo);
}

void getModelName(const char** modelName, const al::PlacementInfo& placementInfo) {
    tryGetModelName(modelName, placementInfo);
}

bool tryGetModelName(const char** modelName, const al::PlacementInfo& placementInfo) {
    return tryGetStringArg(modelName, placementInfo, "ModelName") ||
           tryGetStringArg(modelName, placementInfo, "ArchiveName");
}

bool tryGetModelName(const char** modelName, const al::ActorInitInfo& initInfo) {
    return tryGetModelName(modelName, *initInfo.placementInfo);
}

}  // namespace alPlacementFunction

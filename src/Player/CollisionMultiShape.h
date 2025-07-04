#pragma once

#include <container/seadPtrArray.h>
#include <math/seadVector.h>

namespace al {
class CollisionParts;
class IUseCollision;
class KCollisionServer;
struct KCPrismData;
struct KCPrismHeader;

// TODO: move this in correct header
struct SphereCheckInfo {
    f32 _0;
    f32 _4;
    f32 _8;
};
}  // namespace al

class CollisionShapeKeeper;

class CollisionMultiShape {
public:
    CollisionMultiShape(const al::IUseCollision* collisionUser, s32);

private:
    const al::IUseCollision* _0;
    al::SphereCheckInfo _8;
    CollisionShapeKeeper* _18;
    sead::Vector3f _20;
    sead::Vector3f _2c;
    s32 _38;
    f32 _3c;
    sead::Vector3f _40;
    al::CollisionParts* _50;
    al::KCollisionServer* _58;
    const al::KCPrismHeader* _60;
    sead::PtrArray<const al::KCPrismData> _68;
    bool _78;
};

static_assert(sizeof(CollisionMultiShape) == 0x80);

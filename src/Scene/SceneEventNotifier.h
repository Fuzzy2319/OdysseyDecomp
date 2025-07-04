#pragma once

namespace al {
class FunctorBase;
class IUseSceneObjHolder;
}  // namespace al

namespace rs {
void listenSnapShotModeOnOff(const al::IUseSceneObjHolder*, const al::FunctorBase& actionOnOn,
                             const al::FunctorBase& actionOnOff);
}

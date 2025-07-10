#pragma once

#include <basis/seadTypes.h>

namespace al {
class LiveActor;
}

namespace rs {
bool trySlideIfOnFloorSlide(al::LiveActor*, f32, f32);
}

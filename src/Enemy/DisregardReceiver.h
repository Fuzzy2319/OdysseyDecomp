#pragma once

namespace al {
class LiveActor;
}

class DisregardReceiver {
public:
    DisregardReceiver(al::LiveActor* actor, const char* suffix);

private:
    al::LiveActor* _0;
    unsigned char padding[0x50];
};

static_assert(sizeof(DisregardReceiver) == 0x58);

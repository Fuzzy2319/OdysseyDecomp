#pragma once

#include <container/seadPtrArray.h>
#include <container/seadStrTreeMap.h>
#include <math/seadVector.h>
#include <prim/seadSafeString.h>

struct StagePosInfo;
struct ShinePosInfo;

struct StageDBEntry {
    sead::FixedSafeString<0x80> name;
    sead::FixedSafeString<0x40> category;
    s32 useScenarioNo;
};

struct WorldListEntry {
    const char* mainStageName;
    const char* worldDevelopName;
    s32 questInfoCount;
    s32 clearMainScenarioNo;
    s32 afterEndingScenarioNo;
    s32 moonRockScenarioNo;
    s32* mainQuestIndexes;
    sead::PtrArray<StageDBEntry> stageList;
};

class WorldList {
public:
    WorldList();

    s32 getWorldNum() const;
    s32 getMainQuestMin(s32 worldIdx, s32 questIdx) const;
    const char* getMainStageName(s32 worldIdx) const;
    s32 tryFindWorldIndexByMainStageName(const char* stageName) const;
    s32 tryFindWorldIndexByStageName(const char* stageName) const;
    s32 tryFindWorldIndexByDevelopName(const char* name) const;
    bool isEqualClearMainScenarioNo(s32 worldIdx, s32 scenarioNo) const;
    s32 getAfterEndingScenarioNo(s32 worldIdx) const;
    bool isEqualAfterEndingScenarioNo(s32 worldIdx, s32 scenarioNo) const;
    s32 getMoonRockScenarioNo(s32 worldIdx) const;
    bool isEqualMoonRockScenarioNo(s32 worldIdx, s32 scenarioNo) const;
    const char* getWorldDevelopName(s32 worldIdx) const;
    s32 getWorldScenarioNum(s32 worldIdx) const;
    s32 findUseScenarioNo(const char* stageName) const;
    bool checkNeedTreasureMessageStage(const char* stageName) const;
    bool checkIsMainStage(const char*) const;
    bool tryFindTransOnMainStageByStageName(sead::Vector3f*, const char*, s32) const;
    bool tryFindHintTransByScenarioNo(sead::Vector3f*, s32, s32) const;

private:
    sead::PtrArray<WorldListEntry> mWorldList;
    sead::StrTreeMap<128, StagePosInfo*> mStagePosList;
    sead::PtrArray<ShinePosInfo> mShinePosList;
};

static_assert(sizeof(WorldList) == 0x40);

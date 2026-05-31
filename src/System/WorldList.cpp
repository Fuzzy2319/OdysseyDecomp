#include "System/WorldList.h"

#include "Library/Base/StringUtil.h"
#include "Library/Resource/ResourceFunction.h"
#include "Library/Yaml/ByamlIter.h"

// TODO: finish this
WorldList::WorldList() {
    al::ByamlIter worldListIter = {al::tryGetBymlFromArcName("SystemData/WorldList", "WorldList")};
    s32 worldCount = worldListIter.getSize();
    mWorldList.allocBuffer(worldCount, nullptr);

    for (s32 i = 0; i < worldCount; i++) {
        al::ByamlIter worldIter;
        worldListIter.tryGetIterByIndex(&worldIter, i);

        const char* name = nullptr;
        worldIter.tryGetStringByKey(&name, "Name");
        const char* worldName = nullptr;
        worldIter.tryGetStringByKey(&worldName, "WorldName");
        s32 scenarioNum = 1;
        worldIter.tryGetIntByKey(&scenarioNum, "ScenarioNum");
        s32 clearMainScenarioNo = 1;
        worldIter.tryGetIntByKey(&clearMainScenarioNo, "ClearMainScenario");
        s32 afterEndingScenarioNo = 1;
        worldIter.tryGetIntByKey(&afterEndingScenarioNo, "AfterEndingScenario");
        s32 moonRockScenarioNo = 1;
        worldIter.tryGetIntByKey(&moonRockScenarioNo, "MoonRockScenario");

        WorldListEntry* entry = new WorldListEntry();
        entry->mainStageName = name;
        entry->worldDevelopName = worldName;
        entry->questInfoCount = scenarioNum;
        entry->clearMainScenarioNo = clearMainScenarioNo;
        entry->afterEndingScenarioNo = afterEndingScenarioNo;
        entry->moonRockScenarioNo = moonRockScenarioNo;
        entry->mainQuestIndexes = new s32[entry->questInfoCount];

        al::ByamlIter questIter;
        worldIter.tryGetIterByKey(&questIter, "MainQuestInfo");

        for (s32 j = 0; j < entry->questInfoCount; j++) {
            s32 questIdx = -1;
            questIter.tryGetIntByIndex(&questIdx, j);
            entry->mainQuestIndexes[j] = questIdx;
        }

        mWorldList.pushBack(entry);
    }

    al::ByamlIter worldListDbIter = {
        al::tryGetBymlFromArcName("SystemData/WorldList", "WorldListFromDb")};
    s32 worldDbCount = worldListDbIter.getSize();
    for (s32 i = 0; i < worldDbCount; i++) {
        al::ByamlIter worldIter;
        worldListDbIter.tryGetIterByIndex(&worldIter, i);
        al::ByamlIter stageListIter;
        worldIter.tryGetIterByKey(&stageListIter, "StageList");

        mWorldList[i]->stageList.allocBuffer(stageListIter.getSize(), nullptr);

        for (s32 j = 0; j < stageListIter.getSize(); j++) {
            al::ByamlIter stageIter;
            stageListIter.tryGetIterByIndex(&stageIter, j);

            const char* name = nullptr;
            stageIter.tryGetStringByKey(&name, "name");

            StageDBEntry* stageEntry = new StageDBEntry();
            stageEntry->name.format("%s", name);

            const char* category = nullptr;
            if (stageIter.tryGetStringByKey(&category, "category")) {
                stageEntry->category.format("%s", category);

                if (al::isEqualString(stageEntry->category.cstr(), "MainStage"))
                    stageEntry->useScenarioNo = -1;
                else if (al::isEqualString(stageEntry->category.cstr(), "MainRouteStage"))
                    stageEntry->useScenarioNo = -1;
                else if (al::isEqualString(stageEntry->category.cstr(), "ExStage"))
                    stageEntry->useScenarioNo = 1;
                else if (al::isEqualString(stageEntry->category.cstr(), "SmallStage"))
                    stageEntry->useScenarioNo = 1;
                else if (al::isEqualString(stageEntry->category.cstr(), "PathwayStage"))
                    stageEntry->useScenarioNo = -1;
                else if (al::isEqualString(stageEntry->category.cstr(), "ShopStage"))
                    stageEntry->useScenarioNo = -1;
                else if (al::isEqualString(stageEntry->category.cstr(), "MoonExStage"))
                    stageEntry->useScenarioNo = 1;
                else if (al::isEqualString(stageEntry->category.cstr(), "Demo"))
                    stageEntry->useScenarioNo = -1;
                else if (al::isEqualString(stageEntry->category.cstr(), "MoonFarSideExStage"))
                    stageEntry->useScenarioNo = 2;
                else if (al::isEqualString(stageEntry->category.cstr(), "BossRevenge"))
                    stageEntry->useScenarioNo = 1;
                else if (al::isEqualString(stageEntry->category.cstr(), "MiniGame"))
                    stageEntry->useScenarioNo = -1;
                else if (al::isEqualString(stageEntry->category.cstr(), "Zone"))
                    stageEntry->useScenarioNo = -1;
            } else {
                stageEntry->category = "\0";
                stageEntry->useScenarioNo = -1;
            }

            mWorldList[i]->stageList.pushBack(stageEntry);
        }
    }
}

s32 WorldList::getWorldNum() const {
    return mWorldList.size();
}

s32 WorldList::getMainQuestMin(s32 worldIdx, s32 questIdx) const {
    worldIdx = sead::Mathi::clampMin(worldIdx, 0);

    return mWorldList[worldIdx]->mainQuestIndexes[questIdx - 1];
}

const char* WorldList::getMainStageName(s32 worldIdx) const {
    return mWorldList[worldIdx]->mainStageName;
}

s32 WorldList::tryFindWorldIndexByMainStageName(const char* stageName) const {
    for (s32 i = 0; i < mWorldList.size(); i++)
        if (al::isEqualString(getMainStageName(i), stageName))
            return i;

    return -1;
}

s32 WorldList::tryFindWorldIndexByStageName(const char* stageName) const {
    s32 worldIdx = tryFindWorldIndexByMainStageName(stageName);
    if (worldIdx != -1)
        return worldIdx;

    for (s32 i = 0; i < mWorldList.size(); i++)
        for (s32 j = 0; j < mWorldList[i]->stageList.size(); j++)
            if (al::isEqualString(stageName, mWorldList[i]->stageList[j]->name.cstr()))
                return i;

    return -1;
}

s32 WorldList::tryFindWorldIndexByDevelopName(const char* name) const {
    for (s32 i = 0; i < mWorldList.size(); i++)
        if (al::isEqualString(getWorldDevelopName(i), name))
            return i;

    return -1;
}

bool WorldList::isEqualClearMainScenarioNo(s32 worldIdx, s32 scenarioNo) const {
    return mWorldList[worldIdx]->clearMainScenarioNo == scenarioNo;
}

s32 WorldList::getAfterEndingScenarioNo(s32 worldIdx) const {
    return mWorldList[worldIdx]->afterEndingScenarioNo;
}

bool WorldList::isEqualAfterEndingScenarioNo(s32 worldIdx, s32 scenarioNo) const {
    return getAfterEndingScenarioNo(worldIdx) == scenarioNo;
}

s32 WorldList::getMoonRockScenarioNo(s32 worldIdx) const {
    return mWorldList[worldIdx]->moonRockScenarioNo;
}

bool WorldList::isEqualMoonRockScenarioNo(s32 worldIdx, s32 scenarioNo) const {
    return getMoonRockScenarioNo(worldIdx) == scenarioNo;
}

const char* WorldList::getWorldDevelopName(s32 worldIdx) const {
    worldIdx = sead::Mathi::clampMin(worldIdx, 0);

    return mWorldList[worldIdx]->worldDevelopName;
}

s32 WorldList::getWorldScenarioNum(s32 worldIdx) const {
    return mWorldList[worldIdx]->questInfoCount;
}

s32 WorldList::findUseScenarioNo(const char* stageName) const {
    if (!al::isEqualString(stageName, "CurrentWorldHome")) {
        s32 worldListSize = mWorldList.size();
        for (s32 i = 0; i < worldListSize; i++) {
            WorldListEntry* world = mWorldList[i];
            s32 stageListSize = world->stageList.size();
            for (s32 j = 0; j < stageListSize; j++)
                if (al::isEqualString(world->stageList[j]->name.cstr(), stageName))
                    return world->stageList[j]->useScenarioNo;
        }
    }

    return -1;
}

bool WorldList::checkNeedTreasureMessageStage(const char* stageName) const {
    s32 worldListSize = mWorldList.size();
    if (al::isEqualString(stageName, "ForestWorldWoodsStage") ||
        al::isEqualString(stageName, "SnowWorldLobbyExStage"))
        return false;

    for (s32 i = 0; i < worldListSize; i++) {
        WorldListEntry* world = mWorldList[i];
        s32 stageListSize = world->stageList.size();
        for (s32 j = 0; j < stageListSize; j++) {
            if (!al::isEqualString(world->stageList[j]->name.cstr(), stageName))
                continue;

            const char* category = world->stageList[j]->category.cstr();
            if (!al::isEqualString(category, "BossRevenge") &&
                !al::isEqualString(category, "MainRouteStage") &&
                !al::isEqualString(category, "MainStage") &&
                !al::isEqualString(category, "ShopStage") &&
                !al::isEqualString(category, "MiniGame") &&
                !al::isEqualString(category, "SmallStage"))
                return true;

            return false;
        }
    }

    return false;
}

bool WorldList::checkIsMainStage(const char* stageName) const {
    return tryFindWorldIndexByMainStageName(stageName) != -1;
}

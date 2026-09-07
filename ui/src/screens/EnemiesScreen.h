#pragma once

#include "Screen.h"
#include "../assets/AssetManager.h"
#include "../data/EnemyDatabase.h"
#include "raylib.h"

#include <array>
#include <cstddef>
#include <string>
#include <vector>

class EnemiesScreen : public Screen
{
public:
    enum class Filter
    {
        BossElite,
        All
    };

    static constexpr size_t kEnemySlotCount = 5;
    using EnemySlot = std::vector<std::string>;

    EnemiesScreen(AssetManager& assets, EnemyDatabase& enemies);

    void initialize() override;
    void update(float dt) override;
    void draw() override;

    const std::string& getSelectedEnemyId() const;

    const std::array<EnemySlot, kEnemySlotCount>& getEnemySlots() const
    {
        return enemySlots;
    }

    int getActiveSlot() const
    {
        return activeSlot;
    }

    bool consumeBackRequest();

private:
    Rectangle backBounds();
    Rectangle searchBounds();
    Rectangle filterBounds(Filter selectedFilter);
    Rectangle slotBounds(int slot);
    Rectangle slotSummaryBounds(int slot);
    Rectangle rowBounds(int row);
    Rectangle addBounds();
    Rectangle removeBounds();
    Rectangle clearBounds();

    void rebuildFiltered();
    void selectEnemy(const EnemyInfo& enemy);
    void addSelectedEnemyToActiveSlot();
    void removeLastEnemyFromActiveSlot();
    void clearActiveSlot();

    AssetManager& assets;
    EnemyDatabase& enemies;

    std::string searchText;
    Filter filter = Filter::BossElite;

    std::array<EnemySlot, kEnemySlotCount> enemySlots{};
    int activeSlot = 0;

    std::vector<const EnemyInfo*> filtered;
    int hoveredRow = -1;
    int hoveredSlot = -1;
    std::string selectedEnemyId;

    int scrollOffset = 0;
    int activeSlotScroll = 0;
    bool backRequested = false;
};

#pragma once

#include "Screen.h"
#include "../assets/AssetManager.h"
#include "../data/EnemyDatabase.h"
#include "raylib.h"

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

    EnemiesScreen(AssetManager& assets, EnemyDatabase& enemies);

    void initialize() override;
    void update(float dt) override;
    void draw() override;

    const std::string& getSelectedEnemyId() const { return selectedEnemyId; }
    bool consumeBackRequest();

private:
    Rectangle backBounds();
    Rectangle searchBounds();
    Rectangle filterBounds(Filter selectedFilter);
    Rectangle rowBounds(int row);

    void rebuildFiltered();
    void selectEnemy(const EnemyInfo& enemy);

    AssetManager& assets;
    EnemyDatabase& enemies;

    std::string searchText;
    std::string selectedEnemyId;
    Filter filter = Filter::BossElite;

    std::vector<const EnemyInfo*> filtered;
    int hoveredRow = -1;
    int scrollOffset = 0;
    bool backRequested = false;
};

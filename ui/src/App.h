#pragma once

#include "assets/AssetManager.h"
#include "data/CharacterDatabase.h"
#include "data/RelicSetDatabase.h"
#include "data/LightConeDatabase.h"
#include "data/CharacterLoadout.h"
#include "data/EnemyDatabase.h"
#include "widgets/Sidebar.h"
#include "screens/TeamBuilderScreen.h"
#include "screens/RelicRosterScreen.h"
#include "screens/RelicEditorScreen.h"
#include "screens/LightConeScreen.h"
#include "screens/EnemiesScreen.h"

#include <memory>
#include <string>

class App
{
public:
    App();
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    bool initialize();
    void run();
    void shutdown();

private:
    enum class ActiveView
    {
        TeamBuilder,
        RelicRoster,
        RelicEditor,
        LightCone,
        Enemies
    };

    void goToRelicEditor(const std::string& characterId);
    void goToLightConeScreen(const std::string& characterId);
    void goToEnemiesScreen();

    AssetManager assets;
    CharacterDatabase characters;
    RelicSetDatabase relicSets;
    LightConeDatabase lightCones;
    EnemyDatabase enemies;
    LoadoutStore loadouts;

    Sidebar sidebar;
    int activeNav = 0;
    ActiveView activeView = ActiveView::TeamBuilder;

    std::unique_ptr<TeamBuilderScreen> teamBuilder;
    std::unique_ptr<RelicRosterScreen> relicRoster;
    std::unique_ptr<RelicEditorScreen> relicEditor;
    std::unique_ptr<LightConeScreen> lightConeScreen;
    std::unique_ptr<EnemiesScreen> enemiesScreen;
};

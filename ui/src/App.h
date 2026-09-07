#pragma once

#include "assets/AssetManager.h"
#include "data/CharacterDatabase.h"
#include "data/Relicsetdatabase.h"
#include "data/LightConeDatabase.h"
#include "data/Characterloadout.h"
#include "data/EnemyDatabase.h"
#include "widgets/Sidebar.h"
#include "screens/TeamBuilderScreen.h"
#include "screens/CharactersScreen.h"
#include "screens/Relicrosterscreen.h"
#include "screens/Reliceditorscreen.h"
#include "screens/LightConeScreen.h"
#include "screens/EnemiesScreen.h"
#include "screens/SimulationScreen.h"

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
        Characters,
        RelicRoster,
        RelicEditor,
        LightCone,
        Enemies,
        Simulation
    };

    void goToRelicEditor(const std::string& characterId);
    void goToLightConeScreen(const std::string& characterId);
    void goToEnemiesScreen();
    void goToSimulationScreen(int navIndex);
    void goToCharactersScreen();

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
    std::unique_ptr<CharactersScreen> charactersScreen;
    std::unique_ptr<RelicRosterScreen> relicRoster;
    std::unique_ptr<RelicEditorScreen> relicEditor;
    std::unique_ptr<LightConeScreen> lightConeScreen;
    std::unique_ptr<EnemiesScreen> enemiesScreen;
    std::unique_ptr<SimulationScreen> simulationScreen;
};

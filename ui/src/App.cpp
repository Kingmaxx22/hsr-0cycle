#include "App.h"
#include "raylib.h"

App::App() = default;
App::~App() = default;

bool App::initialize()
{
    InitWindow(1440, 900, "HSR 0-Cycle Simulator");
    SetTargetFPS(60);
    SetExitKey(KEY_F4);

    assets.loadManifest("assets/asset_manifest.csv");

    if (!characters.load(HSR_ENGINE_DATA_DIR))
    {
        TraceLog(LOG_ERROR,
                 "Failed to load character rules from: %s",
                 HSR_ENGINE_DATA_DIR);
        return false;
    }

    if (!relicSets.load(HSR_ENGINE_DATA_DIR))
        TraceLog(LOG_WARNING, "Failed to load relic set rules from: %s",
                 HSR_ENGINE_DATA_DIR);

    if (!lightCones.load(HSR_ENGINE_DATA_DIR))
        TraceLog(LOG_WARNING, "Failed to load light cones from: %s",
                 HSR_ENGINE_DATA_DIR);

    if (!enemies.load(HSR_ENGINE_DATA_DIR))
        TraceLog(LOG_WARNING, "Failed to load monster rules from: %s",
                 HSR_ENGINE_DATA_DIR);

    teamBuilder = std::make_unique<TeamBuilderScreen>(assets, characters);
    teamBuilder->initialize();

    relicRoster = std::make_unique<RelicRosterScreen>(assets, characters);
    relicRoster->initialize();

    relicEditor =
        std::make_unique<RelicEditorScreen>(
            assets, characters, relicSets, loadouts);
    relicEditor->initialize();

    lightConeScreen =
        std::make_unique<LightConeScreen>(
            assets, characters, lightCones, loadouts);
    lightConeScreen->initialize();

    enemiesScreen =
        std::make_unique<EnemiesScreen>(assets, enemies);
    enemiesScreen->initialize();

    simulationScreen =
        std::make_unique<SimulationScreen>(assets, enemies);
    simulationScreen->initialize();

    return true;
}

void App::goToRelicEditor(const std::string& characterId)
{
    relicEditor->setTeamContext(teamBuilder->getTeam(), characterId);
    activeView = ActiveView::RelicEditor;
    activeNav = 3;
}

void App::goToLightConeScreen(const std::string& characterId)
{
    lightConeScreen->setTeamContext(
        teamBuilder->getTeam(), characterId);

    activeView = ActiveView::LightCone;
    activeNav = 2;
}

void App::goToEnemiesScreen()
{
    activeView = ActiveView::Enemies;
    activeNav = 4;
}

void App::goToSimulationScreen()
{
    // Pass selected enemy from EnemiesScreen if available
    std::string selectedEnemy = enemiesScreen->getSelectedEnemyId();
    if (!selectedEnemy.empty()) {
        simulationScreen->setSelectedEnemy(selectedEnemy);
    }
    activeView = ActiveView::Simulation;
    activeNav = 5;
}

void App::run()
{
    while (!WindowShouldClose())
    {
        const float dt = GetFrameTime();

        switch (activeView)
        {
            case ActiveView::TeamBuilder: teamBuilder->update(dt); break;
            case ActiveView::RelicRoster: relicRoster->update(dt); break;
            case ActiveView::RelicEditor: relicEditor->update(dt); break;
            case ActiveView::LightCone:   lightConeScreen->update(dt); break;
            case ActiveView::Enemies:     enemiesScreen->update(dt); break;
            case ActiveView::Simulation:  simulationScreen->update(dt); break;
        }

        BeginDrawing();
        ClearBackground(Color{18, 20, 27, 255});

        const int navClick = sidebar.updateAndDraw(activeNav);

        if (navClick == 0)
        {
            activeView = ActiveView::TeamBuilder;
            activeNav = 0;
        }
        else if (navClick == 2)
        {
            goToLightConeScreen(teamBuilder->getSelectedCharacter());
        }
        else if (navClick == 3)
        {
            goToRelicEditor(teamBuilder->getSelectedCharacter());
        }
        else if (navClick == 4)
        {
            goToEnemiesScreen();
        }
        else if (navClick == 5)
        {
            goToSimulationScreen();
        }
        else if (navClick >= 0)
        {
            activeNav = navClick;
        }

        switch (activeView)
        {
            case ActiveView::TeamBuilder:
            {
                teamBuilder->draw();
                std::string requested;

                if (teamBuilder->consumeEditorRequest(requested))
                    goToRelicEditor(requested);

                break;
            }

            case ActiveView::RelicRoster:
            {
                relicRoster->draw();
                std::string selected;

                if (relicRoster->consumeSelection(selected))
                    goToRelicEditor(selected);

                break;
            }

            case ActiveView::RelicEditor:
                relicEditor->draw();

                if (relicEditor->consumeBackRequest())
                {
                    activeView = ActiveView::TeamBuilder;
                    activeNav = 0;
                }
                break;

            case ActiveView::LightCone:
                lightConeScreen->draw();

                if (lightConeScreen->consumeBackRequest())
                {
                    activeView = ActiveView::TeamBuilder;
                    activeNav = 0;
                }
                break;

            case ActiveView::Enemies:
                enemiesScreen->draw();

                if (enemiesScreen->consumeBackRequest())
                {
                    activeView = ActiveView::TeamBuilder;
                    activeNav = 0;
                }
                break;

            case ActiveView::Simulation:
                simulationScreen->draw();

                if (simulationScreen->consumeBackRequest())
                {
                    activeView = ActiveView::TeamBuilder;
                    activeNav = 0;
                }
                break;
        }

        EndDrawing();
    }
}

void App::shutdown()
{
    simulationScreen.reset();
    enemiesScreen.reset();
    lightConeScreen.reset();
    relicEditor.reset();
    relicRoster.reset();
    teamBuilder.reset();

    assets.unloadAll();
    CloseWindow();
}

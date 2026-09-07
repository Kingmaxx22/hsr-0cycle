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

    charactersScreen =
        std::make_unique<CharactersScreen>(
            assets, characters, relicSets, lightCones, loadouts);
    charactersScreen->initialize();

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

// Section 21.7 + 22.3/22.4: single handoff path from team/loadout data
// into engine configs. Calculated values travel with the config — the
// simulation screen never asks for them again (Sec 21.8). The two input
// modes are mutually exclusive per character (Sec 22.5): a completed
// manual entry wins for its character, otherwise components resolve.
static hsr::CharacterConfig BuildSimCharacter(
    const CharacterInfo& info,
    CharacterLoadout& loadout,
    const LightConeDatabase& lightCones,
    const CharactersScreen* charactersScreen)
{
    hsr::CharacterConfig config;

    if (charactersScreen != nullptr && charactersScreen->isManualStatsMode())
    {
        CharactersScreen::ManualConfig manual = charactersScreen->getManualConfig();
        if (manual.characterId == info.id &&
            (manual.hp + manual.atk + manual.def + manual.spd) > 0)
        {
            loadout::applyManualToCharacterConfig(
                config, info.id, info.name,
                static_cast<double>(manual.hp), static_cast<double>(manual.atk),
                static_cast<double>(manual.def), static_cast<double>(manual.spd),
                manual.critRate, manual.critDmg, manual.elemDmg,
                manual.resPen, manual.ehr, manual.effectRes);
            config.level = loadout.level;
            config.rotation = {"Skill", "Basic", "Basic"};
            // Skill multipliers below are documented fallbacks, not data.
            config.scalingStat = "atk";
            config.basicMultiplier = 1.0;
            config.skillMultiplier = 2.0;
            config.ultMultiplier = 3.0;
            config.fuaMultiplier = 1.0;
            return config;
        }
    }

    ensureLoadoutDefaults(loadout);
    loadout::applyToCharacterConfig(config, info, loadout, lightCones);
    config.rotation = {"Skill", "Basic", "Basic"};
    // Skill multipliers below are documented fallbacks, not data.
    config.scalingStat = "atk";
    config.basicMultiplier = 1.0;
    config.skillMultiplier = 2.0;
    config.ultMultiplier = 3.0;
    config.fuaMultiplier = 1.0;
    return config;
}

void App::goToSimulationScreen(int navIndex)
{
    // Pass selected enemy from EnemiesScreen if available
    std::string selectedEnemy = enemiesScreen->getSelectedEnemyId();
    if (!selectedEnemy.empty()) {
        simulationScreen->setSelectedEnemy(selectedEnemy);
    }
    // Section 21.7: push the configured team straight into the engine.
    bool hasTeam = false;
    for (const auto& id : teamBuilder->getTeam()) {
        if (!id.empty()) {
            hasTeam = true;
            break;
        }
    }
    if (hasTeam) {
        simulationScreen->clearCharacters();
        for (const auto& id : teamBuilder->getTeam()) {
            if (id.empty())
                continue;
            const CharacterInfo* info = characters.get(id);
            if (info != nullptr)
                simulationScreen->addCharacter(
                    BuildSimCharacter(*info, loadouts[id], lightCones,
                                      charactersScreen.get()));
        }
    }
    activeView = ActiveView::Simulation;
    // Preserve the clicked sidebar highlight (ROTATION=5, SIMULATE=6).
    activeNav = navIndex;
}

void App::goToCharactersScreen()
{
    charactersScreen->setTeamContext(teamBuilder->getTeam());
    activeView = ActiveView::Characters;
    activeNav = 1;
}

void App::run()
{
    while (!WindowShouldClose())
    {
        const float dt = GetFrameTime();

        switch (activeView)
        {
            case ActiveView::TeamBuilder: teamBuilder->update(dt); break;
            case ActiveView::Characters: charactersScreen->update(dt); break;
            case ActiveView::RelicRoster: relicRoster->update(dt); break;
            case ActiveView::RelicEditor: relicEditor->update(dt); break;
            case ActiveView::LightCone:   lightConeScreen->update(dt); break;
            case ActiveView::Enemies:     enemiesScreen->update(dt); break;
            case ActiveView::Simulation:  simulationScreen->update(dt); break;
        }

        BeginDrawing();
        ClearBackground(Color{18, 20, 27, 255});

        // Simulation runs as a dedicated full-window view (its own
        // window; Raylib hosts a single OS window). The sidebar is
        // hidden so sim panels own the full 1440x900 area; ESC returns.
        const bool simFullscreen = (activeView == ActiveView::Simulation);
        int navClick = -1;
        if (!simFullscreen)
            navClick = sidebar.updateAndDraw(activeNav);

        if (navClick == 0)
        {
            activeView = ActiveView::TeamBuilder;
            activeNav = 0;
        }
        else if (navClick == 1)
        {
            goToCharactersScreen();
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
        else if (navClick == 5 || navClick == 6)
        {
            // ROTATION (5) and SIMULATE (6) share the simulation view:
            // rotation editing lives inside SimulationScreen.
            goToSimulationScreen(navClick);
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

            case ActiveView::Characters:
            {
                charactersScreen->draw();
                std::string selected;

                // Selection is acknowledged for now; team handoff comes later.
                if (charactersScreen->consumeSelection(selected))
                {
                    TraceLog(LOG_INFO, "CharactersScreen selected: %s",
                             selected.c_str());
                }

                if (charactersScreen->consumeBackRequest())
                {
                    activeView = ActiveView::TeamBuilder;
                    activeNav = 0;
                }
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
    charactersScreen.reset();
    teamBuilder.reset();

    assets.unloadAll();
    CloseWindow();
}

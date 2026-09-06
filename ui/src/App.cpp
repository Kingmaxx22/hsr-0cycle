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
        TraceLog(LOG_ERROR, "Failed to load character rules from: %s", HSR_ENGINE_DATA_DIR);
        return false;
    }

    if (!relicSets.load(HSR_ENGINE_DATA_DIR))
    {
        // Not fatal -- the gear editor still works, it just won't offer any
        // named relic/planar sets to cycle through.
        TraceLog(LOG_WARNING, "Failed to load relic set rules from: %s", HSR_ENGINE_DATA_DIR);
    }

    teamBuilder = std::make_unique<TeamBuilderScreen>(assets, characters);
    teamBuilder->initialize();

    relicRoster = std::make_unique<RelicRosterScreen>(assets, characters);
    relicRoster->initialize();

    relicEditor = std::make_unique<RelicEditorScreen>(assets, characters, relicSets, loadouts);
    relicEditor->initialize();

    return true;
}

void App::goToRelicEditor(const std::string& characterId)
{
    relicEditor->setCharacter(characterId);
    activeView = ActiveView::RelicEditor;
    activeNav = 3; // RELICS
}

void App::run()
{
    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        switch (activeView)
        {
            case ActiveView::TeamBuilder: teamBuilder->update(dt); break;
            case ActiveView::RelicRoster: relicRoster->update(dt); break;
            case ActiveView::RelicEditor: relicEditor->update(dt); break;
        }

        BeginDrawing();
        ClearBackground(Color{18, 20, 27, 255});

        int navClick = sidebar.updateAndDraw(activeNav);
        if (navClick == 0)
        {
            activeView = ActiveView::TeamBuilder;
            activeNav = 0;
        }
        else if (navClick == 3)
        {
            activeView = ActiveView::RelicRoster;
            activeNav = 3;
        }
        else if (navClick >= 0)
        {
            // Other nav entries aren't wired to a screen yet -- just highlight.
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
            {
                relicEditor->draw();
                if (relicEditor->consumeBackRequest())
                {
                    activeView = ActiveView::RelicRoster;
                    activeNav = 3;
                }
                break;
            }
        }

        EndDrawing();
    }
}

void App::shutdown()
{
    relicEditor.reset();
    relicRoster.reset();
    teamBuilder.reset();
    assets.unloadAll();
    CloseWindow();
}

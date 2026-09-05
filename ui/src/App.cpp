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

    teamBuilder = std::make_unique<TeamBuilderScreen>(assets, characters);
    teamBuilder->initialize();

    return true;
}

void App::run()
{
    while (!WindowShouldClose())
    {
        teamBuilder->update(GetFrameTime());

        BeginDrawing();
        teamBuilder->draw();
        EndDrawing();
    }
}

void App::shutdown()
{
    teamBuilder.reset();
    assets.unloadAll();
    CloseWindow();
}

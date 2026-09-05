$appCpp = @"
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
"@
Set-Content -Path "C:\hsr-0cycle\ui\src\App.cpp" -Value $appCpp -Encoding UTF8

$teamBuilderCpp = @"
#include "TeamBuilderScreen.h"
#include "raylib.h"

#include <algorithm>

TeamBuilderScreen::TeamBuilderScreen(AssetManager& assets_, CharacterDatabase& characters_)
    : assets(assets_), characters(characters_)
{
}

void TeamBuilderScreen::initialize()
{
}

void TeamBuilderScreen::update(float dt)
{
    time += dt;

    const auto& roster = characters.all();
    int rows = static_cast<int>((roster.size() + kLibraryCols - 1) / kLibraryCols);
    float contentH = rows * (kLibraryCardH + kLibraryGapY);
    float maxScroll = std::max(0.0f, contentH - kLibraryViewportH);

    Vector2 mouse = GetMousePosition();
    Rectangle libraryViewport{
        kLibraryOriginX - 10.0f, kLibraryOriginY - 10.0f,
        kLibraryCols * (kLibraryCardW + kLibraryGapX) + 10.0f,
        kLibraryViewportH + 20.0f
    };
    if (CheckCollisionPointRec(mouse, libraryViewport))
    {
        libraryScroll -= GetMouseWheelMove() * 40.0f;
        libraryScroll = std::clamp(libraryScroll, 0.0f, maxScroll);
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        for (int i = 0; i < 4; ++i)
        {
            Rectangle r{360.0f + i * 250.0f, 150.0f, 220.0f, 250.0f};
            if (CheckCollisionPointRec(mouse, r))
                selectedSlot = i;
        }

        if (CheckCollisionPointRec(mouse, libraryViewport))
        {
            for (size_t i = 0; i < roster.size(); ++i)
            {
                int col = static_cast<int>(i) % kLibraryCols;
                int row = static_cast<int>(i) / kLibraryCols;

                Rectangle r{
                    kLibraryOriginX + col * (kLibraryCardW + kLibraryGapX),
                    kLibraryOriginY + row * (kLibraryCardH + kLibraryGapY) - libraryScroll,
                    kLibraryCardW,
                    kLibraryCardH
                };

                if (r.y + r.height < kLibraryOriginY || r.y > kLibraryOriginY + kLibraryViewportH)
                    continue;

                if (CheckCollisionPointRec(mouse, r))
                    team[selectedSlot] = roster[i].id;
            }
        }
    }
}

void TeamBuilderScreen::draw()
{
    ClearBackground(Color{18, 20, 27, 255});

    drawSidebar();
    drawHeader();
    drawTeamSlots();
    drawCharacterLibrary();
}

void TeamBuilderScreen::drawSidebar()
{
    DrawRectangle(0, 0, 270, GetScreenHeight(), Color{12, 14, 19, 255});
    DrawRectangle(0, 0, 270, 72, Color{25, 28, 38, 255});

    DrawText("HSR", 28, 17, 34, RAYWHITE);
    DrawText("0-CYCLE", 87, 23, 20, GRAY);

    const char* nav[] = {
        "TEAM BUILDER",
        "CHARACTERS",
        "LIGHT CONES",
        "RELICS",
        "ENEMIES",
        "ROTATION",
        "SIMULATE",
        "RULES"
    };

    for (int i = 0; i < 8; ++i)
    {
        Rectangle r{18.0f, 100.0f + i * 58.0f, 234.0f, 46.0f};

        if (i == activeNav)
            DrawRectangleRounded(r, 0.25f, 8, Color{55, 62, 82, 255});

        DrawText(nav[i], 34, static_cast<int>(r.y + 13), 17,
                 i == activeNav ? RAYWHITE : Color{155, 160, 174, 255});
    }

    DrawText("ENGINE", 28, GetScreenHeight() - 82, 13, GRAY);
    DrawText("Rule engine: connected later", 28, GetScreenHeight() - 58, 13,
             Color{115, 120, 132, 255});
}

void TeamBuilderScreen::drawHeader()
{
    DrawText("Team Builder", 310, 30, 34, RAYWHITE);
    DrawText("Build a team, then configure equipment and combat rules.",
             312, 72, 16, Color{145, 150, 164, 255});

    DrawLine(310, 105, GetScreenWidth() - 30, 105,
             Color{48, 52, 64, 255});
}

void TeamBuilderScreen::drawTeamSlots()
{
    DrawText("YOUR TEAM", 320, 125, 17, Color{180, 185, 198, 255});

    for (int i = 0; i < 4; ++i)
    {
        Rectangle r{360.0f + i * 250.0f, 150.0f, 220.0f, 250.0f};

        bool selected = i == selectedSlot;
        DrawRectangleRounded(r, 0.08f, 8,
            selected ? Color{44, 52, 70, 255} : Color{28, 31, 41, 255});

        DrawRectangleRoundedLines(r, 0.08f, 8,
            selected ? Color{115, 140, 190, 255} : Color{55, 59, 72, 255});

        if (team[i].empty())
        {
            DrawText("+", static_cast<int>(r.x + 92), static_cast<int>(r.y + 72),
                     72, Color{95, 100, 115, 255});
            DrawText("EMPTY SLOT", static_cast<int>(r.x + 62),
                     static_cast<int>(r.y + 165), 13, GRAY);
        }
        else
        {
            Texture2D* tex = assets.character(team[i]);

            if (tex)
            {
                float scale = 180.0f / static_cast<float>(tex->width);
                if (tex->height * scale > 180.0f)
                    scale = 180.0f / static_cast<float>(tex->height);

                float w = tex->width * scale;
                float h = tex->height * scale;

                Rectangle src{0, 0, static_cast<float>(tex->width),
                              static_cast<float>(tex->height)};
                Rectangle dst{
                    r.x + (r.width - w) / 2.0f,
                    r.y + 15.0f,
                    w, h
                };

                DrawTexturePro(*tex, src, dst, {0, 0}, 0, WHITE);
            }

            DrawText(team[i].c_str(), static_cast<int>(r.x + 12),
                     static_cast<int>(r.y + 213), 14, RAYWHITE);
        }

        DrawText(TextFormat("%d", i + 1),
                 static_cast<int>(r.x + 8), static_cast<int>(r.y + 8),
                 13, Color{115, 120, 135, 255});
    }
}

void TeamBuilderScreen::drawCharacterLibrary()
{
    const auto& roster = characters.all();

    DrawText(TextFormat("CHARACTER LIBRARY (%d)", static_cast<int>(roster.size())),
              320, 445, 17, Color{180, 185, 198, 255});

    Rectangle clip{
        kLibraryOriginX - 10.0f, kLibraryOriginY - 10.0f,
        kLibraryCols * (kLibraryCardW + kLibraryGapX) + 10.0f,
        kLibraryViewportH + 20.0f
    };

    BeginScissorMode(static_cast<int>(clip.x), static_cast<int>(clip.y),
                      static_cast<int>(clip.width), static_cast<int>(clip.height));

    for (size_t i = 0; i < roster.size(); ++i)
    {
        int col = static_cast<int>(i) % kLibraryCols;
        int row = static_cast<int>(i) / kLibraryCols;

        Rectangle r{
            kLibraryOriginX + col * (kLibraryCardW + kLibraryGapX),
            kLibraryOriginY + row * (kLibraryCardH + kLibraryGapY) - libraryScroll,
            kLibraryCardW,
            kLibraryCardH
        };

        if (r.y + r.height < kLibraryOriginY || r.y > kLibraryOriginY + kLibraryViewportH)
            continue;

        CharacterCard::draw(assets, roster[i].id, r, roster[i].id == team[selectedSlot]);
    }

    EndScissorMode();

    int rows = static_cast<int>((roster.size() + kLibraryCols - 1) / kLibraryCols);
    float contentH = rows * (kLibraryCardH + kLibraryGapY);
    float maxScroll = std::max(0.0f, contentH - kLibraryViewportH);
    if (maxScroll > 0.0f)
    {
        float trackX = clip.x + clip.width + 6.0f;
        DrawRectangle(static_cast<int>(trackX), static_cast<int>(clip.y), 4,
                      static_cast<int>(clip.height), Color{40, 43, 55, 255});

        float thumbH = clip.height * (kLibraryViewportH / contentH);
        float thumbY = clip.y + (libraryScroll / maxScroll) * (clip.height - thumbH);
        DrawRectangle(static_cast<int>(trackX), static_cast<int>(thumbY), 4,
                      static_cast<int>(thumbH), Color{115, 140, 190, 255});
    }
}
"@
Set-Content -Path "C:\hsr-0cycle\ui\src\screens\TeamBuilderScreen.cpp" -Value $teamBuilderCpp -Encoding UTF8

Write-Host "Done writing App.cpp and TeamBuilderScreen.cpp"
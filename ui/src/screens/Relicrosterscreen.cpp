#include "RelicRosterScreen.h"
#include "../widgets/CharacterCard.h"
#include "raylib.h"

#include <algorithm>
#include <cctype>

namespace
{
    std::string toLowerCopy(const std::string& s)
    {
        std::string out = s;
        std::transform(out.begin(), out.end(), out.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return out;
    }
}

RelicRosterScreen::RelicRosterScreen(AssetManager& assets_, CharacterDatabase& characters_)
    : assets(assets_), characters(characters_)
{
}

void RelicRosterScreen::initialize()
{
    updateFilteredRoster();
}

Rectangle RelicRosterScreen::searchBoxBounds() const
{
    return Rectangle{310.0f, 108.0f, 260.0f, 34.0f};
}

Rectangle RelicRosterScreen::searchClearButtonBounds() const
{
    Rectangle search = searchBoxBounds();
    return Rectangle{search.x + search.width - 28.0f, search.y + 5.0f, 22.0f, 24.0f};
}

void RelicRosterScreen::updateFilteredRoster()
{
    filteredRoster.clear();
    std::string lowerQuery = toLowerCopy(searchQuery);

    for (const auto& c : characters.all())
    {
        if (!lowerQuery.empty())
        {
            std::string lowerName = toLowerCopy(c.name);
            if (lowerName.find(lowerQuery) == std::string::npos)
                continue;
        }
        filteredRoster.push_back(&c);
    }
}

void RelicRosterScreen::update(float dt)
{
    (void)dt;
    updateFilteredRoster();

    int rows = static_cast<int>((filteredRoster.size() + kCols - 1) / kCols);
    float contentH = rows * (kCardH + kGapY);
    float maxScroll = std::max(0.0f, contentH - kViewportH);
    scroll = std::clamp(scroll, 0.0f, maxScroll);

    Vector2 mouse = GetMousePosition();
    Rectangle viewport{
        kOriginX - 10.0f, kOriginY - 10.0f,
        kCols * (kCardW + kGapX) + 10.0f,
        kViewportH + 20.0f
    };

    if (CheckCollisionPointRec(mouse, viewport))
    {
        scroll -= GetMouseWheelMove() * 40.0f;
        scroll = std::clamp(scroll, 0.0f, maxScroll);
    }

    int key = GetCharPressed();
    while (key > 0)
    {
        if (key >= 32 && key <= 126 && searchQuery.size() < 32)
            searchQuery += static_cast<char>(key);
        key = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE) && !searchQuery.empty())
        searchQuery.pop_back();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        if (!searchQuery.empty() && CheckCollisionPointRec(mouse, searchClearButtonBounds()))
            searchQuery.clear();

        if (CheckCollisionPointRec(mouse, viewport))
        {
            for (size_t i = 0; i < filteredRoster.size(); ++i)
            {
                int col = static_cast<int>(i) % kCols;
                int row = static_cast<int>(i) / kCols;

                Rectangle r{
                    kOriginX + col * (kCardW + kGapX),
                    kOriginY + row * (kCardH + kGapY) - scroll,
                    kCardW, kCardH
                };

                if (r.y + r.height < kOriginY || r.y > kOriginY + kViewportH)
                    continue;

                if (CheckCollisionPointRec(mouse, r))
                    pendingSelection = filteredRoster[i]->id;
            }
        }
    }
}

void RelicRosterScreen::draw()
{
    DrawText("Relics & Planar Ornaments", 310, 30, 34, RAYWHITE);
    DrawText("Pick a character to configure their gear.", 312, 72, 16,
             Color{145, 150, 164, 255});
    DrawLine(310, 105, GetScreenWidth() - 30, 105, Color{48, 52, 64, 255});

    Rectangle search = searchBoxBounds();
    DrawRectangleRounded(search, 0.2f, 8, Color{28, 31, 41, 255});
    DrawRectangleRoundedLines(search, 0.2f, 8, Color{55, 59, 72, 255});

    if (searchQuery.empty())
    {
        DrawText("Search characters...", static_cast<int>(search.x + 12),
                 static_cast<int>(search.y + 9), 15, Color{110, 115, 128, 255});
    }
    else
    {
        DrawText(searchQuery.c_str(), static_cast<int>(search.x + 12),
                 static_cast<int>(search.y + 9), 15, RAYWHITE);
        Rectangle clear = searchClearButtonBounds();
        DrawText("x", static_cast<int>(clear.x + 6), static_cast<int>(clear.y + 4), 16,
                 Color{160, 165, 178, 255});
    }

    Rectangle clip{
        kOriginX - 10.0f, kOriginY - 10.0f,
        kCols * (kCardW + kGapX) + 10.0f,
        kViewportH + 20.0f
    };

    BeginScissorMode(static_cast<int>(clip.x), static_cast<int>(clip.y),
                      static_cast<int>(clip.width), static_cast<int>(clip.height));

    for (size_t i = 0; i < filteredRoster.size(); ++i)
    {
        int col = static_cast<int>(i) % kCols;
        int row = static_cast<int>(i) / kCols;

        Rectangle r{
            kOriginX + col * (kCardW + kGapX),
            kOriginY + row * (kCardH + kGapY) - scroll,
            kCardW, kCardH
        };

        if (r.y + r.height < kOriginY || r.y > kOriginY + kViewportH)
            continue;

        CharacterCard::draw(assets, filteredRoster[i]->id, r, false);
    }

    EndScissorMode();
}

bool RelicRosterScreen::consumeSelection(std::string& outCharacterId)
{
    if (pendingSelection.empty())
        return false;
    outCharacterId = pendingSelection;
    pendingSelection.clear();
    return true;
}
#include "TeamBuilderScreen.h"
#include "raylib.h"

#include <algorithm>
#include <cctype>
#include <unordered_set>

static std::string toLowerCopy(const std::string& s)
{
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

// Preferred display order for known values; anything present in the data but
// not listed here is appended alphabetically so nothing is ever silently hidden.
static const std::vector<std::string> kCanonicalElementOrder = {
    "physical", "fire", "ice", "lightning", "wind", "quantum", "imaginary"
};

static const std::vector<std::string> kCanonicalPathOrder = {
    "Destruction", "Hunt", "Erudition", "Harmony", "Nihility",
    "Preservation", "Abundance", "Remembrance", "Elation"
};

static std::vector<std::string> orderedUnique(const std::unordered_set<std::string>& present,
                                               const std::vector<std::string>& canonical)
{
    std::vector<std::string> result;
    for (const auto& v : canonical)
        if (present.count(v))
            result.push_back(v);

    std::vector<std::string> extras;
    for (const auto& v : present)
        if (std::find(canonical.begin(), canonical.end(), v) == canonical.end())
            extras.push_back(v);
    std::sort(extras.begin(), extras.end());
    result.insert(result.end(), extras.begin(), extras.end());
    return result;
}

std::string TeamBuilderScreen::capitalize(const std::string& s)
{
    if (s.empty())
        return s;
    std::string out = s;
    out[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(out[0])));
    return out;
}

Color TeamBuilderScreen::elementColor(const std::string& element)
{
    if (element == "physical")  return Color{200, 200, 200, 255};
    if (element == "fire")      return Color{230, 110, 70, 255};
    if (element == "ice")       return Color{110, 190, 230, 255};
    if (element == "lightning") return Color{175, 120, 230, 255};
    if (element == "wind")      return Color{95, 200, 150, 255};
    if (element == "quantum")   return Color{95, 100, 210, 255};
    if (element == "imaginary") return Color{230, 205, 90, 255};
    return Color{160, 165, 178, 255};
}

TeamBuilderScreen::TeamBuilderScreen(AssetManager& assets_, CharacterDatabase& characters_)
    : assets(assets_), characters(characters_)
{
}

void TeamBuilderScreen::initialize()
{
    std::unordered_set<std::string> elementsPresent;
    std::unordered_set<std::string> pathsPresent;
    for (const auto& c : characters.all())
    {
        if (!c.element.empty())
            elementsPresent.insert(c.element);
        if (!c.path.empty())
            pathsPresent.insert(c.path);
    }

    elementOrder = orderedUnique(elementsPresent, kCanonicalElementOrder);
    pathOrder = orderedUnique(pathsPresent, kCanonicalPathOrder);

    elementEnabled.clear();
    for (const auto& e : elementOrder)
        elementEnabled[e] = true;

    pathEnabled.clear();
    for (const auto& p : pathOrder)
        pathEnabled[p] = true;

    updateFilteredRoster();
}

Rectangle TeamBuilderScreen::searchBoxBounds() const
{
    return Rectangle{320.0f, 438.0f, 250.0f, 34.0f};
}

Rectangle TeamBuilderScreen::searchClearButtonBounds() const
{
    Rectangle search = searchBoxBounds();
    return Rectangle{search.x + search.width - 28.0f, search.y + 5.0f, 22.0f, 24.0f};
}

Rectangle TeamBuilderScreen::fourStarButtonBounds() const
{
    return Rectangle{584.0f, 438.0f, 55.0f, 34.0f};
}

Rectangle TeamBuilderScreen::fiveStarButtonBounds() const
{
    return Rectangle{647.0f, 438.0f, 55.0f, 34.0f};
}

void TeamBuilderScreen::updateFilterChipBounds()
{
    auto layoutRow = [](const std::vector<std::string>& order, float y,
                         float leftPad, const auto& labelFn) {
        std::vector<Rectangle> bounds;
        constexpr float startX = 320.0f;
        constexpr float padX = 14.0f;
        constexpr float gap = 8.0f;

        float x = startX;
        for (const auto& key : order)
        {
            std::string label = labelFn(key);
            int textW = MeasureText(label.c_str(), 14);
            float w = static_cast<float>(textW) + padX + leftPad;
            bounds.push_back(Rectangle{x, y, w, kFilterChipH});
            x += w + gap;
        }
        return bounds;
    };

    // Element chips reserve extra left padding for their color dot.
    elementChipBounds = layoutRow(elementOrder, kElementRowY, 26.0f,
        [](const std::string& e) { return capitalize(e); });
    pathChipBounds = layoutRow(pathOrder, kPathRowY, 14.0f,
        [](const std::string& p) { return p; });
}

void TeamBuilderScreen::updateFilteredRoster()
{
    filteredRoster.clear();

    std::string lowerQuery = toLowerCopy(searchQuery);

    for (const auto& c : characters.all())
    {
        if (c.rarity == 4 && !showFourStar)
            continue;
        if (c.rarity == 5 && !showFiveStar)
            continue;

        auto elemIt = elementEnabled.find(c.element);
        if (elemIt != elementEnabled.end() && !elemIt->second)
            continue;

        auto pathIt = pathEnabled.find(c.path);
        if (pathIt != pathEnabled.end() && !pathIt->second)
            continue;

        if (!lowerQuery.empty())
        {
            std::string lowerName = toLowerCopy(c.name);
            if (lowerName.find(lowerQuery) == std::string::npos)
                continue;
        }

        filteredRoster.push_back(&c);
    }
}

void TeamBuilderScreen::update(float dt)
{
    time += dt;

    updateFilterChipBounds();
    updateFilteredRoster();

    int rows = static_cast<int>((filteredRoster.size() + kLibraryCols - 1) / kLibraryCols);
    float contentH = rows * (kLibraryCardH + kLibraryGapY);
    float maxScroll = std::max(0.0f, contentH - kLibraryViewportH);
    libraryScroll = std::clamp(libraryScroll, 0.0f, maxScroll);

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

    // Search box text input — this screen has only one text field, so it's always active.
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
        for (int i = 0; i < 4; ++i)
        {
            Rectangle r{360.0f + i * 250.0f, 150.0f, 220.0f, 250.0f};
            if (CheckCollisionPointRec(mouse, r))
            {
                selectedSlot = i;
                if (!team[i].empty())
                    pendingEditorRequest = team[i];
            }
        }

        if (!searchQuery.empty() && CheckCollisionPointRec(mouse, searchClearButtonBounds()))
            searchQuery.clear();

        if (CheckCollisionPointRec(mouse, fourStarButtonBounds()))
            showFourStar = !showFourStar;

        if (CheckCollisionPointRec(mouse, fiveStarButtonBounds()))
            showFiveStar = !showFiveStar;

        for (size_t i = 0; i < elementChipBounds.size(); ++i)
        {
            if (CheckCollisionPointRec(mouse, elementChipBounds[i]))
            {
                bool& enabled = elementEnabled[elementOrder[i]];
                enabled = !enabled;
            }
        }

        for (size_t i = 0; i < pathChipBounds.size(); ++i)
        {
            if (CheckCollisionPointRec(mouse, pathChipBounds[i]))
            {
                bool& enabled = pathEnabled[pathOrder[i]];
                enabled = !enabled;
            }
        }

        if (CheckCollisionPointRec(mouse, libraryViewport))
        {
            for (size_t i = 0; i < filteredRoster.size(); ++i)
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
                    team[selectedSlot] = filteredRoster[i]->id;
            }
        }
    }
}

void TeamBuilderScreen::draw()
{
    drawHeader();
    drawTeamSlots();
    drawSearchAndFilters();
    drawCharacterLibrary();
}

bool TeamBuilderScreen::consumeEditorRequest(std::string& outCharacterId)
{
    if (pendingEditorRequest.empty())
        return false;
    outCharacterId = pendingEditorRequest;
    pendingEditorRequest.clear();
    return true;
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

void TeamBuilderScreen::drawSearchAndFilters()
{
    DrawText("CHARACTER LIBRARY", 320, 408, 17, Color{180, 185, 198, 255});

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

    Rectangle four = fourStarButtonBounds();
    DrawRectangleRounded(four, 0.2f, 8,
        showFourStar ? Color{55, 62, 82, 255} : Color{28, 31, 41, 255});
    DrawRectangleRoundedLines(four, 0.2f, 8,
        showFourStar ? Color{115, 140, 190, 255} : Color{55, 59, 72, 255});
    DrawText("4*", static_cast<int>(four.x + 16), static_cast<int>(four.y + 9), 16,
             showFourStar ? RAYWHITE : Color{140, 145, 158, 255});

    Rectangle five = fiveStarButtonBounds();
    DrawRectangleRounded(five, 0.2f, 8,
        showFiveStar ? Color{55, 62, 82, 255} : Color{28, 31, 41, 255});
    DrawRectangleRoundedLines(five, 0.2f, 8,
        showFiveStar ? Color{115, 140, 190, 255} : Color{55, 59, 72, 255});
    DrawText("5*", static_cast<int>(five.x + 16), static_cast<int>(five.y + 9), 16,
             showFiveStar ? RAYWHITE : Color{140, 145, 158, 255});

    DrawText(TextFormat("%d / %d shown",
              static_cast<int>(filteredRoster.size()),
              static_cast<int>(characters.all().size())),
              712, 447, 15, Color{140, 145, 158, 255});

    // Element filter chips — each gets a small color dot for its element.
    for (size_t i = 0; i < elementOrder.size() && i < elementChipBounds.size(); ++i)
    {
        const std::string& key = elementOrder[i];
        const Rectangle& r = elementChipBounds[i];
        bool active = elementEnabled.count(key) ? elementEnabled.at(key) : true;

        DrawRectangleRounded(r, 0.35f, 8,
            active ? Color{44, 48, 62, 255} : Color{24, 26, 34, 255});
        DrawRectangleRoundedLines(r, 0.35f, 8,
            active ? Color{95, 100, 118, 255} : Color{45, 48, 58, 255});

        Color dot = elementColor(key);
        if (!active)
            dot = Color{static_cast<unsigned char>(dot.r * 0.4f),
                        static_cast<unsigned char>(dot.g * 0.4f),
                        static_cast<unsigned char>(dot.b * 0.4f), 255};
        DrawCircle(static_cast<int>(r.x + 15.0f), static_cast<int>(r.y + r.height / 2.0f),
                   5.0f, dot);

        std::string label = capitalize(key);
        DrawText(label.c_str(), static_cast<int>(r.x + 26.0f),
                 static_cast<int>(r.y + 8.0f), 14,
                 active ? RAYWHITE : Color{130, 135, 148, 255});
    }

    // Path filter chips.
    for (size_t i = 0; i < pathOrder.size() && i < pathChipBounds.size(); ++i)
    {
        const std::string& key = pathOrder[i];
        const Rectangle& r = pathChipBounds[i];
        bool active = pathEnabled.count(key) ? pathEnabled.at(key) : true;

        DrawRectangleRounded(r, 0.35f, 8,
            active ? Color{55, 62, 82, 255} : Color{24, 26, 34, 255});
        DrawRectangleRoundedLines(r, 0.35f, 8,
            active ? Color{115, 140, 190, 255} : Color{45, 48, 58, 255});

        DrawText(key.c_str(), static_cast<int>(r.x + 14.0f),
                 static_cast<int>(r.y + 8.0f), 14,
                 active ? RAYWHITE : Color{130, 135, 148, 255});
    }
}

void TeamBuilderScreen::drawCharacterLibrary()
{
    Rectangle clip{
        kLibraryOriginX - 10.0f, kLibraryOriginY - 10.0f,
        kLibraryCols * (kLibraryCardW + kLibraryGapX) + 10.0f,
        kLibraryViewportH + 20.0f
    };

    BeginScissorMode(static_cast<int>(clip.x), static_cast<int>(clip.y),
                      static_cast<int>(clip.width), static_cast<int>(clip.height));

    for (size_t i = 0; i < filteredRoster.size(); ++i)
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

        CharacterCard::draw(assets, filteredRoster[i]->id, r,
                             filteredRoster[i]->id == team[selectedSlot]);
    }

    EndScissorMode();

    int rows = static_cast<int>((filteredRoster.size() + kLibraryCols - 1) / kLibraryCols);
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

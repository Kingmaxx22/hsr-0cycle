#include "LightConeScreen.h"
#include "raylib.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace
{
    const Color kPanelBg{28, 31, 41, 255};
    const Color kPanelBorder{55, 59, 72, 255};
    const Color kAccentBg{55, 62, 82, 255};
    const Color kAccentBorder{115, 140, 190, 255};
    const Color kDimText{140, 145, 158, 255};
    const Color kGold5Star{218, 172, 92, 255};
    const Color kPurple4Star{165, 115, 225, 255};
    const Color kBlue3Star{85, 145, 225, 255};

    std::string toLower(const std::string& s)
    {
        std::string out = s;
        std::transform(out.begin(), out.end(), out.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return out;
    }

    Color rarityColor(int rarity)
    {
        if (rarity >= 5) return kGold5Star;
        if (rarity == 4) return kPurple4Star;
        return kBlue3Star;
    }

    std::string truncateText(const std::string& text, int maxW, int fontSize)
    {
        if (MeasureText(text.c_str(), fontSize) <= maxW)
            return text;

        std::string out = text;
        while (!out.empty() && MeasureText((out + "...").c_str(), fontSize) > maxW)
            out.pop_back();

        return out + "...";
    }

    void drawWrappedText(const std::string& text, float x, float y, float maxW, float maxH, int fontSize, Color color, float lineSpacing = 3.0f)
    {
        std::istringstream words(text);
        std::string word;
        std::string line;
        float currentY = y;

        while (words >> word)
        {
            if (currentY + fontSize > y + maxH)
                break;

            std::string testLine = line.empty() ? word : line + " " + word;
            int textW = MeasureText(testLine.c_str(), fontSize);
            if (static_cast<float>(textW) > maxW && !line.empty())
            {
                DrawText(line.c_str(), static_cast<int>(x), static_cast<int>(currentY), fontSize, color);
                currentY += static_cast<float>(fontSize) + lineSpacing;
                line = word;
            }
            else
            {
                line = testLine;
            }
        }
        if (!line.empty() && currentY + fontSize <= y + maxH)
        {
            DrawText(line.c_str(), static_cast<int>(x), static_cast<int>(currentY), fontSize, color);
        }
    }
}

LightConeScreen::LightConeScreen(AssetManager& assets_, CharacterDatabase& characters_,
                                 LightConeDatabase& lightCones_, LoadoutStore& loadouts_)
    : assets(assets_), characters(characters_), lightCones(lightCones_), loadouts(loadouts_)
{
}

void LightConeScreen::initialize()
{
    updateFilteredList();
}

void LightConeScreen::setCharacter(const std::string& id)
{
    characterId = id;
    searchQuery.clear();
    searchActive = false;
    scrollOffset = 0.0f;

    if (!characterId.empty())
    {
        CharacterLoadout& loadout = loadouts[characterId];
        ensureLoadoutDefaults(loadout);
    }

    updateFilteredList();
}

void LightConeScreen::setTeamContext(const std::array<std::string, 4>& team,
                                     const std::string& activeCharacterId)
{
    teamMembers = team;
    searchQuery.clear();
    searchActive = false;
    scrollOffset = 0.0f;

    if (!activeCharacterId.empty())
    {
        setCharacter(activeCharacterId);
    }
    else
    {
        std::string found;
        for (const auto& member : teamMembers)
        {
            if (!member.empty())
            {
                found = member;
                break;
            }
        }
        setCharacter(found);
    }
}

bool LightConeScreen::consumeBackRequest()
{
    if (!backRequested)
        return false;
    backRequested = false;
    return true;
}

Rectangle LightConeScreen::backButtonBounds() const
{
    return Rectangle{310.0f, 30.0f, 90.0f, 34.0f};
}

Rectangle LightConeScreen::searchBoxBounds() const
{
    return Rectangle{710.0f, 120.0f, 210.0f, 34.0f};
}

Rectangle LightConeScreen::searchClearBounds() const
{
    Rectangle search = searchBoxBounds();
    return Rectangle{search.x + search.width - 26.0f, search.y + 6.0f, 20.0f, 22.0f};
}

Rectangle LightConeScreen::pathFilterBounds() const
{
    return Rectangle{930.0f, 120.0f, 170.0f, 34.0f};
}

Rectangle LightConeScreen::rarityFilterBounds(int rarityIndex) const
{
    // 0: All, 1: 5*, 2: 4*, 3: 3*
    float startX = 1110.0f;
    float widths[] = {48.0f, 54.0f, 54.0f, 54.0f};
    float x = startX;
    for (int i = 0; i < rarityIndex; ++i)
        x += widths[i] + 6.0f;

    return Rectangle{x, 120.0f, widths[rarityIndex], 34.0f};
}

void LightConeScreen::updateFilteredList()
{
    filteredList.clear();
    const CharacterInfo* charInfo = characters.get(characterId);
    std::string charPath = charInfo ? charInfo->path : "";
    std::string queryLower = toLower(searchQuery);

    for (const auto& lc : lightCones.all())
    {
        // Path match filter
        if (filterByPath && !charPath.empty() && lc.path != charPath)
            continue;

        // Rarity filter
        if (selectedRarity > 0 && lc.rarity != selectedRarity)
            continue;

        // Search text filter
        if (!queryLower.empty())
        {
            std::string nameLower = toLower(lc.name);
            if (nameLower.find(queryLower) == std::string::npos)
                continue;
        }

        filteredList.push_back(&lc);
    }

    // Sort by rarity descending, then name ascending
    std::sort(filteredList.begin(), filteredList.end(), [](const LightConeInfo* a, const LightConeInfo* b) {
        if (a->rarity != b->rarity)
            return a->rarity > b->rarity;
        return a->name < b->name;
    });
}

void LightConeScreen::update(float dt)
{
    (void)dt;

    Vector2 mouse = GetMousePosition();
    bool pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    if (characterId.empty())
    {
        if (pressed)
        {
            if (CheckCollisionPointRec(mouse, backButtonBounds()))
                backRequested = true;

            Rectangle emptyPromptBtn{310.0f, 190.0f, 200.0f, 36.0f};
            if (CheckCollisionPointRec(mouse, emptyPromptBtn))
                backRequested = true;
        }
        return;
    }

    CharacterLoadout& loadout = loadouts[characterId];

    // Check search box focus & input
    if (pressed)
    {
        if (CheckCollisionPointRec(mouse, searchBoxBounds()))
        {
            searchActive = true;
        }
        else
        {
            searchActive = false;
        }
    }

    if (searchActive)
    {
        int key = GetCharPressed();
        while (key > 0)
        {
            if (key >= 32 && key <= 125 && searchQuery.size() < 30)
            {
                searchQuery += static_cast<char>(key);
                updateFilteredList();
                scrollOffset = 0.0f;
            }
            key = GetCharPressed();
        }

        if (IsKeyPressed(KEY_BACKSPACE) && !searchQuery.empty())
        {
            searchQuery.pop_back();
            updateFilteredList();
            scrollOffset = 0.0f;
        }
    }

    if (pressed)
    {
        if (CheckCollisionPointRec(mouse, backButtonBounds()))
            backRequested = true;

        // Clear search
        if (!searchQuery.empty() && CheckCollisionPointRec(mouse, searchClearBounds()))
        {
            searchQuery.clear();
            updateFilteredList();
            scrollOffset = 0.0f;
        }

        // Header team switcher tabs
        float tabX = 410.0f;
        for (int i = 0; i < 4; ++i)
        {
            if (!teamMembers[i].empty())
            {
                const CharacterInfo* info = characters.get(teamMembers[i]);
                std::string label = TextFormat("[%d] %s", i + 1, info ? info->name.c_str() : teamMembers[i].c_str());
                int textW = MeasureText(label.c_str(), 14);
                float tabW = static_cast<float>(textW) + 24.0f;
                Rectangle tabRect{tabX, 68.0f, tabW, 26.0f};

                if (CheckCollisionPointRec(mouse, tabRect))
                {
                    setCharacter(teamMembers[i]);
                    return;
                }
                tabX += tabW + 8.0f;
            }
        }

        // Path filter toggle
        if (CheckCollisionPointRec(mouse, pathFilterBounds()))
        {
            filterByPath = !filterByPath;
            updateFilteredList();
            scrollOffset = 0.0f;
        }

        // Rarity filter chips
        int rarities[] = {0, 5, 4, 3};
        for (int i = 0; i < 4; ++i)
        {
            if (CheckCollisionPointRec(mouse, rarityFilterBounds(i)))
            {
                selectedRarity = rarities[i];
                updateFilteredList();
                scrollOffset = 0.0f;
            }
        }

        // Left Panel - Superimposition buttons & Unequip
        if (!loadout.lightConeId.empty())
        {
            // Superimposition buttons S1..S5
            float superX = 330.0f;
            for (int s = 1; s <= 5; ++s)
            {
                Rectangle sBtn{superX, 386.0f, 58.0f, 30.0f};
                if (CheckCollisionPointRec(mouse, sBtn))
                {
                    loadout.lightConeSuperimposition = s;
                }
                superX += 68.0f;
            }

            // Unequip button
            Rectangle unequipBtn{330.0f, 730.0f, 340.0f, 34.0f};
            if (CheckCollisionPointRec(mouse, unequipBtn))
            {
                loadout.lightConeId.clear();
            }
        }

        // Library grid card click to equip
        Rectangle gridArea{710.0f, 168.0f, 690.0f, 700.0f};
        if (CheckCollisionPointRec(mouse, gridArea))
        {
            for (size_t i = 0; i < filteredList.size(); ++i)
            {
                int col = static_cast<int>(i) % kCols;
                int row = static_cast<int>(i) / kCols;
                float cardX = 710.0f + col * (kCardW + kGapX);
                float cardY = 168.0f + row * (kCardH + kGapY) - scrollOffset;
                Rectangle cardRect{cardX, cardY, kCardW, kCardH};

                if (cardY + kCardH >= 168.0f && cardY <= 868.0f)
                {
                    if (CheckCollisionPointRec(mouse, cardRect))
                    {
                        loadout.lightConeId = filteredList[i]->id;
                        if (loadout.lightConeSuperimposition <= 0)
                            loadout.lightConeSuperimposition = 1;
                        break;
                    }
                }
            }
        }
    }

    // Grid scrolling
    Rectangle gridArea{710.0f, 168.0f, 690.0f, 700.0f};
    if (CheckCollisionPointRec(mouse, gridArea))
    {
        int totalRows = (static_cast<int>(filteredList.size()) + kCols - 1) / kCols;
        float totalHeight = static_cast<float>(totalRows) * (kCardH + kGapY);
        float maxScroll = std::max(0.0f, totalHeight - 670.0f);

        scrollOffset -= GetMouseWheelMove() * 40.0f;
        scrollOffset = std::clamp(scrollOffset, 0.0f, maxScroll);
    }
}

void LightConeScreen::draw()
{
    if (characterId.empty())
    {
        DrawText("Light Cones", 310, 30, 34, RAYWHITE);
        DrawLine(310, 105, GetScreenWidth() - 30, 105, Color{48, 52, 64, 255});

        DrawText("No character selected from Team Builder.", 310, 130, 18, Color{210, 180, 140, 255});
        DrawText("Please add a character to your team in Team Builder first.", 310, 158, 15, kDimText);

        Rectangle backBtn{310.0f, 190.0f, 200.0f, 36.0f};
        DrawRectangleRounded(backBtn, 0.2f, 8, kAccentBg);
        DrawRectangleRoundedLines(backBtn, 0.2f, 8, kAccentBorder);
        DrawText("< Go to Team Builder", static_cast<int>(backBtn.x + 18.0f),
                 static_cast<int>(backBtn.y + 10.0f), 15, RAYWHITE);
        return;
    }

    drawHeader();
    drawEquippedPanel();
    drawLibrary();
}

void LightConeScreen::drawHeader()
{
    const CharacterInfo* info = characters.get(characterId);
    std::string displayName = info ? info->name : characterId;

    Rectangle back = backButtonBounds();
    DrawRectangleRounded(back, 0.2f, 8, kPanelBg);
    DrawRectangleRoundedLines(back, 0.2f, 8, kPanelBorder);
    DrawText("< Back", static_cast<int>(back.x + 14.0f), static_cast<int>(back.y + 9.0f),
              15, RAYWHITE);

    DrawText(displayName.c_str(), 410, 28, 28, RAYWHITE);
    if (info)
    {
        std::string sub = info->path + " | " + info->element;
        DrawText(sub.c_str(), 410 + MeasureText(displayName.c_str(), 28) + 16, 36, 16, kDimText);
    }

    // Draw team member switcher tabs
    float tabX = 410.0f;
    int teamMemberCount = 0;
    for (int i = 0; i < 4; ++i)
    {
        if (!teamMembers[i].empty())
        {
            teamMemberCount++;
            bool isCurrent = (teamMembers[i] == characterId);
            const CharacterInfo* memberInfo = characters.get(teamMembers[i]);
            std::string label = TextFormat("[%d] %s", i + 1, memberInfo ? memberInfo->name.c_str() : teamMembers[i].c_str());
            int textW = MeasureText(label.c_str(), 14);
            float tabW = static_cast<float>(textW) + 24.0f;
            Rectangle tabRect{tabX, 68.0f, tabW, 26.0f};

            DrawRectangleRounded(tabRect, 0.25f, 8, isCurrent ? kAccentBg : kPanelBg);
            DrawRectangleRoundedLines(tabRect, 0.25f, 8, isCurrent ? kAccentBorder : kPanelBorder);
            DrawText(label.c_str(), static_cast<int>(tabRect.x + 12.0f),
                     static_cast<int>(tabRect.y + 6.0f), 14, isCurrent ? RAYWHITE : kDimText);

            tabX += tabW + 8.0f;
        }
    }

    if (teamMemberCount == 0)
    {
        DrawText("Light Cone Equipment & Superimposition", 412, 68, 15, kDimText);
    }

    DrawLine(310, 105, GetScreenWidth() - 30, 105, Color{48, 52, 64, 255});
}

void LightConeScreen::drawEquippedPanel()
{
    Rectangle panelRect{310.0f, 120.0f, 380.0f, 720.0f};
    DrawRectangleRounded(panelRect, 0.04f, 8, kPanelBg);
    DrawRectangleRoundedLines(panelRect, 0.04f, 8, kPanelBorder);

    CharacterLoadout& loadout = loadouts[characterId];
    const LightConeInfo* lc = lightCones.get(loadout.lightConeId);

    if (!lc)
    {
        DrawText("EQUIPPED LIGHT CONE", 330, 140, 16, kDimText);

        Rectangle emptySlot{340.0f, 190.0f, 320.0f, 280.0f};
        DrawRectangleRounded(emptySlot, 0.06f, 8, Color{22, 24, 32, 255});
        DrawRectangleRoundedLines(emptySlot, 0.06f, 8, Color{45, 48, 60, 255});

        DrawText("No Light Cone Equipped", 380, 310, 18, Color{180, 185, 195, 255});
        DrawText("Select a Light Cone from the library on the right to equip.", 355, 345, 13, kDimText);
        return;
    }

    Color rColor = rarityColor(lc->rarity);

    DrawText("EQUIPPED LIGHT CONE", 330, 136, 14, kDimText);

    // Artwork image
    Rectangle imgBox{330.0f, 162.0f, 130.0f, 175.0f};
    DrawRectangleRounded(imgBox, 0.06f, 8, Color{20, 22, 30, 255});
    DrawRectangleRoundedLines(imgBox, 0.06f, 8, rColor);

        Texture2D* tex = assets.lightCone(lc->assetId);
    if (tex)
    {
        Rectangle src{0, 0, static_cast<float>(tex->width), static_cast<float>(tex->height)};
        float scale = (imgBox.width - 10.0f) / static_cast<float>(tex->width);
        if (tex->height * scale > imgBox.height - 10.0f)
            scale = (imgBox.height - 10.0f) / static_cast<float>(tex->height);

        float w = tex->width * scale;
        float h = tex->height * scale;
        Rectangle dst{imgBox.x + (imgBox.width - w) / 2.0f, imgBox.y + (imgBox.height - h) / 2.0f, w, h};
        DrawTexturePro(*tex, src, dst, {0, 0}, 0.0f, WHITE);
    }

    // Name and metadata on the right of image
    float infoX = 475.0f;
    std::string truncatedName = truncateText(lc->name, 200, 16);
    DrawText(truncatedName.c_str(), static_cast<int>(infoX), 164, 16, RAYWHITE);

    DrawText(TextFormat("%d-Star  |  %s", lc->rarity, lc->path.c_str()), static_cast<int>(infoX), 190, 14, rColor);

    // Stats box at Lv 80
    DrawText("BASE STATS (Lv 80)", static_cast<int>(infoX), 220, 12, kDimText);
    DrawText(TextFormat("HP:   %d", lc->hp), static_cast<int>(infoX), 242, 14, Color{130, 220, 130, 255});
    DrawText(TextFormat("ATK: %d", lc->atk), static_cast<int>(infoX), 266, 14, Color{240, 130, 120, 255});
    DrawText(TextFormat("DEF: %d", lc->def), static_cast<int>(infoX), 290, 14, Color{120, 180, 240, 255});

    // Superimposition Selector / Slider
    DrawLine(330, 350, 670, 350, Color{45, 48, 60, 255});
    DrawText("SUPERIMPOSITION", 330, 362, 13, kDimText);

    int currentS = std::clamp(loadout.lightConeSuperimposition, 1, 5);
    float superX = 330.0f;
    for (int s = 1; s <= 5; ++s)
    {
        Rectangle sBtn{superX, 386.0f, 58.0f, 30.0f};
        bool active = (s == currentS);
        DrawRectangleRounded(sBtn, 0.25f, 6, active ? kAccentBg : Color{22, 24, 32, 255});
        DrawRectangleRoundedLines(sBtn, 0.25f, 6, active ? kAccentBorder : kPanelBorder);
        DrawText(TextFormat("S%d", s), static_cast<int>(sBtn.x + 18.0f), static_cast<int>(sBtn.y + 7.0f),
                 14, active ? RAYWHITE : kDimText);
        superX += 68.0f;
    }

    // Effect description
    DrawLine(330, 430, 670, 430, Color{45, 48, 60, 255});
    DrawText("PASSIVE EFFECT", 330, 442, 13, kDimText);

    Rectangle descBox{330.0f, 465.0f, 340.0f, 250.0f};
    DrawRectangleRounded(descBox, 0.05f, 6, Color{22, 24, 32, 255});
    DrawRectangleRoundedLines(descBox, 0.05f, 6, Color{40, 44, 55, 255});

    std::string effect = lc->effectDescription.empty() ? "No passive effect description available." : lc->effectDescription;
    drawWrappedText(effect, descBox.x + 12.0f, descBox.y + 10.0f, descBox.width - 24.0f, descBox.height - 20.0f, 13, Color{210, 215, 225, 255}, 4.0f);

    // Unequip button
    Rectangle unequipBtn{330.0f, 730.0f, 340.0f, 34.0f};
    DrawRectangleRounded(unequipBtn, 0.2f, 6, Color{48, 30, 35, 255});
    DrawRectangleRoundedLines(unequipBtn, 0.2f, 6, Color{140, 60, 70, 255});
    DrawText("Unequip Light Cone", static_cast<int>(unequipBtn.x + 95.0f),
             static_cast<int>(unequipBtn.y + 8.0f), 14, Color{240, 160, 170, 255});
}

void LightConeScreen::drawLibrary()
{
    // Search Box
    Rectangle searchBox = searchBoxBounds();
    DrawRectangleRounded(searchBox, 0.2f, 8, kPanelBg);
    DrawRectangleRoundedLines(searchBox, 0.2f, 8, searchActive ? kAccentBorder : kPanelBorder);

    if (searchQuery.empty())
    {
        DrawText("Search light cones...", static_cast<int>(searchBox.x + 12.0f),
                 static_cast<int>(searchBox.y + 9.0f), 14, kDimText);
    }
    else
    {
        DrawText(searchQuery.c_str(), static_cast<int>(searchBox.x + 12.0f),
                 static_cast<int>(searchBox.y + 9.0f), 14, RAYWHITE);
        Rectangle clearBtn = searchClearBounds();
        DrawText("x", static_cast<int>(clearBtn.x + 6.0f), static_cast<int>(clearBtn.y + 2.0f), 14, kDimText);
    }

    // Path Filter Button
    Rectangle pathBtn = pathFilterBounds();
    const CharacterInfo* charInfo = characters.get(characterId);
    std::string charPath = charInfo ? charInfo->path : "Path";
    std::string pathLabel = filterByPath ? (charPath + " Only") : "All Paths";

    DrawRectangleRounded(pathBtn, 0.2f, 8, filterByPath ? kAccentBg : kPanelBg);
    DrawRectangleRoundedLines(pathBtn, 0.2f, 8, filterByPath ? kAccentBorder : kPanelBorder);
    DrawText(pathLabel.c_str(), static_cast<int>(pathBtn.x + 14.0f), static_cast<int>(pathBtn.y + 9.0f),
             14, filterByPath ? RAYWHITE : kDimText);

    // Rarity Filters
    const char* rarityLabels[] = {"All", "5*", "4*", "3*"};
    int rarities[] = {0, 5, 4, 3};
    for (int i = 0; i < 4; ++i)
    {
        Rectangle rBtn = rarityFilterBounds(i);
        bool active = (selectedRarity == rarities[i]);
        DrawRectangleRounded(rBtn, 0.2f, 8, active ? kAccentBg : kPanelBg);
        DrawRectangleRoundedLines(rBtn, 0.2f, 8, active ? kAccentBorder : kPanelBorder);
        DrawText(rarityLabels[i], static_cast<int>(rBtn.x + (rBtn.width - MeasureText(rarityLabels[i], 14)) / 2.0f),
                 static_cast<int>(rBtn.y + 9.0f), 14, active ? RAYWHITE : kDimText);
    }

    // Light Cones Grid
    Rectangle scissorRect{710.0f, 168.0f, 690.0f, 672.0f};
    BeginScissorMode(static_cast<int>(scissorRect.x), static_cast<int>(scissorRect.y),
                     static_cast<int>(scissorRect.width), static_cast<int>(scissorRect.height));

    Vector2 mouse = GetMousePosition();
    CharacterLoadout& currentLoadout = loadouts[characterId];

    for (size_t i = 0; i < filteredList.size(); ++i)
    {
        const LightConeInfo* lc = filteredList[i];
        int col = static_cast<int>(i) % kCols;
        int row = static_cast<int>(i) / kCols;
        float cardX = 710.0f + col * (kCardW + kGapX);
        float cardY = 168.0f + row * (kCardH + kGapY) - scrollOffset;
        Rectangle cardRect{cardX, cardY, kCardW, kCardH};

        if (cardY + kCardH < 168.0f || cardY > 840.0f)
            continue;

        bool isHovered = CheckCollisionPointRec(mouse, cardRect);
        bool isEquipped = (currentLoadout.lightConeId == lc->id);

        Color rColor = rarityColor(lc->rarity);

        DrawRectangleRounded(cardRect, 0.06f, 6, isHovered ? Color{38, 43, 56, 255} : kPanelBg);
        DrawRectangleRoundedLines(cardRect, 0.06f, 6, isEquipped ? Color{120, 200, 255, 255} : (isHovered ? kAccentBorder : kPanelBorder));

        // Top Image
        Rectangle imgBox{cardRect.x + 8.0f, cardRect.y + 8.0f, cardRect.width - 16.0f, 115.0f};
        DrawRectangleRounded(imgBox, 0.05f, 4, Color{20, 22, 28, 255});

        Texture2D* tex = assets.lightCone(lc->assetId);
        if (tex)
        {
            Rectangle src{0, 0, static_cast<float>(tex->width), static_cast<float>(tex->height)};
            float scale = (imgBox.width - 6.0f) / static_cast<float>(tex->width);
            if (tex->height * scale > imgBox.height - 6.0f)
                scale = (imgBox.height - 6.0f) / static_cast<float>(tex->height);

            float w = tex->width * scale;
            float h = tex->height * scale;
            Rectangle dst{imgBox.x + (imgBox.width - w) / 2.0f, imgBox.y + (imgBox.height - h) / 2.0f, w, h};
            DrawTexturePro(*tex, src, dst, {0, 0}, 0.0f, WHITE);
        }

        // Rarity accent line
        DrawLine(static_cast<int>(cardRect.x + 8.0f), static_cast<int>(cardRect.y + 128.0f),
                 static_cast<int>(cardRect.x + cardRect.width - 8.0f), static_cast<int>(cardRect.y + 128.0f), rColor);

        // Name
        std::string displayName = truncateText(lc->name, static_cast<int>(cardRect.width - 16.0f), 12);
        DrawText(displayName.c_str(), static_cast<int>(cardRect.x + 8.0f), static_cast<int>(cardRect.y + 134.0f),
                 12, RAYWHITE);

        // Path & Stats
        DrawText(TextFormat("%s", lc->path.c_str()), static_cast<int>(cardRect.x + 8.0f),
                 static_cast<int>(cardRect.y + 152.0f), 11, kDimText);

        DrawText(TextFormat("ATK %d | HP %d", lc->atk, lc->hp), static_cast<int>(cardRect.x + 8.0f),
                 static_cast<int>(cardRect.y + 168.0f), 11, Color{180, 190, 205, 255});

        // Equipped badge
        if (isEquipped)
        {
            Rectangle eqBadge{cardRect.x + 8.0f, cardRect.y + 186.0f, cardRect.width - 16.0f, 18.0f};
            DrawRectangleRounded(eqBadge, 0.3f, 4, Color{30, 70, 120, 255});
            DrawText("EQUIPPED", static_cast<int>(eqBadge.x + (eqBadge.width - MeasureText("EQUIPPED", 11)) / 2.0f),
                     static_cast<int>(eqBadge.y + 3.0f), 11, Color{150, 220, 255, 255});
        }
    }

    EndScissorMode();

    // Scrollbar
    int totalRows = (static_cast<int>(filteredList.size()) + kCols - 1) / kCols;
    float totalHeight = static_cast<float>(totalRows) * (kCardH + kGapY);
    if (totalHeight > scissorRect.height)
    {
        float trackX = scissorRect.x + scissorRect.width - 5.0f;
        DrawRectangle(static_cast<int>(trackX), static_cast<int>(scissorRect.y), 3,
                      static_cast<int>(scissorRect.height), Color{35, 38, 48, 255});

        float maxScroll = totalHeight - scissorRect.height;
        float thumbH = (scissorRect.height / totalHeight) * scissorRect.height;
        float thumbY = scissorRect.y + (scrollOffset / maxScroll) * (scissorRect.height - thumbH);
        DrawRectangle(static_cast<int>(trackX), static_cast<int>(thumbY), 3,
                      static_cast<int>(thumbH), kAccentBorder);
    }
}

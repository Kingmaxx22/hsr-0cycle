#include "Reliceditorscreen.h"
#include "raylib.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace
{
    const Color kPanelBg{28, 31, 41, 255};
    const Color kPanelBorder{55, 59, 72, 255};
    const Color kAccentBg{55, 62, 82, 255};
    const Color kAccentBorder{115, 140, 190, 255};
    const Color kDimText{140, 145, 158, 255};
    const Color kDisabledBg{22, 24, 31, 255};

    std::string truncateSetName(const std::string& name, size_t maxChars)
    {
        if (name.size() <= maxChars)
            return name;
        return name.substr(0, maxChars - 1) + "...";
    }

    bool isPercentStat(const std::string& key)
    {
        return key == "hp_pct" ||
               key == "atk_pct" ||
               key == "def_pct" ||
               key == "crit_rate_pct" ||
               key == "crit_dmg_pct" ||
               key == "effect_hit_rate_pct" ||
               key == "effect_res_pct" ||
               key == "break_effect_pct";
    }

    std::string formatRoll(double value, bool percent)
    {
        std::ostringstream out;
        out << std::fixed << std::setprecision(3) << value;
        std::string text = out.str();

        while (text.size() > 1 && text.back() == '0')
            text.pop_back();
        if (!text.empty() && text.back() == '.')
            text.pop_back();

        if (percent)
            text += "%";
        return text;
    }
}

RelicEditorScreen::RelicEditorScreen(AssetManager& assets_, CharacterDatabase& characters_,
                                     RelicSetDatabase& relicSets_, LoadoutStore& loadouts_)
    : assets(assets_), characters(characters_), relicSets(relicSets_), loadouts(loadouts_)
{
}

void RelicEditorScreen::initialize()
{
}

void RelicEditorScreen::setCharacter(const std::string& id)
{
    characterId = id;
    activeDropdown = {};
    if (!characterId.empty())
    {
        CharacterLoadout& loadout = loadouts[characterId];
        ensureLoadoutDefaults(loadout);
    }
}

void RelicEditorScreen::setTeamContext(const std::array<std::string, 4>& team,
                                       const std::string& activeCharacterId)
{
    teamMembers = team;

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

bool RelicEditorScreen::consumeBackRequest()
{
    if (!backRequested)
        return false;
    backRequested = false;
    return true;
}

Rectangle RelicEditorScreen::backButtonBounds() const
{
    return Rectangle{310.0f, 30.0f, 90.0f, 34.0f};
}

Rectangle RelicEditorScreen::fourPieceModeButtonBounds() const
{
    return Rectangle{310.0f, 155.0f, 110.0f, 32.0f};
}

Rectangle RelicEditorScreen::twoPieceModeButtonBounds() const
{
    return Rectangle{430.0f, 155.0f, 90.0f, 32.0f};
}

Rectangle RelicEditorScreen::relicSetAButtonBounds() const
{
    if (characterId.empty() || loadouts.find(characterId) == loadouts.end())
        return Rectangle{310.0f, 197.0f, 470.0f, 34.0f};

    const CharacterLoadout& loadout = loadouts.find(characterId)->second;
    float width = loadout.relicFourPiece ? 470.0f : 225.0f;
    return Rectangle{310.0f, 197.0f, width, 34.0f};
}

Rectangle RelicEditorScreen::relicSetBButtonBounds() const
{
    return Rectangle{555.0f, 197.0f, 225.0f, 34.0f};
}

Rectangle RelicEditorScreen::planarSetButtonBounds() const
{
    return Rectangle{820.0f, 197.0f, 290.0f, 34.0f};
}

Rectangle RelicEditorScreen::gearCardBounds(GearSlot slot) const
{
    int index = static_cast<int>(slot);
    int col = index % kCols;
    int row = index / kCols;

    return Rectangle{
        kOriginX + col * (kCardW + kGapX),
        kOriginY + row * (kCardH + kGapY),
        kCardW, kCardH
    };
}

Rectangle RelicEditorScreen::getDropdownRect() const
{
    const float rowHeight = 40.0f;
    const int maxVisible = 7;

    int visible = std::min(static_cast<int>(activeDropdown.items.size()), maxVisible);
    float width = std::max(activeDropdown.anchorRect.width, 260.0f);
    float height = visible * rowHeight + 8.0f;

    float x = activeDropdown.anchorRect.x;
    float y = activeDropdown.anchorRect.y + activeDropdown.anchorRect.height + 4.0f;

    if (y + height > static_cast<float>(GetScreenHeight()) - 10.0f)
        y = activeDropdown.anchorRect.y - height - 4.0f;

    return Rectangle{x, y, width, height};
}

std::string RelicEditorScreen::nextSetName(const std::string& category,
                                            const std::string& current) const
{
    auto sets = relicSets.byCategory(category);
    if (sets.empty())
        return current;

    if (current.empty())
        return sets.front()->name;

    for (size_t i = 0; i < sets.size(); ++i)
    {
        if (sets[i]->name == current)
        {
            size_t nextIdx = i + 1;
            return nextIdx >= sets.size() ? std::string() : sets[nextIdx]->name;
        }
    }

    return sets.front()->name;
}

void RelicEditorScreen::openSetDropdown(ActiveDropdown::Mode mode, const std::string& category,
                                         const std::string& currentValue, Rectangle anchor)
{
    if (activeDropdown.open && activeDropdown.mode == mode)
    {
        activeDropdown.open = false;
        activeDropdown.mode = ActiveDropdown::Mode::None;
        return;
    }

    activeDropdown.open = true;
    activeDropdown.mode = mode;
    activeDropdown.anchorRect = anchor;
    activeDropdown.currentKey = currentValue;
    activeDropdown.hoveredIndex = -1;
    activeDropdown.scrollOffset = 0.0f;
    activeDropdown.slot = GearSlot::Head;
    activeDropdown.substatRow = -1;
    activeDropdown.items.clear();
    activeDropdown.items.push_back(DropdownItem{"", "-- None --", ""});

    for (const auto* set : relicSets.byCategory(category))
        activeDropdown.items.push_back(DropdownItem{set->name, set->name, set->id});
}

void RelicEditorScreen::openMainStatDropdown(GearSlot slot, Rectangle anchor)
{
    const auto& options = mainStatOptions(slot);
    if (options.size() <= 1)
        return;

    if (activeDropdown.open &&
        activeDropdown.mode == ActiveDropdown::Mode::MainStat &&
        activeDropdown.slot == slot)
    {
        activeDropdown.open = false;
        activeDropdown.mode = ActiveDropdown::Mode::None;
        return;
    }

    const GearPiece& piece = loadouts.find(characterId)->second.gear[static_cast<size_t>(slot)];

    activeDropdown.open = true;
    activeDropdown.mode = ActiveDropdown::Mode::MainStat;
    activeDropdown.slot = slot;
    activeDropdown.substatRow = -1;
    activeDropdown.anchorRect = anchor;
    activeDropdown.currentKey = piece.mainStat;
    activeDropdown.hoveredIndex = -1;
    activeDropdown.scrollOffset = 0.0f;
    activeDropdown.items.clear();

    for (const auto& option : options)
        activeDropdown.items.push_back(DropdownItem{option.key, option.label, ""});
}

std::vector<DropdownItem> RelicEditorScreen::buildSubstatItems(const GearPiece& piece, int row) const
{
    std::vector<DropdownItem> items;
    items.push_back(DropdownItem{"", "-- None --", ""});

    for (const auto& option : substatPool())
    {
        if (option.key == piece.mainStat)
            continue;

        bool usedElsewhere = false;
        for (int i = 0; i < 4; ++i)
        {
            if (i != row && piece.substats[i].statKey == option.key)
            {
                usedElsewhere = true;
                break;
            }
        }

        if (!usedElsewhere || option.key == piece.substats[row].statKey)
            items.push_back(DropdownItem{option.key, option.label, ""});
    }

    return items;
}

void RelicEditorScreen::openSubstatDropdown(GearSlot slot, int row, Rectangle anchor)
{
    const auto& loadout = loadouts.find(characterId)->second;
    const GearPiece& piece = loadout.gear[static_cast<size_t>(slot)];

    if (activeDropdown.open &&
        activeDropdown.mode == ActiveDropdown::Mode::Substat &&
        activeDropdown.slot == slot &&
        activeDropdown.substatRow == row)
    {
        activeDropdown.open = false;
        activeDropdown.mode = ActiveDropdown::Mode::None;
        return;
    }

    activeDropdown.open = true;
    activeDropdown.mode = ActiveDropdown::Mode::Substat;
    activeDropdown.slot = slot;
    activeDropdown.substatRow = row;
    activeDropdown.anchorRect = anchor;
    activeDropdown.currentKey = piece.substats[row].statKey;
    activeDropdown.hoveredIndex = -1;
    activeDropdown.scrollOffset = 0.0f;
    activeDropdown.items = buildSubstatItems(piece, row);
}

bool RelicEditorScreen::handleDropdownInput(Vector2 mouse, bool pressed)
{
    if (!activeDropdown.open)
        return false;

    const float rowHeight = 40.0f;
    Rectangle dropRect = getDropdownRect();

    activeDropdown.hoveredIndex = -1;
    if (CheckCollisionPointRec(mouse, dropRect))
    {
        int index = static_cast<int>(
            (mouse.y - dropRect.y - 4.0f + activeDropdown.scrollOffset) / rowHeight);
        if (index >= 0 && index < static_cast<int>(activeDropdown.items.size()))
            activeDropdown.hoveredIndex = index;
    }

    if (CheckCollisionPointRec(mouse, dropRect))
    {
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f)
        {
            float contentHeight = activeDropdown.items.size() * rowHeight;
            float maxScroll = std::max(0.0f, contentHeight - (dropRect.height - 8.0f));
            activeDropdown.scrollOffset =
                std::clamp(activeDropdown.scrollOffset - wheel * rowHeight, 0.0f, maxScroll);
        }
    }

    if (!pressed)
        return true;

    if (activeDropdown.hoveredIndex >= 0)
    {
        const DropdownItem& picked = activeDropdown.items[static_cast<size_t>(activeDropdown.hoveredIndex)];
        CharacterLoadout& loadout = loadouts[characterId];

        switch (activeDropdown.mode)
        {
            case ActiveDropdown::Mode::MainStat:
            {
                GearPiece& piece = loadout.gear[static_cast<size_t>(activeDropdown.slot)];
                piece.mainStat = picked.key;
                // New main stat starts at its 5-star max; stays editable.
                resetMainStatValueDefault(piece);

                for (auto& sub : piece.substats)
                {
                    if (sub.statKey == piece.mainStat)
                    {
                        sub.statKey.clear();
                        sub.valueText.clear();
                    }
                }
                break;
            }

            case ActiveDropdown::Mode::Substat:
            {
                SubstatRoll& sub = loadout.gear[static_cast<size_t>(activeDropdown.slot)]
                                             .substats[activeDropdown.substatRow];
                sub.statKey = picked.key;
                if (picked.key.empty())
                    sub.valueText.clear();
                break;
            }

            case ActiveDropdown::Mode::RelicSetA: loadout.relicSetA = picked.key; break;
            case ActiveDropdown::Mode::RelicSetB: loadout.relicSetB = picked.key; break;
            case ActiveDropdown::Mode::PlanarSet: loadout.planarSet = picked.key; break;
            default: break;
        }

        activeDropdown.open = false;
        activeDropdown.mode = ActiveDropdown::Mode::None;
        return true;
    }

    if (!CheckCollisionPointRec(mouse, dropRect) &&
        !CheckCollisionPointRec(mouse, activeDropdown.anchorRect))
    {
        activeDropdown.open = false;
        activeDropdown.mode = ActiveDropdown::Mode::None;
        return true;
    }

    return false;
}

void RelicEditorScreen::update(float dt)
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

    if (handleDropdownInput(mouse, pressed))
        return;

    if (!pressed)
        return;

    if (CheckCollisionPointRec(mouse, backButtonBounds()))
    {
        backRequested = true;
        return;
    }

    float tabX = 410.0f;
    for (int i = 0; i < 4; ++i)
    {
        if (!teamMembers[i].empty())
        {
            const CharacterInfo* memberInfo = characters.get(teamMembers[i]);
            std::string label = TextFormat(
                "[%d] %s", i + 1,
                memberInfo ? memberInfo->name.c_str() : teamMembers[i].c_str());
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

    if (CheckCollisionPointRec(mouse, fourPieceModeButtonBounds()))
    {
        loadout.relicFourPiece = true;
        return;
    }

    if (CheckCollisionPointRec(mouse, twoPieceModeButtonBounds()))
    {
        loadout.relicFourPiece = false;
        return;
    }

    if (CheckCollisionPointRec(mouse, relicSetAButtonBounds()))
    {
        openSetDropdown(ActiveDropdown::Mode::RelicSetA, "relic",
                        loadout.relicSetA, relicSetAButtonBounds());
        return;
    }

    if (!loadout.relicFourPiece &&
        CheckCollisionPointRec(mouse, relicSetBButtonBounds()))
    {
        openSetDropdown(ActiveDropdown::Mode::RelicSetB, "relic",
                        loadout.relicSetB, relicSetBButtonBounds());
        return;
    }

    if (CheckCollisionPointRec(mouse, planarSetButtonBounds()))
    {
        openSetDropdown(ActiveDropdown::Mode::PlanarSet, "planar_ornament",
                        loadout.planarSet, planarSetButtonBounds());
        return;
    }

    for (int i = 0; i < static_cast<int>(GearSlot::Count); ++i)
    {
        GearSlot slot = static_cast<GearSlot>(i);
        GearPiece& piece = loadout.gear[static_cast<size_t>(slot)];
        Rectangle card = gearCardBounds(slot);

        const auto& mainOptions = mainStatOptions(slot);
        Rectangle mainTypeRect{card.x + 14.0f, card.y + 34.0f, 190.0f, 28.0f};
        Rectangle mainValueRect{card.x + 212.0f, card.y + 34.0f, 124.0f, 28.0f};

        // Main-stat value box first: it sits beside the dropdown trigger.
        if (CheckCollisionPointRec(mouse, mainValueRect))
        {
            activeDropdown.open = false;
            activeDropdown.mode = ActiveDropdown::Mode::None;
            focusedField = -1;
            focusedMainSlot = i;
            return;
        }

        if (mainOptions.size() > 1 &&
            CheckCollisionPointRec(mouse, mainTypeRect))
        {
            openMainStatDropdown(slot, mainTypeRect);
            return;
        }

        for (int row = 0; row < 4; ++row)
        {
            float rowY = card.y + 70.0f + row * 32.0f;
            Rectangle typeRect{card.x + 14.0f, rowY, 180.0f, 26.0f};
            Rectangle valueRect{card.x + 202.0f, rowY, 134.0f, 26.0f};

            if (CheckCollisionPointRec(mouse, typeRect))
            {
                openSubstatDropdown(slot, row, typeRect);
                return;
            }

            if (!piece.substats[row].statKey.empty() &&
                CheckCollisionPointRec(mouse, valueRect))
            {
                // Free-text roll value: select/focus this field so the user
                // can type the exact value shown on the in-game relic.
                activeDropdown.open = false;
                activeDropdown.mode = ActiveDropdown::Mode::None;
                focusedField = i * 4 + row;
                focusedMainSlot = -1;
                return;
            }
        }
    }

    if (focusedMainSlot >= 0)
    {
        std::string& text = loadout.gear[static_cast<size_t>(focusedMainSlot)].mainStatValueText;

        int key = GetCharPressed();
        while (key > 0)
        {
            bool isDigit = key >= '0' && key <= '9';
            bool isDot = key == '.' && text.find('.') == std::string::npos;

            // Same convention as substats: digits + one dot, no "%" stored.
            if ((isDigit || isDot) && text.size() < 8)
                text += static_cast<char>(key);

            key = GetCharPressed();
        }

        if (IsKeyPressed(KEY_BACKSPACE) && !text.empty())
            text.pop_back();

        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE))
            focusedMainSlot = -1;
    }

    if (focusedField >= 0)
    {
        int slotIdx = focusedField / 4;
        int row = focusedField % 4;
        std::string& text = loadout.gear[static_cast<size_t>(slotIdx)]
                                    .substats[static_cast<size_t>(row)].valueText;

        int key = GetCharPressed();
        while (key > 0)
        {
            bool isDigit = key >= '0' && key <= '9';
            bool isDot = key == '.' && text.find('.') == std::string::npos;

            // Allow digits and one decimal point. Do not store the visual "%"
;
            // the renderer adds it for percentage stats.
            if ((isDigit || isDot) && text.size() < 8)
                text += static_cast<char>(key);

            key = GetCharPressed();
        }

        if (IsKeyPressed(KEY_BACKSPACE) && !text.empty())
            text.pop_back();

        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE))
            focusedField = -1;
    }
}

void RelicEditorScreen::draw()
{
    if (characterId.empty())
    {
        DrawText("Relics & Planar Ornaments", 310, 30, 34, RAYWHITE);
        DrawLine(310, 105, GetScreenWidth() - 30, 105, Color{48, 52, 64, 255});

        DrawText("No character selected from Team Builder.", 310, 130, 18,
                 Color{210, 180, 140, 255});
        DrawText("Please add a character to your team in Team Builder first.",
                 310, 158, 15, kDimText);

        Rectangle backBtn{310.0f, 190.0f, 200.0f, 36.0f};
        DrawRectangleRounded(backBtn, 0.2f, 8, kAccentBg);
        DrawRectangleRoundedLines(backBtn, 0.2f, 8, kAccentBorder);
        DrawText("< Go to Team Builder", static_cast<int>(backBtn.x + 18.0f),
                 static_cast<int>(backBtn.y + 10.0f), 15, RAYWHITE);
        return;
    }

    drawHeader();
    drawSetPanel();

    for (int i = 0; i < static_cast<int>(GearSlot::Count); ++i)
        drawGearCard(static_cast<GearSlot>(i));

    drawDropdown();
}

void RelicEditorScreen::drawHeader()
{
    const CharacterInfo* info = characters.get(characterId);
    std::string displayName = info ? info->name : characterId;

    Rectangle back = backButtonBounds();
    DrawRectangleRounded(back, 0.2f, 8, kPanelBg);
    DrawRectangleRoundedLines(back, 0.2f, 8, kPanelBorder);
    DrawText("< Back", static_cast<int>(back.x + 14.0f),
             static_cast<int>(back.y + 9.0f), 15, RAYWHITE);

    DrawText(displayName.c_str(), 410, 28, 28, RAYWHITE);
    if (info)
    {
        std::string sub = info->path + " | " + info->element;
        DrawText(sub.c_str(),
                 410 + MeasureText(displayName.c_str(), 28) + 16, 36, 16, kDimText);
    }

    float tabX = 410.0f;
    int teamMemberCount = 0;
    for (int i = 0; i < 4; ++i)
    {
        if (!teamMembers[i].empty())
        {
            teamMemberCount++;
            bool isCurrent = (teamMembers[i] == characterId);
            const CharacterInfo* memberInfo = characters.get(teamMembers[i]);
            std::string label = TextFormat(
                "[%d] %s", i + 1,
                memberInfo ? memberInfo->name.c_str() : teamMembers[i].c_str());
            int textW = MeasureText(label.c_str(), 14);
            float tabW = static_cast<float>(textW) + 24.0f;
            Rectangle tabRect{tabX, 68.0f, tabW, 26.0f};

            DrawRectangleRounded(tabRect, 0.25f, 8,
                                 isCurrent ? kAccentBg : kPanelBg);
            DrawRectangleRoundedLines(tabRect, 0.25f, 8,
                                      isCurrent ? kAccentBorder : kPanelBorder);
            DrawText(label.c_str(), static_cast<int>(tabRect.x + 12.0f),
                     static_cast<int>(tabRect.y + 6.0f), 14,
                     isCurrent ? RAYWHITE : kDimText);

            tabX += tabW + 8.0f;
        }
    }

    if (teamMemberCount == 0)
        DrawText("Relics & Planar Ornaments", 412, 68, 15, kDimText);

    DrawLine(310, 105, GetScreenWidth() - 30, 105, Color{48, 52, 64, 255});
}

void RelicEditorScreen::drawSetPanel()
{
    CharacterLoadout& loadout = loadouts[characterId];

    DrawText("RELIC SET", 310, 130, 15, kDimText);

    Rectangle fourBtn = fourPieceModeButtonBounds();
    DrawRectangleRounded(fourBtn, 0.2f, 8,
                         loadout.relicFourPiece ? kAccentBg : kPanelBg);
    DrawRectangleRoundedLines(fourBtn, 0.2f, 8,
                              loadout.relicFourPiece ? kAccentBorder : kPanelBorder);
    DrawText("4-PIECE", static_cast<int>(fourBtn.x + 14.0f),
             static_cast<int>(fourBtn.y + 8.0f), 14,
             loadout.relicFourPiece ? RAYWHITE : kDimText);

    Rectangle twoBtn = twoPieceModeButtonBounds();
    DrawRectangleRounded(twoBtn, 0.2f, 8,
                         !loadout.relicFourPiece ? kAccentBg : kPanelBg);
    DrawRectangleRoundedLines(twoBtn, 0.2f, 8,
                              !loadout.relicFourPiece ? kAccentBorder : kPanelBorder);
    DrawText("2 + 2", static_cast<int>(twoBtn.x + 18.0f),
             static_cast<int>(twoBtn.y + 8.0f), 14,
             !loadout.relicFourPiece ? RAYWHITE : kDimText);

    const float iconSize = 26.0f;

    auto drawSetIcon = [&](Rectangle box, const std::string& setName) -> float
    {
        if (const RelicSetInfo* info = relicSets.getByName(setName))
        {
            if (Texture2D* icon = assets.relicSet(info->id))
            {
                Rectangle dst{
                    box.x + 6.0f,
                    box.y + (box.height - iconSize) * 0.5f,
                    iconSize, iconSize
                };
                Rectangle src{0, 0, static_cast<float>(icon->width),
                              static_cast<float>(icon->height)};
                DrawTexturePro(*icon, src, dst, Vector2{0, 0}, 0.0f, WHITE);
                return dst.x + iconSize + 8.0f;
            }
        }
        return box.x + 12.0f;
    };

    Rectangle setA = relicSetAButtonBounds();
    bool setAOpen = activeDropdown.open &&
                    activeDropdown.mode == ActiveDropdown::Mode::RelicSetA;
    DrawRectangleRounded(setA, 0.15f, 8, kPanelBg);
    DrawRectangleRoundedLines(setA, 0.15f, 8,
                              setAOpen ? kAccentBorder : kPanelBorder);
    float textXA = drawSetIcon(setA, loadout.relicSetA);
    std::string labelA = loadout.relicSetA.empty() ? "-- None --" : loadout.relicSetA;
    DrawText(truncateSetName(labelA, loadout.relicFourPiece ? 48 : 22).c_str(),
             static_cast<int>(textXA), static_cast<int>(setA.y + 9.0f), 14, RAYWHITE);
    DrawText("v", static_cast<int>(setA.x + setA.width - 20.0f),
             static_cast<int>(setA.y + 9.0f), 14, kDimText);

    if (!loadout.relicFourPiece)
    {
        Rectangle setB = relicSetBButtonBounds();
        bool setBOpen = activeDropdown.open &&
                        activeDropdown.mode == ActiveDropdown::Mode::RelicSetB;
        DrawRectangleRounded(setB, 0.15f, 8, kPanelBg);
        DrawRectangleRoundedLines(setB, 0.15f, 8,
                                  setBOpen ? kAccentBorder : kPanelBorder);
        float textXB = drawSetIcon(setB, loadout.relicSetB);
        std::string labelB = loadout.relicSetB.empty() ? "-- None --" : loadout.relicSetB;
        DrawText(truncateSetName(labelB, 22).c_str(),
                 static_cast<int>(textXB), static_cast<int>(setB.y + 9.0f), 14, RAYWHITE);
        DrawText("v", static_cast<int>(setB.x + setB.width - 20.0f),
                 static_cast<int>(setB.y + 9.0f), 14, kDimText);
    }

    DrawText("PLANAR SET", 820, 130, 15, kDimText);
    Rectangle planar = planarSetButtonBounds();
    bool planarOpen = activeDropdown.open &&
                      activeDropdown.mode == ActiveDropdown::Mode::PlanarSet;
    DrawRectangleRounded(planar, 0.15f, 8, kPanelBg);
    DrawRectangleRoundedLines(planar, 0.15f, 8,
                              planarOpen ? kAccentBorder : kPanelBorder);
    float textXP = drawSetIcon(planar, loadout.planarSet);
    std::string planarLabel = loadout.planarSet.empty() ? "-- None --" : loadout.planarSet;
    DrawText(truncateSetName(planarLabel, 30).c_str(),
             static_cast<int>(textXP), static_cast<int>(planar.y + 9.0f), 14, RAYWHITE);
    DrawText("v", static_cast<int>(planar.x + planar.width - 20.0f),
             static_cast<int>(planar.y + 9.0f), 14, kDimText);
}

void RelicEditorScreen::drawDropdown()
{
    if (!activeDropdown.open)
        return;

    const float rowHeight = 40.0f;
    const float iconSize = 28.0f;
    Rectangle dropRect = getDropdownRect();

    DrawRectangleRounded(dropRect, 0.06f, 8, Color{18, 20, 27, 255});
    DrawRectangleRoundedLines(dropRect, 0.06f, 8, kAccentBorder);

    BeginScissorMode(static_cast<int>(dropRect.x), static_cast<int>(dropRect.y),
                     static_cast<int>(dropRect.width), static_cast<int>(dropRect.height));

    for (size_t i = 0; i < activeDropdown.items.size(); ++i)
    {
        float rowY = dropRect.y + 4.0f +
                     static_cast<float>(i) * rowHeight - activeDropdown.scrollOffset;

        if (rowY + rowHeight < dropRect.y ||
            rowY > dropRect.y + dropRect.height)
            continue;

        Rectangle rowRect{
            dropRect.x + 4.0f, rowY,
            dropRect.width - 8.0f, rowHeight - 2.0f
        };

        const DropdownItem& item = activeDropdown.items[i];
        bool isHovered = static_cast<int>(i) == activeDropdown.hoveredIndex;
        bool isCurrent = item.key == activeDropdown.currentKey;

        if (isHovered)
            DrawRectangleRounded(rowRect, 0.15f, 6, kAccentBg);
        else if (isCurrent)
            DrawRectangleRounded(rowRect, 0.15f, 6, Color{40, 44, 56, 255});

        float textX = rowRect.x + 10.0f;

        if (!item.assetId.empty())
        {
            if (Texture2D* icon = assets.relicSet(item.assetId))
            {
                Rectangle dst{
                    rowRect.x + 4.0f,
                    rowRect.y + (rowRect.height - iconSize) * 0.5f,
                    iconSize, iconSize
                };
                Rectangle src{0, 0, static_cast<float>(icon->width),
                              static_cast<float>(icon->height)};
                DrawTexturePro(*icon, src, dst, Vector2{0, 0}, 0.0f, WHITE);
                textX = dst.x + iconSize + 10.0f;
            }
        }

        DrawText(item.label.c_str(), static_cast<int>(textX),
                 static_cast<int>(rowRect.y + (rowRect.height - 14.0f) * 0.5f),
                 14, (isHovered || isCurrent) ? RAYWHITE : kDimText);
    }

    EndScissorMode();
}

void RelicEditorScreen::drawGearCard(GearSlot slot)
{
    CharacterLoadout& loadout = loadouts[characterId];
    const GearPiece& piece = loadout.gear[static_cast<size_t>(slot)];
    Rectangle card = gearCardBounds(slot);

    DrawRectangleRounded(card, 0.06f, 8, kPanelBg);
    DrawRectangleRoundedLines(card, 0.06f, 8, kPanelBorder);

    DrawText(gearSlotLabel(slot), static_cast<int>(card.x + 14.0f),
             static_cast<int>(card.y + 10.0f), 16, RAYWHITE);

    const auto& mainOptions = mainStatOptions(slot);
    Rectangle mainTypeRect{card.x + 14.0f, card.y + 34.0f, 190.0f, 28.0f};
    Rectangle mainValueRect{card.x + 212.0f, card.y + 34.0f, 124.0f, 28.0f};
    bool mainClickable = mainOptions.size() > 1;
    bool mainOpen = activeDropdown.open &&
                    activeDropdown.mode == ActiveDropdown::Mode::MainStat &&
                    activeDropdown.slot == slot;

    DrawRectangleRounded(mainTypeRect, 0.2f, 8,
                         mainClickable ? kPanelBg : kDisabledBg);
    if (mainClickable)
        DrawRectangleRoundedLines(mainTypeRect, 0.2f, 8,
                                  mainOpen ? kAccentBorder : kPanelBorder);

    std::string mainLabel = "MAIN: " + statLabel(mainOptions, piece.mainStat);
    DrawText(mainLabel.c_str(), static_cast<int>(mainTypeRect.x + 10.0f),
             static_cast<int>(mainTypeRect.y + 6.0f), 14,
             mainClickable ? RAYWHITE : kDimText);

    if (mainClickable)
        DrawText("v", static_cast<int>(mainTypeRect.x + mainTypeRect.width - 18.0f),
                 static_cast<int>(mainTypeRect.y + 6.0f), 14, kDimText);

    // Editable main-stat value (Sec 22.2): the exact number on the relic.
    bool mainFocused = (focusedMainSlot == static_cast<int>(slot));
    DrawRectangleRounded(mainValueRect, 0.2f, 8, kPanelBg);
    DrawRectangleRoundedLines(mainValueRect, 0.2f, 8,
                              mainFocused ? kAccentBorder : kPanelBorder);
    {
        std::string valueLabel = piece.mainStatValueText;
        if (!valueLabel.empty() && isPercentStat(piece.mainStat))
            valueLabel += "%";
        if (mainFocused)
            valueLabel += "_";
        if (valueLabel.empty())
            valueLabel = "-";
        DrawText(valueLabel.c_str(), static_cast<int>(mainValueRect.x + 10.0f),
                 static_cast<int>(mainValueRect.y + 6.0f), 14,
                 piece.mainStatValueText.empty() && !mainFocused ? kDimText : RAYWHITE);
    }

    for (int row = 0; row < 4; ++row)
    {
        float rowY = card.y + 70.0f + row * 32.0f;
        Rectangle typeRect{card.x + 14.0f, rowY, 180.0f, 26.0f};
        Rectangle valueRect{card.x + 202.0f, rowY, 134.0f, 26.0f};

        const SubstatRoll& roll = piece.substats[row];
        bool hasType = !roll.statKey.empty();

        bool typeOpen = activeDropdown.open &&
                        activeDropdown.mode == ActiveDropdown::Mode::Substat &&
                        activeDropdown.slot == slot &&
                        activeDropdown.substatRow == row;

        bool focused = (false);
        DrawRectangleRounded(typeRect, 0.2f, 8, kPanelBg);
        DrawRectangleRoundedLines(typeRect, 0.2f, 8,
                                  typeOpen ? kAccentBorder : kPanelBorder);

        std::string typeLabel = hasType ? statLabel(substatPool(), roll.statKey)
                                        : "+ Select stat";
        DrawText(typeLabel.c_str(), static_cast<int>(typeRect.x + 10.0f),
                 static_cast<int>(typeRect.y + 5.0f), 14,
                 hasType ? RAYWHITE : kDimText);
        DrawText("v", static_cast<int>(typeRect.x + typeRect.width - 18.0f),
                 static_cast<int>(typeRect.y + 4.0f), 14, kDimText);

        DrawRectangleRounded(valueRect, 0.2f, 8,
                             hasType ? kPanelBg : kDisabledBg);
        if (hasType)
        {
            bool valueFocused = (focusedField == static_cast<int>(slot) * 4 + row);
            DrawRectangleRoundedLines(valueRect, 0.2f, 8,
                                      valueFocused ? kAccentBorder : kPanelBorder);

            std::string valueLabel = roll.valueText.empty() ? "" : roll.valueText;
            if (!roll.valueText.empty() && isPercentStat(roll.statKey))
            {
                if (valueLabel.back() != '%')
                    valueLabel += "%";
            }

            DrawText(valueLabel.c_str(), static_cast<int>(valueRect.x + 10.0f),
                     static_cast<int>(valueRect.y + 5.0f), 14, RAYWHITE);

            if (valueFocused)
            {
                int textW = MeasureText(valueLabel.c_str(), 14);
                float caretX = valueRect.x + 10.0f + static_cast<float>(textW) + 2.0f;
                DrawLine(static_cast<int>(caretX),
                         static_cast<int>(valueRect.y + 4.0f),
                         static_cast<int>(caretX),
                         static_cast<int>(valueRect.y + 20.0f),
                         RAYWHITE);
            }
        }
    }
}

#include "RelicEditorScreen.h"
#include "raylib.h"

#include <algorithm>

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
    focusedField = -1;

    CharacterLoadout& loadout = loadouts[characterId]; // creates a default entry if new
    ensureLoadoutDefaults(loadout);
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
    const CharacterLoadout& loadout = loadouts.at(characterId);
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

    return sets.front()->name; // current not recognized (shouldn't happen) — reset
}

void RelicEditorScreen::update(float dt)
{
    (void)dt;

    if (characterId.empty())
        return;

    CharacterLoadout& loadout = loadouts[characterId];

    Vector2 mouse = GetMousePosition();
    bool pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    bool clickedOnValueField = false;

    if (pressed)
    {
        if (CheckCollisionPointRec(mouse, backButtonBounds()))
            backRequested = true;

        if (CheckCollisionPointRec(mouse, fourPieceModeButtonBounds()))
            loadout.relicFourPiece = true;

        if (CheckCollisionPointRec(mouse, twoPieceModeButtonBounds()))
            loadout.relicFourPiece = false;

        if (CheckCollisionPointRec(mouse, relicSetAButtonBounds()))
            loadout.relicSetA = nextSetName("relic", loadout.relicSetA);

        if (!loadout.relicFourPiece && CheckCollisionPointRec(mouse, relicSetBButtonBounds()))
            loadout.relicSetB = nextSetName("relic", loadout.relicSetB);

        if (CheckCollisionPointRec(mouse, planarSetButtonBounds()))
            loadout.planarSet = nextSetName("planar_ornament", loadout.planarSet);

        for (int i = 0; i < static_cast<int>(GearSlot::Count); ++i)
        {
            GearSlot slot = static_cast<GearSlot>(i);
            GearPiece& piece = loadout.gear[i];
            Rectangle card = gearCardBounds(slot);

            const auto& mainOptions = mainStatOptions(slot);
            Rectangle mainRect{card.x + 14.0f, card.y + 34.0f, card.width - 28.0f, 28.0f};

            if (mainOptions.size() > 1 && CheckCollisionPointRec(mouse, mainRect))
            {
                piece.mainStat = nextStatOption(mainOptions, piece.mainStat);

                // A substat can't duplicate the piece's main stat — clear
                // any row that now collides with the newly picked one.
                for (auto& s : piece.substats)
                    if (s.statKey == piece.mainStat)
                        s.statKey.clear();
            }

            for (int row = 0; row < 4; ++row)
            {
                float rowY = card.y + 70.0f + row * 32.0f;
                Rectangle typeRect{card.x + 14.0f, rowY, 180.0f, 26.0f};
                Rectangle valueRect{card.x + 202.0f, rowY, 134.0f, 26.0f};

                if (CheckCollisionPointRec(mouse, typeRect))
                    piece.substats[row].statKey = nextSubstatOption(piece, row);

                if (!piece.substats[row].statKey.empty() &&
                    CheckCollisionPointRec(mouse, valueRect))
                {
                    focusedField = i * 4 + row;
                    clickedOnValueField = true;
                }
            }
        }

        if (!clickedOnValueField)
            focusedField = -1;
    }

    if (focusedField >= 0)
    {
        int slotIdx = focusedField / 4;
        int row = focusedField % 4;
        std::string& text = loadout.gear[slotIdx].substats[row].valueText;

        int key = GetCharPressed();
        while (key > 0)
        {
            bool isDigit = key >= '0' && key <= '9';
            bool isDot = key == '.' && text.find('.') == std::string::npos;
            if ((isDigit || isDot) && text.size() < 8)
                text += static_cast<char>(key);
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && !text.empty())
            text.pop_back();
        if (IsKeyPressed(KEY_ENTER))
            focusedField = -1;
    }
}

void RelicEditorScreen::draw()
{
    if (characterId.empty())
    {
        DrawText("No character selected.", 320, 200, 20, RAYWHITE);
        return;
    }

    drawHeader();
    drawSetPanel();

    for (int i = 0; i < static_cast<int>(GearSlot::Count); ++i)
        drawGearCard(static_cast<GearSlot>(i));
}

void RelicEditorScreen::drawHeader()
{
    const CharacterInfo* info = characters.get(characterId);
    std::string displayName = info ? info->name : characterId;

    Rectangle back = backButtonBounds();
    DrawRectangleRounded(back, 0.2f, 8, kPanelBg);
    DrawRectangleRoundedLines(back, 0.2f, 8, kPanelBorder);
    DrawText("< Back", static_cast<int>(back.x + 14.0f), static_cast<int>(back.y + 9.0f),
              15, RAYWHITE);

    DrawText(displayName.c_str(), 410, 30, 30, RAYWHITE);
    DrawText("Relics & Planar Ornaments", 412, 68, 15, kDimText);

    DrawLine(310, 105, GetScreenWidth() - 30, 105, Color{48, 52, 64, 255});
}

void RelicEditorScreen::drawSetPanel()
{
    const CharacterLoadout& loadout = loadouts.at(characterId);

    DrawText("RELIC SET", 310, 130, 15, kDimText);

    Rectangle fourBtn = fourPieceModeButtonBounds();
    DrawRectangleRounded(fourBtn, 0.2f, 8, loadout.relicFourPiece ? kAccentBg : kPanelBg);
    DrawRectangleRoundedLines(fourBtn, 0.2f, 8,
        loadout.relicFourPiece ? kAccentBorder : kPanelBorder);
    DrawText("4-PIECE", static_cast<int>(fourBtn.x + 14.0f), static_cast<int>(fourBtn.y + 8.0f),
              14, loadout.relicFourPiece ? RAYWHITE : kDimText);

    Rectangle twoBtn = twoPieceModeButtonBounds();
    DrawRectangleRounded(twoBtn, 0.2f, 8, !loadout.relicFourPiece ? kAccentBg : kPanelBg);
    DrawRectangleRoundedLines(twoBtn, 0.2f, 8,
        !loadout.relicFourPiece ? kAccentBorder : kPanelBorder);
    DrawText("2 + 2", static_cast<int>(twoBtn.x + 18.0f), static_cast<int>(twoBtn.y + 8.0f),
              14, !loadout.relicFourPiece ? RAYWHITE : kDimText);

    Rectangle setA = relicSetAButtonBounds();
    DrawRectangleRounded(setA, 0.15f, 8, kPanelBg);
    DrawRectangleRoundedLines(setA, 0.15f, 8, kPanelBorder);
    std::string labelA = loadout.relicSetA.empty() ? "-- None --" : loadout.relicSetA;
    DrawText(truncateSetName(labelA, loadout.relicFourPiece ? 55 : 26).c_str(),
              static_cast<int>(setA.x + 12.0f), static_cast<int>(setA.y + 9.0f), 14, RAYWHITE);

    if (!loadout.relicFourPiece)
    {
        Rectangle setB = relicSetBButtonBounds();
        DrawRectangleRounded(setB, 0.15f, 8, kPanelBg);
        DrawRectangleRoundedLines(setB, 0.15f, 8, kPanelBorder);
        std::string labelB = loadout.relicSetB.empty() ? "-- None --" : loadout.relicSetB;
        DrawText(truncateSetName(labelB, 26).c_str(),
                  static_cast<int>(setB.x + 12.0f), static_cast<int>(setB.y + 9.0f), 14, RAYWHITE);
    }

    DrawText("PLANAR SET", 820, 130, 15, kDimText);
    Rectangle planar = planarSetButtonBounds();
    DrawRectangleRounded(planar, 0.15f, 8, kPanelBg);
    DrawRectangleRoundedLines(planar, 0.15f, 8, kPanelBorder);
    std::string planarLabel = loadout.planarSet.empty() ? "-- None --" : loadout.planarSet;
    DrawText(truncateSetName(planarLabel, 34).c_str(),
              static_cast<int>(planar.x + 12.0f), static_cast<int>(planar.y + 9.0f), 14, RAYWHITE);
}

void RelicEditorScreen::drawGearCard(GearSlot slot)
{
    const CharacterLoadout& loadout = loadouts.at(characterId);
    const GearPiece& piece = loadout.gear[static_cast<size_t>(slot)];
    Rectangle card = gearCardBounds(slot);

    DrawRectangleRounded(card, 0.06f, 8, kPanelBg);
    DrawRectangleRoundedLines(card, 0.06f, 8, kPanelBorder);

    DrawText(gearSlotLabel(slot), static_cast<int>(card.x + 14.0f),
              static_cast<int>(card.y + 10.0f), 16, RAYWHITE);

    const auto& mainOptions = mainStatOptions(slot);
    Rectangle mainRect{card.x + 14.0f, card.y + 34.0f, card.width - 28.0f, 28.0f};
    bool mainClickable = mainOptions.size() > 1;

    DrawRectangleRounded(mainRect, 0.2f, 8, mainClickable ? kPanelBg : kDisabledBg);
    if (mainClickable)
        DrawRectangleRoundedLines(mainRect, 0.2f, 8, kPanelBorder);

    std::string mainLabel = "MAIN: " + statLabel(mainOptions, piece.mainStat);
    DrawText(mainLabel.c_str(), static_cast<int>(mainRect.x + 10.0f),
              static_cast<int>(mainRect.y + 6.0f), 14, mainClickable ? RAYWHITE : kDimText);

    for (int row = 0; row < 4; ++row)
    {
        float rowY = card.y + 70.0f + row * 32.0f;
        Rectangle typeRect{card.x + 14.0f, rowY, 180.0f, 26.0f};
        Rectangle valueRect{card.x + 202.0f, rowY, 134.0f, 26.0f};

        const SubstatRoll& roll = piece.substats[row];
        bool hasType = !roll.statKey.empty();

        DrawRectangleRounded(typeRect, 0.2f, 8, kPanelBg);
        DrawRectangleRoundedLines(typeRect, 0.2f, 8, kPanelBorder);
        std::string typeLabel = hasType ? statLabel(substatPool(), roll.statKey) : "+ Select stat";
        DrawText(typeLabel.c_str(), static_cast<int>(typeRect.x + 10.0f),
                  static_cast<int>(typeRect.y + 5.0f), 14, hasType ? RAYWHITE : kDimText);

        bool focused = (focusedField == static_cast<int>(slot) * 4 + row);
        DrawRectangleRounded(valueRect, 0.2f, 8, hasType ? kPanelBg : kDisabledBg);
        DrawRectangleRoundedLines(valueRect, 0.2f, 8, focused ? kAccentBorder : kPanelBorder);

        std::string valueLabel = roll.valueText.empty() ? (hasType ? "0" : "") : roll.valueText;
        DrawText(valueLabel.c_str(), static_cast<int>(valueRect.x + 10.0f),
                  static_cast<int>(valueRect.y + 5.0f), 14, hasType ? RAYWHITE : kDimText);

        if (focused)
        {
            int textW = MeasureText(valueLabel.c_str(), 14);
            float caretX = valueRect.x + 10.0f + static_cast<float>(textW) + 2.0f;
            DrawLine(static_cast<int>(caretX), static_cast<int>(valueRect.y + 4.0f),
                     static_cast<int>(caretX), static_cast<int>(valueRect.y + 20.0f), RAYWHITE);
        }
    }
}

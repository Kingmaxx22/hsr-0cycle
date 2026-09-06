#pragma once

#include "Screen.h"
#include "../assets/AssetManager.h"
#include "../data/CharacterDatabase.h"
#include "../data/RelicSetDatabase.h"
#include "../data/CharacterLoadout.h"
#include "../data/GearRules.h"

#include <string>

// Per-character gear editor: 6 slots (Head/Hands/Body/Feet/Planar
// Sphere/Link Rope), each with a main stat and up to 4 substats (type +
// typed-in value), plus relic-set (4pc, or 2pc+2pc) and planar-set pickers.
class RelicEditorScreen : public Screen
{
public:
    RelicEditorScreen(AssetManager& assets, CharacterDatabase& characters,
                       RelicSetDatabase& relicSets, LoadoutStore& loadouts);

    void initialize() override;
    void update(float dt) override;
    void draw() override;

    void setCharacter(const std::string& characterId);
    bool consumeBackRequest();

private:
    Rectangle backButtonBounds() const;
    Rectangle fourPieceModeButtonBounds() const;
    Rectangle twoPieceModeButtonBounds() const;
    Rectangle relicSetAButtonBounds() const;
    Rectangle relicSetBButtonBounds() const;
    Rectangle planarSetButtonBounds() const;
    Rectangle gearCardBounds(GearSlot slot) const;

    void drawHeader();
    void drawSetPanel();
    void drawGearCard(GearSlot slot);

    // Cycles through relicSets.byCategory(category) starting from `current`,
    // with an empty string acting as a wrap-around "— None —" state.
    std::string nextSetName(const std::string& category, const std::string& current) const;

    AssetManager& assets;
    CharacterDatabase& characters;
    RelicSetDatabase& relicSets;
    LoadoutStore& loadouts;

    std::string characterId;
    bool backRequested = false;

    int focusedField = -1; // slotIndex * 4 + substatIndex; -1 = no field focused

    static constexpr float kCardW = 350.0f;
    static constexpr float kCardH = 220.0f;
    static constexpr float kGapX = 20.0f;
    static constexpr float kGapY = 20.0f;
    static constexpr float kOriginX = 310.0f;
    static constexpr float kOriginY = 260.0f;
    static constexpr int kCols = 3;
};
#pragma once

#include "Screen.h"
#include "../assets/AssetManager.h"
#include "../data/CharacterDatabase.h"

#include <string>
#include <vector>

// Simple searchable character grid whose only job is: pick a character to
// go configure their relics/planar ornaments in RelicEditorScreen.
class RelicRosterScreen : public Screen
{
public:
    RelicRosterScreen(AssetManager& assets, CharacterDatabase& characters);

    void initialize() override;
    void update(float dt) override;
    void draw() override;

    // Returns true (and fills outCharacterId) once if a character card was
    // clicked this frame.
    bool consumeSelection(std::string& outCharacterId);

private:
    void updateFilteredRoster();
    Rectangle searchBoxBounds() const;
    Rectangle searchClearButtonBounds() const;

    AssetManager& assets;
    CharacterDatabase& characters;

    static constexpr int kCols = 7;
    static constexpr float kCardW = 150.0f;
    static constexpr float kCardH = 160.0f;
    static constexpr float kGapX = 20.0f;
    static constexpr float kGapY = 20.0f;
    static constexpr float kOriginX = 310.0f;
    static constexpr float kOriginY = 150.0f;
    static constexpr float kViewportH = 700.0f;

    std::string searchQuery;
    std::vector<const CharacterInfo*> filteredRoster;
    float scroll = 0.0f;

    std::string pendingSelection;
};

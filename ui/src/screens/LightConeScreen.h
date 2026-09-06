#pragma once

#include "Screen.h"
#include "../assets/AssetManager.h"
#include "../data/CharacterDatabase.h"
#include "../data/LightConeDatabase.h"
#include "../data/Characterloadout.h"

#include <array>
#include <string>
#include <vector>

class LightConeScreen : public Screen
{
public:
    LightConeScreen(AssetManager& assets, CharacterDatabase& characters,
                    LightConeDatabase& lightCones, LoadoutStore& loadouts);

    void initialize() override;
    void update(float dt) override;
    void draw() override;

    void setCharacter(const std::string& characterId);
    void setTeamContext(const std::array<std::string, 4>& team, const std::string& activeCharacterId);
    bool consumeBackRequest();

    const std::string& getCharacterId() const { return characterId; }

private:
    Rectangle backButtonBounds() const;
    Rectangle searchBoxBounds() const;
    Rectangle searchClearBounds() const;
    Rectangle pathFilterBounds() const;
    Rectangle rarityFilterBounds(int rarityIndex) const; // 0: All, 1: 5*, 2: 4*, 3: 3*

    void updateFilteredList();
    void drawHeader();
    void drawEquippedPanel();
    void drawLibrary();

    AssetManager& assets;
    CharacterDatabase& characters;
    LightConeDatabase& lightCones;
    LoadoutStore& loadouts;

    std::array<std::string, 4> teamMembers{"", "", "", ""};
    std::string characterId;
    bool backRequested = false;

    // Filters
    std::string searchQuery;
    bool searchActive = false;
    bool filterByPath = true; // Match active character's path by default
    int selectedRarity = 0;   // 0: All, 5: 5*, 4: 4*, 3: 3*

    std::vector<const LightConeInfo*> filteredList;
    float scrollOffset = 0.0f;

    static constexpr float kCardW = 160.0f;
    static constexpr float kCardH = 210.0f;
    static constexpr float kGapX = 14.0f;
    static constexpr float kGapY = 14.0f;
    static constexpr int kCols = 4;
};

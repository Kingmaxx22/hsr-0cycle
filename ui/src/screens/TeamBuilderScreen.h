#pragma once

#include "Screen.h"
#include "../assets/AssetManager.h"
#include "../data/CharacterDatabase.h"
#include "../widgets/Button.h"
#include "../widgets/Panel.h"
#include "../widgets/CharacterCard.h"

#include <array>
#include <string>
#include <vector>

class TeamBuilderScreen : public Screen
{
public:
    TeamBuilderScreen(AssetManager& assets, CharacterDatabase& characters);

    void initialize() override;
    void update(float dt) override;
    void draw() override;

private:
    void drawSidebar();
    void drawHeader();
    void drawTeamSlots();
    void drawSearchAndFilters();
    void drawCharacterLibrary();
    void updateFilteredRoster();

    Rectangle searchBoxBounds() const;
    Rectangle searchClearButtonBounds() const;
    Rectangle fourStarButtonBounds() const;
    Rectangle fiveStarButtonBounds() const;

    AssetManager& assets;
    CharacterDatabase& characters;

    std::array<std::string, 4> team = {"", "", "", ""};

    static constexpr int kLibraryCols = 6;
    static constexpr float kLibraryCardW = 150.0f;
    static constexpr float kLibraryCardH = 160.0f;
    static constexpr float kLibraryGapX = 20.0f;
    static constexpr float kLibraryGapY = 20.0f;
    static constexpr float kLibraryOriginX = 350.0f;
    static constexpr float kLibraryOriginY = 490.0f;
    static constexpr float kLibraryViewportH = 380.0f;

    float libraryScroll = 0.0f;

    std::string searchQuery;
    bool showFourStar = true;
    bool showFiveStar = true;
    std::vector<const CharacterInfo*> filteredRoster;

    int selectedSlot = 0;
    int activeNav = 0;
    float time = 0.0f;
};

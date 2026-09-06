#pragma once

#include "Screen.h"
#include "../assets/AssetManager.h"
#include "../data/CharacterDatabase.h"
#include "../widgets/Button.h"
#include "../widgets/Panel.h"
#include "../widgets/CharacterCard.h"

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

class TeamBuilderScreen : public Screen
{
public:
    TeamBuilderScreen(AssetManager& assets, CharacterDatabase& characters);

    void initialize() override;
    void update(float dt) override;
    void draw() override;

    // Returns true (and fills outCharacterId) once if a filled team slot was
    // clicked this frame, requesting that character's gear editor be opened.
    bool consumeEditorRequest(std::string& outCharacterId);

private:
    void drawHeader();
    void drawTeamSlots();
    void drawSearchAndFilters();
    void drawCharacterLibrary();
    void updateFilteredRoster();
    void updateFilterChipBounds();

    Rectangle searchBoxBounds() const;
    Rectangle searchClearButtonBounds() const;
    Rectangle fourStarButtonBounds() const;
    Rectangle fiveStarButtonBounds() const;

    static std::string capitalize(const std::string& s);
    static Color elementColor(const std::string& element);

    AssetManager& assets;
    CharacterDatabase& characters;

    std::array<std::string, 4> team = {"", "", "", ""};

    static constexpr int kLibraryCols = 6;
    static constexpr float kLibraryCardW = 150.0f;
    static constexpr float kLibraryCardH = 160.0f;
    static constexpr float kLibraryGapX = 20.0f;
    static constexpr float kLibraryGapY = 20.0f;
    static constexpr float kLibraryOriginX = 350.0f;
    static constexpr float kLibraryOriginY = 560.0f;
    static constexpr float kLibraryViewportH = 310.0f;

    static constexpr float kElementRowY = 480.0f;
    static constexpr float kPathRowY = 518.0f;
    static constexpr float kFilterChipH = 30.0f;

    float libraryScroll = 0.0f;

    std::string searchQuery;
    bool showFourStar = true;
    bool showFiveStar = true;
    std::vector<const CharacterInfo*> filteredRoster;

    // Element/path filters — populated in initialize() from whatever values
    // actually appear in the loaded character data, so new elements/paths
    // (game updates) show up automatically without a code change.
    std::vector<std::string> elementOrder;
    std::vector<std::string> pathOrder;
    std::unordered_map<std::string, bool> elementEnabled;
    std::unordered_map<std::string, bool> pathEnabled;
    std::vector<Rectangle> elementChipBounds;
    std::vector<Rectangle> pathChipBounds;

    int selectedSlot = 0;
    float time = 0.0f;
    std::string pendingEditorRequest;
};

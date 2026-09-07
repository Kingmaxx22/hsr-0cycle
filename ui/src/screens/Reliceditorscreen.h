#pragma once

#include "Screen.h"
#include "../assets/AssetManager.h"
#include "../data/CharacterDatabase.h"
#include "../data/Relicsetdatabase.h"
#include "../data/Characterloadout.h"
#include "../data/Gearrules.h"

#include <array>
#include <string>
#include <vector>

struct DropdownItem
{
    std::string key;
    std::string label;
    std::string assetId;
};

struct ActiveDropdown
{
    enum class Mode
    {
        None,
        MainStat,
        Substat,
         RelicSetA,
        RelicSetB,
        PlanarSet
    };

    bool open = false;
    Mode mode = Mode::None;
    GearSlot slot = GearSlot::Head;
    int substatRow = -1;
    Rectangle anchorRect{};
    std::vector<DropdownItem> items;
    std::string currentKey;
    int hoveredIndex = -1;
    float scrollOffset = 0.0f;
};

class RelicEditorScreen : public Screen
{
public:
    RelicEditorScreen(AssetManager& assets, CharacterDatabase& characters,
                       RelicSetDatabase& relicSets, LoadoutStore& loadouts);

    void initialize() override;
    void update(float dt) override;
    void draw() override;

    void setCharacter(const std::string& characterId);
    void setTeamContext(const std::array<std::string, 4>& team, const std::string& activeCharacterId);
    bool consumeBackRequest();

    const std::string& getCharacterId() const { return characterId; }

private:
    Rectangle backButtonBounds() const;
    Rectangle fourPieceModeButtonBounds() const;
    Rectangle twoPieceModeButtonBounds() const;
    Rectangle relicSetAButtonBounds() const;
    Rectangle relicSetBButtonBounds() const;
    Rectangle planarSetButtonBounds() const;
    Rectangle gearCardBounds(GearSlot slot) const;

    Rectangle getDropdownRect() const;
    void drawHeader();
    void drawSetPanel();
    void drawGearCard(GearSlot slot);
    void drawDropdown();

    void openSetDropdown(ActiveDropdown::Mode mode, const std::string& category,
                         const std::string& currentValue, Rectangle anchor);
    void openMainStatDropdown(GearSlot slot, Rectangle anchor);
    void openSubstatDropdown(GearSlot slot, int row, Rectangle anchor);
     bool handleDropdownInput(Vector2 mouse, bool pressed);

    std::vector<DropdownItem> buildSubstatItems(const GearPiece& piece, int row) const;
 
    std::string nextSetName(const std::string& category, const std::string& current) const;

    AssetManager& assets;
    CharacterDatabase& characters;
    RelicSetDatabase& relicSets;
    LoadoutStore& loadouts;

    std::array<std::string, 4> teamMembers{"", "", "", ""};
    std::string characterId;
    bool backRequested = false;

    int focusedField = -1; // slotIndex * 4 + substatRow; -1 = no value field focused
    int focusedMainSlot = -1; // GearSlot index with focused main-stat value; -1 = none
    ActiveDropdown activeDropdown;

    static constexpr float kCardW = 350.0f;
    static constexpr float kCardH = 220.0f;
    static constexpr float kGapX = 20.0f;
    static constexpr float kGapY = 20.0f;
    static constexpr float kOriginX = 310.0f;
    static constexpr float kOriginY = 260.0f;
    static constexpr int kCols = 3;
};

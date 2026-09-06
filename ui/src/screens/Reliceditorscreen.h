#pragma once

#include "Screen.h"
#include "../assets/AssetManager.h"
#include "../data/CharacterDatabase.h"
#include "../data/RelicSetDatabase.h"
#include "../data/CharacterLoadout.h"
#include "../data/GearRules.h"

#include <array>
#include <string>
#include <vector>

struct DropdownItem
{
    std::string key;      // Value/stat key or set name/id
    std::string label;    // Display text
    std::string assetId;  // Asset normalized ID for thumbnail texture (empty if none)
};

struct ActiveDropdown
{
    enum class Mode { None, MainStat, Substat, RelicSetA, RelicSetB, PlanarSet };

    bool open = false;
    Mode mode = Mode::None;
    GearSlot slot = GearSlot::Head;
    int substatRow = -1; // -1 for main stat, 0..3 for substats
    Rectangle anchorRect{};
    std::vector<DropdownItem> items;
    std::string currentKey;
    int hoveredIndex = -1;
    float scrollOffset = 0.0f;
};

// Per-character gear editor: 6 slots (Head/Hands/Body/Feet/Planar
// Sphere/Link Rope), each with a main stat dropdown and up to 4 substat dropdowns
// (type dropdown from available permutations + typed-in numeric value), plus relic-set
// (4pc, or 2pc+2pc) and planar-set dropdown pickers with asset artwork icons.
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

    std::string nextSetName(const std::string& category, const std::string& current) const;

    AssetManager& assets;
    CharacterDatabase& characters;
    RelicSetDatabase& relicSets;
    LoadoutStore& loadouts;

    std::array<std::string, 4> teamMembers{"", "", "", ""};
    std::string characterId;
    bool backRequested = false;

    int focusedField = -1; // slotIndex * 4 + substatIndex; -1 = no field focused
    ActiveDropdown activeDropdown;

    static constexpr float kCardW = 350.0f;
    static constexpr float kCardH = 220.0f;
    static constexpr float kGapX = 20.0f;
    static constexpr float kGapY = 20.0f;
    static constexpr float kOriginX = 310.0f;
    static constexpr float kOriginY = 260.0f;
    static constexpr int kCols = 3;
};

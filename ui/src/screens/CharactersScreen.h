#pragma once

#include "Screen.h"
#include "../assets/AssetManager.h"
#include "../data/CharacterDatabase.h"
#include "../data/Characterloadout.h"
#include "../data/RelicSetDatabase.h"
#include "../data/LightConeDatabase.h"
#include "../data/LoadoutResolver.h"

#include <array>
#include <string>
#include <vector>

class CharactersScreen : public Screen
{
public:
    // --- Enter Completed Character workflow (Sec 22.4: entered finals) ---
    // Percent fields are decimals (0.70 = 70% crit rate).
    struct ManualConfig {
        std::string characterId;
        int hp = 0;
        int atk = 0;
        int def = 0;
        int spd = 0;
        double critRate = 0.0;
        double critDmg = 0.0;
        double elemDmg = 0.0;
        double resPen = 0.0;
        double ehr = 0.0;
        double effectRes = 0.0;
    };

    CharactersScreen(AssetManager& assets, CharacterDatabase& characters,
                     RelicSetDatabase& relicSets, LightConeDatabase& lightCones,
                     LoadoutStore& loadouts);

    // Team context for loadout-backed editing (same pattern as RelicEditor).
    void setTeamContext(const std::array<std::string, 4>& team);

    void initialize() override;
    void update(float dt) override;
    void draw() override;

    // Returns true (and fills outCharacterId) once if a character was selected.
    bool consumeSelection(std::string& outCharacterId);

    // Returns true once if back navigation (ESC / < Back) was requested.
    bool consumeBackRequest();

    // Returns the currently selected workflow mode
    bool isManualStatsMode() const { return m_manualStatsMode; }

    // Parsed manual stats (Enter Completed Character workflow).
    // Values are committed from the text buffers on ENTER / focus change.
    ManualConfig getManualConfig() const { return m_manualConfig; }

private:
    // --- Build from Components workflow ---
    struct ComponentConfig {
        std::string characterId;
        // Base stats from character
        double baseHp;
        double baseAtk;
        double baseDef;
        double baseSpd;
        // Percentage bonuses
        double hpPct;
        double atkPct;
        double defPct;
        double spdPct;
        // Flat bonuses
        double flatHp;
        double flatAtk;
        double flatDef;
        double flatSpd;
        // Light Cone
        std::string lightConeId;
        int lightConeSuperimposition; // 1-5
        // Relics
        bool relicFourPiece; // true = one 4pc, false = two 2pcs
        std::string relicSetA;
        std::string relicSetB;
        std::string planarSet;
        // Substats (simplified: 4 main stats + 1 substat)
        struct Substat {
            std::string key;      // e.g., "atk_pct"
            std::string label;    // e.g., "ATK%"
            double value;         // e.g., 0.15 for 15%
        };
        std::array<Substat, 5> substats;
    };

    // UI state
    void updateFilteredRoster();
    Rectangle searchBoxBounds() const;
    Rectangle searchClearButtonBounds() const;
    Rectangle manualFieldBounds(int index) const;
    float manualContentTop() const;
    static int parseStatText(const std::string& text);
    static double parsePercentText(const std::string& text);
    void commitManualTexts();
    void syncManualCharacterId();

    // --- Build-tab component editors (Sec 22.2) ---
    // Extra/other-bonus + base-override fields, single focus index.
    // 0-9: other bonuses, 10-13: base HP/ATK/DEF/SPD. -1 = none focused.
    static constexpr int kExtraFieldCount = 14;
    Rectangle extraFieldBounds(int index) const;
    Rectangle baseToggleBounds() const;
    Rectangle levelMinusBounds() const;
    Rectangle levelPlusBounds() const;
    bool extraFieldIsPercent(int index) const;
    std::string& extraFieldText(int index);
    void syncExtraTexts();
    void commitExtraField(int index);
    CharacterLoadout& loadoutFor(const std::string& characterId);
    const CharacterInfo* selectedInfo() const;

    // Q3: equipped conditional set effects, stable order, for opt-in toggles.
    struct CondToggle {
        std::string effectId;
        std::string label;
        bool autoActive = false; // derived from turn state (element match)
    };
    std::vector<CondToggle> conditionalToggles(const CharacterLoadout& lo) const;
    Rectangle toggleRowBounds(int index) const;
    // Phase 2: major-trace toggle rows sit below the set-effect rows.
    // setRows = number of drawn set-toggle rows (0..3).
    Rectangle traceRowBounds(int index, int setRows) const;

    // Layout constants
    static constexpr int kLibraryCols = 5;
    static constexpr float kLibraryCardW = 180.0f;
    static constexpr float kLibraryCardH = 200.0f;
    static constexpr float kLibraryGapX = 15.0f;
    static constexpr float kLibraryGapY = 25.0f;
    // Content starts right of the 270px sidebar (all other screens use ~310).
    static constexpr float kLibraryOriginX = 310.0f;
    static constexpr float kLibraryOriginY = 160.0f;
    // Viewport is intentionally shorter than the full window so the
    // workflow detail section below the grid stays visible/clickable.
    static constexpr float kViewportH = 340.0f;

    // Character grid
    AssetManager& m_assets;
    CharacterDatabase& m_characters;
    RelicSetDatabase& m_relicSets;
    LightConeDatabase& m_lightCones;
    LoadoutStore& m_loadouts;
    std::array<std::string, 4> m_team{"", "", "", ""};

    std::string m_searchQuery;
    std::vector<const CharacterInfo*> m_filteredRoster;
    float m_scroll = 0.0f;

    // Workflow mode: false = build from components, true = enter completed
    bool m_manualStatsMode = false;

    // Selected character
    std::string m_selectedCharacterId;
    bool m_pendingSelection = false;

    // Component config (for build workflow)
    ComponentConfig m_componentConfig;

    // Manual config (for enter completed workflow)
    ManualConfig m_manualConfig;

    // Raw text buffers for the manual fields (Sec 22.4).
    // 0 HP, 1 ATK, 2 DEF, 3 SPD (integers); 4 CRIT%, 5 CRIT DMG%,
    // 6 DMG%, 7 RES PEN%, 8 EHR%, 9 EFF RES% (percent-numbers).
    static constexpr int kManualFieldCount = 10;
    std::array<std::string, kManualFieldCount> m_manualTexts{};
    // Focused manual field: -1 = none, else 0..9.
    int m_focusedManualField = -1;

    // Build-tab editor buffers: 0-8 other bonuses, 9-12 base override.
    std::array<std::string, kExtraFieldCount> m_extraTexts{};
    int m_focusedExtraField = -1;

    // Damage-table declarations (skill_scaling_raw.csv extraction):
    // one editable (base, boosted) pair per shown damage action, writing
    // the same scalingTables the SCALING screen edits. Focus ids start at
    // kScalingFieldBase to avoid colliding with extra/manual fields.
    static constexpr int kScalingFieldBase = 100;
    static constexpr int kScalingMaxRows = 3;
    // Parallel: shown action keys (basic/skill/ult/fua/memosprite) and
    // the primary extraction variable per row ("" when none).
    std::vector<std::string> m_scalingActions;
    std::vector<std::string> m_scalingTexts; // 2 per row: base, boosted
    int m_focusedScalingField = -1;
    // Primary extraction row per damage action (largest base wins;
    // multi-hit kits need a declared effective total — never inferred).
    struct ScalingDeclRow {
        std::string actionKey;
        std::string variable;
        int extraCount = 0;
        bool literal = true;
    };
    std::vector<ScalingDeclRow> scalingDeclRows() const;
    void ensureScalingPrefill(const CharacterInfo& info, CharacterLoadout& lo);
    void syncScalingTexts();
    void commitScalingField(int field);
    Rectangle scalingFieldBounds(int field) const;

    // Back-navigation request (ESC with no field focused, or < Back button).
    bool m_backRequested = false;
    Rectangle backButtonBounds() const;

    // Tab / mode selection UI
    // Tabs are right-aligned dynamically in draw() so the full labels fit.
    static constexpr float kTabY = 40.0f;
    static constexpr float kTabH = 40.0f;
    static constexpr float kTabW = 230.0f;
    static constexpr float kTabGap = 20.0f;

    Rectangle m_buildTabBounds;
    Rectangle m_manualTabBounds;
    bool m_buildTabHover = false;
    bool m_manualTabHover = false;
};
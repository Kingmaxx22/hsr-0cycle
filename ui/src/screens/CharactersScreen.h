#pragma once

#include "Screen.h"
#include "../assets/AssetManager.h"
#include "../data/CharacterDatabase.h"
#include "../data/RelicSetDatabase.h"
#include "../data/LightConeDatabase.h"

#include <array>
#include <string>
#include <vector>

class CharactersScreen : public Screen
{
public:
    // --- Enter Completed Character workflow ---
    struct ManualConfig {
        std::string characterId;
        int hp = 0;
        int atk = 0;
        int def = 0;
        int spd = 0;
    };

    CharactersScreen(AssetManager& assets, CharacterDatabase& characters,
                     RelicSetDatabase& relicSets, LightConeDatabase& lightCones);

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
    void commitManualTexts();
    void syncManualCharacterId();

    // Layout constants
    static constexpr int kLibraryCols = 5;
    static constexpr float kLibraryCardW = 180.0f;
    static constexpr float kLibraryCardH = 200.0f;
    static constexpr float kLibraryGapX = 15.0f;
    static constexpr float kLibraryGapY = 25.0f;
    static constexpr float kLibraryOriginX = 50.0f;
    static constexpr float kLibraryOriginY = 80.0f;
    // Viewport is intentionally shorter than the full window so the
    // workflow detail section below the grid stays visible/clickable.
    static constexpr float kViewportH = 440.0f;

    // Character grid
    AssetManager& m_assets;
    CharacterDatabase& m_characters;
    RelicSetDatabase& m_relicSets;
    LightConeDatabase& m_lightCones;

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

    // Raw text buffers for the manual HP/ATK/DEF/SPD fields.
    // Digits only; parsed into m_manualConfig on commit.
    std::string m_manualHpText;
    std::string m_manualAtkText;
    std::string m_manualDefText;
    std::string m_manualSpdText;
    // Focused manual field: -1 = none, 0 = HP, 1 = ATK, 2 = DEF, 3 = SPD.
    int m_focusedManualField = -1;

    // Back-navigation request (ESC with no field focused, or < Back button).
    bool m_backRequested = false;
    Rectangle backButtonBounds() const;

    // Tab / mode selection UI
    static constexpr float kTabY = 40.0f;
    static constexpr float kTabH = 40.0f;
    static constexpr float kTabW = 180.0f;
    static constexpr float kTabGap = 20.0f;
    static constexpr float kTabOriginX = 50.0f;

    Rectangle m_buildTabBounds;
    Rectangle m_manualTabBounds;
    bool m_buildTabHover = false;
    bool m_manualTabHover = false;
};
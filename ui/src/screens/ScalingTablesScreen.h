#pragma once

#include "Screen.h"
#include "../assets/AssetManager.h"
#include "../data/CharacterDatabase.h"
#include "../data/Characterloadout.h"
#include "raylib.h"

#include <array>
#include <string>
#include <vector>

// Manual damage tables ("#1[i]%" values) + Eidolon level, per character.
//
// The optimizer-extracted skill_scaling_raw.csv is a first-pass regex dump,
// not verified ground truth — so scaling values stay manual entry here.
// Per action: base multiplier + Eidolon-boosted multiplier (decimals,
// percent-number entry like the Characters screen: "260" = 260%). The
// boosted value applies when the Eidolon level reaches the E-number whose
// skillLevels list the action (data-driven from character_eidolons_rules.json,
// varies per character; shown as boost@EN or "no E boost").
//
// Pure team scope: one row per team member; numbers live on CharacterLoadout.
class ScalingTablesScreen : public Screen
{
public:
    ScalingTablesScreen(AssetManager& assets, CharacterDatabase& characters,
                        LoadoutStore& loadouts);

    // Team context (same pattern as CharactersScreen/RelicEditor).
    void setTeamContext(const std::array<std::string, 4>& team);

    void initialize() override;
    void update(float dt) override;
    void draw() override;

    bool consumeBackRequest();

private:
    static constexpr int kActionCount = 5;
    static constexpr int kFieldCount = kActionCount * 2; // base + boosted

    const char* actionKey(int action) const;   // basic/skill/ult/fua/memosprite
    const char* actionLabel(int action) const; // Basic/Skill/Ult/FUA/Memosprite
    const char* abilityName(int action) const; // Basic ATK/Skill/Ultimate/...
    // Min E-number raising this action for the selected character (0 = none).
    int requiredEidolon(int action) const;

    CharacterLoadout& loadoutFor(const std::string& characterId);
    const CharacterInfo* selectedInfo() const;

    void syncTexts();              // buffers <- loadout (unfocused only)
    void commitField(int field);   // buffer -> loadout

    Rectangle backBounds() const;
    Rectangle teamRowBounds(int index) const;
    Rectangle eidolonMinusBounds() const;
    Rectangle eidolonPlusBounds() const;
    float fieldsBaseY() const;
    Rectangle fieldBounds(int field) const;

    AssetManager& m_assets;
    CharacterDatabase& m_characters;
    LoadoutStore& m_loadouts;

    std::array<std::string, 4> m_team{};
    std::string m_selectedCharacterId;
    std::array<std::string, kFieldCount> m_texts{};
    int m_focusedField = -1;
    bool m_backRequested = false;
};

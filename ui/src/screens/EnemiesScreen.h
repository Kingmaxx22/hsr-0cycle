#pragma once

#include "Screen.h"
#include "../assets/AssetManager.h"
#include "../data/EnemyDatabase.h"
#include "raylib.h"

#include <array>
#include <string>
#include <vector>

// One enemy placement: instance id + mid-combat spawn clock (0 = present
// from the start). Kept data-light; full stats resolve from EnemyDatabase
// at encounter build time (single source of truth, Sec 22.9).
struct SlotEntry
{
    std::string id;
    int spawnAv = 0;
    // Per-entry RES override in percent-points (-1 = Q2 auto-rule).
    // Cycles auto -> 0 -> 20 -> 40 -> auto in the slot editor.
    double resOverride = -1.0;
    // Phase 4.3: user-asserted Exo-Toughness (0 = none). No data source
    // carries Exo values, so this is manual per-encounter configuration.
    // Cycles 0 -> 30 -> 60 -> 90 -> 0 in the slot editor.
    int exoToughness = 0;
};

class EnemiesScreen : public Screen
{
public:
    enum class Filter
    {
        BossElite,
        All
    };

    // Exactly five encounter slots (AGENTS.md Sec 19/35); each holds
    // zero or more entries. Empty slots are valid. The same enemy id
    // may appear multiple times in one slot (stacked duplicates).
    static constexpr int kSlotCount = 5;
    // Visible enemy rows in the scroll list.
    static constexpr int kVisibleRows = 9;

    EnemiesScreen(AssetManager& assets, EnemyDatabase& enemies);

    void initialize() override;
    void update(float dt) override;
    void draw() override;

    const std::array<std::vector<SlotEntry>, kSlotCount>& getSlots() const { return slots; }
    // Per-slot wave arming: a sequential slot fights one entry at a time,
    // spawning the next when the current one dies.
    const std::array<bool, kSlotCount>& getSequential() const { return slotSequential; }
    bool consumeBackRequest();

private:
    Rectangle backBounds();
    Rectangle searchBounds();
    Rectangle filterBounds(Filter selectedFilter);
    Rectangle rowBounds(int row);
    Rectangle rowAddBounds(int row);
    Rectangle slotTabBounds(int slot);
    Rectangle slotEntryBounds(int entryRow) const;
    Rectangle slotEntryRemoveBounds(int entryRow) const;
    Rectangle slotEntrySpawnMinusBounds(int entryRow) const;
    Rectangle slotEntrySpawnPlusBounds(int entryRow) const;
    Rectangle slotEntryResBounds(int entryRow) const;
    Rectangle slotEntryExoBounds(int entryRow) const;
    Rectangle slotClearBounds() const;
    Rectangle slotWaveBounds() const;

    void rebuildFiltered();
    void toggleSlotEntry(const EnemyInfo& enemy);
    void addSlotEntry(const EnemyInfo& enemy);
    void drawSlotContents();

    AssetManager& assets;
    EnemyDatabase& enemies;

    std::string searchText;
    Filter filter = Filter::BossElite;

    std::array<std::vector<SlotEntry>, kSlotCount> slots;
    int activeSlot = 0;
    // Wave arming per slot (default ON: one entry active at a time).
    std::array<bool, kSlotCount> slotSequential = {true, true, true, true, true};

    std::vector<const EnemyInfo*> filtered;
    int hoveredRow = -1;
    int scrollOffset = 0;
    bool backRequested = false;
};

#include "EnemiesScreen.h"
#include "raylib.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <sstream>

namespace
{
    const Color kPanelBg{28, 31, 41, 255};
    const Color kPanelBorder{55, 59, 72, 255};
    const Color kAccentBg{55, 62, 82, 255};
    const Color kAccentBorder{115, 140, 190, 255};
    const Color kDimText{140, 145, 158, 255};

    std::string lowerCopy(std::string value)
    {
        std::transform(
            value.begin(), value.end(), value.begin(),
            [](unsigned char c)
            {
                return static_cast<char>(std::tolower(c));
            });

        return value;
    }

    std::string prettyElement(std::string value)
    {
        if (!value.empty())
        {
            value[0] = static_cast<char>(
                std::toupper(static_cast<unsigned char>(value[0])));
        }

        return value;
    }

    std::string weaknessText(const EnemyInfo& enemy)
    {
        if (enemy.weaknesses.empty())
            return "None";

        std::string out;

        for (size_t i = 0; i < enemy.weaknesses.size(); ++i)
        {
            if (i)
                out += ", ";

            out += prettyElement(enemy.weaknesses[i]);
        }

        return out;
    }


    void drawWrappedText(const std::string& text, int x, int y, int fontSize, Color color, int maxWidth, int lineSpacing)
    {
        std::istringstream words(text);
        std::string word;
        std::string line;
        int drawY = y;

        while (words >> word)
        {
            const std::string candidate = line.empty() ? word : line + " " + word;

            if (!line.empty() && MeasureText(candidate.c_str(), fontSize) > maxWidth)
            {
                DrawText(line.c_str(), x, drawY, fontSize, color);
                drawY += lineSpacing;
                line = word;
            }
            else
            {
                line = candidate;
            }
        }

        if (!line.empty())
            DrawText(line.c_str(), x, drawY, fontSize, color);
    }

    std::string compactNumber(double value)
    {
        char buffer[64]{};

        if (value >= 1000000.0)
            std::snprintf(
                buffer, sizeof(buffer), "%.2fM",
                value / 1000000.0);
        else if (value >= 1000.0)
            std::snprintf(
                buffer, sizeof(buffer), "%.1fk",
                value / 1000.0);
        else
            std::snprintf(
                buffer, sizeof(buffer), "%.0f",
                value);

        return buffer;
    }
}

EnemiesScreen::EnemiesScreen(
    AssetManager& assets_,
    EnemyDatabase& enemies_)
    : assets(assets_), enemies(enemies_)
{
}

void EnemiesScreen::initialize()
{
    rebuildFiltered();
}

Rectangle EnemiesScreen::backBounds()
{
    return Rectangle{286.0f, 28.0f, 86.0f, 34.0f};
}

Rectangle EnemiesScreen::searchBounds()
{
    return Rectangle{310.0f, 104.0f, 470.0f, 38.0f};
}

Rectangle EnemiesScreen::filterBounds(Filter selectedFilter)
{
    if (selectedFilter == Filter::BossElite)
        return Rectangle{800.0f, 104.0f, 180.0f, 38.0f};

    return Rectangle{990.0f, 104.0f, 120.0f, 38.0f};
}

Rectangle EnemiesScreen::rowBounds(int row)
{
    return Rectangle{
        310.0f,
        236.0f + row * 66.0f,
        800.0f,
        56.0f
    };
}

Rectangle EnemiesScreen::rowAddBounds(int row)
{
    Rectangle r = rowBounds(row);
    return Rectangle{
        r.x + r.width - 78.0f,
        r.y + 10.0f,
        66.0f,
        36.0f
    };
}

Rectangle EnemiesScreen::slotTabBounds(int slot)
{
    return Rectangle{
        310.0f + slot * 140.0f,
        132.0f,
        132.0f,
        38.0f
    };
}

// Slot-content rows live at the bottom of the right detail panel.
Rectangle EnemiesScreen::slotEntryBounds(int entryRow) const
{
    return Rectangle{1142.0f, 666.0f + entryRow * 26.0f, 290.0f, 24.0f};
}

Rectangle EnemiesScreen::slotEntryRemoveBounds(int entryRow) const
{
    Rectangle r = slotEntryBounds(entryRow);
    return Rectangle{r.x + r.width - 24.0f, r.y + 2.0f, 20.0f, 20.0f};
}

Rectangle EnemiesScreen::slotEntrySpawnMinusBounds(int entryRow) const
{
    Rectangle r = slotEntryBounds(entryRow);
    return Rectangle{r.x + 76.0f, r.y + 2.0f, 20.0f, 20.0f};
}

Rectangle EnemiesScreen::slotEntrySpawnPlusBounds(int entryRow) const
{
    Rectangle r = slotEntryBounds(entryRow);
    return Rectangle{r.x + 98.0f, r.y + 2.0f, 20.0f, 20.0f};
}

Rectangle EnemiesScreen::slotEntryResBounds(int entryRow) const
{
    Rectangle r = slotEntryBounds(entryRow);
    return Rectangle{r.x + 150.0f, r.y + 2.0f, 58.0f, 20.0f};
}

Rectangle EnemiesScreen::slotEntryExoBounds(int entryRow) const
{
    Rectangle r = slotEntryBounds(entryRow);
    return Rectangle{r.x + 210.0f, r.y + 2.0f, 50.0f, 20.0f};
}

Rectangle EnemiesScreen::slotClearBounds() const
{
    return Rectangle{1142.0f, 828.0f, 120.0f, 26.0f};
}

Rectangle EnemiesScreen::slotWaveBounds() const
{
    // Wave arming for the active slot, right of the slot header.
    return Rectangle{1290.0f, 636.0f, 120.0f, 26.0f};
}

void EnemiesScreen::drawSlotContents()
{
    const auto& slot = slots[static_cast<size_t>(activeSlot)];

    std::string header = "SLOT " + std::to_string(activeSlot + 1) +
        " (" + std::to_string(slot.size()) + ")";
    DrawText(header.c_str(), 1150, 642, 14, RAYWHITE);

    // WAVE toggle: sequential slots fight one entry at a time.
    {
        Rectangle waveR = slotWaveBounds();
        bool wave = slotSequential[static_cast<size_t>(activeSlot)];
        DrawRectangleRounded(waveR, 0.14f, 6,
            wave ? kAccentBg : kPanelBg);
        DrawRectangleRoundedLines(waveR, 0.14f, 6,
            wave ? kAccentBorder : kPanelBorder);
        std::string waveLabel = wave ? "WAVE: ON" : "WAVE: OFF";
        DrawText(waveLabel.c_str(), static_cast<int>(waveR.x + 14),
                 static_cast<int>(waveR.y + 6), 13,
                 wave ? RAYWHITE : kDimText);
    }

    size_t shown = std::min(slot.size(), static_cast<size_t>(6));
    for (size_t e = 0; e < shown; ++e)
    {
        int row = static_cast<int>(e);
        Rectangle r = slotEntryBounds(row);
        const EnemyInfo* info = enemies.get(slot[e].id);

        DrawRectangleRounded(r, 0.12f, 6, kPanelBg);
        DrawRectangleRoundedLines(r, 0.12f, 6, kPanelBorder);

        std::string name = info != nullptr ? info->name : slot[e].id;
        if (name.size() > 9)
            name = name.substr(0, 8) + ".";
        DrawText(name.c_str(), static_cast<int>(r.x + 6),
                 static_cast<int>(r.y + 5), 12, RAYWHITE);

        // Spawn clock (0 = present from the start).
        std::string av = (slot[e].spawnAv <= 0) ? "0"
                         : std::to_string(slot[e].spawnAv);
        DrawText(av.c_str(), static_cast<int>(r.x + 120),
                 static_cast<int>(r.y + 5), 11, kDimText);

        Rectangle minusR = slotEntrySpawnMinusBounds(row);
        Rectangle plusR = slotEntrySpawnPlusBounds(row);
        Rectangle resR = slotEntryResBounds(row);
        Rectangle exoR = slotEntryExoBounds(row);
        Rectangle xR = slotEntryRemoveBounds(row);
        DrawRectangleRounded(minusR, 0.2f, 4, kPanelBg);
        DrawRectangleRoundedLines(minusR, 0.2f, 4, kPanelBorder);
        DrawText("-", static_cast<int>(minusR.x + 7),
                 static_cast<int>(minusR.y + 2), 12, RAYWHITE);
        DrawRectangleRounded(plusR, 0.2f, 4, kPanelBg);
        DrawRectangleRoundedLines(plusR, 0.2f, 4, kPanelBorder);
        DrawText("+", static_cast<int>(plusR.x + 6),
                 static_cast<int>(plusR.y + 2), 12, RAYWHITE);
        // Per-entry RES override: auto-rule unless cycled otherwise.
        std::string resLabel = "R:auto";
        if (slot[e].resOverride >= 0.0)
            resLabel = "R:" + std::to_string(static_cast<int>(slot[e].resOverride));
        DrawRectangleRounded(resR, 0.2f, 4, kPanelBg);
        DrawRectangleRoundedLines(resR, 0.2f, 4, kPanelBorder);
        DrawText(resLabel.c_str(), static_cast<int>(resR.x + 5),
                 static_cast<int>(resR.y + 3), 11,
                 slot[e].resOverride >= 0.0 ? RAYWHITE : kDimText);
        // Phase 4.3: user-asserted Exo-Toughness (0 = none).
        std::string exoLabel = (slot[e].exoToughness <= 0)
            ? "E:-" : "E:" + std::to_string(slot[e].exoToughness);
        DrawRectangleRounded(exoR, 0.2f, 4, kPanelBg);
        DrawRectangleRoundedLines(exoR, 0.2f, 4, kPanelBorder);
        DrawText(exoLabel.c_str(), static_cast<int>(exoR.x + 5),
                 static_cast<int>(exoR.y + 3), 11,
                 slot[e].exoToughness > 0 ? RAYWHITE : kDimText);
        DrawRectangleRounded(xR, 0.2f, 4, Color{90, 30, 30, 255});
        DrawText("x", static_cast<int>(xR.x + 6),
                 static_cast<int>(xR.y + 2), 12, RAYWHITE);
    }
    if (slot.size() > shown)
    {
        std::string more = "+" + std::to_string(slot.size() - shown) + " more";
        DrawText(more.c_str(), 1150,
                 static_cast<int>(slotEntryBounds(static_cast<int>(shown)).y + 4),
                 12, kDimText);
    }

    if (!slot.empty())
    {
        Rectangle clearR = slotClearBounds();
        DrawRectangleRounded(clearR, 0.14f, 6, kPanelBg);
        DrawRectangleRoundedLines(clearR, 0.14f, 6, kPanelBorder);
        DrawText("Clear slot", static_cast<int>(clearR.x + 12),
                 static_cast<int>(clearR.y + 6), 13, kDimText);
    }
}

void EnemiesScreen::rebuildFiltered()
{
    filtered.clear();

    const std::string query = lowerCopy(searchText);

    for (const EnemyInfo& enemy : enemies.all())
    {
        if (filter == Filter::BossElite &&
            !enemy.isBossOrElite())
        {
            continue;
        }

        if (!query.empty())
        {
            const std::string name = lowerCopy(enemy.name);
            const std::string id = lowerCopy(enemy.id);

            if (name.find(query) == std::string::npos &&
                id.find(query) == std::string::npos)
            {
                continue;
            }
        }

        filtered.push_back(&enemy);
    }

    std::stable_sort(
        filtered.begin(),
        filtered.end(),
        [](const EnemyInfo* a, const EnemyInfo* b)
        {
            if (a->isBossOrElite() != b->isBossOrElite())
                return a->isBossOrElite() > b->isBossOrElite();

            if (a->hp != b->hp)
                return a->hp > b->hp;

            if (a->level != b->level)
                return a->level > b->level;

            return a->name < b->name;
        });

    const int maxScroll =
        std::max(0, static_cast<int>(filtered.size()) - kVisibleRows);

    scrollOffset =
        std::clamp(scrollOffset, 0, maxScroll);
}

void EnemiesScreen::toggleSlotEntry(const EnemyInfo& enemy)
{
    // Click on a row body toggles the id in the ACTIVE slot: removes one
    // instance if present, otherwise adds one. Duplicates stacked via the
    // ADD button are removed one at a time this way.
    auto& slot = slots[static_cast<size_t>(activeSlot)];
    for (auto it = slot.begin(); it != slot.end(); ++it)
    {
        if (it->id == enemy.id)
        {
            slot.erase(it);
            return;
        }
    }
    slot.push_back(SlotEntry{enemy.id, 0});
}

void EnemiesScreen::addSlotEntry(const EnemyInfo& enemy)
{
    // ADD button: always appends another instance, so the same enemy can
    // be stacked multiple times in one slot. The encounter builder turns
    // each entry into its own engine instance.
    slots[static_cast<size_t>(activeSlot)].push_back(SlotEntry{enemy.id, 0});
}

bool EnemiesScreen::consumeBackRequest()
{
    if (!backRequested)
        return false;

    backRequested = false;
    return true;
}

void EnemiesScreen::update(float dt)
{
    (void)dt;

    const Vector2 mouse = GetMousePosition();
    const bool pressed =
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    if (CheckCollisionPointRec(mouse, searchBounds()))
    {
        int key = GetCharPressed();

        while (key > 0)
        {
            if (key >= 32 &&
                key <= 126 &&
                searchText.size() < 48)
            {
                searchText.push_back(
                    static_cast<char>(key));
                rebuildFiltered();
            }

            key = GetCharPressed();
        }

        if (IsKeyPressed(KEY_BACKSPACE) &&
            !searchText.empty())
        {
            searchText.pop_back();
            rebuildFiltered();
        }

        if (IsKeyPressed(KEY_ESCAPE))
        {
            searchText.clear();
            rebuildFiltered();
        }
    }

    // Slot tabs + slot-content controls (detail panel).
    if (pressed)
    {
        for (int s = 0; s < kSlotCount; ++s)
        {
            if (CheckCollisionPointRec(mouse, slotTabBounds(s)))
            {
                activeSlot = s;
                return;
            }
        }

        const auto& slot = slots[static_cast<size_t>(activeSlot)];
        size_t shown = std::min(slot.size(), static_cast<size_t>(7));
        for (size_t e = 0; e < shown; ++e)
        {
            int row = static_cast<int>(e);
            if (CheckCollisionPointRec(mouse, slotEntryRemoveBounds(row)))
            {
                slots[static_cast<size_t>(activeSlot)].erase(
                    slots[static_cast<size_t>(activeSlot)].begin() + row);
                return;
            }
            if (CheckCollisionPointRec(mouse, slotEntrySpawnMinusBounds(row)))
            {
                SlotEntry& entry = slots[static_cast<size_t>(activeSlot)][static_cast<size_t>(row)];
                entry.spawnAv = std::max(0, entry.spawnAv - 500);
                return;
            }
            if (CheckCollisionPointRec(mouse, slotEntrySpawnPlusBounds(row)))
            {
                SlotEntry& entry = slots[static_cast<size_t>(activeSlot)][static_cast<size_t>(row)];
                entry.spawnAv = std::min(15000, entry.spawnAv + 500);
                return;
            }
            if (CheckCollisionPointRec(mouse, slotEntryResBounds(row)))
            {
                // Cycle auto -> 0 -> 20 -> 40 -> auto (percent-points).
                SlotEntry& entry = slots[static_cast<size_t>(activeSlot)][static_cast<size_t>(row)];
                if (entry.resOverride < 0.0)
                    entry.resOverride = 0.0;
                else if (entry.resOverride < 0.5)
                    entry.resOverride = 20.0;
                else if (entry.resOverride < 20.5)
                    entry.resOverride = 40.0;
                else
                    entry.resOverride = -1.0;
                return;
            }
            if (CheckCollisionPointRec(mouse, slotEntryExoBounds(row)))
            {
                // Phase 4.3: cycle Exo-Toughness 0 -> 30 -> 60 -> 90 -> 0.
                SlotEntry& entry = slots[static_cast<size_t>(activeSlot)][static_cast<size_t>(row)];
                if (entry.exoToughness < 30)
                    entry.exoToughness = 30;
                else if (entry.exoToughness < 60)
                    entry.exoToughness = 60;
                else if (entry.exoToughness < 90)
                    entry.exoToughness = 90;
                else
                    entry.exoToughness = 0;
                return;
            }
        }
        if (!slot.empty() && CheckCollisionPointRec(mouse, slotClearBounds()))
        {
            slots[static_cast<size_t>(activeSlot)].clear();
            return;
        }
        if (CheckCollisionPointRec(mouse, slotWaveBounds()))
        {
            bool& wave = slotSequential[static_cast<size_t>(activeSlot)];
            wave = !wave;
            return;
        }
    }

    const float wheel = GetMouseWheelMove();

    if (wheel != 0.0f)
    {
        const Rectangle listArea{
            310.0f, 234.0f, 800.0f, 600.0f
        };

        if (CheckCollisionPointRec(mouse, listArea))
        {
            const int maxScroll =
                std::max(0, static_cast<int>(filtered.size()) - kVisibleRows);

            scrollOffset =
                std::clamp(
                    scrollOffset -
                        static_cast<int>(wheel),
                    0,
                    maxScroll);
        }
    }

    hoveredRow = -1;

    for (int row = 0; row < kVisibleRows; ++row)
    {
        const int index = scrollOffset + row;

        if (index >= static_cast<int>(filtered.size()))
            break;

        if (CheckCollisionPointRec(
                mouse,
                rowBounds(row)))
        {
            hoveredRow = index;

            if (pressed)
            {
                // ADD button stacks a duplicate; row body toggles.
                if (CheckCollisionPointRec(mouse, rowAddBounds(row)))
                    addSlotEntry(
                        *filtered[static_cast<size_t>(index)]);
                else
                    toggleSlotEntry(
                        *filtered[static_cast<size_t>(index)]);
            }

            break;
        }
    }

    if (!pressed)
        return;

    if (CheckCollisionPointRec(mouse, backBounds()))
    {
        backRequested = true;
        return;
    }

    if (CheckCollisionPointRec(
            mouse,
            filterBounds(Filter::BossElite)))
    {
        filter = Filter::BossElite;
        scrollOffset = 0;
        rebuildFiltered();
        return;
    }

    if (CheckCollisionPointRec(
            mouse,
            filterBounds(Filter::All)))
    {
        filter = Filter::All;
        scrollOffset = 0;
        rebuildFiltered();
        return;
    }
}

void EnemiesScreen::draw()
{
    DrawText(
        "Enemies / MoC Boss Selector",
        390, 28, 30, RAYWHITE);

    DrawRectangleRounded(
        backBounds(), 0.2f, 8, kPanelBg);
    DrawRectangleRoundedLines(
        backBounds(), 0.2f, 8, kPanelBorder);

    DrawText("< Back", 299, 37, 15, RAYWHITE);

    DrawText(
        "TARGET ENEMY",
        310, 76, 14, kDimText);

    const Rectangle search = searchBounds();

    DrawRectangleRounded(
        search, 0.14f, 8, kPanelBg);
    DrawRectangleRoundedLines(
        search, 0.14f, 8, kPanelBorder);

    const std::string searchLabel =
        searchText.empty()
            ? "Search enemy name or ID..."
            : searchText;

    DrawText(
        searchLabel.c_str(),
        324, 116, 15,
        searchText.empty()
            ? kDimText
            : RAYWHITE);

    const char* filterLabels[] = {
        "BOSS / ELITE",
        "ALL MONSTERS"
    };

    for (int i = 0; i < 2; ++i)
    {
        const Filter current =
            i == 0
                ? Filter::BossElite
                : Filter::All;

        const Rectangle r =
            filterBounds(current);

        const bool active =
            filter == current;

        DrawRectangleRounded(
            r, 0.14f, 8,
            active ? kAccentBg : kPanelBg);

        DrawRectangleRoundedLines(
            r, 0.14f, 8,
            active
                ? kAccentBorder
                : kPanelBorder);

        DrawText(
            filterLabels[i],
            static_cast<int>(r.x + 12),
            static_cast<int>(r.y + 12),
            13,
            active ? RAYWHITE : kDimText);
    }

    DrawText(
        TextFormat(
            "%d matches",
            static_cast<int>(filtered.size())),
        1012, 154, 13, kDimText);

    // Encounter slot tabs: exactly five slots, click to arm one for edits.
    for (int s = 0; s < kSlotCount; ++s)
    {
        Rectangle tab = slotTabBounds(s);
        const bool armed = (s == activeSlot);
        DrawRectangleRounded(
            tab, 0.14f, 8,
            armed ? kAccentBg : kPanelBg);
        DrawRectangleRoundedLines(
            tab, 0.14f, 8,
            armed ? kAccentBorder : kPanelBorder);

        std::string label = "SLOT " + std::to_string(s + 1) +
            " (" + std::to_string(slots[static_cast<size_t>(s)].size()) + ")";
        DrawText(
            label.c_str(),
            static_cast<int>(tab.x + 14),
            static_cast<int>(tab.y + 11),
            14,
            armed ? RAYWHITE : kDimText);
    }



    for (int row = 0; row < kVisibleRows; ++row)
    {
        const int index = scrollOffset + row;

        if (index >= static_cast<int>(filtered.size()))
            break;

        const EnemyInfo& enemy =
            *filtered[static_cast<size_t>(index)];

        const Rectangle r = rowBounds(row);

        // Selected = present in the ACTIVE slot (toggle model). Duplicates
        // stacked via ADD still highlight; the badge shows the count.
        int inSlot = 0;
        for (const auto& entry : slots[static_cast<size_t>(activeSlot)])
        {
            if (entry.id == enemy.id)
                ++inSlot;
        }
        const bool selected = inSlot > 0;

        const bool hovered =
            index == hoveredRow;

        DrawRectangleRounded(
            r, 0.08f, 8,
            selected
                ? kAccentBg
                : (hovered
                    ? Color{38, 42, 54, 255}
                    : kPanelBg));

        DrawRectangleRoundedLines(
            r, 0.08f, 8,
            selected
                ? kAccentBorder
                : kPanelBorder);

        // Enemy portrait. The asset is named after the enemy.
        if (Texture2D* icon =
                assets.enemy(enemy.id, enemy.name))
        {
            const Rectangle src{
                0, 0,
                static_cast<float>(icon->width),
                static_cast<float>(icon->height)
            };

            const Rectangle dst{
                r.x + 7.0f,
                r.y + 5.0f,
                46.0f,
                46.0f
            };

            DrawTexturePro(
                *icon,
                src,
                dst,
                Vector2{0, 0},
                0.0f,
                WHITE);
        }

        const int textX =
            static_cast<int>(r.x + 62.0f);

        DrawText(
            enemy.name.c_str(),
            textX,
            static_cast<int>(r.y + 8.0f),
            16,
            RAYWHITE);

        const std::string meta =
            TextFormat(
                "Lv.%d   HP %s   SPD %.0f   Toughness %.0f",
                enemy.level,
                compactNumber(enemy.hp).c_str(),
                enemy.spd,
                enemy.toughness);

        DrawText(
            meta.c_str(),
            textX,
            static_cast<int>(r.y + 31.0f),
            12,
            kDimText);

        if (!enemy.rating.empty())
        {
            DrawText(
                enemy.rating.c_str(),
                static_cast<int>(r.x + 620.0f),
                static_cast<int>(r.y + 10.0f),
                11,
                Color{210, 180, 140, 255});
        }

        // Duplicate badge: how many copies of this enemy sit in the
        // armed slot (only when stacked more than once).
        if (inSlot > 1)
        {
            DrawText(
                TextFormat("x%d", inSlot),
                static_cast<int>(r.x + 620.0f),
                static_cast<int>(r.y + 30.0f),
                12,
                Color{140, 200, 140, 255});
        }

        // ADD button: stacks another copy of this enemy in the armed slot.
        const Rectangle addR = rowAddBounds(row);
        const bool addHover = CheckCollisionPointRec(
            GetMousePosition(), addR);
        DrawRectangleRounded(
            addR, 0.2f, 6,
            addHover ? kAccentBg : kPanelBg);
        DrawRectangleRoundedLines(
            addR, 0.2f, 6,
            addHover ? kAccentBorder : kPanelBorder);
        const char* addLabel = "ADD";
        DrawText(
            addLabel,
            static_cast<int>(addR.x + 19.0f),
            static_cast<int>(addR.y + 10.0f),
            14,
            addHover ? RAYWHITE : kDimText);
    }

    const Rectangle detail{
        1130.0f, 104.0f, 280.0f, 764.0f
    };

    DrawRectangleRounded(
        detail, 0.06f, 8, kPanelBg);
    DrawRectangleRoundedLines(
        detail, 0.06f, 8, kPanelBorder);

    // Detail target: hovered row wins, else first entry of active slot.
    const EnemyInfo* selected = nullptr;
    if (hoveredRow >= 0 &&
        hoveredRow < static_cast<int>(filtered.size()))
        selected = filtered[static_cast<size_t>(hoveredRow)];
    if (selected == nullptr)
    {
        const auto& slot = slots[static_cast<size_t>(activeSlot)];
        if (!slot.empty())
            selected = enemies.get(slot.front().id);
    }

    if (!selected)
    {
        DrawText(
            "SELECT AN ENEMY",
            1150, 128, 15, kDimText);

        DrawText(
            "Click a row to add it to",
            1150, 158, 13, kDimText);

        DrawText(
            "the active slot below.",
            1150, 178, 13, kDimText);

        DrawText(
            "Click again to remove,",
            1150, 204, 12, kDimText);

        DrawText(
            "ADD stacks duplicates.",
            1150, 222, 12, kDimText);

        DrawText(
            "WAVE fights one at a time.",
            1150, 240, 12, kDimText);

        drawSlotContents();
        return;
    }

    DrawText(
        selected->name.c_str(),
        1150, 124, 18, RAYWHITE);

    DrawText(
        TextFormat(
            "ID %s",
            selected->id.c_str()),
        1150, 151, 12, kDimText);

    if (Texture2D* portrait =
            assets.enemy(
                selected->id,
                selected->name))
    {
        const Rectangle src{
            0, 0,
            static_cast<float>(portrait->width),
            static_cast<float>(portrait->height)
        };

        const Rectangle dst{
            1160.0f,
            175.0f,
            220.0f,
            150.0f
        };

        DrawTexturePro(
            *portrait,
            src,
            dst,
            Vector2{0, 0},
            0.0f,
            WHITE);
    }

    int y = 350;

    auto statRow =
        [&](const char* label, const char* value)
        {
            DrawText(
                label,
                1150, y, 12, kDimText);

            DrawText(
                value,
                1280, y, 14, RAYWHITE);

            y += 27;
        };

    statRow(
        "Level",
        TextFormat("%d", selected->level));

    statRow(
        "HP",
        compactNumber(selected->hp).c_str());

    statRow(
        "ATK",
        compactNumber(selected->atk).c_str());

    statRow(
        "DEF",
        compactNumber(selected->def).c_str());

    statRow(
        "SPD",
        TextFormat("%.0f", selected->spd));

    statRow(
        "Toughness",
        TextFormat("%.0f", selected->toughness));

    statRow(
        "Effect RES",
        TextFormat("%.1f%%", selected->effectRes));

    y += 5;

    DrawText(
        "WEAKNESSES",
        1150, y, 12, kDimText);

    y += 22;

    // Keep the weaknesses inside the detail card instead of letting the
    // raw string run through the right edge of the window.
    drawWrappedText(
        weaknessText(*selected),
        1150, y, 12, RAYWHITE,
        238, 18);

    drawSlotContents();

    DrawText(
        "Mouse wheel: scroll list | Click a row to toggle, ADD stacks duplicates",
        310, 868, 12, kDimText);
}

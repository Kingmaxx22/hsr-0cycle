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
    const Color kDangerBg{72, 45, 50, 255};
    const Color kDangerBorder{125, 75, 82, 255};

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

    void drawWrappedText(
        const std::string& text,
        int x,
        int y,
        int fontSize,
        Color color,
        int maxWidth,
        int lineSpacing)
    {
        std::istringstream words(text);
        std::string word;
        std::string line;
        int drawY = y;

        while (words >> word)
        {
            const std::string candidate =
                line.empty() ? word : line + " " + word;

            if (!line.empty() &&
                MeasureText(candidate.c_str(), fontSize) > maxWidth)
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
        {
            std::snprintf(
                buffer,
                sizeof(buffer),
                "%.2fM",
                value / 1000000.0);
        }
        else if (value >= 1000.0)
        {
            std::snprintf(
                buffer,
                sizeof(buffer),
                "%.1fk",
                value / 1000.0);
        }
        else
        {
            std::snprintf(
                buffer,
                sizeof(buffer),
                "%.0f",
                value);
        }

        return buffer;
    }

    // Returns the original string when it fits, otherwise truncates it
    // with "..." so text can never run through another UI element.
    std::string fitText(
        const std::string& text,
        int fontSize,
        int maxWidth)
    {
        if (MeasureText(text.c_str(), fontSize) <= maxWidth)
            return text;

        const std::string ellipsis = "...";

        if (MeasureText(ellipsis.c_str(), fontSize) > maxWidth)
            return "";

        std::string result = text;

        while (!result.empty())
        {
            result.pop_back();

            const std::string candidate =
                result + ellipsis;

            if (MeasureText(candidate.c_str(), fontSize) <= maxWidth)
                return candidate;
        }

        return ellipsis;
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

Rectangle EnemiesScreen::slotBounds(int slot)
{
    return Rectangle{
        310.0f + slot * 160.0f,
        92.0f,
        148.0f,
        118.0f
    };
}

Rectangle EnemiesScreen::slotSummaryBounds(int slot)
{
    (void)slot;
    return Rectangle{1130.0f, 104.0f, 280.0f, 180.0f};
}

Rectangle EnemiesScreen::searchBounds()
{
    return Rectangle{310.0f, 230.0f, 470.0f, 38.0f};
}

Rectangle EnemiesScreen::filterBounds(Filter selectedFilter)
{
    if (selectedFilter == Filter::BossElite)
        return Rectangle{800.0f, 230.0f, 180.0f, 38.0f};

    return Rectangle{990.0f, 230.0f, 120.0f, 38.0f};
}

Rectangle EnemiesScreen::addBounds()
{
    return Rectangle{1130.0f, 306.0f, 280.0f, 38.0f};
}

Rectangle EnemiesScreen::removeBounds()
{
    return Rectangle{1130.0f, 350.0f, 136.0f, 34.0f};
}

Rectangle EnemiesScreen::clearBounds()
{
    return Rectangle{1274.0f, 350.0f, 136.0f, 34.0f};
}

Rectangle EnemiesScreen::rowBounds(int row)
{
    return Rectangle{
        310.0f,
        294.0f + row * 58.0f,
        800.0f,
        50.0f
    };
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
        std::max(0, static_cast<int>(filtered.size()) - 9);

    scrollOffset =
        std::clamp(scrollOffset, 0, maxScroll);
}

void EnemiesScreen::selectEnemy(const EnemyInfo& enemy)
{
    selectedEnemyId = enemy.id;
}

void EnemiesScreen::addSelectedEnemyToActiveSlot()
{
    if (selectedEnemyId.empty())
        return;

    enemySlots[
        static_cast<size_t>(activeSlot)
    ].push_back(selectedEnemyId);

    const int maxScroll =
        std::max(
            0,
            static_cast<int>(
                enemySlots[
                    static_cast<size_t>(activeSlot)].size()) - 6);

    activeSlotScroll =
        std::clamp(activeSlotScroll, 0, maxScroll);
}

void EnemiesScreen::removeLastEnemyFromActiveSlot()
{
    auto& slot =
        enemySlots[
            static_cast<size_t>(activeSlot)
        ];

    if (!slot.empty())
        slot.pop_back();

    const int maxScroll =
        std::max(0, static_cast<int>(slot.size()) - 6);

    activeSlotScroll =
        std::clamp(activeSlotScroll, 0, maxScroll);
}

void EnemiesScreen::clearActiveSlot()
{
    enemySlots[
        static_cast<size_t>(activeSlot)
    ].clear();

    activeSlotScroll = 0;
}

const std::string& EnemiesScreen::getSelectedEnemyId() const
{
    static const std::string empty;

    const auto& slot =
        enemySlots[
            static_cast<size_t>(activeSlot)
        ];

    if (slot.empty())
        return empty;

    return slot.front();
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

    // Search input.
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

    // Scroll the active slot list when the mouse is over the
    // top-right active-slot panel. Otherwise scroll the enemy list.
    const float wheel = GetMouseWheelMove();

    if (wheel != 0.0f)
    {
        const Rectangle activeListArea =
            slotSummaryBounds(0);

        if (CheckCollisionPointRec(mouse, activeListArea))
        {
            const auto& slot =
                enemySlots[static_cast<size_t>(activeSlot)];

            const int maxScroll =
                std::max(
                    0,
                    static_cast<int>(slot.size()) - 6);

            activeSlotScroll =
                std::clamp(
                    activeSlotScroll -
                        static_cast<int>(wheel),
                    0,
                    maxScroll);
        }
        else
        {
            const Rectangle listArea{
                310.0f,
                292.0f,
                800.0f,
                540.0f
            };

            if (CheckCollisionPointRec(mouse, listArea))
            {
                const int maxScroll =
                    std::max(
                        0,
                        static_cast<int>(filtered.size()) - 9);

                scrollOffset =
                    std::clamp(
                        scrollOffset -
                            static_cast<int>(wheel),
                        0,
                        maxScroll);
            }
        }
    }

    hoveredSlot = -1;

    // Select active slot.
    for (int slot = 0;
         slot < static_cast<int>(kEnemySlotCount);
         ++slot)
    {
        if (CheckCollisionPointRec(
                mouse,
                slotBounds(slot)))
        {
            hoveredSlot = slot;

            if (pressed)
            {
                activeSlot = slot;
                activeSlotScroll = 0;
            }

            break;
        }
    }

    // Enemy list hover/select.
    hoveredRow = -1;

    for (int row = 0; row < 9; ++row)
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
                selectEnemy(
                    *filtered[
                        static_cast<size_t>(index)]);
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

    if (CheckCollisionPointRec(mouse, addBounds()))
    {
        addSelectedEnemyToActiveSlot();
        return;
    }

    if (CheckCollisionPointRec(mouse, removeBounds()))
    {
        removeLastEnemyFromActiveSlot();
        return;
    }

    if (CheckCollisionPointRec(mouse, clearBounds()))
    {
        clearActiveSlot();
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
    // ------------------------------------------------------------
    // Header
    // ------------------------------------------------------------

    DrawText(
        "Enemies / MoC Enemy Setup",
        390,
        28,
        30,
        RAYWHITE);

    DrawRectangleRounded(
        backBounds(),
        0.2f,
        8,
        kPanelBg);

    DrawRectangleRoundedLines(
        backBounds(),
        0.2f,
        8,
        kPanelBorder);

    DrawText(
        "< Back",
        299,
        37,
        15,
        RAYWHITE);

    DrawText(
        TextFormat(
            "ACTIVE SLOT: %d",
            activeSlot + 1),
        1130,
        76,
        13,
        kDimText);

    DrawText(
        "ENEMY POSITIONS",
        310,
        76,
        14,
        kDimText);

    // ------------------------------------------------------------
    // Five main enemy slots
    // ------------------------------------------------------------

    for (int slot = 0;
         slot < static_cast<int>(kEnemySlotCount);
         ++slot)
    {
        const Rectangle r =
            slotBounds(slot);

        const bool active =
            slot == activeSlot;

        const bool hovered =
            slot == hoveredSlot;

        DrawRectangleRounded(
            r,
            0.08f,
            8,
            active
                ? kAccentBg
                : (hovered
                    ? Color{38, 42, 54, 255}
                    : kPanelBg));

        DrawRectangleRoundedLines(
            r,
            0.08f,
            8,
            active
                ? kAccentBorder
                : kPanelBorder);

        DrawText(
            TextFormat(
                "SLOT %d",
                slot + 1),
            static_cast<int>(r.x + 10),
            static_cast<int>(r.y + 10),
            13,
            active ? RAYWHITE : kDimText);

        const auto& slotEnemies =
            enemySlots[
                static_cast<size_t>(slot)];

        if (slotEnemies.empty())
        {
            DrawText(
                "Empty",
                static_cast<int>(r.x + 10),
                static_cast<int>(r.y + 42),
                13,
                kDimText);

            continue;
        }

        const int maxVisible = 3;

        for (int i = 0;
             i < static_cast<int>(slotEnemies.size()) &&
             i < maxVisible;
             ++i)
        {
            const std::string& id =
                slotEnemies[
                    static_cast<size_t>(i)];

            const EnemyInfo* enemy =
                enemies.get(id);

            std::string name =
                enemy
                    ? enemy->name
                    : id;

            name = fitText(
                name,
                10,
                126);

            DrawText(
                name.c_str(),
                static_cast<int>(r.x + 10),
                static_cast<int>(r.y + 39 + i * 20),
                10,
                RAYWHITE);
        }

        if (slotEnemies.size() > 3)
        {
            DrawText(
                TextFormat(
                    "+ %d more",
                    static_cast<int>(
                        slotEnemies.size() - 3)),
                static_cast<int>(r.x + 10),
                static_cast<int>(r.y + 101),
                9,
                kDimText);
        }
    }

    // ------------------------------------------------------------
    // Active slot enemy list - ONE scrollable list, top right.
    // This shows only the currently selected battlefield slot.
    // ------------------------------------------------------------

    {
        const Rectangle panel = slotSummaryBounds(0);

        DrawRectangleRounded(
            panel,
            0.06f,
            8,
            kPanelBg);

        DrawRectangleRoundedLines(
            panel,
            0.06f,
            8,
            kAccentBorder);

        DrawText(
            TextFormat(
                "ACTIVE SLOT %d",
                activeSlot + 1),
            1150,
            116,
            14,
            RAYWHITE);

        const auto& slot =
            enemySlots[
                static_cast<size_t>(activeSlot)];

        DrawText(
            TextFormat(
                "%d ENEMY ID(S)",
                static_cast<int>(slot.size())),
            1150,
            138,
            10,
            kDimText);

        if (slot.empty())
        {
            DrawText(
                "Empty",
                1150,
                170,
                12,
                kDimText);
        }
        else
        {
            constexpr int visibleRows = 6;

            for (int i = 0; i < visibleRows; ++i)
            {
                const int index =
                    activeSlotScroll + i;

                if (index >= static_cast<int>(slot.size()))
                    break;

                const std::string& id =
                    slot[static_cast<size_t>(index)];

                const EnemyInfo* enemy =
                    enemies.get(id);

                std::string name =
                    enemy
                        ? enemy->name
                        : id;

                name = fitText(
                    name,
                    11,
                    220);

                DrawText(
                    TextFormat(
                        "%d.",
                        index + 1),
                    1150,
                    164 + i * 18,
                    10,
                    kDimText);

                DrawText(
                    name.c_str(),
                    1170,
                    163 + i * 18,
                    11,
                    RAYWHITE);
            }

            if (slot.size() > static_cast<size_t>(visibleRows))
            {
                DrawText(
                    TextFormat(
                        "Mouse wheel to scroll  %d-%d / %d",
                        activeSlotScroll + 1,
                        std::min(
                            activeSlotScroll + visibleRows,
                            static_cast<int>(slot.size())),
                        static_cast<int>(slot.size())),
                    1150,
                    270,
                    9,
                    kDimText);
            }
        }
    }

    // ------------------------------------------------------------
    // Search / filter / controls
    // ------------------------------------------------------------

    const Rectangle search =
        searchBounds();

    DrawRectangleRounded(
        search,
        0.14f,
        8,
        kPanelBg);

    DrawRectangleRoundedLines(
        search,
        0.14f,
        8,
        kPanelBorder);

    const std::string searchLabel =
        searchText.empty()
            ? "Search enemy name or ID..."
            : searchText;

    DrawText(
        searchLabel.c_str(),
        324,
        362,
        15,
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
            r,
            0.14f,
            8,
            active
                ? kAccentBg
                : kPanelBg);

        DrawRectangleRoundedLines(
            r,
            0.14f,
            8,
            active
                ? kAccentBorder
                : kPanelBorder);

        DrawText(
            filterLabels[i],
            static_cast<int>(r.x + 12),
            static_cast<int>(r.y + 12),
            13,
            active
                ? RAYWHITE
                : kDimText);
    }

    // Add button.
    {
        const Rectangle r = addBounds();

        DrawRectangleRounded(
            r,
            0.14f,
            8,
            kAccentBg);

        DrawRectangleRoundedLines(
            r,
            0.14f,
            8,
            kAccentBorder);

        DrawText(
            "ADD SELECTED TO SLOT",
            static_cast<int>(r.x + 14),
            static_cast<int>(r.y + 12),
            13,
            RAYWHITE);
    }

    // Remove button.
    {
        const Rectangle r = removeBounds();

        DrawRectangleRounded(
            r,
            0.14f,
            8,
            kPanelBg);

        DrawRectangleRoundedLines(
            r,
            0.14f,
            8,
            kPanelBorder);

        DrawText(
            "REMOVE LAST",
            static_cast<int>(r.x + 13),
            static_cast<int>(r.y + 10),
            12,
            RAYWHITE);
    }

    // Clear button.
    {
        const Rectangle r = clearBounds();

        DrawRectangleRounded(
            r,
            0.14f,
            8,
            kDangerBg);

        DrawRectangleRoundedLines(
            r,
            0.14f,
            8,
            kDangerBorder);

        DrawText(
            "CLEAR SLOT",
            static_cast<int>(r.x + 13),
            static_cast<int>(r.y + 10),
            12,
            RAYWHITE);
    }

    DrawText(
        TextFormat(
            "%d matches",
            static_cast<int>(filtered.size())),
        1000,
        274,
        13,
        kDimText);

    // ------------------------------------------------------------
    // Enemy list
    // ------------------------------------------------------------

    for (int row = 0; row < 9; ++row)
    {
        const int index =
            scrollOffset + row;

        if (index >= static_cast<int>(filtered.size()))
            break;

        const EnemyInfo& enemy =
            *filtered[
                static_cast<size_t>(index)];

        const Rectangle r =
            rowBounds(row);

        const bool selected =
            enemy.id == selectedEnemyId;

        const bool hovered =
            index == hoveredRow;

        DrawRectangleRounded(
            r,
            0.08f,
            8,
            selected
                ? kAccentBg
                : (hovered
                    ? Color{38, 42, 54, 255}
                    : kPanelBg));

        DrawRectangleRoundedLines(
            r,
            0.08f,
            8,
            selected
                ? kAccentBorder
                : kPanelBorder);

        if (Texture2D* icon =
                assets.enemy(
                    enemy.id,
                    enemy.name))
        {
            const Rectangle src{
                0,
                0,
                static_cast<float>(icon->width),
                static_cast<float>(icon->height)
            };

            const Rectangle dst{
                r.x + 7.0f,
                r.y + 5.0f,
                40.0f,
                40.0f
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
            static_cast<int>(r.x + 56.0f);

        const std::string displayName =
            fitText(
                enemy.name,
                15,
                520);

        DrawText(
            displayName.c_str(),
            textX,
            static_cast<int>(r.y + 7.0f),
            15,
            RAYWHITE);

        const std::string meta =
            TextFormat(
                "ID %s   Lv.%d   HP %s   SPD %.0f   Toughness %.0f",
                enemy.id.c_str(),
                enemy.level,
                compactNumber(enemy.hp).c_str(),
                enemy.spd,
                enemy.toughness);

        DrawText(
            meta.c_str(),
            textX,
            static_cast<int>(r.y + 29.0f),
            11,
            kDimText);

        if (!enemy.rating.empty())
        {
            DrawText(
                fitText(
                    enemy.rating,
                    11,
                    130).c_str(),
                static_cast<int>(r.x + 655.0f),
                static_cast<int>(r.y + 10.0f),
                11,
                Color{210, 180, 140, 255});
        }
    }

    // ------------------------------------------------------------
    // Detail panel
    // ------------------------------------------------------------

    const Rectangle detail{
        1130.0f,
        400.0f,
        280.0f,
        450.0f
    };

    DrawRectangleRounded(
        detail,
        0.06f,
        8,
        kPanelBg);

    DrawRectangleRoundedLines(
        detail,
        0.06f,
        8,
        kPanelBorder);

    const EnemyInfo* selected =
        enemies.get(selectedEnemyId);

    if (!selected)
    {
        DrawText(
            "SELECT AN ENEMY",
            1150,
            424,
            15,
            kDimText);

        DrawText(
            "Pick a target from the list.",
            1150,
            454,
            13,
            kDimText);

        return;
    }

    const std::string detailName =
        fitText(
            selected->name,
            17,
            238);

    DrawText(
        detailName.c_str(),
        1150,
        460,
        17,
        RAYWHITE);

    DrawText(
        TextFormat(
            "ID %s",
            selected->id.c_str()),
        1150,
        486,
        11,
        kDimText);

    if (Texture2D* portrait =
            assets.enemy(
                selected->id,
                selected->name))
    {
        const Rectangle src{
            0,
            0,
            static_cast<float>(portrait->width),
            static_cast<float>(portrait->height)
        };

        const Rectangle dst{
            1160.0f,
            470.0f,
            220.0f,
            115.0f
        };

        DrawTexturePro(
            *portrait,
            src,
            dst,
            Vector2{0, 0},
            0.0f,
            WHITE);
    }

    int y = 605;

    auto statRow =
        [&](const char* label, const char* value)
        {
            DrawText(
                label,
                1150,
                y,
                11,
                kDimText);

            DrawText(
                value,
                1280,
                y,
                13,
                RAYWHITE);

            y += 24;
        };

    statRow(
        "Level",
        TextFormat(
            "%d",
            selected->level));

    statRow(
        "HP",
        compactNumber(
            selected->hp).c_str());

    statRow(
        "ATK",
        compactNumber(
            selected->atk).c_str());

    statRow(
        "DEF",
        compactNumber(
            selected->def).c_str());

    statRow(
        "SPD",
        TextFormat(
            "%.0f",
            selected->spd));

    statRow(
        "Toughness",
        TextFormat(
            "%.0f",
            selected->toughness));

    DrawText(
        "WEAKNESSES",
        1150,
        y + 2,
        11,
        kDimText);

    drawWrappedText(
        weaknessText(*selected),
        1150,
        y + 23,
        11,
        RAYWHITE,
        238,
        17);

    DrawText(
        TextFormat(
            "Slot %d contains %d enemy ID(s)",
            activeSlot + 1,
            static_cast<int>(
                enemySlots[
                    static_cast<size_t>(activeSlot)].size())),
        1150,
        822,
        10,
        kDimText);

    DrawText(
        "Mouse wheel: scroll enemy list",
        310,
        868,
        11,
        kDimText);
}

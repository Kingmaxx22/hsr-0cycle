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
        168.0f + row * 66.0f,
        800.0f,
        56.0f
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
        std::max(0, static_cast<int>(filtered.size()) - 10);

    scrollOffset =
        std::clamp(scrollOffset, 0, maxScroll);
}

void EnemiesScreen::selectEnemy(const EnemyInfo& enemy)
{
    selectedEnemyId = enemy.id;
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

    const float wheel = GetMouseWheelMove();

    if (wheel != 0.0f)
    {
        const Rectangle listArea{
            310.0f, 166.0f, 800.0f, 690.0f
        };

        if (CheckCollisionPointRec(mouse, listArea))
        {
            const int maxScroll =
                std::max(0, static_cast<int>(filtered.size()) - 10);

            scrollOffset =
                std::clamp(
                    scrollOffset -
                        static_cast<int>(wheel),
                    0,
                    maxScroll);
        }
    }

    hoveredRow = -1;

    for (int row = 0; row < 10; ++row)
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
                selectEnemy(
                    *filtered[static_cast<size_t>(index)]);

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
        1000, 154, 13, kDimText);

    for (int row = 0; row < 10; ++row)
    {
        const int index = scrollOffset + row;

        if (index >= static_cast<int>(filtered.size()))
            break;

        const EnemyInfo& enemy =
            *filtered[static_cast<size_t>(index)];

        const Rectangle r = rowBounds(row);

        const bool selected =
            enemy.id == selectedEnemyId;

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
    }

    const Rectangle detail{
        1130.0f, 104.0f, 280.0f, 560.0f
    };

    DrawRectangleRounded(
        detail, 0.06f, 8, kPanelBg);
    DrawRectangleRoundedLines(
        detail, 0.06f, 8, kPanelBorder);

    const EnemyInfo* selected =
        enemies.get(selectedEnemyId);

    if (!selected)
    {
        DrawText(
            "SELECT AN ENEMY",
            1150, 128, 15, kDimText);

        DrawText(
            "Pick a target from the list.",
            1150, 158, 13, kDimText);

        DrawText(
            "The selected ID will be used by",
            1150, 204, 12, kDimText);

        DrawText(
            "the combat setup / simulator.",
            1150, 224, 12, kDimText);

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

    DrawText(
        "Mouse wheel: scroll list",
        310, 868, 12, kDimText);
}

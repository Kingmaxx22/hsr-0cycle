#include "CharactersScreen.h"
#include "../widgets/CharacterCard.h"
#include "raylib.h"

#include <algorithm>
#include <cctype>
#include <unordered_set>

namespace
{
    std::string toLowerCopy(const std::string& s)
    {
        std::string out = s;
        std::transform(out.begin(), out.end(), out.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return out;
    }
} // anonymous namespace

CharactersScreen::CharactersScreen(AssetManager& assets, CharacterDatabase& characters,
                                    RelicSetDatabase& relicSets, LightConeDatabase& lightCones)
    : m_assets(assets), m_characters(characters), m_relicSets(relicSets), m_lightCones(lightCones)
{
}

void CharactersScreen::initialize()
{
    updateFilteredRoster();
}

Rectangle CharactersScreen::searchBoxBounds() const
{
    // Sits in the library header row so it never overlaps the card grid.
    return Rectangle{700.0f, 122.0f, 260.0f, 32.0f};
}

Rectangle CharactersScreen::backButtonBounds() const
{
    return Rectangle{50.0f, 28.0f, 86.0f, 34.0f};
}

float CharactersScreen::manualContentTop() const
{
    return kLibraryOriginY + kViewportH + 20.0f;
}

Rectangle CharactersScreen::manualFieldBounds(int index) const
{
    float top = manualContentTop() + 90.0f + static_cast<float>(index) * 40.0f;
    return Rectangle{150.0f, top, 200.0f, 30.0f};
}

int CharactersScreen::parseStatText(const std::string& text)
{
    if (text.empty())
        return 0;
    int value = 0;
    for (char c : text)
    {
        if (c < '0' || c > '9')
            return 0;
        value = value * 10 + (c - '0');
        if (value > 99999)
            return 99999;
    }
    return value;
}

void CharactersScreen::commitManualTexts()
{
    m_manualConfig.hp = parseStatText(m_manualHpText);
    m_manualConfig.atk = parseStatText(m_manualAtkText);
    m_manualConfig.def = parseStatText(m_manualDefText);
    m_manualConfig.spd = parseStatText(m_manualSpdText);
}

void CharactersScreen::syncManualCharacterId()
{
    // Grid selection pre-fills the manual form's character ID.
    if (!m_selectedCharacterId.empty())
        m_manualConfig.characterId = m_selectedCharacterId;
}

Rectangle CharactersScreen::searchClearButtonBounds() const
{
    Rectangle search = searchBoxBounds();
    return Rectangle{search.x + search.width - 28.0f, search.y + 5.0f, 22.0f, 24.0f};
}

void CharactersScreen::updateFilteredRoster()
{
    m_filteredRoster.clear();
    std::string lowerQuery = toLowerCopy(m_searchQuery);

    for (const auto& c : m_characters.all())
    {
        if (!lowerQuery.empty())
        {
            std::string lowerName = toLowerCopy(c.name);
            if (lowerName.find(lowerQuery) == std::string::npos)
                continue;
        }
        m_filteredRoster.push_back(&c);
    }
}

void CharactersScreen::update(float dt)
{
    (void)dt;
    updateFilteredRoster();

    // Update tab hover state
    Vector2 mouse = GetMousePosition();
    const bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    if (clicked && CheckCollisionPointRec(mouse, backButtonBounds()))
    {
        m_backRequested = true;
        return;
    }

    // Build tab
    m_buildTabHover = CheckCollisionPointRec(mouse, m_buildTabBounds);
    if (m_buildTabHover && clicked)
        m_manualStatsMode = false;

    // Manual tab
    m_manualTabHover = CheckCollisionPointRec(mouse, m_manualTabBounds);
    if (m_manualTabHover && clicked)
        m_manualStatsMode = true;

    // Manual stat field focus (click to focus, click elsewhere to commit)
    if (m_manualStatsMode && clicked)
    {
        int hitField = -1;
        for (int i = 0; i < 4; ++i)
        {
            if (CheckCollisionPointRec(mouse, manualFieldBounds(i)))
            {
                hitField = i;
                break;
            }
        }
        if (hitField >= 0)
        {
            commitManualTexts();
            m_focusedManualField = hitField;
        }
        else if (m_focusedManualField >= 0)
        {
            // Clicking outside a field commits its content.
            commitManualTexts();
            m_focusedManualField = -1;
        }
    }

    std::string* focusedText = nullptr;
    if (m_manualStatsMode && m_focusedManualField >= 0)
    {
        switch (m_focusedManualField)
        {
            case 0: focusedText = &m_manualHpText; break;
            case 1: focusedText = &m_manualAtkText; break;
            case 2: focusedText = &m_manualDefText; break;
            case 3: focusedText = &m_manualSpdText; break;
            default: break;
        }
    }

    // Route typed characters to exactly one destination per frame:
    // the focused manual field wins, otherwise the library search box.
    int key = GetCharPressed();
    while (key > 0)
    {
        if (focusedText != nullptr)
        {
            // Manual stats are whole numbers: digits only, max 5 chars.
            if (key >= '0' && key <= '9' && focusedText->size() < 5)
                focusedText->push_back(static_cast<char>(key));
        }
        else if (key >= 32 && key <= 126 && m_searchQuery.size() < 32)
        {
            m_searchQuery += static_cast<char>(key);
        }
        key = GetCharPressed();
    }

    if (focusedText != nullptr)
    {
        if (IsKeyPressed(KEY_BACKSPACE) && !focusedText->empty())
            focusedText->pop_back();
        if (IsKeyPressed(KEY_TAB))
        {
            commitManualTexts();
            m_focusedManualField = (m_focusedManualField + 1) % 4;
        }
    }
    else if (IsKeyPressed(KEY_BACKSPACE) && !m_searchQuery.empty())
    {
        m_searchQuery.pop_back();
    }

    // ENTER commits manual input (and advances) or confirms grid selection.
    // ESC defocuses a field first; with nothing focused it requests back.
    if (IsKeyPressed(KEY_ENTER))
    {
        if (m_focusedManualField >= 0)
        {
            commitManualTexts();
            m_focusedManualField = (m_focusedManualField + 1) % 4;
            if (m_focusedManualField == 0)
                m_focusedManualField = -1; // full pass done: defocus
        }
        else if (!m_selectedCharacterId.empty())
        {
            syncManualCharacterId();
            commitManualTexts();
            m_pendingSelection = true;
        }
    }
    if (IsKeyPressed(KEY_ESCAPE))
    {
        if (m_focusedManualField >= 0)
        {
            commitManualTexts();
            m_focusedManualField = -1;
        }
        else
        {
            m_backRequested = true;
        }
    }

    // Character grid interaction
    int rows = static_cast<int>((m_filteredRoster.size() + kLibraryCols - 1) / kLibraryCols);
    float contentH = rows * (kLibraryCardH + kLibraryGapY);
    float maxScroll = std::max(0.0f, contentH - kViewportH);
    m_scroll = std::clamp(m_scroll, 0.0f, maxScroll);

    Rectangle viewport{
        kLibraryOriginX - 10.0f, kLibraryOriginY - 10.0f,
        kLibraryCols * (kLibraryCardW + kLibraryGapX) + 10.0f,
        kViewportH + 20.0f
    };

    if (CheckCollisionPointRec(mouse, viewport))
    {
        m_scroll -= GetMouseWheelMove() * 40.0f;
        m_scroll = std::clamp(m_scroll, 0.0f, maxScroll);
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        if (!m_searchQuery.empty() && CheckCollisionPointRec(mouse, searchClearButtonBounds()))
            m_searchQuery.clear();

        if (CheckCollisionPointRec(mouse, viewport))
        {
            for (size_t i = 0; i < m_filteredRoster.size(); ++i)
            {
                int col = static_cast<int>(i) % kLibraryCols;
                int row = static_cast<int>(i) / kLibraryCols;

                Rectangle r{
                    kLibraryOriginX + col * (kLibraryCardW + kLibraryGapX),
                    kLibraryOriginY + row * (kLibraryCardH + kLibraryGapY) - m_scroll,
                    kLibraryCardW, kLibraryCardH
                };

                if (r.y + r.height < kLibraryOriginY || r.y > kLibraryOriginY + kViewportH)
                    continue;

                if (CheckCollisionPointRec(mouse, r))
                {
                    m_selectedCharacterId = m_filteredRoster[i]->id;
                    syncManualCharacterId();
                    m_pendingSelection = true;
                }
            }
        }
    }
}

void CharactersScreen::draw()
{
    DrawText("Characters — Team Builder", 300, 30, 34, RAYWHITE);
    DrawText("Configure characters for combat simulation", 312, 72, 16,
             Color{145, 150, 164, 255});
    DrawLine(310, 105, GetScreenWidth() - 30, 105, Color{48, 52, 64, 255});

    // --- Back button ---
    DrawRectangleRounded(backButtonBounds(), 0.2f, 8, Color{28, 31, 41, 255});
    DrawRectangleRoundedLines(backButtonBounds(), 0.2f, 8, Color{55, 59, 72, 255});
    DrawText("< Back", static_cast<int>(backButtonBounds().x + 12),
             static_cast<int>(backButtonBounds().y + 9), 15, RAYWHITE);

    // --- Mode Tabs ---
    float tabY = kTabY;
    m_buildTabBounds = { kTabOriginX, tabY, kTabW, kTabH };
    m_manualTabBounds = { kTabOriginX + kTabW + kTabGap, tabY, kTabW, kTabH };

    // Build from Components tab
    Color buildColor = m_manualStatsMode ? Color{55, 59, 72, 255} : Color{44, 52, 70, 255};
    Color buildBorder = m_manualStatsMode ? Color{75, 80, 100, 255} : Color{115, 140, 190, 255};
    DrawRectangleRounded(m_buildTabBounds, 0.15f, 8, buildColor);
    DrawRectangleRoundedLines(m_buildTabBounds, 0.15f, 8, buildBorder);
    DrawText("Build from Components", static_cast<int>(m_buildTabBounds.x + 10),
             static_cast<int>(m_buildTabBounds.y + 10), 16,
             m_manualStatsMode ? Color{130, 135, 148, 255} : RAYWHITE);

    // Enter Completed Character tab
    Color manualColor = !m_manualStatsMode ? Color{55, 59, 72, 255} : Color{44, 52, 70, 255};
    Color manualBorder = !m_manualStatsMode ? Color{75, 80, 100, 255} : Color{115, 140, 190, 255};
    DrawRectangleRounded(m_manualTabBounds, 0.15f, 8, manualColor);
    DrawRectangleRoundedLines(m_manualTabBounds, 0.15f, 8, manualBorder);
    DrawText("Enter Completed Character", static_cast<int>(m_manualTabBounds.x + 10),
             static_cast<int>(m_manualTabBounds.y + 10), 16,
             !m_manualStatsMode ? Color{130, 135, 148, 255} : RAYWHITE);

    // --- Character Grid ---
    DrawText("CHARACTER LIBRARY", 50, 130, 17, Color{180, 185, 198, 255});

    // Search box (same bounds as update() hit-testing)
    {
        Rectangle search = searchBoxBounds();
        DrawRectangleRounded(search, 0.14f, 8, Color{28, 31, 41, 255});
        DrawRectangleRoundedLines(search, 0.14f, 8, Color{55, 59, 72, 255});
        std::string label = m_searchQuery.empty() ? "Search name..." : m_searchQuery;
        DrawText(label.c_str(), static_cast<int>(search.x + 12),
                 static_cast<int>(search.y + 8), 15,
                 m_searchQuery.empty() ? Color{140, 145, 158, 255} : RAYWHITE);
        if (!m_searchQuery.empty())
        {
            Rectangle clear = searchClearButtonBounds();
            DrawText("x", static_cast<int>(clear.x + 6),
                     static_cast<int>(clear.y + 2), 16, GRAY);
        }
    }

    Rectangle clip{
        kLibraryOriginX - 10.0f, kLibraryOriginY - 10.0f,
        kLibraryCols * (kLibraryCardW + kLibraryGapX) + 10.0f,
        kViewportH + 20.0f
    };

    BeginScissorMode(static_cast<int>(clip.x), static_cast<int>(clip.y),
                      static_cast<int>(clip.width), static_cast<int>(clip.height));

    for (size_t i = 0; i < m_filteredRoster.size(); ++i)
    {
        int col = static_cast<int>(i) % kLibraryCols;
        int row = static_cast<int>(i) / kLibraryCols;

        Rectangle r{
            kLibraryOriginX + col * (kLibraryCardW + kLibraryGapX),
            kLibraryOriginY + row * (kLibraryCardH + kLibraryGapY) - m_scroll,
            kLibraryCardW, kLibraryCardH
        };

        if (r.y + r.height < kLibraryOriginY || r.y > kLibraryOriginY + kViewportH)
            continue;

        // Draw character card
        bool selected = (m_selectedCharacterId == m_filteredRoster[i]->id);
        CharacterCard::draw(m_assets, m_filteredRoster[i]->id, r, selected);

        // Show workflow indicator based on current mode
        if (!m_manualStatsMode) {
            // Build from Components: show small note
            std::string note = "Build from components";
            int noteW = MeasureText(note.c_str(), 10);
            DrawText(note.c_str(),
                     static_cast<int>(r.x + kLibraryCardW/2 - noteW/2),
                     static_cast<int>(r.y + kLibraryCardH - 25), 10, GRAY);
        } else {
            // Enter Completed: show indicator
            DrawText("Entered", static_cast<int>(r.x + 5), static_cast<int>(r.y + 5), 10, YELLOW);
        }
    }
    EndScissorMode();

    // --- Mode-specific content ---
    float modeContentY = manualContentTop();

    if (!m_manualStatsMode) {
        // --- BUILD FROM COMPONENTS WORKFLOW ---
        DrawText("BUILD FROM COMPONENTS", 50, static_cast<int>(modeContentY), 18, WHITE);
        DrawText("Configure character stats using base stats, Light Cone, and relics.", 52, static_cast<int>(modeContentY + 25), 14, LIGHTGRAY);

        // Display current character's computed stats if a character is selected
        if (!m_selectedCharacterId.empty()) {
            // Find the character info
            const CharacterInfo* ci = nullptr;
            for (const auto& c : m_characters.all()) {
                if (c.id == m_selectedCharacterId) {
                    ci = &c;
                    break;
                }
            }

            if (ci) {
                int yPos = static_cast<int>(modeContentY + 50);
                std::string idLabel = "ID: " + ci->id;
                DrawText(idLabel.c_str(), 50, yPos, 14, LIGHTGRAY);

                yPos += 25;
                std::string hpLabel = "Base HP: " + std::to_string(static_cast<int>(ci->baseStats.at("hp")));
                DrawText(hpLabel.c_str(), 50, yPos, 14, LIGHTGRAY);

                yPos += 25;
                std::string atkLabel = "Base ATK: " + std::to_string(static_cast<int>(ci->baseStats.at("atk")));
                DrawText(atkLabel.c_str(), 50, yPos, 14, LIGHTGRAY);

                yPos += 25;
                std::string spdLabel = "Base SPD: " + std::to_string(static_cast<int>(ci->baseStats.at("spd")));
                DrawText(spdLabel.c_str(), 50, yPos, 14, LIGHTGRAY);

                yPos += 40;
                DrawText("Light Cone:", 50, yPos, 14, WHITE);
                yPos += 25;

                // Light cone selection - simplified: show available LCs for this path
                auto lcs = m_lightCones.byPath(ci->path);
                if (!lcs.empty()) {
                    DrawText(lcs[0]->name.c_str(), 70, yPos, 14, LIGHTGRAY);
                } else {
                    DrawText("(no Light Cones loaded)", 70, yPos, 14, GRAY);
                }

                yPos += 40;
                DrawText("Relic Sets:", 50, yPos, 14, WHITE);
                yPos += 25;

                auto sets = m_relicSets.all();
                if (!sets.empty()) {
                    DrawText(sets[0].name.c_str(), 70, yPos, 14, LIGHTGRAY);
                } else {
                    DrawText("(no relic sets loaded)", 70, yPos, 14, GRAY);
                }

                yPos += 40;
                DrawText("Configure substats (ATK%, DEF%, HP%, SPD%, CRIT%) in the simulation screen.", 50, yPos, 14, LIGHTGRAY);
            }
        }

        // Instruction
        if (m_selectedCharacterId.empty()) {
            DrawText("Select a character from the grid to configure their build.", 50, static_cast<int>(modeContentY + 200), 16, GRAY);
        }
    }
    else {
        // --- ENTER COMPLETED CHARACTER WORKFLOW ---
        DrawText("ENTER COMPLETED CHARACTER", 50, static_cast<int>(modeContentY), 18, WHITE);
        DrawText("Click a field and type digits. ENTER commits and advances, TAB cycles.", 52, static_cast<int>(modeContentY + 25), 14, LIGHTGRAY);

        std::string charIdLabel = "Character: " +
            (m_manualConfig.characterId.empty() ? "(select from grid above)" : m_manualConfig.characterId);
        DrawText(charIdLabel.c_str(), 50, static_cast<int>(modeContentY + 48), 14, LIGHTGRAY);

        const char* fieldNames[4] = {"HP", "ATK", "DEF", "SPD"};
        const std::string* fieldTexts[4] = {&m_manualHpText, &m_manualAtkText, &m_manualDefText, &m_manualSpdText};

        for (int i = 0; i < 4; ++i)
        {
            Rectangle field = manualFieldBounds(i);
            const bool focused = (m_focusedManualField == i);
            DrawText(fieldNames[i], 50, static_cast<int>(field.y + 7), 14, WHITE);
            DrawRectangleRounded(field, 0.2f, 8,
                focused ? Color{44, 52, 70, 255} : Color{28, 31, 41, 255});
            DrawRectangleRoundedLines(field, 0.2f, 8,
                focused ? Color{115, 140, 190, 255} : Color{55, 59, 72, 255});
            std::string shown = *fieldTexts[i];
            if (focused)
                shown += "_";
            if (shown.empty() && !focused)
                shown = "-";
            DrawText(shown.c_str(), static_cast<int>(field.x + 10),
                     static_cast<int>(field.y + 7), 15,
                     fieldTexts[i]->empty() && !focused ? GRAY : RAYWHITE);
        }

        int summaryY = static_cast<int>(manualFieldBounds(3).y + 40.0f);
        std::string summary = "Entered Stats (manual): HP " + std::to_string(m_manualConfig.hp) +
            " / ATK " + std::to_string(m_manualConfig.atk) +
            " / DEF " + std::to_string(m_manualConfig.def) +
            " / SPD " + std::to_string(m_manualConfig.spd);
        DrawText(summary.c_str(), 50, summaryY, 14, YELLOW);
    }

    // --- Selection prompt (input itself is handled in update()) ---
    DrawText("Click a portrait or press ENTER to select, ESC to return", 50, GetScreenHeight() - 50, 16, GRAY);
}

bool CharactersScreen::consumeSelection(std::string& outCharacterId)
{
    if (!m_pendingSelection)
        return false;
    m_pendingSelection = false;
    if (m_selectedCharacterId.empty())
        return false;
    outCharacterId = m_selectedCharacterId;
    return true;
}

bool CharactersScreen::consumeBackRequest()
{
    if (!m_backRequested)
        return false;
    m_backRequested = false;
    return true;
}
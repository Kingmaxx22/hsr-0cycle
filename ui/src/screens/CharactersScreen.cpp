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
                                     RelicSetDatabase& relicSets, LightConeDatabase& lightCones,
                                     LoadoutStore& loadouts)
    : m_assets(assets), m_characters(characters), m_relicSets(relicSets),
      m_lightCones(lightCones), m_loadouts(loadouts)
{
}

void CharactersScreen::setTeamContext(const std::array<std::string, 4>& team)
{
    m_team = team;
}

void CharactersScreen::initialize()
{
    updateFilteredRoster();
}

Rectangle CharactersScreen::searchBoxBounds() const
{
    // Sits in the library header row so it never overlaps the card grid.
    return Rectangle{700.0f, 112.0f, 260.0f, 32.0f};
}

Rectangle CharactersScreen::backButtonBounds() const
{
    // Matches EnemiesScreen: right of the 270px sidebar.
    return Rectangle{286.0f, 28.0f, 86.0f, 34.0f};
}

float CharactersScreen::manualContentTop() const
{
    return kLibraryOriginY + kViewportH + 20.0f;
}

Rectangle CharactersScreen::manualFieldBounds(int index) const
{
    // Two columns x five rows: left HP/ATK/DEF/SPD/CRIT, right the rest.
    int col = index / 5;
    int row = index % 5;
    float top = manualContentTop() + 80.0f + static_cast<float>(row) * 34.0f;
    float x = (col == 0) ? 410.0f : 800.0f;
    return Rectangle{x, top, 200.0f, 28.0f};
}

Rectangle CharactersScreen::extraFieldBounds(int index) const
{
    // Other bonuses: 5 cols x 2 rows at right; base override: 4-in-a-row.
    if (index < 10)
    {
        int col = index % 5;
        int row = index / 5;
        return Rectangle{720.0f + col * 132.0f, 554.0f + row * 44.0f, 120.0f, 28.0f};
    }
    int col = index - 10;
    return Rectangle{720.0f + col * 160.0f, 720.0f, 140.0f, 28.0f};
}

Rectangle CharactersScreen::baseToggleBounds() const
{
    return Rectangle{720.0f, 672.0f, 240.0f, 28.0f};
}

Rectangle CharactersScreen::levelMinusBounds() const
{
    return Rectangle{1020.0f, 672.0f, 30.0f, 28.0f};
}

Rectangle CharactersScreen::levelPlusBounds() const
{
    return Rectangle{1100.0f, 672.0f, 30.0f, 28.0f};
}

Rectangle CharactersScreen::toggleRowBounds(int index) const
{
    return Rectangle{310.0f, 780.0f + index * 24.0f, 390.0f, 22.0f};
}

std::vector<CharactersScreen::CondToggle> CharactersScreen::conditionalToggles(
    const CharacterLoadout& lo) const
{
    // Equipped conditional effects in stable order: A-2pc, A-4pc (when
    // 4pc mode), B-2pc (when 2+2 mode), planar-2pc.
    std::vector<CondToggle> out;
    const CharacterInfo* ci = selectedInfo();
    const std::string attackerElement = (ci != nullptr) ? ci->element : "";
    auto collect = [&](const std::string& setId, bool allowFourPiece, bool isPlanar) {
        if (setId.empty())
            return;
        const RelicSetInfo* set = m_relicSets.get(setId);
        if (set == nullptr)
            return;
        auto collectList = [&](const std::vector<SetEffect>& list, const std::string& piece) {
            for (size_t i = 0; i < list.size(); ++i)
            {
                if (!list[i].hasCondition)
                    continue;
                std::string valueText = formatStatValueText(
                    list[i].value * 100.0, "atk_pct") + "%";
                std::string label = set->name + " " + piece + "pc: +" +
                    valueText + " " + list[i].stat;
                bool autoOn = loadout::isSetEffectAutoActive(
                    list[i].condition, attackerElement);
                out.push_back({loadout::setEffectId(setId, piece, i), label, autoOn});
            }
        };
        collectList(set->twoPiece, "2");
        if (allowFourPiece && !isPlanar)
            collectList(set->fourPiece, "4");
    };
    collect(lo.relicSetA, lo.relicFourPiece, false);
    if (!lo.relicFourPiece)
        collect(lo.relicSetB, false, false);
    collect(lo.planarSet, false, true);
    return out;
}

bool CharactersScreen::extraFieldIsPercent(int index) const
{
    // Index 3 is flat SPD; 10-13 are flat base stats; rest percent-numbers.
    if (index == 3 || index >= 10)
        return false;
    return true;
}

const CharacterInfo* CharactersScreen::selectedInfo() const
{
    if (m_selectedCharacterId.empty())
        return nullptr;
    return m_characters.get(m_selectedCharacterId);
}

CharacterLoadout& CharactersScreen::loadoutFor(const std::string& characterId)
{
    auto it = m_loadouts.find(characterId);
    if (it == m_loadouts.end())
    {
        m_loadouts[characterId] = CharacterLoadout{};
        it = m_loadouts.find(characterId);
    }
    ensureLoadoutDefaults(it->second);
    return it->second;
}

std::string& CharactersScreen::extraFieldText(int index)
{
    return m_extraTexts[static_cast<size_t>(index)];
}

void CharactersScreen::syncExtraTexts()
{
    // Refresh every unfocused buffer from the loadout so edits made in the
    // relic/LC screens show up here without clobbering active typing.
    const CharacterInfo* info = selectedInfo();
    if (info == nullptr)
        return;
    const CharacterLoadout& lo = loadoutFor(info->id);
    const double pct[10] = {
        lo.otherBonuses.atkPct, lo.otherBonuses.hpPct, lo.otherBonuses.defPct,
        lo.otherBonuses.flatSpd, lo.otherBonuses.critRate, lo.otherBonuses.critDmg,
        lo.otherBonuses.elemDmgPct, lo.otherBonuses.resPen, lo.otherBonuses.ehr,
        lo.otherBonuses.breakDmgIncrease
    };
    const char* pctKeys[10] = {
        "atk_pct", "hp_pct", "def_pct", "spd", "crit_rate_pct",
        "crit_dmg_pct", "atk_pct", "atk_pct", "effect_hit_rate_pct",
        "break_effect_pct"
    };
    for (int i = 0; i < 10; ++i)
    {
        if (i == m_focusedExtraField)
            continue;
        double shown = extraFieldIsPercent(i) ? pct[i] * 100.0 : pct[i];
        m_extraTexts[static_cast<size_t>(i)] = formatStatValueText(shown, pctKeys[i]);
    }
    const double base[4] = {
        lo.manualBase.hp, lo.manualBase.atk, lo.manualBase.def, lo.manualBase.spd
    };
    for (int i = 0; i < 4; ++i)
    {
        int idx = 10 + i;
        if (idx == m_focusedExtraField)
            continue;
        m_extraTexts[static_cast<size_t>(idx)] = formatStatValueText(base[i], "hp");
    }
}

void CharactersScreen::commitExtraField(int index)
{
    const CharacterInfo* info = selectedInfo();
    if (info == nullptr)
        return;
    CharacterLoadout& lo = loadoutFor(info->id);
    std::string& text = m_extraTexts[static_cast<size_t>(index)];
    double number = 0.0;
    try { number = text.empty() ? 0.0 : std::stod(text); }
    catch (...) { number = 0.0; }
    double value = extraFieldIsPercent(index) ? number / 100.0 : number;
    switch (index)
    {
        case 0: lo.otherBonuses.atkPct = value; break;
        case 1: lo.otherBonuses.hpPct = value; break;
        case 2: lo.otherBonuses.defPct = value; break;
        case 3: lo.otherBonuses.flatSpd = value; break;
        case 4: lo.otherBonuses.critRate = value; break;
        case 5: lo.otherBonuses.critDmg = value; break;
        case 6: lo.otherBonuses.elemDmgPct = value; break;
        case 7: lo.otherBonuses.resPen = value; break;
        case 8: lo.otherBonuses.ehr = value; break;
        case 9: lo.otherBonuses.breakDmgIncrease = value; break;
        case 10: lo.manualBase.hp = value; break;
        case 11: lo.manualBase.atk = value; break;
        case 12: lo.manualBase.def = value; break;
        case 13: lo.manualBase.spd = value; break;
        default: break;
    }
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

double CharactersScreen::parsePercentText(const std::string& text)
{
    if (text.empty())
        return 0.0;
    try { return std::stod(text) / 100.0; }
    catch (...) { return 0.0; }
}

void CharactersScreen::commitManualTexts()
{
    m_manualConfig.hp = parseStatText(m_manualTexts[0]);
    m_manualConfig.atk = parseStatText(m_manualTexts[1]);
    m_manualConfig.def = parseStatText(m_manualTexts[2]);
    m_manualConfig.spd = parseStatText(m_manualTexts[3]);
    m_manualConfig.critRate = parsePercentText(m_manualTexts[4]);
    m_manualConfig.critDmg = parsePercentText(m_manualTexts[5]);
    m_manualConfig.elemDmg = parsePercentText(m_manualTexts[6]);
    m_manualConfig.resPen = parsePercentText(m_manualTexts[7]);
    m_manualConfig.ehr = parsePercentText(m_manualTexts[8]);
    m_manualConfig.effectRes = parsePercentText(m_manualTexts[9]);
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
        for (int i = 0; i < kManualFieldCount; ++i)
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

    // Build-tab component editors (Sec 22.2): other bonuses, base toggle,
    // base values, level stepper. Only for the selected character.
    if (!m_manualStatsMode && selectedInfo() != nullptr)
    {
        syncExtraTexts();
        if (clicked)
        {
            CharacterLoadout& lo = loadoutFor(m_selectedCharacterId);
            bool hitSomething = false;
            for (int i = 0; i < kExtraFieldCount; ++i)
            {
                // Base fields are inert unless the override is enabled.
                if (i >= 10 && !lo.manualBase.useOverride)
                    continue;
                if (CheckCollisionPointRec(mouse, extraFieldBounds(i)))
                {
                    if (m_focusedExtraField >= 0)
                        commitExtraField(m_focusedExtraField);
                    m_focusedExtraField = i;
                    hitSomething = true;
                    break;
                }
            }
            if (!hitSomething && m_focusedExtraField >= 0)
            {
                commitExtraField(m_focusedExtraField);
                m_focusedExtraField = -1;
            }
            if (!hitSomething)
            {
                if (CheckCollisionPointRec(mouse, baseToggleBounds()))
                {
                    lo.manualBase.useOverride = !lo.manualBase.useOverride;
                    if (lo.manualBase.useOverride)
                    {
                        // Start from database values so the player edits
                        // real numbers instead of a blank form.
                        const CharacterInfo* info = selectedInfo();
                        auto base = [&](const char* k) {
                            auto it = info->baseStats.find(k);
                            return it != info->baseStats.end() ? it->second : 0.0;
                        };
                        lo.manualBase.hp = base("hp");
                        lo.manualBase.atk = base("atk");
                        lo.manualBase.def = base("def");
                        lo.manualBase.spd = base("spd");
                    }
                }
                else if (CheckCollisionPointRec(mouse, levelMinusBounds()))
                {
                    lo.level = std::max(1, lo.level - 1);
                }
                else if (CheckCollisionPointRec(mouse, levelPlusBounds()))
                {
                    lo.level = std::min(90, lo.level + 1);
                }
                else
                {
                    // Q3 conditional set-effect opt-in toggles (max 3 drawn).
                    auto toggles = conditionalToggles(lo);
                    size_t shown = std::min(toggles.size(), static_cast<size_t>(3));
                    for (size_t ti = 0; ti < shown; ++ti)
                    {
                        if (CheckCollisionPointRec(mouse, toggleRowBounds(static_cast<int>(ti))))
                        {
                            // Auto-derived effects need no click; manual ones
                            // flip the opt-in (inserting false = stays OFF).
                            if (!toggles[ti].autoActive)
                            {
                                bool& active = lo.setEffectActive[toggles[ti].effectId];
                                active = !active;
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
    else if (m_focusedExtraField >= 0)
    {
        commitExtraField(m_focusedExtraField);
        m_focusedExtraField = -1;
    }

    std::string* focusedText = nullptr;
    bool focusedIsDecimal = false;
    if (m_manualStatsMode && m_focusedManualField >= 0)
    {
        focusedText = &m_manualTexts[static_cast<size_t>(m_focusedManualField)];
        focusedIsDecimal = (m_focusedManualField >= 4);
    }
    else if (!m_manualStatsMode && m_focusedExtraField >= 0)
    {
        focusedText = &extraFieldText(m_focusedExtraField);
        focusedIsDecimal = true;
    }

    // Route typed characters to exactly one destination per frame:
    // focused stat field wins, otherwise the library search box.
    int key = GetCharPressed();
    while (key > 0)
    {
        if (focusedText != nullptr)
        {
            bool isDigit = key >= '0' && key <= '9';
            bool isDot = focusedIsDecimal && key == '.' &&
                         focusedText->find('.') == std::string::npos;
            size_t cap = focusedIsDecimal ? 8 : 5;
            if ((isDigit || isDot) && focusedText->size() < cap)
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
            if (m_manualStatsMode)
            {
                commitManualTexts();
                m_focusedManualField = (m_focusedManualField + 1) % kManualFieldCount;
            }
            else
            {
                commitExtraField(m_focusedExtraField);
                m_focusedExtraField = (m_focusedExtraField + 1) % kExtraFieldCount;
            }
        }
    }
    else if (IsKeyPressed(KEY_BACKSPACE) && !m_searchQuery.empty())
    {
        m_searchQuery.pop_back();
    }

    // ENTER commits field input (and advances) or confirms grid selection.
    // ESC defocuses a field first; with nothing focused it requests back.
    if (IsKeyPressed(KEY_ENTER))
    {
        if (m_focusedManualField >= 0)
        {
            commitManualTexts();
            m_focusedManualField = (m_focusedManualField + 1) % kManualFieldCount;
            if (m_focusedManualField == 0)
                m_focusedManualField = -1; // full pass done: defocus
        }
        else if (m_focusedExtraField >= 0)
        {
            commitExtraField(m_focusedExtraField);
            m_focusedExtraField = (m_focusedExtraField + 1) % kExtraFieldCount;
            if (m_focusedExtraField == 0)
                m_focusedExtraField = -1;
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
        else if (m_focusedExtraField >= 0)
        {
            commitExtraField(m_focusedExtraField);
            m_focusedExtraField = -1;
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
    // NOTE: hyphen, not em-dash — the default font has no em-dash glyph.
    DrawText("Characters - Team Builder", 390, 30, 34, RAYWHITE);
    DrawText("Configure characters for combat simulation", 312, 72, 16,
             Color{145, 150, 164, 255});
    DrawLine(310, 105, GetScreenWidth() - 30, 105, Color{48, 52, 64, 255});

    // --- Back button ---
    DrawRectangleRounded(backButtonBounds(), 0.2f, 8, Color{28, 31, 41, 255});
    DrawRectangleRoundedLines(backButtonBounds(), 0.2f, 8, Color{55, 59, 72, 255});
    DrawText("< Back", static_cast<int>(backButtonBounds().x + 12),
             static_cast<int>(backButtonBounds().y + 9), 15, RAYWHITE);

    // --- Mode Tabs (right-aligned so the full labels fit on screen) ---
    float tabX = static_cast<float>(GetScreenWidth()) - 30.0f -
        (2.0f * kTabW + kTabGap);
    m_buildTabBounds = { tabX, kTabY, kTabW, kTabH };
    m_manualTabBounds = { tabX + kTabW + kTabGap, kTabY, kTabW, kTabH };

    // Build from Components tab
    Color buildColor = m_manualStatsMode ? Color{55, 59, 72, 255} : Color{44, 52, 70, 255};
    Color buildBorder = m_manualStatsMode ? Color{75, 80, 100, 255} : Color{115, 140, 190, 255};
    DrawRectangleRounded(m_buildTabBounds, 0.15f, 8, buildColor);
    DrawRectangleRoundedLines(m_buildTabBounds, 0.15f, 8, buildBorder);
    {
        const char* label = "Build from Components";
        int labelW = MeasureText(label, 14);
        DrawText(label, static_cast<int>(m_buildTabBounds.x + (kTabW - labelW) / 2.0f),
                 static_cast<int>(m_buildTabBounds.y + 12), 14,
                 m_manualStatsMode ? Color{130, 135, 148, 255} : RAYWHITE);
    }

    // Enter Completed Character tab
    Color manualColor = !m_manualStatsMode ? Color{55, 59, 72, 255} : Color{44, 52, 70, 255};
    Color manualBorder = !m_manualStatsMode ? Color{75, 80, 100, 255} : Color{115, 140, 190, 255};
    DrawRectangleRounded(m_manualTabBounds, 0.15f, 8, manualColor);
    DrawRectangleRoundedLines(m_manualTabBounds, 0.15f, 8, manualBorder);
    {
        const char* label = "Enter Completed Character";
        int labelW = MeasureText(label, 14);
        DrawText(label, static_cast<int>(m_manualTabBounds.x + (kTabW - labelW) / 2.0f),
                 static_cast<int>(m_manualTabBounds.y + 12), 14,
                 !m_manualStatsMode ? Color{130, 135, 148, 255} : RAYWHITE);
    }

    // --- Character Grid ---
    DrawText("CHARACTER LIBRARY", 310, 118, 17, Color{180, 185, 198, 255});

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
        // NOTE: no extra text is drawn over the card bottom — CharacterCard
        // already renders the character name at bounds.y + height - 25.
        bool selected = (m_selectedCharacterId == m_filteredRoster[i]->id);
        CharacterCard::draw(m_assets, m_filteredRoster[i]->id, r, selected);

        // In manual mode, tag the card corner (clear of the bottom name).
        if (m_manualStatsMode) {
            DrawText("Entered", static_cast<int>(r.x + 5), static_cast<int>(r.y + 5), 10, YELLOW);
        }
    }
    EndScissorMode();

    // --- Mode-specific content ---
    float modeContentY = manualContentTop();

    if (!m_manualStatsMode) {
        // --- BUILD FROM COMPONENTS WORKFLOW (Sec 22.2/22.3) ---
        DrawText("BUILD FROM COMPONENTS", 310, static_cast<int>(modeContentY), 18, WHITE);
        DrawText("Totals resolve automatically from base + Light Cone + gear + bonuses.", 312, static_cast<int>(modeContentY + 25), 14, LIGHTGRAY);

        const CharacterInfo* ci = selectedInfo();
        if (ci != nullptr) {
            CharacterLoadout& lo = loadoutFor(ci->id);
            loadout::ResolvedTotals totals = loadout::resolveTotals(*ci, lo, m_lightCones, m_relicSets);

            int yPos = static_cast<int>(modeContentY + 55);
            DrawText(("Final HP:  " + std::to_string(static_cast<int>(totals.hp))).c_str(), 310, yPos, 15, WHITE);
            yPos += 24;
            DrawText(("Final ATK: " + std::to_string(static_cast<int>(totals.atk))).c_str(), 310, yPos, 15, WHITE);
            yPos += 24;
            DrawText(("Final DEF: " + std::to_string(static_cast<int>(totals.def))).c_str(), 310, yPos, 15, WHITE);
            yPos += 24;
            DrawText(("Final SPD: " + std::to_string(static_cast<int>(totals.spd))).c_str(), 310, yPos, 15, WHITE);
            yPos += 30;

            std::string baseSrc = lo.manualBase.useOverride ? "Base: custom" : "Base: database";
            DrawText(baseSrc.c_str(), 310, yPos, 13, LIGHTGRAY);
            yPos += 22;
            const LightConeInfo* lc = lo.lightConeId.empty() ? nullptr : m_lightCones.get(lo.lightConeId);
            std::string lcLabel = std::string("Light Cone: ") + (lc != nullptr ? lc->name : "(none — see LIGHT CONES)");
            DrawText(lcLabel.c_str(), 310, yPos, 13, LIGHTGRAY);
            yPos += 22;
            std::string setLabel = "Relics: " + (lo.relicSetA.empty() ? "(none — see RELICS)" : lo.relicSetA);
            if (!lo.relicFourPiece && !lo.relicSetB.empty())
                setLabel += " + " + lo.relicSetB;
            if (!lo.planarSet.empty())
                setLabel += " / " + lo.planarSet;
            DrawText(setLabel.c_str(), 310, yPos, 13, LIGHTGRAY);
            yPos += 22;
            DrawText("Edit gear in RELICS, Light Cone in LIGHT CONES.", 310, yPos, 13, GRAY);

            // Q3 conditional set-effect opt-ins (manual-only, default OFF).
            auto toggles = conditionalToggles(lo);
            if (!toggles.empty())
            {
                DrawText("CONDITIONAL SET FX — opt-in, default OFF", 310, 760, 13, YELLOW);
                size_t shown = std::min(toggles.size(), static_cast<size_t>(3));
                for (size_t ti = 0; ti < shown; ++ti)
                {
                    Rectangle row = toggleRowBounds(static_cast<int>(ti));
                    auto it = lo.setEffectActive.find(toggles[ti].effectId);
                    bool manual = (it != lo.setEffectActive.end() && it->second);
                    bool on = manual || toggles[ti].autoActive;
                    DrawRectangle(static_cast<int>(row.x), static_cast<int>(row.y + 3),
                                  14, 14, on ? GREEN : DARKGRAY);
                    if (on)
                        DrawText("x", static_cast<int>(row.x + 3),
                                 static_cast<int>(row.y + 2), 14, BLACK);
                    std::string label = toggles[ti].label;
                    if (toggles[ti].autoActive)
                        label += " [AUTO]";
                    if (label.size() > 52)
                        label = label.substr(0, 49) + "...";
                    DrawText(label.c_str(), static_cast<int>(row.x + 22),
                             static_cast<int>(row.y + 3), 13, on ? RAYWHITE : LIGHTGRAY);
                }
                if (toggles.size() > shown)
                {
                    std::string more = "+" + std::to_string(toggles.size() - shown) + " more";
                    DrawText(more.c_str(), 310, 780 + static_cast<int>(shown) * 24, 12, GRAY);
                }
            }

            // --- Right column: other bonuses / base override / level ---
            DrawText("OTHER BONUSES", 720, static_cast<int>(modeContentY), 15, Color{180, 185, 198, 255});
            const char* extraNames[kExtraFieldCount] = {
                "ATK%", "HP%", "DEF%", "SPD", "CRIT%",
                "CRIT DMG%", "DMG%", "RES PEN%", "EHR%", "BRK DMG+%",
                "Base HP", "Base ATK", "Base DEF", "Base SPD"
            };
            for (int i = 0; i < kExtraFieldCount; ++i)
            {
                // Base fields live in their own row below; skip them here.
                if (i >= 10)
                    continue;
                Rectangle field = extraFieldBounds(i);
                const bool focused = (m_focusedExtraField == i);
                DrawText(extraNames[i], static_cast<int>(field.x),
                         static_cast<int>(field.y - 15), 12, LIGHTGRAY);
                DrawRectangleRounded(field, 0.2f, 8,
                    focused ? Color{44, 52, 70, 255} : Color{28, 31, 41, 255});
                DrawRectangleRoundedLines(field, 0.2f, 8,
                    focused ? Color{115, 140, 190, 255} : Color{55, 59, 72, 255});
                std::string shown = m_extraTexts[static_cast<size_t>(i)];
                if (focused)
                    shown += "_";
                if (shown.empty() && !focused)
                    shown = "-";
                DrawText(shown.c_str(), static_cast<int>(field.x + 10),
                         static_cast<int>(field.y + 6), 14,
                         m_extraTexts[static_cast<size_t>(i)].empty() && !focused ? GRAY : RAYWHITE);
            }

            Rectangle toggle = baseToggleBounds();
            DrawRectangleRounded(toggle, 0.2f, 8,
                lo.manualBase.useOverride ? Color{44, 52, 70, 255} : Color{28, 31, 41, 255});
            DrawRectangleRoundedLines(toggle, 0.2f, 8, Color{55, 59, 72, 255});
            std::string toggleLabel = std::string("Custom base: ") +
                (lo.manualBase.useOverride ? "ON" : "OFF");
            DrawText(toggleLabel.c_str(), static_cast<int>(toggle.x + 12),
                     static_cast<int>(toggle.y + 7), 13, RAYWHITE);

            DrawText("Lv", 990, 679, 14, WHITE);
            Rectangle minusR = levelMinusBounds();
            Rectangle plusR = levelPlusBounds();
            DrawRectangleRounded(minusR, 0.2f, 6, Color{28, 31, 41, 255});
            DrawRectangleRoundedLines(minusR, 0.2f, 6, Color{55, 59, 72, 255});
            DrawText("-", static_cast<int>(minusR.x + 11), static_cast<int>(minusR.y + 5), 15, RAYWHITE);
            DrawText(std::to_string(lo.level).c_str(), 1058, 679, 14, RAYWHITE);
            DrawRectangleRounded(plusR, 0.2f, 6, Color{28, 31, 41, 255});
            DrawRectangleRoundedLines(plusR, 0.2f, 6, Color{55, 59, 72, 255});
            DrawText("+", static_cast<int>(plusR.x + 10), static_cast<int>(plusR.y + 5), 15, RAYWHITE);

            for (int i = 10; i < kExtraFieldCount; ++i)
            {
                Rectangle field = extraFieldBounds(i);
                const bool focused = (m_focusedExtraField == i);
                const bool enabled = lo.manualBase.useOverride;
                DrawText(extraNames[i], static_cast<int>(field.x),
                         static_cast<int>(field.y - 15), 12,
                         enabled ? LIGHTGRAY : GRAY);
                DrawRectangleRounded(field, 0.2f, 8,
                    focused ? Color{44, 52, 70, 255} : Color{22, 24, 31, 255});
                DrawRectangleRoundedLines(field, 0.2f, 8,
                    focused ? Color{115, 140, 190, 255} : Color{55, 59, 72, 255});
                std::string shown = enabled ? m_extraTexts[static_cast<size_t>(i)] : "";
                if (focused)
                    shown += "_";
                if (shown.empty() && !focused)
                    shown = "-";
                DrawText(shown.c_str(), static_cast<int>(field.x + 10),
                         static_cast<int>(field.y + 6), 14,
                         (!enabled || m_extraTexts[static_cast<size_t>(i)].empty()) && !focused ? GRAY : RAYWHITE);
            }
            DrawText("% fields take percent-numbers (15 = 15%).",
                     720, 772, 12, GRAY);
        }

        // Instruction
        if (m_selectedCharacterId.empty()) {
            DrawText("Select a character from the grid to configure their build.", 310, static_cast<int>(modeContentY + 200), 16, GRAY);
        }
    }
    else {
        // --- ENTER COMPLETED CHARACTER WORKFLOW (Sec 22.4) ---
        DrawText("ENTER COMPLETED CHARACTER", 310, static_cast<int>(modeContentY), 18, WHITE);
        DrawText("Type final combat stats. Percent fields take percent-numbers (70 = 70%).", 312, static_cast<int>(modeContentY + 25), 14, LIGHTGRAY);

        std::string charIdLabel = "Character: " +
            (m_manualConfig.characterId.empty() ? "(select from grid above)" : m_manualConfig.characterId);
        DrawText(charIdLabel.c_str(), 310, static_cast<int>(modeContentY + 48), 14, LIGHTGRAY);

        const char* fieldNames[kManualFieldCount] = {
            "HP", "ATK", "DEF", "SPD", "CRIT%",
            "CRIT DMG%", "DMG%", "RES PEN%", "EHR%", "EFF RES%"
        };
        const float labelX[kManualFieldCount] = {
            330.0f, 330.0f, 330.0f, 330.0f, 330.0f,
            700.0f, 700.0f, 700.0f, 700.0f, 700.0f
        };

        for (int i = 0; i < kManualFieldCount; ++i)
        {
            Rectangle field = manualFieldBounds(i);
            const bool focused = (m_focusedManualField == i);
            DrawText(fieldNames[i], static_cast<int>(labelX[i]), static_cast<int>(field.y + 7), 14, WHITE);
            DrawRectangleRounded(field, 0.2f, 8,
                focused ? Color{44, 52, 70, 255} : Color{28, 31, 41, 255});
            DrawRectangleRoundedLines(field, 0.2f, 8,
                focused ? Color{115, 140, 190, 255} : Color{55, 59, 72, 255});
            std::string shown = m_manualTexts[static_cast<size_t>(i)];
            if (focused)
                shown += "_";
            if (shown.empty() && !focused)
                shown = "-";
            DrawText(shown.c_str(), static_cast<int>(field.x + 10),
                     static_cast<int>(field.y + 6), 14,
                     m_manualTexts[static_cast<size_t>(i)].empty() && !focused ? GRAY : RAYWHITE);
        }

        int summaryY = static_cast<int>(manualFieldBounds(9).y + 38.0f);
        std::string summary = "Manual: HP " + std::to_string(m_manualConfig.hp) +
            " ATK " + std::to_string(m_manualConfig.atk) +
            " DEF " + std::to_string(m_manualConfig.def) +
            " SPD " + std::to_string(m_manualConfig.spd);
        DrawText(summary.c_str(), 310, summaryY, 13, YELLOW);
        std::string summary2 = "CRIT " + std::to_string(static_cast<int>(m_manualConfig.critRate * 100.0)) +
            "%/" + std::to_string(static_cast<int>(m_manualConfig.critDmg * 100.0)) +
            "% DMG " + std::to_string(static_cast<int>(m_manualConfig.elemDmg * 100.0)) +
            "% PEN " + std::to_string(static_cast<int>(m_manualConfig.resPen * 100.0)) +
            "% EHR " + std::to_string(static_cast<int>(m_manualConfig.ehr * 100.0)) + "%";
        DrawText(summary2.c_str(), 310, summaryY + 18, 13, YELLOW);
    }

    // --- Selection prompt (input itself is handled in update()) ---
    DrawText("Click a portrait or press ENTER to select, ESC to return", 310, GetScreenHeight() - 50, 16, GRAY);
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
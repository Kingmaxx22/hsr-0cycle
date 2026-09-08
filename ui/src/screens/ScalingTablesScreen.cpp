#include "ScalingTablesScreen.h"

#include <algorithm>
#include <cmath>

ScalingTablesScreen::ScalingTablesScreen(AssetManager& assets,
                                         CharacterDatabase& characters,
                                         LoadoutStore& loadouts)
    : m_assets(assets), m_characters(characters), m_loadouts(loadouts)
{
}

void ScalingTablesScreen::setTeamContext(const std::array<std::string, 4>& team)
{
    m_team = team;
    m_selectedCharacterId.clear();
    for (const auto& id : m_team)
    {
        if (!id.empty())
        {
            m_selectedCharacterId = id;
            break;
        }
    }
    m_focusedField = -1;
    syncTexts();
}

void ScalingTablesScreen::initialize()
{
}

const char* ScalingTablesScreen::actionKey(int action) const
{
    static const char* keys[kActionCount] = {
        "basic", "skill", "ult", "fua", "memosprite"
    };
    return keys[action];
}

const char* ScalingTablesScreen::actionLabel(int action) const
{
    static const char* labels[kActionCount] = {
        "Basic", "Skill", "Ult", "FUA", "Memosprite"
    };
    return labels[action];
}

const char* ScalingTablesScreen::abilityName(int action) const
{
    static const char* abilities[kActionCount] = {
        "Basic ATK", "Skill", "Ultimate", "Talent", "Memosprite Skill"
    };
    return abilities[action];
}

int ScalingTablesScreen::requiredEidolon(int action) const
{
    const CharacterInfo* info = selectedInfo();
    if (info == nullptr)
        return 0;
    const std::string ability = abilityName(action);
    int required = 0;
    for (const auto& eidolon : info->eidolons)
    {
        for (const auto& skill : eidolon.skillLevels)
        {
            if (skill == ability &&
                (required == 0 || eidolon.eidolon < required))
                required = eidolon.eidolon;
        }
    }
    return required;
}

CharacterLoadout& ScalingTablesScreen::loadoutFor(const std::string& characterId)
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

const CharacterInfo* ScalingTablesScreen::selectedInfo() const
{
    if (m_selectedCharacterId.empty())
        return nullptr;
    return m_characters.get(m_selectedCharacterId);
}

void ScalingTablesScreen::syncTexts()
{
    if (m_selectedCharacterId.empty())
        return;
    const CharacterLoadout& lo = loadoutFor(m_selectedCharacterId);
    for (int a = 0; a < kActionCount; ++a)
    {
        for (int b = 0; b < 2; ++b)
        {
            int field = a * 2 + b;
            if (field == m_focusedField)
                continue;
            double value = 0.0;
            auto it = lo.scalingTables.find(actionKey(a));
            if (it != lo.scalingTables.end())
                value = (b == 0) ? it->second.base : it->second.boosted;
            m_texts[static_cast<size_t>(field)] =
                formatStatValueText(value * 100.0, "atk_pct");
        }
    }
}

void ScalingTablesScreen::commitField(int field)
{
    if (m_selectedCharacterId.empty())
        return;
    CharacterLoadout& lo = loadoutFor(m_selectedCharacterId);
    const std::string& text = m_texts[static_cast<size_t>(field)];
    double number = 0.0;
    try { number = text.empty() ? 0.0 : std::stod(text); }
    catch (...) { number = 0.0; }
    double value = std::max(0.0, number / 100.0);
    int action = field / 2;
    CharacterLoadout::ScalingEntry& entry =
        lo.scalingTables[actionKey(action)]; // default 0/0
    if (field % 2 == 0)
        entry.base = value;
    else
        entry.boosted = value;
}

Rectangle ScalingTablesScreen::backBounds() const
{
    return Rectangle{310.0f, 40.0f, 120.0f, 32.0f};
}

Rectangle ScalingTablesScreen::teamRowBounds(int index) const
{
    return Rectangle{310.0f, 160.0f + index * 64.0f, 250.0f, 56.0f};
}

Rectangle ScalingTablesScreen::eidolonMinusBounds() const
{
    return Rectangle{760.0f, 158.0f, 30.0f, 28.0f};
}

Rectangle ScalingTablesScreen::eidolonPlusBounds() const
{
    return Rectangle{860.0f, 158.0f, 30.0f, 28.0f};
}

Rectangle ScalingTablesScreen::fieldBounds(int field) const
{
    int action = field / 2;
    int col = field % 2; // 0 = base, 1 = boosted
    return Rectangle{
        760.0f + col * 170.0f,
        fieldsBaseY() + action * 62.0f,
        150.0f,
        30.0f
    };
}

float ScalingTablesScreen::fieldsBaseY() const
{
    // Must match draw(): ey starts at 200, +22 per eidolon row
    // (24 when the list is empty), then +12.
    const CharacterInfo* info = selectedInfo();
    float ey = 200.0f;
    if (info == nullptr || info->eidolons.empty())
        ey += 24.0f;
    else
        ey += static_cast<float>(info->eidolons.size()) * 22.0f;
    return ey + 12.0f;
}

void ScalingTablesScreen::update(float dt)
{
    (void)dt;
    Vector2 mouse = GetMousePosition();
    const bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    if (clicked && CheckCollisionPointRec(mouse, backBounds()))
    {
        m_backRequested = true;
        return;
    }

    if (clicked)
    {
        // Team selection.
        for (int i = 0; i < 4; ++i)
        {
            if (!m_team[static_cast<size_t>(i)].empty() &&
                CheckCollisionPointRec(mouse, teamRowBounds(i)))
            {
                if (m_focusedField >= 0)
                {
                    commitField(m_focusedField);
                    m_focusedField = -1;
                }
                m_selectedCharacterId = m_team[static_cast<size_t>(i)];
                syncTexts();
                break;
            }
        }

        if (!m_selectedCharacterId.empty())
        {
            CharacterLoadout& lo = loadoutFor(m_selectedCharacterId);
            if (CheckCollisionPointRec(mouse, eidolonMinusBounds()))
                lo.eidolonLevel = std::max(0, lo.eidolonLevel - 1);
            else if (CheckCollisionPointRec(mouse, eidolonPlusBounds()))
                lo.eidolonLevel = std::min(6, lo.eidolonLevel + 1);

            int hitField = -1;
            for (int f = 0; f < kFieldCount; ++f)
            {
                if (CheckCollisionPointRec(mouse, fieldBounds(f)))
                {
                    hitField = f;
                    break;
                }
            }
            if (hitField >= 0)
            {
                if (m_focusedField >= 0)
                    commitField(m_focusedField);
                m_focusedField = hitField;
            }
            else if (m_focusedField >= 0)
            {
                commitField(m_focusedField);
                m_focusedField = -1;
            }
        }
    }

    // Typed input to the focused field (percent-numbers, same convention
    // as the Characters screen other-bonus fields).
    if (m_focusedField >= 0)
    {
        std::string& text = m_texts[static_cast<size_t>(m_focusedField)];
        int key = GetCharPressed();
        while (key > 0)
        {
            bool isDigit = key >= '0' && key <= '9';
            bool isDot = key == '.' && text.find('.') == std::string::npos;
            if ((isDigit || isDot) && text.size() < 8)
                text.push_back(static_cast<char>(key));
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && !text.empty())
            text.pop_back();
        if (IsKeyPressed(KEY_TAB))
        {
            commitField(m_focusedField);
            m_focusedField = (m_focusedField + 1) % kFieldCount;
        }
        if (IsKeyPressed(KEY_ENTER))
        {
            commitField(m_focusedField);
            m_focusedField = -1;
        }
    }
}

void ScalingTablesScreen::draw()
{
    DrawText("< Back", static_cast<int>(backBounds().x + 12),
             static_cast<int>(backBounds().y + 7), 14, RAYWHITE);
    DrawText("DAMAGE TABLES (manual #1[i]% values)", 310, 90, 16, RAYWHITE);
    DrawText("Base + Eidolon-boosted multipliers per action. Boosted applies",
             310, 114, 12, GRAY);
    DrawText("at the E-number whose skillLevels list the action (data-driven).",
             310, 130, 12, GRAY);

    // Team list.
    for (int i = 0; i < 4; ++i)
    {
        Rectangle row = teamRowBounds(i);
        const std::string& id = m_team[static_cast<size_t>(i)];
        if (id.empty())
            continue;
        const CharacterInfo* info = m_characters.get(id);
        std::string name = (info != nullptr) ? info->name : id;
        bool selected = (id == m_selectedCharacterId);
        DrawRectangleRounded(row, 0.15f, 6,
            selected ? Color{55, 62, 82, 255} : Color{28, 31, 41, 255});
        DrawText(name.c_str(), static_cast<int>(row.x + 12),
                 static_cast<int>(row.y + 17), 15,
                 selected ? RAYWHITE : LIGHTGRAY);
    }

    const CharacterInfo* info = selectedInfo();
    if (info == nullptr)
    {
        DrawText("Select a team member.", 600, 200, 14, GRAY);
        return;
    }
    CharacterLoadout& lo = loadoutFor(m_selectedCharacterId);

    DrawText(info->name.c_str(), 600, 120, 18, RAYWHITE);

    // Eidolon stepper.
    DrawText("EIDOLON", 600, 163, 13, LIGHTGRAY);
    Rectangle minusR = eidolonMinusBounds();
    Rectangle plusR = eidolonPlusBounds();
    DrawRectangleRounded(minusR, 0.2f, 4, Color{28, 31, 41, 255});
    DrawText("-", static_cast<int>(minusR.x + 11),
             static_cast<int>(minusR.y + 4), 14, RAYWHITE);
    DrawRectangleRounded(plusR, 0.2f, 4, Color{28, 31, 41, 255});
    DrawText("+", static_cast<int>(plusR.x + 10),
             static_cast<int>(plusR.y + 4), 14, RAYWHITE);
    DrawText(TextFormat("E%d", lo.eidolonLevel), 800, 162, 15, RAYWHITE);

    // Eidolon list with skill-raise annotations.
    float ey = 200.0f;
    if (info->eidolons.empty())
    {
        DrawText("No Eidolon data for this character.", 600,
                 static_cast<int>(ey), 12, GRAY);
        ey += 24.0f;
    }
    for (const auto& eidolon : info->eidolons)
    {
        bool unlocked = eidolon.eidolon <= lo.eidolonLevel;
        std::string label = "E" + std::to_string(eidolon.eidolon) + " " +
            eidolon.title;
        if (!eidolon.skillLevels.empty())
        {
            label += " [up: ";
            for (size_t s = 0; s < eidolon.skillLevels.size(); ++s)
            {
                if (s > 0)
                    label += "/";
                label += eidolon.skillLevels[s];
            }
            label += "]";
        }
        if (label.size() > 72)
            label = label.substr(0, 69) + "...";
        DrawText(label.c_str(), 600, static_cast<int>(ey), 12,
                 unlocked ? RAYWHITE : GRAY);
        ey += 22.0f;
    }

    // Action rows: base + boosted fields.
    float baseY = fieldsBaseY();
    DrawText("BASE", 760, static_cast<int>(baseY - 18), 12, LIGHTGRAY);
    DrawText("BOOSTED", 930, static_cast<int>(baseY - 18), 12, LIGHTGRAY);
    for (int a = 0; a < kActionCount; ++a)
    {
        float rowY = baseY + a * 62.0f;
        std::string label = actionLabel(a);
        int req = requiredEidolon(a);
        if (req > 0)
            label += " (boost@E" + std::to_string(req) + ")";
        else
            label += " (no E boost)";
        DrawText(label.c_str(), 600, static_cast<int>(rowY + 6), 13, RAYWHITE);
        for (int b = 0; b < 2; ++b)
        {
            int field = a * 2 + b;
            Rectangle f = fieldBounds(field);
            bool focused = (m_focusedField == field);
            DrawRectangleRounded(f, 0.2f, 8,
                focused ? Color{44, 52, 70, 255} : Color{28, 31, 41, 255});
            std::string shown = m_texts[static_cast<size_t>(field)];
            if (focused)
                shown += "_";
            if (shown.empty() && !focused)
                shown = "-";
            DrawText(shown.c_str(), static_cast<int>(f.x + 10),
                     static_cast<int>(f.y + 6), 14, RAYWHITE);
        }
    }
}

bool ScalingTablesScreen::consumeBackRequest()
{
    if (!m_backRequested)
        return false;
    m_backRequested = false;
    return true;
}

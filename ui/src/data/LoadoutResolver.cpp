#include "LoadoutResolver.h"

#include <cstdlib>

namespace loadout {
namespace {

double parseNumber(const std::string& text)
{
    if (text.empty())
        return 0.0;
    // Accept a trailing "%" gracefully (buffers normally exclude it).
    std::string copy = text;
    if (!copy.empty() && copy.back() == '%')
        copy.pop_back();
    char* end = nullptr;
    double value = std::strtod(copy.c_str(), &end);
    if (end == copy.c_str())
        return 0.0;
    return value;
}

// Adds one (key, display-number) pair into the bonus aggregates.
// Percent-keys arrive as percent-numbers ("43.2" = 43.2%) and are stored
// as decimals; flat keys are stored raw.
void accumulate(ResolvedBonuses& out, const std::string& key, double number)
{
    if (key == "hp") { out.flatHp += number; return; }
    if (key == "atk") { out.flatAtk += number; return; }
    if (key == "def") { out.flatDef += number; return; }
    if (key == "spd") { out.flatSpd += number; return; }
    if (key == "hp_pct") { out.hpPct += number / 100.0; return; }
    if (key == "atk_pct") { out.atkPct += number / 100.0; return; }
    if (key == "def_pct") { out.defPct += number / 100.0; return; }
    if (key == "crit_rate_pct") { out.critRate += number / 100.0; return; }
    if (key == "crit_dmg_pct") { out.critDmg += number / 100.0; return; }
    if (key == "effect_hit_rate_pct") { out.ehr += number / 100.0; return; }
    if (key == "effect_res_pct") { out.effectRes += number / 100.0; return; }
    // Any elemental DMG% main (sphere) feeds the generic elemental pool;
    // the engine applies it as the attacker's elemental DMG%.
    const char* elements[] = {
        "physical_dmg_pct", "fire_dmg_pct", "ice_dmg_pct",
        "lightning_dmg_pct", "wind_dmg_pct", "quantum_dmg_pct",
        "imaginary_dmg_pct"
    };
    for (const char* el : elements) {
        if (key == el) {
            out.elemDmgPct += number / 100.0;
            return;
        }
    }
    // No engine field (yet): outgoing healing, break effect, energy regen.
    // Deliberately ignored rather than misrouted into an unrelated stat.
}

double lookupBase(const CharacterInfo& info, const char* key)
{
    auto it = info.baseStats.find(key);
    return it != info.baseStats.end() ? it->second : 0.0;
}

} // namespace

double parseStatValueText(const std::string& text, const std::string& key)
{
    double number = parseNumber(text);
    if (isPercentStatKey(key))
        return number / 100.0;
    return number;
}

ResolvedBonuses resolveGearBonuses(const CharacterLoadout& loadout)
{
    ResolvedBonuses out;
    for (size_t i = 0; i < loadout.gear.size(); ++i)
    {
        const GearPiece& piece = loadout.gear[i];
        if (!piece.mainStat.empty())
            accumulate(out, piece.mainStat, parseNumber(piece.mainStatValueText));
        for (const auto& sub : piece.substats)
        {
            if (!sub.statKey.empty())
                accumulate(out, sub.statKey, parseNumber(sub.valueText));
        }
    }
    out.hpPct += loadout.otherBonuses.hpPct;
    out.atkPct += loadout.otherBonuses.atkPct;
    out.defPct += loadout.otherBonuses.defPct;
    out.flatSpd += loadout.otherBonuses.flatSpd;
    out.critRate += loadout.otherBonuses.critRate;
    out.critDmg += loadout.otherBonuses.critDmg;
    out.elemDmgPct += loadout.otherBonuses.elemDmgPct;
    out.resPen += loadout.otherBonuses.resPen;
    out.ehr += loadout.otherBonuses.ehr;
    return out;
}

void resolveBaseStats(const CharacterInfo& info, const CharacterLoadout& loadout,
                      double& hp, double& atk, double& def, double& spd)
{
    if (loadout.manualBase.useOverride)
    {
        hp = loadout.manualBase.hp;
        atk = loadout.manualBase.atk;
        def = loadout.manualBase.def;
        spd = loadout.manualBase.spd;
        return;
    }
    hp = lookupBase(info, "hp");
    atk = lookupBase(info, "atk");
    def = lookupBase(info, "def");
    spd = lookupBase(info, "spd");
}

void resolveLightConeBase(const CharacterLoadout& loadout,
                          const LightConeDatabase& lightCones,
                          double& hp, double& atk, double& def)
{
    hp = 0.0;
    atk = 0.0;
    def = 0.0;
    if (loadout.lightConeId.empty())
        return;
    const LightConeInfo* lc = lightCones.get(loadout.lightConeId);
    if (lc == nullptr)
        return;
    hp = static_cast<double>(lc->hp);
    atk = static_cast<double>(lc->atk);
    def = static_cast<double>(lc->def);
}

ResolvedTotals resolveTotals(const CharacterInfo& info,
                             const CharacterLoadout& loadout,
                             const LightConeDatabase& lightCones)
{
    double baseHp, baseAtk, baseDef, baseSpd;
    resolveBaseStats(info, loadout, baseHp, baseAtk, baseDef, baseSpd);
    double lcHp, lcAtk, lcDef;
    resolveLightConeBase(loadout, lightCones, lcHp, lcAtk, lcDef);
    ResolvedBonuses b = resolveGearBonuses(loadout);

    // Sec 9: LC base merges with character base FIRST, then % applies.
    // Speed takes no LC base (Light Cones grant no base Speed).
    ResolvedTotals t;
    t.hp = (baseHp + lcHp) * (1.0 + b.hpPct) + b.flatHp;
    t.atk = (baseAtk + lcAtk) * (1.0 + b.atkPct) + b.flatAtk;
    t.def = (baseDef + lcDef) * (1.0 + b.defPct) + b.flatDef;
    // Speed % bonuses scale base speed only (Sec 15). No %Spd source
    // exists in gear/other bonuses today, so this is base + flat; the
    // engine's own calculateTotalSpeed keeps the full % stage.
    t.spd = baseSpd + b.flatSpd;
    return t;
}

void applyToCharacterConfig(hsr::CharacterConfig& config,
                            const CharacterInfo& info,
                            const CharacterLoadout& loadout,
                            const LightConeDatabase& lightCones)
{
    config.id = info.id;
    config.name = info.name;
    config.manualStats = false;
    config.level = loadout.level;

    double baseHp, baseAtk, baseDef, baseSpd;
    resolveBaseStats(info, loadout, baseHp, baseAtk, baseDef, baseSpd);
    config.baseHp = baseHp;
    config.baseAtk = baseAtk;
    config.baseDef = baseDef;
    config.baseSpd = baseSpd;

    double lcHp, lcAtk, lcDef;
    resolveLightConeBase(loadout, lightCones, lcHp, lcAtk, lcDef);
    config.lightConeBaseHp = lcHp;
    config.lightConeBaseAtk = lcAtk;
    config.lightConeBaseDef = lcDef;

    ResolvedBonuses b = resolveGearBonuses(loadout);
    config.hpPct = b.hpPct;
    config.atkPct = b.atkPct;
    config.defPct = b.defPct;
    config.spdPct = 0.0; // No %Spd gear source; Sec 15 stage preserved.
    config.flatHp = b.flatHp;
    config.flatAtk = b.flatAtk;
    config.flatDef = b.flatDef;
    config.flatSpd = b.flatSpd;

    // Base crit values are game constants; bonuses stack on top.
    config.critRate = 0.05 + b.critRate;
    config.critDmg = 0.50 + b.critDmg;
    config.elementalDmgPct = b.elemDmgPct;
    config.resPen = b.resPen;
    config.ehr = b.ehr;
    config.effectRes = b.effectRes;
}

void applyManualToCharacterConfig(hsr::CharacterConfig& config,
                                  const std::string& id,
                                  const std::string& name,
                                  double hp, double atk, double def, double spd,
                                  double critRate, double critDmg,
                                  double elemDmgPct, double resPen,
                                  double ehr, double effectRes)
{
    config.id = id;
    config.name = name;
    config.manualStats = true;
    config.finalHp = hp;
    config.finalAtk = atk;
    config.finalDef = def;
    config.speed = static_cast<int>(spd);
    config.critRate = critRate;
    config.critDmg = critDmg;
    config.elementalDmgPct = elemDmgPct;
    config.resPen = resPen;
    config.ehr = ehr;
    config.effectRes = effectRes;
}

} // namespace loadout

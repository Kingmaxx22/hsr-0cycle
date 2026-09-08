#include "LoadoutResolver.h"

#include <cctype>
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
    if (key == "spd_pct") { out.spdPct += number / 100.0; return; }
    if (key == "alltype_pct") { out.allTypeDmgPct += number / 100.0; return; }
    // Set-data "damage" is always elemental-conditional in the current data
    // (e.g. +10% lightning DMG). The user asserts the match via the opt-in
    // toggle, so it joins the elemental pool, not the generic all-type pool.
    if (key == "elemdmg_pct") { out.elemDmgPct += number / 100.0; return; }
    if (key == "break_effect_pct") { out.breakEffect += number / 100.0; return; }
    if (key == "outgoing_healing_pct") { out.healingBoost += number / 100.0; return; }
    if (key == "energy_regen_pct") { out.energyRegen += number / 100.0; return; }
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

std::string setEffectId(const std::string& setId, const std::string& piece,
                        size_t index)
{
    return setId + ":" + piece + ":" + std::to_string(index);
}

namespace {
// JSON set-effect stat names (unconditional scale: percent types already
// decimal, e.g. 0.12) -> internal resolver keys (percent-number convention).
const char* setStatToKey(const std::string& stat)
{
    if (stat == "atk") return "atk_pct";
    if (stat == "max_hp") return "hp_pct";
    if (stat == "def") return "def_pct";
    if (stat == "spd") return "spd_pct";
    if (stat == "crit_rate") return "crit_rate_pct";
    if (stat == "crit_dmg") return "crit_dmg_pct";
    if (stat == "effect_hit_rate") return "effect_hit_rate_pct";
    if (stat == "effect_res") return "effect_res_pct";
    if (stat == "damage") return "elemdmg_pct";
    if (stat == "break_effect") return "break_effect_pct";
    if (stat == "outgoing_healing") return "outgoing_healing_pct";
    if (stat == "energy_regen") return "energy_regen_pct";
    return "";
}
} // namespace

namespace {
bool elementEquals(const std::string& a, const std::string& b)
{
    if (a.size() != b.size() || a.empty())
        return false;
    for (size_t i = 0; i < a.size(); ++i)
    {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i])))
            return false;
    }
    return true;
}
} // namespace

bool isSetEffectAutoActive(const SetEffectCondition& condition,
                           const std::string& attackerElement)
{
    // Only element-match conditions are derivable from turn state so far:
    // the attacker's element is known on every action.
    if (condition.kind != "damage_type")
        return false;
    if (condition.elementArg.empty() || attackerElement.empty())
        return false;
    return elementEquals(condition.elementArg, attackerElement);
}

bool isSetEffectActive(const SetEffectCondition& condition,
                       bool manualToggle, const std::string& attackerElement)
{
    if (manualToggle)
        return true;
    return isSetEffectAutoActive(condition, attackerElement);
}

ResolvedBonuses resolveSetBonuses(const CharacterLoadout& loadout,
                                  const RelicSetDatabase& relicSets,
                                  const std::string& attackerElement)
{
    ResolvedBonuses out;
    auto applyList = [&](const RelicSetInfo* set, const std::vector<SetEffect>& list,
                         const std::string& piece) {
        if (set == nullptr)
            return;
        for (size_t i = 0; i < list.size(); ++i)
        {
            const SetEffect& fx = list[i];
            if (fx.hasCondition)
            {
                // Manual opt-in OR auto-derivation. Missing key = false.
                auto it = loadout.setEffectActive.find(setEffectId(set->id, piece, i));
                bool manual = (it != loadout.setEffectActive.end() && it->second);
                if (!isSetEffectActive(fx.condition, manual, attackerElement))
                    continue;
            }
            const char* key = setStatToKey(fx.stat);
            if (key[0] != '\0')
                accumulate(out, key, fx.value * 100.0);
        }
    };
    auto applyRelicId = [&](const std::string& id, bool allowFourPiece) {
        if (id.empty())
            return;
        const RelicSetInfo* set = relicSets.get(id);
        if (set == nullptr || set->category != "relic")
            return;
        applyList(set, set->twoPiece, "2");
        if (allowFourPiece)
            applyList(set, set->fourPiece, "4");
    };
    applyRelicId(loadout.relicSetA, loadout.relicFourPiece);
    if (!loadout.relicFourPiece)
        applyRelicId(loadout.relicSetB, false);
    if (!loadout.planarSet.empty())
    {
        const RelicSetInfo* planar = relicSets.get(loadout.planarSet);
        if (planar != nullptr)
            applyList(planar, planar->twoPiece, "2");
    }
    return out;
}

namespace {
void mergeBonuses(ResolvedBonuses& into, const ResolvedBonuses& add)
{
    into.hpPct += add.hpPct;
    into.atkPct += add.atkPct;
    into.defPct += add.defPct;
    into.spdPct += add.spdPct;
    into.flatHp += add.flatHp;
    into.flatAtk += add.flatAtk;
    into.flatDef += add.flatDef;
    into.flatSpd += add.flatSpd;
    into.critRate += add.critRate;
    into.critDmg += add.critDmg;
    into.elemDmgPct += add.elemDmgPct;
    into.allTypeDmgPct += add.allTypeDmgPct;
    into.resPen += add.resPen;
    into.ehr += add.ehr;
    into.effectRes += add.effectRes;
    into.breakEffect += add.breakEffect;
    into.healingBoost += add.healingBoost;
    into.energyRegen += add.energyRegen;
    into.breakDmgIncrease += add.breakDmgIncrease;
}
} // namespace

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
    out.breakDmgIncrease += loadout.otherBonuses.breakDmgIncrease;
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
                             const LightConeDatabase& lightCones,
                             const RelicSetDatabase& relicSets)
{
    double baseHp, baseAtk, baseDef, baseSpd;
    resolveBaseStats(info, loadout, baseHp, baseAtk, baseDef, baseSpd);
    double lcHp, lcAtk, lcDef;
    resolveLightConeBase(loadout, lightCones, lcHp, lcAtk, lcDef);
    ResolvedBonuses b = resolveGearBonuses(loadout);
    mergeBonuses(b, resolveSetBonuses(loadout, relicSets, info.element));

    // Sec 9: LC base merges with character base FIRST, then % applies.
    // Speed takes no LC base (Light Cones grant no base Speed).
    ResolvedTotals t;
    t.hp = (baseHp + lcHp) * (1.0 + b.hpPct) + b.flatHp;
    t.atk = (baseAtk + lcAtk) * (1.0 + b.atkPct) + b.flatAtk;
    t.def = (baseDef + lcDef) * (1.0 + b.defPct) + b.flatDef;
    // Speed % scales base speed only (Sec 15); %Spd arrives via set bonuses.
    t.spd = baseSpd * (1.0 + b.spdPct) + b.flatSpd;
    return t;
}

void applyToCharacterConfig(hsr::CharacterConfig& config,
                            const CharacterInfo& info,
                            const CharacterLoadout& loadout,
                            const LightConeDatabase& lightCones,
                            const RelicSetDatabase& relicSets)
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
    mergeBonuses(b, resolveSetBonuses(loadout, relicSets, info.element));
    config.hpPct = b.hpPct;
    config.atkPct = b.atkPct;
    config.defPct = b.defPct;
    config.spdPct = b.spdPct;
    config.flatHp = b.flatHp;
    config.flatAtk = b.flatAtk;
    config.flatDef = b.flatDef;
    config.flatSpd = b.flatSpd;

    // Per-skill tuning (Sec 22 DB): copied verbatim, engine falls back
    // wherever entries are absent/zero. Keys normalized to engine action
    // names ("basic" -> "Basic"); unknown keys are ignored.
    for (const auto& kv : info.skills)
    {
        std::string action;
        if (kv.first == "basic") action = "Basic";
        else if (kv.first == "skill") action = "Skill";
        else if (kv.first == "ult") action = "Ult";
        else if (kv.first == "fua") action = "FUA";
        else if (kv.first == "memosprite") action = "Memosprite";
        else continue;
        hsr::CharacterConfig::SkillActionTuning tuning;
        tuning.toughnessDamage = kv.second.toughness;
        tuning.toughnessAdjacent = kv.second.toughnessAdjacent;
        tuning.healMultiplier = kv.second.heal;
        tuning.shieldMultiplier = kv.second.shield;
        tuning.energyGain = kv.second.energy;
        tuning.damageMultiplier = kv.second.multPrimary;
        tuning.adjacentMultiplier = kv.second.multAdjacent;
        tuning.bounceHits = kv.second.bounceHits;
        tuning.targetType = kv.second.targetType;
        config.skillActions[action] = tuning;
    }

    // Base crit values are game constants; bonuses stack on top.
    config.critRate = 0.05 + b.critRate;
    config.critDmg = 0.50 + b.critDmg;
    config.elementalDmgPct = b.elemDmgPct;
    config.allTypeDmgPct = b.allTypeDmgPct;
    config.resPen = b.resPen;
    config.ehr = b.ehr;
    config.effectRes = b.effectRes;
    config.breakDmgIncrease = b.breakDmgIncrease;
    // DoT base-chance default from dot_base_chance.csv. The DoT still
    // needs a user-configured type/turns/scale to fire (Sec 13 gate).
    if (info.dotBaseChance > 0.0)
        config.breakDotChance = info.dotBaseChance;
    // Q1 store-only: resolved for display/debug, not consumed by calcs yet.
    config.breakEffect = b.breakEffect;
    config.outgoingHealingBoost = b.healingBoost;
    config.energyRegen = b.energyRegen;

    // Phase 2: manually enabled passives travel as informational notes
    // (§29: no numeric effect inferred). Trace names come from the DB.
    config.passiveNotes.clear();
    for (const auto& trace : info.traces)
    {
        auto it = loadout.traceActive.find(trace.slot);
        if (it != loadout.traceActive.end() && it->second)
            config.passiveNotes.push_back("Trace " + trace.slot + " " +
                                          trace.name + ": ON");
    }
    if (loadout.lcPassiveActive && !loadout.lightConeId.empty())
    {
        const LightConeInfo* lc = lightCones.getById(loadout.lightConeId);
        std::string lcName = (lc != nullptr) ? lc->name : loadout.lightConeId;
        config.passiveNotes.push_back("LC passive " + lcName + ": ON");
    }

    // Phase 3: Eidolons. Skill-raising Eidolons select boosted manual
    // scaling values data-driven (min E-number listing the action); all
    // other unlocked Eidolons travel as informational notes only.
    const int eidolonLevel = std::max(0, std::min(6, loadout.eidolonLevel));
    auto engineActionToAbility = [](const std::string& action) -> std::string {
        if (action == "basic") return "Basic ATK";
        if (action == "skill") return "Skill";
        if (action == "ult") return "Ultimate";
        if (action == "fua") return "Talent";
        if (action == "memosprite") return "Memosprite Skill";
        return "";
    };
    for (const auto& kv : loadout.scalingTables)
    {
        const std::string ability = engineActionToAbility(kv.first);
        if (ability.empty())
            continue;
        // Min unlocked-or-not E-number raising this action (0 = none).
        int requiredE = 0;
        for (const auto& eidolon : info.eidolons)
        {
            for (const auto& skill : eidolon.skillLevels)
            {
                if (skill == ability &&
                    (requiredE == 0 || eidolon.eidolon < requiredE))
                    requiredE = eidolon.eidolon;
            }
        }
        double value = kv.second.base;
        if (kv.second.boosted > 0.0 && requiredE > 0 && eidolonLevel >= requiredE)
            value = kv.second.boosted;
        if (value <= 0.0)
            continue;
        std::string engineAction;
        if (kv.first == "basic") engineAction = "Basic";
        else if (kv.first == "skill") engineAction = "Skill";
        else if (kv.first == "ult") engineAction = "Ult";
        else if (kv.first == "fua") engineAction = "FUA";
        else if (kv.first == "memosprite") engineAction = "Memosprite";
        else continue;
        // Manual tables win over parsed data (explicit user entry).
        config.skillActions[engineAction].damageMultiplier = value;
    }
    for (const auto& eidolon : info.eidolons)
    {
        if (eidolon.eidolon > eidolonLevel)
            continue;
        if (!eidolon.skillLevels.empty())
            continue; // skill raises act through scaling selection above
        config.passiveNotes.push_back("E" + std::to_string(eidolon.eidolon) +
                                      " " + eidolon.title + ": ON");
    }
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

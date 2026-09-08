#pragma once

#include <string>
#include <unordered_map>
#include <vector>

struct EnemyInfo
{
    std::string id;
    std::string name;
    std::string rating;
    std::string assetId;
    std::string assetPath;

    int level = 0;
    double hp = 0.0;
    double atk = 0.0;
    double def = 0.0;
    double spd = 0.0;
    double toughness = 0.0;
    double effectHitRate = 0.0;
    double effectRes = 0.0;
    double minimumRes = 0.0;
    double critDmg = 0.0;
    double firstTurnDelay = 0.0;

    bool isBoss = false;
    bool isElite = false;

    std::vector<std::string> weaknesses;
    std::unordered_map<std::string, double> resistances;

    // Phase 4.1: offense targeting parsed from monsters_data.csv skill
    // text ("to all targets" -> AoE, "adjacent" -> Blast, else Single).
    // "" = Single fallback. Most severe skill wins (enemies alternate
    // attacks in game; no rotation model here — documented simplification).
    std::string offenseTargetType;

    bool isBossOrElite() const { return isBoss || isElite; }
};

class EnemyDatabase
{
public:
    bool load(const std::string& dataDir);

    const EnemyInfo* get(const std::string& id) const;
    const std::vector<EnemyInfo>& all() const { return enemies; }

private:
    std::vector<EnemyInfo> enemies;
    std::unordered_map<std::string, size_t> idIndex;
};

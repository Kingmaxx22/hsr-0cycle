#pragma once

#include <string>
#include <vector>

struct RelicSetInfo
{
    std::string id;
    std::string name;
    std::string category; // "relic" (4-slot sets) or "planar_ornament" (2-slot sets)
};

class RelicSetDatabase
{
public:
    // dataDir = path to engine/hsr_engine/data
    bool load(const std::string& dataDir);

    std::vector<const RelicSetInfo*> byCategory(const std::string& category) const;
    const std::vector<RelicSetInfo>& all() const { return sets; }

private:
    std::vector<RelicSetInfo> sets;
};
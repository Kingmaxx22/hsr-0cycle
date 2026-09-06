#include "LightConeDatabase.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <regex>
#include <sstream>

namespace
{
    std::string toLowerCopy(const std::string& s)
    {
        std::string out = s;
        std::transform(out.begin(), out.end(), out.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return out;
    }

    std::string normalizeId(const std::string& s)
    {
        std::string out;
        out.reserve(s.size());
        for (char c : s)
        {
            if (std::isalnum(static_cast<unsigned char>(c)))
                out += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            else if (c == '-' || c == '_' || c == ' ' || c == '.' || c == '\'')
                out += '_';
        }
        // Collapse multiple underscores
        std::string clean;
        bool lastUnderscore = false;
        for (char c : out)
        {
            if (c == '_')
            {
                if (!lastUnderscore && !clean.empty())
                    clean += '_';
                lastUnderscore = true;
            }
            else
            {
                clean += c;
                lastUnderscore = false;
            }
        }
        if (!clean.empty() && clean.back() == '_')
            clean.pop_back();

        return clean;
    }

    std::vector<std::vector<std::string>> parseCSV(std::istream& stream)
    {
        std::vector<std::vector<std::string>> rows;
        std::vector<std::string> row;
        std::string cell;
        bool inQuotes = false;
        char ch = 0;

        while (stream.get(ch))
        {
            if (ch == '"')
            {
                if (inQuotes && stream.peek() == '"')
                {
                    stream.get(ch);
                    cell += '"';
                }
                else
                {
                    inQuotes = !inQuotes;
                }
            }
            else if (ch == ',' && !inQuotes)
            {
                row.push_back(cell);
                cell.clear();
            }
            else if ((ch == '\r' || ch == '\n') && !inQuotes)
            {
                if (ch == '\r' && stream.peek() == '\n')
                    stream.get(ch);
                row.push_back(cell);
                cell.clear();
                if (!row.empty() && !(row.size() == 1 && row[0].empty()))
                    rows.push_back(std::move(row));
                row.clear();
            }
            else
            {
                cell += ch;
            }
        }

        if (!cell.empty() || !row.empty())
        {
            row.push_back(cell);
            rows.push_back(std::move(row));
        }

        return rows;
    }
}

bool LightConeDatabase::load(const std::string& dataDir)
{
    std::string path = dataDir + "/light_cones.csv";
    std::ifstream file(path);
    if (!file)
        return false;

    auto rows = parseCSV(file);
    if (rows.empty())
        return false;

    lightCones.clear();
    idIndex.clear();
    nameIndex.clear();

    std::regex statRegex(R"(HP\s*\+\s*(\d+)\s+ATK\s*\+\s*(\d+)\s+DEF\s*\+\s*(\d+))");
    std::regex prefixRegex(R"(^(Rarity:\s*\d\s*★\s*Path:\s*\w+\s*))");

    for (size_t r = 1; r < rows.size(); ++r)
    {
        const auto& cols = rows[r];
        if (cols.size() < 4)
            continue;

        std::string name = cols[0];
        std::string slug = cols[1];
        std::string rarityStr = cols[2];
        std::string pathType = cols[3];

        // Strip UTF-8 BOM if present on first column
        if (name.size() >= 3 && static_cast<unsigned char>(name[0]) == 0xEF &&
            static_cast<unsigned char>(name[1]) == 0xBB &&
            static_cast<unsigned char>(name[2]) == 0xBF)
        {
            name = name.substr(3);
        }

        // Trim whitespace
        name.erase(0, name.find_first_not_of(" \t\r\n"));
        name.erase(name.find_last_not_of(" \t\r\n") + 1);
        slug.erase(0, slug.find_first_not_of(" \t\r\n"));
        slug.erase(slug.find_last_not_of(" \t\r\n") + 1);
        rarityStr.erase(0, rarityStr.find_first_not_of(" \t\r\n"));
        rarityStr.erase(rarityStr.find_last_not_of(" \t\r\n") + 1);
        pathType.erase(0, pathType.find_first_not_of(" \t\r\n"));
        pathType.erase(pathType.find_last_not_of(" \t\r\n") + 1);

        if (rarityStr != "3" && rarityStr != "4" && rarityStr != "5")
            continue;
        if (name == "Honkai: Star Rail" || name == "4★" || name == "5★" || name == "3★" || name.empty())
            continue;

        int rarity = std::stoi(rarityStr);
        int hp = 0, atk = 0, def = 0;
        std::string desc;

        if (cols.size() >= 8)
        {
            // Clean format: name, slug, rarity, path, hp, atk, def, effect_description
            try {
                hp = std::stoi(cols[4]);
                atk = std::stoi(cols[5]);
                def = std::stoi(cols[6]);
            } catch (...) {}
            desc = cols[7];
        }
        else if (cols.size() >= 5)
        {
            // Legacy / unparsed single-cell format
            desc = cols[4];
            std::smatch sm;
            if (std::regex_search(desc, sm, statRegex) && sm.size() >= 4)
            {
                hp = std::stoi(sm[1].str());
                atk = std::stoi(sm[2].str());
                def = std::stoi(sm[3].str());
                desc = desc.substr(0, sm.position());
            }
            desc = std::regex_replace(desc, prefixRegex, "");
        }

        desc.erase(0, desc.find_first_not_of(" \t\r\n"));
        desc.erase(desc.find_last_not_of(" \t\r\n") + 1);

        // Normalize assetId to match asset_manifest.csv
        std::string assetId = normalizeId(slug);
        // If it has _new, try both
        if (assetId.rfind("_new") != std::string::npos && assetId.rfind("_new") == assetId.size() - 4)
        {
            assetId = assetId.substr(0, assetId.size() - 4) + "new";
        }

        LightConeInfo info;
        info.id = slug;
        info.assetId = assetId;
        info.name = name;
        info.rarity = rarity;
        info.path = pathType;
        info.hp = hp;
        info.atk = atk;
        info.def = def;
        info.effectDescription = desc;

        size_t index = lightCones.size();
        idIndex[info.id] = index;
        idIndex[normalizeId(info.id)] = index;
        idIndex[info.assetId] = index;
        nameIndex[toLowerCopy(info.name)] = index;

        lightCones.push_back(std::move(info));
    }

    return !lightCones.empty();
}

const LightConeInfo* LightConeDatabase::get(const std::string& idOrName) const
{
    if (idOrName.empty())
        return nullptr;

    const auto* byId = getById(idOrName);
    if (byId)
        return byId;

    return getByName(idOrName);
}

const LightConeInfo* LightConeDatabase::getById(const std::string& id) const
{
    auto it = idIndex.find(id);
    if (it != idIndex.end())
        return &lightCones[it->second];

    std::string norm = normalizeId(id);
    it = idIndex.find(norm);
    if (it != idIndex.end())
        return &lightCones[it->second];

    return nullptr;
}

const LightConeInfo* LightConeDatabase::getByName(const std::string& name) const
{
    auto it = nameIndex.find(toLowerCopy(name));
    if (it != nameIndex.end())
        return &lightCones[it->second];

    return nullptr;
}

std::vector<const LightConeInfo*> LightConeDatabase::byPath(const std::string& path) const
{
    std::vector<const LightConeInfo*> result;
    for (const auto& lc : lightCones)
    {
        if (lc.path == path)
            result.push_back(&lc);
    }
    return result;
}

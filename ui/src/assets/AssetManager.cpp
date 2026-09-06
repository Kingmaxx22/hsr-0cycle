#include "AssetManager.h"
#include "raylib.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

static std::vector<std::string> splitCsv(const std::string& line)
{
    std::vector<std::string> result;
    std::string field;
    bool quoted = false;

    for (char c : line)
    {
        if (c == '"')
            quoted = !quoted;
        else if (c == ',' && !quoted)
        {
            result.push_back(field);
            field.clear();
        }
        else
            field += c;
    }

    result.push_back(field);
    return result;
}

bool AssetManager::loadManifest(const std::string& manifestPath)
{
    std::string fullManifestPath = manifestPath;

    if (!IsPathAbsolute(manifestPath.c_str()))
        fullManifestPath =
            std::string(GetApplicationDirectory()) + "/" + manifestPath;

    manifestDirectory = directoryOf(fullManifestPath);

    std::ifstream file(fullManifestPath);
    if (!file)
    {
        TraceLog(LOG_ERROR,
                 "Could not open asset manifest: %s",
                 fullManifestPath.c_str());
        return false;
    }

    entries.clear();

    std::string line;
    bool header = true;

    while (std::getline(file, line))
    {
        if (line.empty())
            continue;

        if (header)
        {
            header = false;
            continue;
        }

        const auto fields = splitCsv(line);
        if (fields.size() < 6)
            continue;

        AssetEntry asset;
        asset.path = fields[0];
        asset.filename = fields[1];
        asset.normalizedId = fields[3];
        asset.category = fields[5];

        entries[asset.normalizedId] = asset;
    }

    TraceLog(LOG_INFO,
             "Loaded %zu assets from manifest",
             entries.size());

    return !entries.empty();
}

bool AssetManager::has(const std::string& id) const
{
    return entries.find(id) != entries.end();
}

const AssetEntry* AssetManager::entry(const std::string& id) const
{
    const auto it = entries.find(id);
    return it == entries.end() ? nullptr : &it->second;
}

Texture2D* AssetManager::texture(const std::string& id)
{
    const auto loaded = textures.find(id);
    if (loaded != textures.end())
        return &loaded->second;

    if (!loadTextureFor(id))
        return nullptr;

    return &textures[id];
}

static const std::unordered_map<std::string, std::string>& characterAliases()
{
    static const std::unordered_map<std::string, std::string> aliases = {
        {"pearl", "pearl4_6"},
        {"march-7th-swordmaster", "march_7th_the_hunt"},
        {"blade-mortenax", "mortenax_blade"},
        {"imbibitor-lunae", "dan_heng_imbibitor_lunae"},
        {"robin-summeretto", "robin_summerettonew"},
        {"topaz", "topaz_numby"},
        {"aventurine-waveflair", "aventurine_waveflair4_5"},
    };
    return aliases;
}

Texture2D* AssetManager::character(const std::string& id)
{
    if (has(id))
        return texture(id);

    const auto& aliases = characterAliases();
    const auto aliasIt = aliases.find(id);

    if (aliasIt != aliases.end() && has(aliasIt->second))
        return texture(aliasIt->second);

    std::string normalized = id;
    for (char& c : normalized)
        if (c == '-')
            c = '_';

    return has(normalized) ? texture(normalized) : nullptr;
}

Texture2D* AssetManager::lightCone(const std::string& id)
{
    if (has(id))
        return texture(id);

    const std::string withNew = id + "new";
    return has(withNew) ? texture(withNew) : nullptr;
}

Texture2D* AssetManager::relicSet(const std::string& id)
{
    if (has(id))
        return texture(id);

    const std::string withNew = id + "new";
    return has(withNew) ? texture(withNew) : nullptr;
}

static std::string lowerEnemy(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c)
                   {
                       return static_cast<char>(std::tolower(c));
                   });
    return value;
}

static std::string stripEnemyVariant(std::string value)
{
    value = lowerEnemy(std::move(value));
    const size_t open = value.rfind(" (");
    if (open != std::string::npos && !value.empty() && value.back() == ')')
        value.erase(open);
    return value;
}

std::vector<std::string> AssetManager::enemyNameCandidates(
    const std::string& displayName) const
{
    std::vector<std::string> candidates;
    if (displayName.empty())
        return candidates;

    auto add = [&](std::string value)
    {
        if (value.empty())
            return;
        if (std::find(candidates.begin(), candidates.end(), value) == candidates.end())
            candidates.push_back(std::move(value));
    };

    auto addForms = [&](const std::string& name)
    {
        const std::string lower = lowerEnemy(name);
        add(lower + ".png");

        std::string underscore = lower;
        for (char& c : underscore)
        {
            if (c == ' ' || c == '\t')
                c = '_';
            else if (c == ':')
                c = '_';
        }
        add(underscore + ".png");

        std::string hyphen = lower;
        for (char& c : hyphen)
            if (c == ' ' || c == '\t')
                c = '-';
        add(hyphen + ".png");

        std::string compact;
        bool lastUnderscore = false;
        for (char c : underscore)
        {
            if (c == '_')
            {
                if (lastUnderscore)
                    continue;
                lastUnderscore = true;
            }
            else
            {
                lastUnderscore = false;
            }
            compact.push_back(c);
        }
        while (!compact.empty() && compact.front() == '_') compact.erase(compact.begin());
        while (!compact.empty() && compact.back() == '_') compact.pop_back();
        add(compact + ".png");
    };

    addForms(displayName);
    const std::string base = stripEnemyVariant(displayName);
    if (base != lowerEnemy(displayName))
        addForms(base);

    return candidates;
}

Texture2D* AssetManager::enemy(
    const std::string& id,
    const std::string& displayName)
{
    const std::string cacheKey = "__enemy__" + id;
    const auto loaded = textures.find(cacheKey);
    if (loaded != textures.end())
        return &loaded->second;

    const std::string enemyDirectory =
        joinPath(manifestDirectory, "prydwen_assets/enemies");

    for (const std::string& filename : enemyNameCandidates(displayName))
    {
        const std::string path = joinPath(enemyDirectory, filename);
        if (FileExists(path.c_str()) && loadTextureAt(cacheKey, path))
            return &textures[cacheKey];
    }

    const std::vector<std::string> legacy = {
        "Monster_" + id + ".png",
        "Monster_" + id + ".webp",
        id + ".png",
        id + ".webp"
    };

    for (const std::string& filename : legacy)
    {
        const std::string path = joinPath(enemyDirectory, filename);
        if (FileExists(path.c_str()) && loadTextureAt(cacheKey, path))
            return &textures[cacheKey];
    }

    static std::unordered_set<std::string> warned;
    if (warned.insert(id).second)
    {
        TraceLog(LOG_WARNING,
                 "Enemy artwork not found for '%s' (id=%s)",
                 displayName.c_str(), id.c_str());
    }
    return nullptr;
}

bool AssetManager::loadTextureFor(const std::string& id)
{
    const AssetEntry* asset = entry(id);

    if (!asset)
    {
        TraceLog(LOG_WARNING,
                 "Asset ID not found in manifest: %s",
                 id.c_str());
        return false;
    }

    return loadTextureAt(
        id,
        joinPath(manifestDirectory, asset->path));
}

bool AssetManager::loadTextureAt(
    const std::string& key,
    const std::string& path)
{
    if (!FileExists(path.c_str()))
        return false;

    const Texture2D loaded = LoadTexture(path.c_str());

    if (loaded.id == 0)
        return false;

    textures[key] = loaded;
    return true;
}

std::string AssetManager::directoryOf(const std::string& path) const
{
    const auto position = path.find_last_of("/\\");
    return position == std::string::npos
        ? "."
        : path.substr(0, position);
}

std::string AssetManager::joinPath(
    const std::string& a,
    const std::string& b) const
{
    if (a.empty())
        return b;

    if (a.back() == '/' || a.back() == '\\')
        return a + b;

    return a + "/" + b;
}

void AssetManager::unloadAll()
{
    for (auto& [id, texture] : textures)
        UnloadTexture(texture);

    textures.clear();
}

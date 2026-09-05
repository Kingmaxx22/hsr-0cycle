#include "AssetManager.h"
#include "raylib.h"

#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>

static std::vector<std::string> splitCsv(const std::string& line)
{
    std::vector<std::string> result;
    std::string field;
    bool quoted = false;

    for (char c : line)
    {
        if (c == '"')
        {
            quoted = !quoted;
        }
        else if (c == ',' && !quoted)
        {
            result.push_back(field);
            field.clear();
        }
        else
        {
            field += c;
        }
    }

    result.push_back(field);
    return result;
}

bool AssetManager::loadManifest(const std::string& manifestPath)
{
    // Always resolve relative resource paths from the executable directory.
    std::string fullManifestPath = manifestPath;

    if (!IsPathAbsolute(manifestPath.c_str()))
    {
        fullManifestPath =
            std::string(GetApplicationDirectory()) + "/" + manifestPath;
    }

    TraceLog(LOG_INFO, "Loading asset manifest: %s",
             fullManifestPath.c_str());

    std::ifstream file(fullManifestPath);

    if (!file)
    {
        TraceLog(LOG_ERROR, "Could not open asset manifest: %s",
                 fullManifestPath.c_str());
        return false;
    }

    manifestDirectory = directoryOf(fullManifestPath);

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

        auto fields = splitCsv(line);

        if (fields.size() < 6)
            continue;

        AssetEntry entryData;

        entryData.path = fields[0];
        entryData.filename = fields[1];
        entryData.normalizedId = fields[3];
        entryData.category = fields[5];

        entries[entryData.normalizedId] = entryData;
    }

    TraceLog(LOG_INFO, "Loaded %zu assets from manifest",
             entries.size());

    return !entries.empty();
}

bool AssetManager::has(const std::string& id) const
{
    return entries.find(id) != entries.end();
}

const AssetEntry* AssetManager::entry(const std::string& id) const
{
    auto it = entries.find(id);

    if (it == entries.end())
        return nullptr;

    return &it->second;
}

Texture2D* AssetManager::texture(const std::string& id)
{
    auto loaded = textures.find(id);

    if (loaded != textures.end())
        return &loaded->second;

    if (!loadTextureFor(id))
        return nullptr;

    return &textures[id];
}

// Ids that don't survive simple hyphen->underscore normalization because
// the rules-engine scrape and the asset scrape genuinely disagree on naming.
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
    // Direct hit first.
    if (has(id))
        return texture(id);

    // Known mismatches between the rules-engine slug and the asset filename.
    auto& aliases = characterAliases();
    auto aliasIt = aliases.find(id);
    if (aliasIt != aliases.end() && has(aliasIt->second))
        return texture(aliasIt->second);

    // Fall back to hyphen->underscore normalization (covers most cases).
    std::string normalized = id;
    for (char& c : normalized)
        if (c == '-') c = '_';

    if (has(normalized))
        return texture(normalized);

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

    std::string fullPath = joinPath(manifestDirectory, asset->path);

    TraceLog(LOG_INFO,
             "Loading texture [%s]: %s",
             id.c_str(),
             fullPath.c_str());

    if (!FileExists(fullPath.c_str()))
    {
        TraceLog(LOG_ERROR,
                 "Texture file does not exist: %s",
                 fullPath.c_str());
        return false;
    }

    Texture2D texture = LoadTexture(fullPath.c_str());

    if (texture.id == 0)
    {
        TraceLog(LOG_ERROR,
                 "Raylib failed to load texture: %s",
                 fullPath.c_str());
        return false;
    }

    textures[id] = texture;

    return true;
}

std::string AssetManager::directoryOf(const std::string& path) const
{
    const auto position = path.find_last_of("/\\");

    if (position == std::string::npos)
        return ".";

    return path.substr(0, position);
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
    {
        UnloadTexture(texture);
    }

    textures.clear();
}

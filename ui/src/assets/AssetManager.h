#pragma once

#include "raylib.h"

#include <string>
#include <unordered_map>
#include <vector>

struct AssetEntry
{
    std::string path;
    std::string filename;
    std::string normalizedId;
    std::string category;
};

class AssetManager
{
public:
    bool loadManifest(const std::string& manifestPath);

    bool has(const std::string& id) const;
    const AssetEntry* entry(const std::string& id) const;

    Texture2D* texture(const std::string& id);
    Texture2D* character(const std::string& id);
    Texture2D* lightCone(const std::string& id);
    Texture2D* relicSet(const std::string& id);

    // Enemy artwork is name-based in the current asset pack.
    // The optional displayName lets the loader find:
    // assets/prydwen_assets/enemies/<enemy name>.png
    Texture2D* enemy(const std::string& id, const std::string& displayName = "");

    void unloadAll();

private:
    std::string directoryOf(const std::string& path) const;
    std::string joinPath(const std::string& a, const std::string& b) const;
    bool loadTextureFor(const std::string& id);
    bool loadTextureAt(const std::string& key, const std::string& path);

    std::vector<std::string> enemyNameCandidates(const std::string& displayName) const;

    std::unordered_map<std::string, AssetEntry> entries;
    std::unordered_map<std::string, Texture2D> textures;
    std::string manifestDirectory;
};

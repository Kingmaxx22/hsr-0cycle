#pragma once
#include "../assets/AssetManager.h"
#include "raylib.h"
#include <string>

class CharacterCard
{
public:
    static void draw(AssetManager& assets,
                     const std::string& id,
                     Rectangle bounds,
                     bool selected = false);
};

#include "CharacterCard.h"

void CharacterCard::draw(AssetManager& assets,
                         const std::string& id,
                         Rectangle bounds,
                         bool selected)
{
    DrawRectangleRounded(bounds, 0.08f, 8,
        selected ? Color{44, 52, 70, 255} : Color{28, 31, 41, 255});

    Texture2D* tex = assets.character(id);

    if (tex)
    {
        float scale = (bounds.width - 20.0f) / static_cast<float>(tex->width);
        if (tex->height * scale > bounds.height - 40.0f)
            scale = (bounds.height - 40.0f) / static_cast<float>(tex->height);

        float w = tex->width * scale;
        float h = tex->height * scale;

        Rectangle src{0, 0, static_cast<float>(tex->width),
                      static_cast<float>(tex->height)};
        Rectangle dst{
            bounds.x + (bounds.width - w) / 2.0f,
            bounds.y + 8.0f,
            w, h
        };

        DrawTexturePro(*tex, src, dst, {0, 0}, 0, WHITE);
    }

    DrawText(id.c_str(), static_cast<int>(bounds.x + 10),
             static_cast<int>(bounds.y + bounds.height - 25),
             13, RAYWHITE);
}

#include "Panel.h"

void Panel::draw(Rectangle bounds, Color fill, Color border)
{
    DrawRectangleRounded(bounds, 0.08f, 8, fill);

    if (border.a != 0)
        DrawRectangleRoundedLines(bounds, 0.08f, 8, border);
}

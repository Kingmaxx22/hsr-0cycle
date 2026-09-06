#pragma once

#include "raylib.h"

class Sidebar
{
public:
    // Draws the sidebar and returns the nav index clicked this frame, or -1.
    int updateAndDraw(int activeNav) const;

    static Rectangle itemBounds(int index);
    static int itemCount();
};

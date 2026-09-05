#pragma once

class Screen
{
public:
    virtual ~Screen() = default;
    virtual void initialize() {}
    virtual void update(float dt) = 0;
    virtual void draw() = 0;
};

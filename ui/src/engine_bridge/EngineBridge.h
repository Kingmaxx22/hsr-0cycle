#pragma once

// Reserved boundary between the C++ UI and the Python HSR rules engine.
// No combat formulas belong in the UI.

class EngineBridge
{
public:
    bool available() const { return false; }
};

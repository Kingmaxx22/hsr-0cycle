#pragma once

#include "assets/AssetManager.h"
#include "data/CharacterDatabase.h"
#include "screens/TeamBuilderScreen.h"

#include <memory>

class App
{
public:
    App();
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    bool initialize();
    void run();
    void shutdown();

private:
    AssetManager assets;
    CharacterDatabase characters;
    std::unique_ptr<TeamBuilderScreen> teamBuilder;
};
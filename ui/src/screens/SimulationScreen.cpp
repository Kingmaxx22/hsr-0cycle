#include "SimulationScreen.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <cstring>

// Include the simulation engine header
#include "simulation/SimulationEngine.h"
#include "simulation/EngineBridge.h"

SimulationScreen::SimulationScreen(AssetManager& assets, EnemyDatabase& enemies)
    : assets(assets), enemies(enemies)
    , isRunning(false)
    , showResults(false)
    , scrollOffset(0)
    , hoveredActionIndex(-1)
    , zoomLevel(1.0f)
    , usePythonEngine(false)
    , isEditingCharacter(false)
    , editingIndex(0)
    , backRequested(false)
{
    std::strcpy(speedInput, "100");
    std::strcpy(rotationInput, "Skill,Basic,Basic");
}

SimulationScreen::~SimulationScreen() {
    resetSimulation();
}

void SimulationScreen::initialize() {
    resetSimulation();
    
    // Add some default characters for testing
    hsr::CharacterConfig testChar;
    testChar.id = "test_dps";
    testChar.name = "Test DPS";
    testChar.speed = 134;
    testChar.maxSp = 5;
    testChar.currentSp = 3;
    testChar.energy = 50;
    testChar.maxEnergy = 120;
    testChar.rotation = {"Skill", "Basic", "Basic"};
    testChar.isAuto = false;
    
    characters.push_back(testChar);
    
    std::cout << "[SimulationScreen] Initialized with " << characters.size() << " character(s)" << std::endl;
}

void SimulationScreen::setSelectedEnemy(const std::string& enemyId) {
    selectedEnemyId = enemyId;
    
    // Load enemy data from database
    const EnemyInfo* enemyInfo = enemies.get(enemyId);
    if (enemyInfo) {
        currentEnemy.id = enemyInfo->id;
        currentEnemy.name = enemyInfo->name;
        currentEnemy.maxHp = static_cast<int>(enemyInfo->hp);
        currentEnemy.currentHp = static_cast<int>(enemyInfo->hp);
        currentEnemy.toughness = static_cast<int>(enemyInfo->toughness);
        
        // Get physical resistance if available, otherwise default to 0
        auto resIt = enemyInfo->resistances.find("Physical");
        currentEnemy.resistance = (resIt != enemyInfo->resistances.end()) ? resIt->second : 0.0f;
        
        std::cout << "[SimulationScreen] Set enemy: " << currentEnemy.name << std::endl;
    }
}

void SimulationScreen::addCharacter(const hsr::CharacterConfig& config) {
    characters.push_back(config);
    std::cout << "[SimulationScreen] Added character: " << config.name << std::endl;
}

void SimulationScreen::removeCharacter(size_t index) {
    if (index < characters.size()) {
        characters.erase(characters.begin() + index);
        std::cout << "[SimulationScreen] Removed character at index " << index << std::endl;
    }
}

void SimulationScreen::clearCharacters() {
    characters.clear();
    resetSimulation();
    std::cout << "[SimulationScreen] Cleared all characters" << std::endl;
}

void SimulationScreen::resetSimulation() {
    isRunning = false;
    showResults = false;
    scrollOffset = 0;
    hoveredActionIndex = -1;
    lastResult = hsr::SimulationResult();
}

void SimulationScreen::toggleEngine() {
    usePythonEngine = !usePythonEngine;
    std::cout << "[SimulationScreen] Switched to " 
              << (usePythonEngine ? "Python" : "C++") << " engine" << std::endl;
}

void SimulationScreen::runSimulation() {
    if (characters.empty()) {
        std::cerr << "[SimulationScreen] No characters to simulate!" << std::endl;
        return;
    }
    
    if (selectedEnemyId.empty()) {
        std::cerr << "[SimulationScreen] No enemy selected!" << std::endl;
        return;
    }
    
    isRunning = true;
    showResults = false;
    
    std::cout << "[SimulationScreen] Running simulation with " 
              << characters.size() << " character(s) against " 
              << currentEnemy.name << std::endl;
    
    // Run simulation using C++ engine (Python bridge would be used if implemented)
    hsr::SimulationEngine engine;
    lastResult = engine.runSimulation(characters, currentEnemy, 15000);
    
    isRunning = false;
    showResults = true;
    
    std::cout << "[SimulationScreen] Simulation complete!" << std::endl;
    std::cout << "  - Zero Cycle Clear: " << (lastResult.isZeroCycleClear ? "YES" : "NO") << std::endl;
    std::cout << "  - Total Actions: " << lastResult.totalActions << std::endl;
    std::cout << "  - Total Damage: " << lastResult.totalDamage << std::endl;
    std::cout << "  - Timeline entries: " << lastResult.timeline.size() << std::endl;
}

Rectangle SimulationScreen::headerBounds() {
    return {50, 20, GetScreenWidth() - 100, 60};
}

Rectangle SimulationScreen::timelineBounds() {
    float headerHeight = 80;
    float controlsHeight = 100;
    return {50, headerHeight, GetScreenWidth() - 100, 300};
}

Rectangle SimulationScreen::controlsBounds() {
    float timelineBottom = 80 + 300;
    return {50, timelineBottom + 20, GetScreenWidth() - 100, 80};
}

Rectangle SimulationScreen::resultsBounds() {
    float controlsBottom = 80 + 300 + 20 + 80;
    return {50, controlsBottom + 20, GetScreenWidth() - 100, 200};
}

Rectangle SimulationScreen::characterSlotBounds(size_t index) {
    float startX = 50;
    float startY = 80 + 300 + 20 + 80 + 20;
    float slotWidth = 200;
    float slotHeight = 60;
    float spacing = 10;
    
    return {
        startX + (slotWidth + spacing) * index,
        startY,
        slotWidth,
        slotHeight
    };
}

void SimulationScreen::update(float dt) {
    // Handle keyboard input
    if (IsKeyPressed(KEY_R)) {
        runSimulation();
    }
    
    if (IsKeyPressed(KEY_E)) {
        toggleEngine();
    }
    
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_B)) {
        backRequested = true;
    }
    
    // Handle mouse wheel for scrolling timeline
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        scrollOffset += static_cast<int>(wheel * 20);
        scrollOffset = std::max(0, scrollOffset);
    }
    
    // Update hovered action index
    Rectangle timelineRect = timelineBounds();
    Vector2 mousePos = GetMousePosition();
    
    if (CheckCollisionPointRec(mousePos, timelineRect)) {
        // Calculate which action is hovered
        if (!lastResult.timeline.empty()) {
            float actionWidth = 40 * zoomLevel;
            float relativeX = mousePos.x - timelineRect.x - scrollOffset;
            hoveredActionIndex = static_cast<int>(relativeX / actionWidth);
            
            if (hoveredActionIndex < 0 || hoveredActionIndex >= static_cast<int>(lastResult.timeline.size())) {
                hoveredActionIndex = -1;
            }
        }
    } else {
        hoveredActionIndex = -1;
    }
}

bool SimulationScreen::consumeBackRequest() {
    bool result = backRequested;
    backRequested = false;
    return result;
}

void SimulationScreen::draw() {
    // Draw header
    DrawRectangleRec(headerBounds(), DARKGRAY);
    DrawText("0-Cycle Simulation", headerBounds().x + 10, headerBounds().y + 20, 24, WHITE);
    
    std::string enemyText = "Enemy: " + (currentEnemy.name.empty() ? "None Selected" : currentEnemy.name);
    DrawText(enemyText.c_str(), headerBounds().x + 10, headerBounds().y + 45, 16, LIGHTGRAY);
    
    std::string engineText = usePythonEngine ? "Engine: Python" : "Engine: C++";
    DrawText(engineText.c_str(), headerBounds().x + GetScreenWidth() - 200, headerBounds().y + 20, 16, GREEN);
    
    // Draw timeline
    drawTimeline();
    
    // Draw controls
    drawControls();
    
    // Draw results if available
    if (showResults) {
        drawResults();
    }
    
    // Draw character slots
    drawCharacterSlots();
    
    // Draw tooltip if hovering over action
    if (hoveredActionIndex >= 0 && hoveredActionIndex < static_cast<int>(lastResult.timeline.size())) {
        drawActionTooltip(lastResult.timeline[hoveredActionIndex]);
    }
    
    // Draw instructions
    DrawText("Press R to Run | E to Toggle Engine | ESC to Reset", 50, GetScreenHeight() - 30, 16, GRAY);
}

void SimulationScreen::drawTimeline() {
    Rectangle rect = timelineBounds();
    
    // Background
    DrawRectangleRec(rect, Fade(BLACK, 0.3f));
    DrawRectangleLinesEx(rect, 2, GRAY);
    
    // Draw AV markers (0, 50, 100, 150)
    float markerInterval = rect.width / 3.0f;
    for (int i = 0; i <= 3; ++i) {
        float x = rect.x + i * markerInterval;
        DrawLineV({x, rect.y}, {x, rect.y + rect.height}, Fade(GRAY, 0.3f));
        
        std::string label = std::to_string(i * 50) + " AV";
        DrawText(label.c_str(), x - 20, rect.y + 5, 14, GRAY);
    }
    
    // Draw 150 AV limit line
    float limitX = rect.x + rect.width - 5;
    DrawLineV({limitX, rect.y}, {limitX, rect.y + rect.height}, RED);
    DrawText("150 AV Limit", limitX - 50, rect.y + rect.height - 20, 12, RED);
    
    // Draw action timeline
    if (!lastResult.timeline.empty()) {
        float actionWidth = 40 * zoomLevel;
        float spacing = 5;
        
        for (size_t i = 0; i < lastResult.timeline.size(); ++i) {
            const auto& action = lastResult.timeline[i];
            
            // Calculate position based on AV
            float avPercent = static_cast<float>(action.currentAv) / 15000.0f;
            float x = rect.x + avPercent * rect.width - actionWidth / 2;
            float y = rect.y + 40 + (i % 4) * 35; // Stagger multiple actions
            
            // Determine color based on action type
            Color actionColor = BLUE;
            if (action.actionType == "Skill") actionColor = ORANGE;
            else if (action.actionType == "Ult") actionColor = PURPLE;
            else if (action.actionType == "FUA") actionColor = PINK;
            
            // Draw action box
            Rectangle actionRect = {x, y, actionWidth, 25};
            DrawRectangleRec(actionRect, actionColor);
            DrawRectangleLinesEx(actionRect, 1, WHITE);
            
            // Draw character initial or short name
            std::string charLabel = action.characterName.substr(0, 3);
            int textWidth = MeasureText(charLabel.c_str(), 12);
            DrawText(charLabel.c_str(), x + actionWidth/2 - textWidth/2, y + 7, 12, WHITE);
            
            // Highlight if hovered
            if (static_cast<int>(i) == hoveredActionIndex) {
                DrawRectangleLinesEx(actionRect, 3, YELLOW);
            }
        }
    } else {
        // Show placeholder text
        const char* placeholder = "No simulation data - Press R to run";
        int width = MeasureText(placeholder, 20);
        DrawText(placeholder, rect.x + rect.width/2 - width/2, rect.y + rect.height/2 - 10, 20, GRAY);
    }
}

void SimulationScreen::drawCharacterSlots() {
    float startY = 80 + 300 + 20 + 80 + 20;
    
    DrawText("Characters:", 50, startY - 20, 18, WHITE);
    
    for (size_t i = 0; i < characters.size(); ++i) {
        Rectangle slot = characterSlotBounds(i);
        
        // Slot background
        DrawRectangleRec(slot, Fade(DARKBLUE, 0.5f));
        DrawRectangleLinesEx(slot, 2, BLUE);
        
        // Character info
        std::string name = characters[i].name;
        std::string spd = "SPD: " + std::to_string(characters[i].speed);
        
        DrawText(name.c_str(), slot.x + 10, slot.y + 10, 16, WHITE);
        DrawText(spd.c_str(), slot.x + 10, slot.y + 30, 14, LIGHTGRAY);
        
        // Remove button (small X)
        Rectangle removeBtn = {slot.x + slot.width - 30, slot.y + 5, 25, 25};
        DrawRectangleRec(removeBtn, RED);
        DrawText("X", removeBtn.x + 8, removeBtn.y + 5, 18, WHITE);
        
        // Check if clicked
        if (CheckCollisionPointRec(GetMousePosition(), removeBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            removeCharacter(i);
            break;
        }
    }
    
    // Add character button (if less than 4)
    if (characters.size() < 4) {
        float addX = 50 + (200 + 10) * characters.size();
        Rectangle addBtn = {addX, startY, 200, 60};
        DrawRectangleRec(addBtn, Fade(GREEN, 0.3f));
        DrawRectangleLinesEx(addBtn, 2, GREEN);
        DrawText("+ Add Character", addBtn.x + 30, addBtn.y + 20, 18, GREEN);
    }
}

void SimulationScreen::drawControls() {
    Rectangle rect = controlsBounds();
    
    // Background panel
    DrawRectangleRec(rect, Fade(DARKGRAY, 0.8f));
    DrawRectangleLinesEx(rect, 2, GRAY);
    
    // Run button
    Rectangle runBtn = {rect.x + 20, rect.y + 20, 150, 40};
    Color runColor = isRunning ? GRAY : GREEN;
    DrawRectangleRec(runBtn, runColor);
    DrawRectangleLinesEx(runBtn, 2, WHITE);
    
    const char* runText = isRunning ? "Running..." : "Run Simulation (R)";
    int runWidth = MeasureText(runText, 20);
    DrawText(runText, runBtn.x + runBtn.width/2 - runWidth/2, runBtn.y + 10, 20, WHITE);
    
    // Check if clicked
    if (CheckCollisionPointRec(GetMousePosition(), runBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !isRunning) {
        runSimulation();
    }
    
    // Engine toggle button
    Rectangle engineBtn = {rect.x + 200, rect.y + 20, 150, 40};
    Color engineColor = usePythonEngine ? PURPLE : BLUE;
    DrawRectangleRec(engineBtn, engineColor);
    DrawRectangleLinesEx(engineBtn, 2, WHITE);
    
    std::string engineText = usePythonEngine ? "Python Engine" : "C++ Engine";
    int engineWidth = MeasureText(engineText.c_str(), 18);
    DrawText(engineText.c_str(), engineBtn.x + engineBtn.width/2 - engineWidth/2, engineBtn.y + 12, 18, WHITE);
    
    // Check if clicked
    if (CheckCollisionPointRec(GetMousePosition(), engineBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        toggleEngine();
    }
    
    // Zoom controls
    DrawText("Zoom:", rect.x + 400, rect.y + 25, 18, WHITE);
    
    Rectangle zoomOutBtn = {rect.x + 470, rect.y + 20, 30, 40};
    DrawRectangleRec(zoomOutBtn, GRAY);
    DrawText("-", zoomOutBtn.x + 12, zoomOutBtn.y + 10, 24, WHITE);
    
    Rectangle zoomInBtn = {rect.x + 510, rect.y + 20, 30, 40};
    DrawRectangleRec(zoomInBtn, GRAY);
    DrawText("+", zoomInBtn.x + 10, zoomInBtn.y + 10, 24, WHITE);
    
    if (CheckCollisionPointRec(GetMousePosition(), zoomOutBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        zoomLevel = std::max(0.5f, zoomLevel - 0.1f);
    }
    
    if (CheckCollisionPointRec(GetMousePosition(), zoomInBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        zoomLevel = std::min(2.0f, zoomLevel + 0.1f);
    }
}

void SimulationScreen::drawResults() {
    Rectangle rect = resultsBounds();
    
    // Background panel
    DrawRectangleRec(rect, Fade(BLACK, 0.7f));
    DrawRectangleLinesEx(rect, 2, lastResult.isZeroCycleClear ? GOLD : GRAY);
    
    // Title
    const char* title = lastResult.isZeroCycleClear ? "✓ 0-CYCLE CLEAR!" : "✗ Not a 0-Cycle Clear";
    Color titleColor = lastResult.isZeroCycleClear ? GOLD : RED;
    int titleWidth = MeasureText(title, 24);
    DrawText(title, rect.x + rect.width/2 - titleWidth/2, rect.y + 10, 24, titleColor);
    
    // Stats
    int yPos = rect.y + 50;
    int lineHeight = 25;
    
    std::string actionsStr = "Total Actions: " + std::to_string(lastResult.totalActions);
    DrawText(actionsStr.c_str(), rect.x + 20, yPos, 18, WHITE);
    
    std::string damageStr = "Total Damage: " + std::to_string(static_cast<int>(lastResult.totalDamage));
    DrawText(damageStr.c_str(), rect.x + 20, yPos + lineHeight, 18, WHITE);
    
    std::string breakStr = "Break Damage: " + std::to_string(static_cast<int>(lastResult.totalBreakDamage));
    DrawText(breakStr.c_str(), rect.x + 20, yPos + lineHeight * 2, 18, WHITE);
    
    std::string cyclesStr = "Cycles: " + std::to_string(lastResult.totalCycles);
    DrawText(cyclesStr.c_str(), rect.x + 20, yPos + lineHeight * 3, 18, WHITE);
    
    // Timeline summary
    std::string timelineStr = "Timeline: " + std::to_string(lastResult.timeline.size()) + " actions";
    DrawText(timelineStr.c_str(), rect.x + 300, yPos, 18, LIGHTGRAY);
    
    if (lastResult.success) {
        std::string avUsed = "AV Used: " + std::to_string(lastResult.timeline.back().currentAv / 100) + "." + 
                            std::to_string(lastResult.timeline.back().currentAv % 100);
        DrawText(avUsed.c_str(), rect.x + 300, yPos + lineHeight, 18, LIGHTGRAY);
    }
}

void SimulationScreen::drawActionTooltip(const hsr::ActionEvent& action) {
    Vector2 mousePos = GetMousePosition();
    
    // Tooltip dimensions
    int width = 200;
    int height = 120;
    float x = mousePos.x + 15;
    float y = mousePos.y + 15;
    
    // Keep tooltip on screen
    if (x + width > GetScreenWidth()) x = mousePos.x - width - 15;
    if (y + height > GetScreenHeight()) y = mousePos.y - height - 15;
    
    // Background
    DrawRectangle(x, y, width, height, Fade(BLACK, 0.9f));
    DrawRectangleLines(x, y, width, height, WHITE);
    
    // Content
    int yPos = y + 10;
    int lineHeight = 20;
    
    DrawText(action.characterName.c_str(), x + 10, yPos, 16, YELLOW);
    DrawText(("Action: " + action.actionType).c_str(), x + 10, yPos + lineHeight, 14, WHITE);
    DrawText(("AV Cost: " + std::to_string(action.avCost / 100) + "." + 
             std::to_string(action.avCost % 100)).c_str(), x + 10, yPos + lineHeight * 2, 14, WHITE);
    DrawText(("AV Time: " + std::to_string(action.currentAv / 100) + "." + 
             std::to_string(action.currentAv % 100)).c_str(), x + 10, yPos + lineHeight * 3, 14, WHITE);
    DrawText(("Damage: " + std::to_string(action.damageDealt)).c_str(), x + 10, yPos + lineHeight * 4, 14, GREEN);
    DrawText(("SP Change: " + std::to_string(action.spChange)).c_str(), x + 10, yPos + lineHeight * 5, 14, 
             action.spChange > 0 ? GREEN : (action.spChange < 0 ? RED : WHITE));
}

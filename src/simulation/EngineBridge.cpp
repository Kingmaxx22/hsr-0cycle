#include "EngineBridge.h"
#include <iostream>
#include <sstream>

// Python C API headers
#define PY_SSIZE_T_CLEAN
#include <Python.h>

namespace hsr {

EngineBridge::EngineBridge() 
    : m_initialized(false), m_pPythonState(nullptr) {
}

EngineBridge::~EngineBridge() {
    shutdown();
}

bool EngineBridge::initialize() {
    if (m_initialized) {
        return true;
    }

    // Initialize Python interpreter
    Py_Initialize();
    
    if (!Py_IsInitialized()) {
        std::cerr << "[EngineBridge] Failed to initialize Python interpreter" << std::endl;
        return false;
    }

    // Add the engine directory to Python path
    PyObject* sysPath = PySys_GetObject("path");
    if (sysPath) {
        PyList_Append(sysPath, PyUnicode_FromString("/workspace/engine/hsr_engine"));
    }

    // Try to import the simulation module
    PyObject* pModule = PyImport_ImportModule("hsr_engine.simulation");
    if (!pModule) {
        PyErr_Print();
        std::cerr << "[EngineBridge] Failed to import hsr_engine.simulation module" << std::endl;
        // Don't fail completely - we can still use C++ engine
        m_initialized = true;
        return true;
    }
    
    Py_DECREF(pModule);
    m_initialized = true;
    
    std::cout << "[EngineBridge] Python integration initialized successfully" << std::endl;
    return true;
}

void EngineBridge::shutdown() {
    if (Py_IsInitialized()) {
        Py_Finalize();
    }
    m_initialized = false;
}

bool EngineBridge::isReady() const {
    return m_initialized;
}

std::string EngineBridge::buildPythonCharacterDict(const CharacterConfig& config) {
    std::stringstream ss;
    ss << "{";
    ss << "\"id\": \"" << config.id << "\",";
    ss << "\"name\": \"" << config.name << "\",";
    ss << "\"speed\": " << config.speed << ",";
    ss << "\"max_sp\": " << config.maxSp << ",";
    ss << "\"current_sp\": " << config.currentSp << ",";
    ss << "\"energy\": " << config.energy << ",";
    ss << "\"max_energy\": " << config.maxEnergy << ",";
    
    // Build rotation array
    ss << "\"rotation\": [";
    for (size_t i = 0; i < config.rotation.size(); ++i) {
        ss << "\"" << config.rotation[i] << "\"";
        if (i < config.rotation.size() - 1) ss << ", ";
    }
    ss << "],";
    
    ss << "\"is_auto\": " << (config.isAuto ? "true" : "false");
    ss << "}";
    
    return ss.str();
}

std::string EngineBridge::buildPythonEnemyDict(const EnemyConfig& config) {
    std::stringstream ss;
    ss << "{";
    ss << "\"id\": \"" << config.id << "\",";
    ss << "\"name\": \"" << config.name << "\",";
    ss << "\"max_hp\": " << config.maxHp << ",";
    ss << "\"current_hp\": " << config.currentHp << ",";
    ss << "\"toughness\": " << config.toughness << ",";
    ss << "\"resistance\": " << config.resistance;
    ss << "}";
    
    return ss.str();
}

SimulationResult EngineBridge::parsePythonResult(const std::string& jsonResult) {
    // Simple JSON parsing (in production, use a proper JSON library like nlohmann/json)
    SimulationResult result;
    result.success = false;
    result.isZeroCycleClear = false;
    result.totalActions = 0;
    result.totalDamage = 0.0f;
    result.totalBreakDamage = 0.0f;
    
    // Very basic parsing - look for key fields
    if (jsonResult.find("\"success\": true") != std::string::npos ||
        jsonResult.find("\"success\":true") != std::string::npos) {
        result.success = true;
    }
    
    if (jsonResult.find("\"zero_cycle_clear\": true") != std::string::npos ||
        jsonResult.find("\"zero_cycle_clear\":true") != std::string::npos) {
        result.isZeroCycleClear = true;
    }
    
    // Extract timeline actions (simplified)
    size_t timelinePos = jsonResult.find("\"timeline\"");
    if (timelinePos != std::string::npos) {
        // Count action objects in timeline array
        size_t arrayStart = jsonResult.find('[', timelinePos);
        size_t arrayEnd = jsonResult.find(']', arrayStart);
        if (arrayStart != std::string::npos && arrayEnd != std::string::npos) {
            std::string timelineStr = jsonResult.substr(arrayStart, arrayEnd - arrayStart + 1);
            // Count occurrences of "action_type" to estimate number of actions
            size_t pos = 0;
            while ((pos = timelineStr.find("action_type", pos)) != std::string::npos) {
                result.totalActions++;
                pos++;
            }
        }
    }
    
    return result;
}

SimulationResult EngineBridge::runPythonSimulation(
    const std::vector<CharacterConfig>& characters,
    const EnemyConfig& enemy,
    int avLimit) {
    
    // First try with Python engine
    if (m_initialized && Py_IsInitialized()) {
        PyObject *pName, *pModule, *pFunc, *pArgs, *pValue;
        
        pName = PyUnicode_DecodeFSDefault("hsr_engine.simulation");
        pModule = PyImport_Import(pName);
        Py_DECREF(pName);
        
        if (pModule) {
            pFunc = PyObject_GetAttrString(pModule, "run_simulation");
            
            if (pFunc && PyCallable_Check(pFunc)) {
                // Build arguments
                pArgs = PyTuple_New(3);
                
                // Build characters list
                PyObject* pChars = PyList_New(characters.size());
                for (size_t i = 0; i < characters.size(); ++i) {
                    std::string charJson = buildPythonCharacterDict(characters[i]);
                    PyObject* pCharStr = PyUnicode_FromString(charJson.c_str());
                    PyList_SetItem(pChars, i, pCharStr);
                }
                PyTuple_SetItem(pArgs, 0, pChars);
                
                // Build enemy dict
                std::string enemyJson = buildPythonEnemyDict(enemy);
                PyObject* pEnemyStr = PyUnicode_FromString(enemyJson.c_str());
                PyTuple_SetItem(pArgs, 1, pEnemyStr);
                
                // AV limit
                PyTuple_SetItem(pArgs, 2, PyLong_FromLong(avLimit));
                
                pValue = PyObject_CallObject(pFunc, pArgs);
                
                if (pValue) {
                    // Convert result to string for parsing
                    PyObject* pStr = PyObject_Str(pValue);
                    const char* cStr = PyUnicode_AsUTF8(pStr);
                    
                    SimulationResult result = parsePythonResult(cStr);
                    
                    Py_DECREF(pStr);
                    Py_DECREF(pValue);
                    Py_DECREF(pArgs);
                    Py_DECREF(pFunc);
                    Py_DECREF(pModule);
                    
                    return result;
                } else {
                    PyErr_Print();
                    std::cerr << "[EngineBridge] Python simulation call failed" << std::endl;
                    Py_DECREF(pArgs);
                    Py_DECREF(pFunc);
                    Py_DECREF(pModule);
                }
            } else {
                if (PyErr_Occurred()) PyErr_Print();
                std::cerr << "[EngineBridge] Cannot find run_simulation function" << std::endl;
                Py_XDECREF(pFunc);
                Py_DECREF(pModule);
            }
        } else {
            PyErr_Print();
            std::cerr << "[EngineBridge] Failed to load simulation module" << std::endl;
        }
    }
    
    // Fallback to C++ engine
    std::cout << "[EngineBridge] Using C++ simulation engine as fallback" << std::endl;
    SimulationEngine cppEngine;
    return cppEngine.runSimulation(characters, enemy, avLimit);
}

std::vector<SimulationEngine::SpeedBreakpoint> EngineBridge::getPythonBreakpoints(
    int baseSpeed, int avLimit) {
    
    // For now, use C++ implementation
    // Could be extended to call Python version if needed
    SimulationEngine engine;
    return engine.calculateBreakpoints(baseSpeed, avLimit);
}

CharacterConfig EngineBridge::parseCharacterFromJson(const std::string& json) {
    // Simple JSON parsing (use proper library in production)
    CharacterConfig config;
    config.id = "unknown";
    config.name = "Unknown";
    config.speed = 100;
    config.maxSp = 5;
    config.currentSp = 3;
    config.energy = 0;
    config.maxEnergy = 100;
    config.isAuto = true;
    
    // Basic field extraction (simplified)
    size_t pos = json.find("\"speed\"");
    if (pos != std::string::npos) {
        size_t colon = json.find(':', pos);
        if (colon != std::string::npos) {
            config.speed = std::stoi(json.substr(colon + 1));
        }
    }
    
    return config;
}

EnemyConfig EngineBridge::parseEnemyFromJson(const std::string& json) {
    EnemyConfig config;
    config.id = "unknown";
    config.name = "Unknown";
    config.maxHp = 100000;
    config.currentHp = 100000;
    config.toughness = 180;
    config.resistance = 0.1f;
    
    return config;
}

} // namespace hsr

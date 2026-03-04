#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <Adafruit_INA3221.h>
#include <Adafruit_MCP23X17.h>
#include <Adafruit_NeoPixel.h>
#include <vector>

// Mutexes
extern SemaphoreHandle_t i2cMutex;
extern SemaphoreHandle_t dataMutex;
extern SemaphoreHandle_t logMutex; 
extern SemaphoreHandle_t NeoPixelMutex;

// Task Priorities
#define PRIORITY_STARTUP_TASK 5
#define PRIORITY_WEBSERVER_TASK 4
#define PRIORITY_POWER_SENSING_TASK 3
#define PRIORITY_CONTROL_GPIO_TASK 3
#define PRIORITY_NEOPIXEL_TASK 2
#define PRIORITY_DEBUG_TASK 1
#define PRIORITY_MPPT_TASK 3
#define PRIORITY_DEEPSLEEP_TASK 2

// Task stack sizes
#define STACK_SIZE_STARTUP_TASK 4096
#define STACK_SIZE_WEBSERVER_TASK 8192
#define STACK_SIZE_POWER_SENSING_TASK 4096
#define STACK_SIZE_CONTROL_GPIO_TASK 4096
#define STACK_SIZE_NEOPIXEL_TASK 4096
#define STACK_SIZE_DEBUG_TASK 4096
#define STACK_SIZE_MPPT_TASK 4096
#define STACK_SIZE_DEEPSLEEP_TASK 4096

// Task frequencies (in Hz)
#define FREQUENCY_POWER_SENSING_TASK 100
#define FREQUENCY_CONTROL_GPIO_TASK 100
#define FREQUENCY_NEOPIXEL_TASK 100
#define FREQUENCY_MPPT_TASK 10
#define FREQUENCY_DEEPSLEEP_TASK 10

// Task periods (in FreeRTOS ticks)
#define PERIOD_POWER_SENSING_TASK pdMS_TO_TICKS(1000 / FREQUENCY_POWER_SENSING_TASK)
#define PERIOD_CONTROL_GPIO_TASK pdMS_TO_TICKS(1000 / FREQUENCY_CONTROL_GPIO_TASK)
#define PERIOD_NEOPIXEL_TASK pdMS_TO_TICKS(1000 / FREQUENCY_NEOPIXEL_TASK)
#define PERIOD_MPPT_TASK pdMS_TO_TICKS(1000 / FREQUENCY_MPPT_TASK)
#define PERIOD_DEEPSLEEP_TASK pdMS_TO_TICKS(1000 / FREQUENCY_DEEPSLEEP_TASK)

// Data Structures
struct SystemState {
    // INA3221 Data
    float busVoltage[3]; 
    float current_mA[3];
    float power_mW[3];
    
    // MCP23017 Status
    int solarActiveCount;
    int batteryActiveCount;
    bool constantLoadOn;
    bool nightLoadOn;
    bool heavyLoadOn;

    bool isSimActive;           // Ist der Zyklus aktiv?
    bool isDayPhase;            // Sind wir gerade am Tag?
    unsigned long simTimerStart; // Wann hat die aktuelle Phase begonnen?
    int dayDurationSec;         // Dauer Tag in Sekunden
    int nightDurationSec;       // Dauer Nacht in Sekunden

    // --- Gespeicherte Start-Konfiguration ---
    int configSolarCount;   // User-Einstellung für Solar
    int configBatteryCount; // User-Einstellung für Kondensatoren (NEU)
    // Zeitpläne für fiktive Simulation (AUTO-Modus)
    bool schedConstActive;
    int schedConstStartH, schedConstStartM;
    int schedConstEndH, schedConstEndM;

    bool schedNightActive;
    int schedNightStartH, schedNightStartM;
    int schedNightEndH, schedNightEndM;

    bool schedHeavyActive;
    int schedHeavyStartH, schedHeavyStartM;
    int schedHeavyEndH, schedHeavyEndM;
    // --- NEU: Zyklen ---
    int targetCycles;       // Wie viele Zyklen sollen laufen?
    int currentCycle;       // Wo sind wir gerade?

    bool mppt_auto_mode = true; // true = MPPT Algorithmus, false = Manueller Slider
    int manual_pwm_value = 240; // Startwert für manuell (240 = wenig Leistung/Start)
};

// Struktur für einen Datenpunkt
struct SimDataPoint {
    unsigned long timestamp; // Zeit in ms seit Start der Sim
    float vSolar, iSolar;
    float vBat, iBat;
};

// Global shared data 
extern Adafruit_INA3221 CurrentSensor;
extern Adafruit_MCP23X17 GPIOExpander;
extern Adafruit_NeoPixel Neopixels;
extern SystemState sysState;

extern std::vector<SimDataPoint> simulationLog; // <--- NEU
extern SemaphoreHandle_t logMutex;              // <--- NEU
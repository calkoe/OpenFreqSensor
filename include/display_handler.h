#ifndef DISPLAY_HANDLER_H
#define DISPLAY_HANDLER_H

#include <LiquidCrystal_I2C.h>
#include <Arduino.h>
#include "frequency_analyzer.h"  // For FrequencyAnalysis
#include "frequency_interpreter.h"  // For FrequencyAlert
#include "config.h"

// Custom icons (5x8)
static byte wifiIcon[8] = {
  B00000,
  B01110,
  B10001,
  B00100,
  B01010,
  B00000,
  B00100,
  B00000
};

static byte mqttIcon[8] = {
  B00000,
  B00100,
  B01110,
  B10101,
  B00100,
  B00100,
  B00100,
  B00000
};

static byte clockIcon[8] = {
  B00000,
  B11111,
  B00100,
  B00100,
  B00100,
  B00100,
  B00100,
  B00000
};

static byte full[8] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B00000,
};

static byte horzLine[8] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B11111,
  B00000,
};

class DisplayHandler {
public:
    DisplayHandler();  // Private constructor for singleton
    void begin();  // Added initialization method
    void updateWifiStatus(bool status); 
    void updateMqttStatus(bool status); 
    void updateNTPStatus(bool status); 
    void updateAnalysis(const FrequencyAnalysis& analysis);  // Modified to accept FrequencyAnalysis
    void addAlert(FrequencyAlert alert);  // Added overload for FrequencyAlert
    void handleUpButton();
    void handleDownButton();
    void handleMuteButton();
    void loop();

private:
    LiquidCrystal_I2C lcd;
    bool hasAnalysis;
    FrequencyAnalysis currentAnalysis;
    bool currentWifiStatus;
    bool currentMqttStatus;
    bool currentNTPStatus;
    bool needsUpdate;
    unsigned long lastAlarmAdded;
    FrequencyAlert alarmHistory[MAX_ALARMS];
    uint16_t writeIndex;
    bool writeOverflow;
    uint16_t numAlarms;
    uint16_t scrollPosition;
    void drawFrequencyBar(uint8_t row, float value);
};

#endif // DISPLAY_HANDLER_H
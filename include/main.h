#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include "config.h"
#include "networking.h"
#include "frequency_analyzer.h"
#include "frequency_interpreter.h"
#include "frequency_transmitter.h"
#include "display_handler.h"

// Global variables
extern hw_timer_t* timer;
extern uint16_t ringBuffer[];
extern unsigned long lastAnalysis;
extern Networking* networking;
extern FrequencyAnalyzer* analyzer;
extern FrequencyInterpreter* interpreter;
extern FrequencyTransmitter* transmitter;
extern DisplayHandler* display;

#endif // MAIN_H
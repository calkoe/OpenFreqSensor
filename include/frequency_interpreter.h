#ifndef FREQUENCY_INTERPRETER_H
#define FREQUENCY_INTERPRETER_H

#include "frequency_analyzer.h"
#include "frequency_analyzer.h"
#include "config.h"

// Interpreter Constants
struct FrequencyAlert {
    bool valid;
    bool hasAlert;
    const char* alertType;
    FrequencyAnalysis frequencyAnalysis;
    float deviation;
    float ramp;
    char message[LCD_COLS];
    unsigned long analyzingDelay;
};

class FrequencyInterpreter {
    public:
        FrequencyInterpreter();
        FrequencyAlert interpret(const FrequencyAnalysis& analysis);

    private:
        unsigned long lastRun{0};
        float lastFreq{0};
        float lastRamp{0};
        uint8_t warmup{10};
};

#endif // FREQUENCY_INTERPRETER_H
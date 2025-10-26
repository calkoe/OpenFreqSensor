#ifndef FREQUENCY_ANALYZER_H
#define FREQUENCY_ANALYZER_H

#include <Arduino.h>
#include <arduinoFFT.h>
#include "config.h"


struct AdcDataSlice {
    uint16_t adcData[ANALYSIS_SIZE];
    unsigned long millis;   // Time of Measurement
    struct timeval time;    // Time of measurement with microsecond precision
};

struct FrequencyAnalysis {
    double frequency;        // Detected frequency in Hz
    double amplitude;       // Signal amplitude
    double quality;          // Quality metric of the measurement
    double rawFrequency;     // Raw frequency before correction
    bool isValidSignal;     // Indicates if the signal amplitude is above threshold
    unsigned long millis;   // Time of Measurement
    struct timeval time;    // Time of measurement with microsecond precision
};

class FrequencyAnalyzer {
public:
    FrequencyAnalyzer();
    IRAM_ATTR void addDatapoint(uint16_t value);
    bool getNextSliceAnalysis(FrequencyAnalysis*);

private:
    
    // Ring buffer management ISR CONTEXT
    QueueHandle_t adcDataSliceQueue;
    uint16_t ringBuffer[RING_BUFFER_SIZE]{0};
    volatile uint32_t writeIndex __attribute__((aligned(4))){0};  // 32-bit aligned for atomic access
    unsigned long lastSliceCopy{0};

    // Analyzing Management
    double frequencyAvg{50};
    double interpolateFrequency(double* vReal, uint16_t maxIndex, double maxAmplitude);
    double calculateBinError(double p);

};

#endif // FREQUENCY_ANALYZER_H
#ifndef FREQUENCY_ANALYZER_H
#define FREQUENCY_ANALYZER_H

#include <Arduino.h>
#include <arduinoFFT.h>
#include "config.h"

struct FrequencyAnalysis {
    float frequency;        // Detected frequency in Hz
    double amplitude;       // Signal amplitude
    float quality;          // Quality metric of the measurement
    float rawFrequency;     // Raw frequency before correction
    bool isValidSignal;     // Indicates if the signal amplitude is above threshold
    unsigned long millis;   // Time of Measurement
    struct timeval time;    // Time of measurement with microsecond precision
};

class FrequencyAnalyzer {
public:
    FrequencyAnalyzer();
    FrequencyAnalysis analyze();
    IRAM_ATTR void addDatapoint(uint16_t value);

private:
    arduinoFFT FFT;
    double* vReal;
    double* vImag;
    const uint16_t analysisSize = ANALYSIS_SIZE;
    const uint16_t samplingFreq = SAMPLING_FREQUENCY;
    
    // Ring buffer management
    uint16_t ringBuffer[RING_BUFFER_SIZE];
    volatile uint32_t writeIndex __attribute__((aligned(4)));  // 32-bit aligned for atomic access
    uint16_t getReadStartIndex();
    void copyDataToAnalysisBuffer(double* buffer);
    float interpolateFrequency(uint16_t maxIndex, double maxAmplitude);
    float calculateBinError(float p);
};

#endif // FREQUENCY_ANALYZER_H
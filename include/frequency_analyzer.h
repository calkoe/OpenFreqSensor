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
    void beginSampling();               // Creates the sampler task (call before enabling the timer)
    IRAM_ATTR void notifySampleFromISR();
    bool getNextSliceAnalysis(FrequencyAnalysis*);

private:

    // Sampling runs in a dedicated high-priority task, triggered by the
    // hardware timer ISR via task notification. analogRead()/gettimeofday()
    // are not ISR-safe, so they must not be called from interrupt context.
    static void samplerTaskEntry(void* arg);
    void processSample();
    TaskHandle_t samplerTaskHandle{nullptr};
    QueueHandle_t adcDataSliceQueue;
    uint16_t ringBuffer[RING_BUFFER_SIZE]{0};
    uint32_t writeIndex{0};
    unsigned long lastSliceCopy{0};
    AdcDataSlice sliceScratch;  // member, not stack local: ~1KB

    // Analyzing Management
    double frequencyAvg{50};
    double interpolateFrequency(double* vReal, uint16_t maxIndex, double maxAmplitude);
    double calculateBinError(double p);

};

#endif // FREQUENCY_ANALYZER_H

#include "frequency_analyzer.h"

FrequencyAnalyzer::FrequencyAnalyzer() {
    writeIndex = 0;
    vReal = new double[ANALYSIS_SIZE];
    vImag = new double[ANALYSIS_SIZE];
    memset(ringBuffer, 0, sizeof(ringBuffer));
}

void IRAM_ATTR FrequencyAnalyzer::addDatapoint(uint16_t value) {
    uint32_t currentIndex = __atomic_fetch_add(&writeIndex, 1, __ATOMIC_SEQ_CST) % RING_BUFFER_SIZE;
    ringBuffer[currentIndex] = value;
}

uint16_t FrequencyAnalyzer::getReadStartIndex() {
    uint32_t currentIndex;
    __atomic_load(&writeIndex, &currentIndex, __ATOMIC_SEQ_CST);
    return (currentIndex + RING_BUFFER_SIZE - ANALYSIS_SIZE) % RING_BUFFER_SIZE;
}

void FrequencyAnalyzer::copyDataToAnalysisBuffer(double* buffer) {
    uint16_t startIdx = getReadStartIndex();
    
    // Calculate average for DC offset removal
    double avg = 0;
    for (uint16_t i = 0; i < ANALYSIS_SIZE; i++) {
        avg += ringBuffer[(startIdx + i) % RING_BUFFER_SIZE];
    }
    avg /= ANALYSIS_SIZE;
    
    // Copy data and remove DC offset
    for (uint16_t i = 0; i < ANALYSIS_SIZE; i++) {
        buffer[i] = ringBuffer[(startIdx + i) % RING_BUFFER_SIZE] - avg;
    }
}

FrequencyAnalysis FrequencyAnalyzer::analyze() {
    FrequencyAnalysis result = {0};
    
    // Copy data from ring buffer and remove DC offset
    copyDataToAnalysisBuffer(vReal);

    // Set Millis
    result.millis = millis();

    // Set Timestamp with microsecond precision
    gettimeofday(&result.time, nullptr);

    // Clear imaginary components
    for (uint16_t i = 0; i < ANALYSIS_SIZE; i++) {
        vImag[i] = 0;
    }

    // Perform FFT
    FFT.Windowing(vReal, analysisSize, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.Compute(vReal, vImag, analysisSize, FFT_FORWARD);
    FFT.ComplexToMagnitude(vReal, vImag, analysisSize);

    // Find peak frequency around power grid frequency (45-55 Hz)
    double maxAmplitude = 0;
    uint16_t maxIndex = 0;
    uint16_t startBin = 45 * analysisSize / samplingFreq;
    uint16_t endBin = 55 * analysisSize / samplingFreq;
    
    for (uint16_t i = startBin; i <= endBin; i++) {
        if (vReal[i] > maxAmplitude) {
            maxAmplitude = vReal[i];
            maxIndex = i;
        }
    }

    result.amplitude = maxAmplitude;
    result.isValidSignal = maxAmplitude > AMPLITUDE_THRESHOLD && maxIndex > 0 && maxIndex < (analysisSize - 1);

    if (result.isValidSignal) {
        result.frequency = interpolateFrequency(maxIndex, maxAmplitude);
        float binError = calculateBinError(result.frequency - maxIndex);
        result.rawFrequency = result.frequency;
        result.frequency += binError;
        
        // Calculate quality metric
        if (maxIndex > 0 && vReal[maxIndex] > 0) {
            float beta = log(max(1.0, vReal[maxIndex]));
            float alpha = log(max(1.0, vReal[maxIndex-1]));
            float gamma = log(max(1.0, vReal[maxIndex+1]));
            float d2 = (alpha + gamma - 2*beta);
            result.quality = abs(beta) > 1e-6 ? -d2 / (beta * beta) : 0;
        }
    }

    return result;
}

float FrequencyAnalyzer::interpolateFrequency(uint16_t maxIndex, double maxAmplitude) {
    float alpha = log(max(1.0, vReal[maxIndex-1]));
    float beta = log(max(1.0, vReal[maxIndex]));
    float gamma = log(max(1.0, vReal[maxIndex+1]));
    
    float denom = (alpha - 2*beta + gamma);
    if (abs(denom) > 1e-6) {
        float p = 0.5 * (alpha - gamma) / denom;
        float d2 = (alpha + gamma - 2*beta);
        
        if (d2 < 0) {
            p = p / (1 + 0.125 * d2 * p * p);
        }
        
        return (maxIndex + p) * (float)samplingFreq / analysisSize;
    }
    
    return maxIndex * (float)samplingFreq / analysisSize;
}

float FrequencyAnalyzer::calculateBinError(float p) {
    if (p >= 0) {
        return -0.06f * (0.5f - fabsf(p - 0.5f));
    }
    return 0.06f * (0.5f - fabsf(p + 0.5f));
}
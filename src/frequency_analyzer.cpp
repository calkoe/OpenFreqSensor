#include "frequency_analyzer.h"

// Public

FrequencyAnalyzer::FrequencyAnalyzer() {
    // Create adcDataSliceQueue (slices arrive every ANALYSIS_INTERVAL_MS and
    // are consumed in loop(), so a few slots of headroom are enough)
    adcDataSliceQueue = xQueueCreate(4, sizeof(AdcDataSlice));
    if (adcDataSliceQueue == NULL) {
        Serial.println("Error creating adcDataSliceQueue! Restarting...");
        delay(1000);
        ESP.restart();
    }
}

void FrequencyAnalyzer::beginSampling() {
    xTaskCreatePinnedToCore(samplerTaskEntry, "sampler", 4096, this, 10, &samplerTaskHandle, 1);
}

// ISR context: only an IRAM-safe task notification, nothing else
void IRAM_ATTR FrequencyAnalyzer::notifySampleFromISR() {
    if (samplerTaskHandle == nullptr) return;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(samplerTaskHandle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void FrequencyAnalyzer::samplerTaskEntry(void* arg) {
    FrequencyAnalyzer* self = static_cast<FrequencyAnalyzer*>(arg);
    for (;;) {
        // Acts as counting semaphore: catches up if samples queued up
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        self->processSample();
    }
}

void FrequencyAnalyzer::processSample() {
    ringBuffer[writeIndex] = analogRead(ADC_PIN);
    if(millis() - lastSliceCopy > ANALYSIS_INTERVAL_MS){
        // Calculate currentStartIndex
        lastSliceCopy = millis();
        uint32_t currentStartIndex = (writeIndex + RING_BUFFER_SIZE - ANALYSIS_SIZE) % RING_BUFFER_SIZE;

        // Generate Data Slice // Set Millis & Timestamp
        sliceScratch.millis = lastSliceCopy;
        gettimeofday(&sliceScratch.time, nullptr);

        // Copy Data
        for (uint16_t i = 0; i < ANALYSIS_SIZE; i++) {
            sliceScratch.adcData[i] = ringBuffer[(currentStartIndex + i) % RING_BUFFER_SIZE];
        }

        // Send Data Slice to Queue (drop slice if queue is full)
        xQueueSend(adcDataSliceQueue, &sliceScratch, 0);
    }
    writeIndex = (writeIndex + 1) % RING_BUFFER_SIZE;
}

bool FrequencyAnalyzer::getNextSliceAnalysis(FrequencyAnalysis* frequencyAnalysis) {

    // Setup
    static AdcDataSlice adcDataSlice;
    static arduinoFFT FFT;
    static double vReal[ANALYSIS_SIZE];
    static double vImag[ANALYSIS_SIZE];

    // Wait up to 1s for new data
    if (xQueueReceive(adcDataSliceQueue, &adcDataSlice, 0) == pdTRUE) {
        
        // Copy Time Data
        frequencyAnalysis->millis = adcDataSlice.millis;  
        frequencyAnalysis->time = adcDataSlice.time;   

        // Calculate average for DC offset removal
        double avg = 0;
        for (uint16_t i = 0; i < ANALYSIS_SIZE; i++) {
            avg += adcDataSlice.adcData[i];
        }
        avg /= ANALYSIS_SIZE;
        
        // Fill vReal and vImag
        for (uint16_t i = 0; i < ANALYSIS_SIZE; i++) {
            vReal[i] = adcDataSlice.adcData[i] - avg;
            vImag[i] = 0;
        }

        // Perform FFT
        FFT.Windowing(vReal, ANALYSIS_SIZE, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
        FFT.Compute(vReal, vImag, ANALYSIS_SIZE, FFT_FORWARD);
        FFT.ComplexToMagnitude(vReal, vImag, ANALYSIS_SIZE);
        
        // Find peak frequency around power grid frequency (45-55 Hz)
        double maxAmplitude = 0;
        uint16_t maxIndex = 0;
        uint16_t startBin = 45 * ANALYSIS_SIZE / SAMPLING_FREQUENCY;
        uint16_t endBin = 55 * ANALYSIS_SIZE / SAMPLING_FREQUENCY;
        
        for (uint16_t i = startBin; i <= endBin; i++) {
            if (vReal[i] > maxAmplitude) {
                maxAmplitude = vReal[i];
                maxIndex = i;
            }
        }

        frequencyAnalysis->amplitude = maxAmplitude;
        frequencyAnalysis->isValidSignal = maxAmplitude > AMPLITUDE_THRESHOLD && maxIndex > 0 && maxIndex < (ANALYSIS_SIZE - 1);

        if (frequencyAnalysis->isValidSignal) {
            frequencyAnalysis->frequency = interpolateFrequency(vReal, maxIndex, maxAmplitude);
            double binError = calculateBinError(frequencyAnalysis->frequency - maxIndex);
            frequencyAnalysis->rawFrequency = frequencyAnalysis->frequency;
            frequencyAnalysis->frequency += binError;
            frequencyAvg = frequencyAvg * 0.75 + frequencyAnalysis->frequency * 0.25;
            frequencyAnalysis->frequency = frequencyAvg;
            
            // Calculate quality metric
            if (maxIndex > 0 && vReal[maxIndex] > 0) {
                double beta = log(max(1.0, vReal[maxIndex]));
                double alpha = log(max(1.0, vReal[maxIndex-1]));
                double gamma = log(max(1.0, vReal[maxIndex+1]));
                double d2 = (alpha + gamma - 2*beta);
                frequencyAnalysis->quality = abs(beta) > 1e-6 ? -d2 / (beta * beta) : 0;
            }
        }

        return true;

    }

    return false;

}

double FrequencyAnalyzer::interpolateFrequency(double* vReal, uint16_t maxIndex, double maxAmplitude) {
    double alpha = log(max(1.0, vReal[maxIndex-1]));
    double beta = log(max(1.0, vReal[maxIndex]));
    double gamma = log(max(1.0, vReal[maxIndex+1]));
    
    double denom = (alpha - 2*beta + gamma);
    if (abs(denom) > 1e-6) {
        double p = 0.5 * (alpha - gamma) / denom;
        double d2 = (alpha + gamma - 2*beta);
        
        if (d2 < 0) {
            p = p / (1 + 0.125 * d2 * p * p);
        }
        
        return (maxIndex + p) * (double)SAMPLING_FREQUENCY / ANALYSIS_SIZE;
    }
    
    return maxIndex * (double)SAMPLING_FREQUENCY / ANALYSIS_SIZE;
}

double FrequencyAnalyzer::calculateBinError(double p) {
    if (p >= 0) {
        return -0.06f * (0.5f - fabsf(p - 0.5f));
    }
    return 0.06f * (0.5f - fabsf(p + 0.5f));
}
#include "frequency_interpreter.h"

FrequencyInterpreter::FrequencyInterpreter(){}

FrequencyAlert FrequencyInterpreter::interpret(const FrequencyAnalysis& analysis) {
    FrequencyAlert alert = {false, false, "none", analysis, 0, 0, "none", 0};

    // Warmup period to stabilize measurements
    if(warmup) {
        warmup--;
    } else {
        alert.valid = true;
    }

    // Check for amplitude error (signal quality)
    if (!analysis.isValidSignal) {
        warmup = 10;
        alert.hasAlert = true;
        alert.alertType = "AMPL";
        snprintf(alert.message, LCD_COLS, "AMPL: %.0f", alert.frequencyAnalysis.amplitude);
        return alert;
    }

    // Calculate frequency metrics
    alert.deviation = fabsf(analysis.frequency - TARGET_FREQUENCY);
    alert.analyzingDelay = alert.frequencyAnalysis.millis - lastRun;
    
    // Calculate current ramp (RoCoF) and 
    float currentRamp = ((float)fabsf(analysis.frequency - lastFreq) / (float)alert.analyzingDelay) * 1000;
    alert.ramp = (currentRamp + lastRamp) / 2.0f;
    
    // Update state for next iteration
    lastRun = alert.frequencyAnalysis.millis;
    lastFreq = analysis.frequency;
    lastRamp = currentRamp;

    // Rate of Change (RoCoF) check - highest priority
    if (alert.ramp >= RAMP_THRESHOLD && alert.ramp < 10) {
        alert.hasAlert = true;
        alert.alertType = "ROCOF";
        snprintf(alert.message, LCD_COLS, "RoCoF: %.3f Hz/s", alert.ramp);
        return alert;
    }

    // Frequency deviation checks - staged alerts
    if (alert.deviation >= CRITICAL_RANGE_THRESHOLD) {
        alert.hasAlert = true;
        alert.alertType = "CRIT";
        snprintf(alert.message, LCD_COLS, "CRIT: %.3f Hz", alert.frequencyAnalysis.frequency);
        return alert;
    }

    if (alert.deviation >= ALERT_RANGE_THRESHOLD) {
        alert.hasAlert = true;
        alert.alertType = "ALERT";
        snprintf(alert.message, LCD_COLS, "ALERT: %.3f Hz", alert.frequencyAnalysis.frequency);
        return alert;
    }

    return alert;
}
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
    
    // Update state for next iteration
    alert.ramp = ((float)fabsf(analysis.frequency - lastFreq) / (float)alert.analyzingDelay) * 1000;
    lastRun = alert.frequencyAnalysis.millis;
    lastFreq = analysis.frequency;

    // Rate of Change (RoCoF) check - highest priority
    if (alert.ramp >= ROCOF_THRESHOLD && alert.ramp < 10) {
        alert.hasAlert = true;
        alert.alertType = "ROCOF";
        snprintf(alert.message, LCD_COLS, "RoCoF: %.3f Hz/s", alert.ramp);
        return alert;
    }

    // Frequency deviation checks - staged alerts
    if (alert.deviation >= ALERT_RANGE_THRESHOLD) {
        alert.hasAlert = true;
        alert.alertType = "ALERT_RANGE_THRESHOLD";
        snprintf(alert.message, LCD_COLS, "ALERT: %.3f Hz", alert.frequencyAnalysis.frequency);
        return alert;
    }

    if (alert.deviation >= LEVEL1_EMERGENCY_THRESHOLD) {
        alert.hasAlert = true;
        alert.alertType = "LEVEL1_EMERGENCY_THRESHOLD";
        snprintf(alert.message, LCD_COLS, "EMERG1: %.3f Hz", alert.frequencyAnalysis.frequency);
        return alert;
    }

    if (alert.deviation >= LEVEL2_EMERGENCY_THRESHOLD) {
        alert.hasAlert = true;
        alert.alertType = "LEVEL2_EMERGENCY_THRESHOLD";
        snprintf(alert.message, LCD_COLS, "EMERG2: %.3f Hz", alert.frequencyAnalysis.frequency);
        return alert;
    }

    return alert;
}
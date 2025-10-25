#include "frequency_transmitter.h"

FrequencyTransmitter::FrequencyTransmitter(PubSubClient& client)
    : mqttClient(client) {
}

void FrequencyTransmitter::transmit(const FrequencyAlert& alert) {
    char message[800];  // Increased buffer size for additional metrics
    
    // Get system metrics
    uint32_t freeHeap = ESP.getFreeHeap();
    uint8_t cpuFreq = ESP.getCpuFreqMHz();
    int8_t rssi = WiFi.RSSI();
    float heapUsagePercent = 100.0f * (1.0f - (float)freeHeap / (float)ESP.getHeapSize());
    
    // Get timestamp with microsecond precision
    struct timeval tv = alert.frequencyAnalysis.time;
    uint64_t timestamp_ms = ((uint64_t)tv.tv_sec * 1000) + (tv.tv_usec / 1000); // Convert to milliseconds
    
    snprintf(message, sizeof(message),
             "{\"sensorId\":\"%s\",\"time\":%llu,\"freq\":%.3f,\"amp\":%.1f,\"quality\":%.3f,\"alert\":%s,"
             "\"alertType\":\"%s\",\"deviation\":%.3f,\"ramp\":%.9f,\"analyzingDelay\":%i,"
             "\"freeHeap\":%u,\"heapUsage\":%.1f,\"cpuFreq\":%u,\"wifiRSSI\":%d}",
             SENSOR_ID,
             timestamp_ms,
             alert.frequencyAnalysis.frequency,
             alert.frequencyAnalysis.amplitude,
             alert.frequencyAnalysis.quality,
             alert.hasAlert ? "true" : "false",
             alert.alertType,
             alert.deviation,
             alert.ramp,
             alert.analyzingDelay,
             freeHeap,
             heapUsagePercent,
             cpuFreq,
             rssi
            );
             
    if (WiFi.status() == WL_CONNECTED && mqttClient.connected()) {
        mqttClient.publish(MQTT_TOPIC, message);
    } else {
        Serial.println("Skipped publish (not connected).");
    }
}
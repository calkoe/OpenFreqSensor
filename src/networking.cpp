#include "networking.h"

Networking::Networking() : mqttClient(espClient){}

void Networking::begin() {
    if (WIFI_CONFIGURED) {
        setupWiFi();
        setupNTP();  // Only setup NTP if WiFi is configured
        if (MQTT_CONFIGURED) {
            setupMqtt();
        } else {
            Serial.println("MQTT not configured - running in offline mode");
        }
    } else {
        Serial.println("WiFi not configured - running in offline mode");
        WiFi.mode(WIFI_OFF);  // Disable WiFi to save power
    }
}

void Networking::setupWiFi() {
    delay(10);
    Serial.println("Connecting to WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    WiFi.setAutoReconnect(true);
}

void Networking::setupMqtt() {
    Serial.println("Setting up MQTT...");
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    espClient.setInsecure(); 
}

void Networking::reconnectWiFi() {
    Serial.println("WiFi lost connection. Reconnecting...");
    WiFi.disconnect();
    WiFi.reconnect();
    delay(1000);
}

void Networking::reconnectMQTT() {
    Serial.println("Attempting MQTT connection...");
    String clientId = "FreqSensor-";
    clientId += String(random(0xffff), HEX);
    mqttClient.disconnect();
    mqttClient.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD);
    bool connected = mqttClient.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD);
    if (connected) {
        Serial.println("MQTT connected!");
    } else {
        Serial.print("MQTT connection failed, rc=");
        Serial.println(mqttClient.state());
    }
}

void Networking::setupNTP() {
    configTime(0, 0, NTP_SERVER);  // First, get UTC time
    setenv("TZ", TIME_ZONE, 1);    // Set timezone
    tzset();
}

bool Networking::isTimeSet() {
    time_t now;
    time(&now);
    return now > 1600000000;  // Time is after September 2020
}

void Networking::loop() {

    // Try to Reconnect
    if (WIFI_CONFIGURED && millis() - lastReconnectTry > 5000) {
        lastReconnectTry = millis();

        // Update WiFi
        bool wifiConnected = WiFi.isConnected() && WiFi.status() == WL_CONNECTED;
        display->updateWifiStatus(wifiConnected);
        if (!wifiConnected) {
            reconnectWiFi();
            display->updateMqttStatus(false);
        }

        // Update MQTT if configured
        if (MQTT_CONFIGURED) {
            bool mqttConnected = mqttClient.connected();
            display->updateMqttStatus(mqttConnected);
            if (wifiConnected && !mqttConnected) {
                reconnectMQTT();
            }
        }

        // Update NTP
        display->updateNTPStatus(isTimeSet());

    }

    if (MQTT_CONFIGURED) {
        mqttClient.loop();
    }
    
}


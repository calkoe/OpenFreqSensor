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
    Serial.println("Connecting to WiFi...");
    WiFi.persistent(false);   // No NVS flash writes on every begin/disconnect
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);     // Modem sleep causes drops on flaky networks
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void Networking::setupMqtt() {
    Serial.println("Setting up MQTT...");
    espClient.setInsecure();
    espClient.setTimeout(NET_TIMEOUT_S);           // seconds (socket ops)
    espClient.setHandshakeTimeout(NET_TIMEOUT_S);  // seconds (TLS handshake)
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setSocketTimeout(NET_TIMEOUT_S);
    mqttClient.setKeepAlive(30);
}

void Networking::forceWiFiReconnect() {
    Serial.println("WiFi down for a while - forcing full reconnect...");
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void Networking::reconnectMQTT() {
    Serial.println("Attempting MQTT connection...");
    String clientId = "FreqSensor-";
    clientId += String(random(0xffff), HEX);
    if (mqttClient.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
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

    if (WIFI_CONFIGURED && millis() - lastStatusCheck > NET_STATUS_INTERVAL_MS) {
        lastStatusCheck = millis();

        bool wifiConnected = WiFi.status() == WL_CONNECTED;
        display->updateWifiStatus(wifiConnected);
        display->updateNTPStatus(isTimeSet());

        if (!wifiConnected) {
            display->updateMqttStatus(false);
            // Short dropouts are handled by the driver's auto-reconnect.
            // Only force a full re-association if we stay offline too long.
            if (!wifiWasDown) {
                wifiWasDown = true;
                wifiDownSince = millis();
            } else if (millis() - wifiDownSince > WIFI_FORCE_RECONNECT_MS) {
                forceWiFiReconnect();
                wifiDownSince = millis();
            }
        } else {
            wifiWasDown = false;

            if (MQTT_CONFIGURED) {
                bool mqttConnected = mqttClient.connected();
                display->updateMqttStatus(mqttConnected);
                // connect() blocks for up to 2 x NET_TIMEOUT_S, so rate-limit it
                if (!mqttConnected && millis() - lastMqttAttempt > MQTT_RETRY_INTERVAL_MS) {
                    lastMqttAttempt = millis();
                    reconnectMQTT();
                }
            }
        }
    }

    if (MQTT_CONFIGURED) {
        mqttClient.loop();
    }

}

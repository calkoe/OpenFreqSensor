#ifndef NETWORKING_H
#define NETWORKING_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <time.h>
#include "config.h"
#include "display_handler.h"

extern DisplayHandler* display;

class Networking {
    public:
        Networking();
        void begin();
        void loop();
        bool isConnected() { return mqttClient.connected(); }
        PubSubClient& getMqttClient() { return mqttClient; }

    private:
        // Network clients
        WiFiClientSecure espClient;
        PubSubClient mqttClient;
        unsigned long lastReconnectTry;
        void setupWiFi();
        void setupMqtt();
        void reconnectWiFi();
        void reconnectMQTT();
        void setupNTP();
        bool isTimeSet();
};

#endif // NETWORKING_H
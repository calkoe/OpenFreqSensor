#ifndef FREQUENCY_TRANSMITTER_H
#define FREQUENCY_TRANSMITTER_H

#include <WiFi.h>
#include <PubSubClient.h>
#include "config.h"
#include "frequency_analyzer.h"
#include "frequency_interpreter.h"

class FrequencyTransmitter {
public:
    FrequencyTransmitter(PubSubClient& client);
    void transmit(const FrequencyAlert& alert);

private:
    PubSubClient& mqttClient;
    const char* mqttTopic;
};

#endif // FREQUENCY_TRANSMITTER_H
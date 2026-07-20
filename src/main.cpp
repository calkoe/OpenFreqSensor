#include "main.h"
#include <esp_task_wdt.h>

// Global variables initialization
hw_timer_t *timer = NULL;
Networking *networking = nullptr;
FrequencyAnalyzer *analyzer = nullptr;
FrequencyInterpreter *interpreter = nullptr;
FrequencyTransmitter *transmitter = nullptr;
DisplayHandler *display = nullptr;

// ISR must stay minimal: analogRead() & friends are not ISR-safe (flash
// resident, take locks) and crash/hang when WiFi does flash writes.
// The actual sampling happens in the analyzer's high-priority task.
void IRAM_ATTR onTimer(){
    analyzer->notifySampleFromISR();
}

void setup_timer(){

    // Configure timer for stable 512Hz sampling with larger prescaler
    uint32_t apb_freq = getApbFrequency();
    uint32_t prescaler = 2;
    uint32_t timer_divider = apb_freq / (prescaler * SAMPLING_FREQUENCY);

    timer = timerBegin(0, prescaler, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, timer_divider, true);
    timerAlarmEnable(timer);

    Serial.printf("APB Freq: %lu Hz, Timer Divider: %lu, Actual sampling rate: %.2f Hz\n",
                  apb_freq, timer_divider, (float)apb_freq / (prescaler * timer_divider));

}

void setup(){

    // Set CPU frequency
    setCpuFrequencyMhz(CPU_FREQUENCY_MHZ);
    Serial.begin(115200);

    // Watchdog: reboot instead of hanging forever if loop() ever stalls
    // (e.g. network stack wedged). Timeout must exceed the worst-case
    // blocking TLS connect (2 x NET_TIMEOUT_S).
    esp_task_wdt_init(WDT_TIMEOUT_S, true);
    esp_task_wdt_add(NULL);

    // Initialize Display
    display = new DisplayHandler();
    display->begin();

    // Initialize networking
    networking = new Networking();
    networking->begin();

    // Initialize frequency analysis components
    analyzer = new FrequencyAnalyzer();
    interpreter = new FrequencyInterpreter();
    transmitter = new FrequencyTransmitter(networking->getMqttClient());

    // Start sampling task, then the timer that triggers it
    pinMode(ADC_PIN,INPUT);
    analyzer->beginSampling();
    setup_timer();

}

void loop(){

      // Feed the watchdog
      esp_task_wdt_reset();

      // Analyze Data
      FrequencyAnalysis frequencyAnalysis{0};
      if(analyzer->getNextSliceAnalysis(&frequencyAnalysis)){
        FrequencyAlert alert = interpreter->interpret(frequencyAnalysis);
        display->updateAnalysis(frequencyAnalysis);
        if(alert.valid){

            // Update display and handle alerts
            if (alert.hasAlert) display->addAlert(alert);

            // Transmit the results via MQTT
            transmitter->transmit(alert);

        }

        // Debug information
        Serial.print("Freq: ");
        Serial.print(frequencyAnalysis.frequency, 3);
        Serial.print(", Ampl: ");
        Serial.print(frequencyAnalysis.amplitude);
        Serial.print(", Quality: ");
        Serial.println(frequencyAnalysis.quality, 3);
      };
      
      // Networking Data
      networking->loop();

      // Display Loop
      display->loop();

      // Add small delay to prevent task hogging CPU
      vTaskDelay(pdMS_TO_TICKS(10)); 

}
#include "main.h"

// Global variables initialization
hw_timer_t *timer = NULL;
Networking *networking = nullptr;
FrequencyAnalyzer *analyzer = nullptr;
FrequencyInterpreter *interpreter = nullptr;
FrequencyTransmitter *transmitter = nullptr;
DisplayHandler *display = nullptr;
const int QUEUE_LENGTH = 64; // ring buffer depth

// --- FreeRTOS handles ---
TaskHandle_t FrequencyAnalyzerTaskHandle = NULL;
TaskHandle_t FrequencyInterpreterTaskHandle = NULL;
TaskHandle_t NetworkingTaskHandle = NULL;
TaskHandle_t DisplayTaskHandle = NULL;
QueueHandle_t FrequencyAnalyzerQueue;

void IRAM_ATTR onTimer(){
    analyzer->addDatapoint(analogRead(ADC_PIN));
}

// FrequencyAnalyzerTask ---
void FrequencyAnalyzerTask(void *pvParameters){
  FrequencyAnalysis frequencyAnalysis;

  // Wait for Warmup
  vTaskDelay(5000*portTICK_PERIOD_MS);
  
  TickType_t lastWake = xTaskGetTickCount();
  for (;;) {
    frequencyAnalysis = analyzer->analyze();

    // Send to queue with proper overflow handling
    if (xQueueSend(FrequencyAnalyzerQueue, &frequencyAnalysis, 0) != pdTRUE) {
      // Queue full - remove oldest item and try again
      FrequencyAnalysis dummy;
      xQueueReceive(FrequencyAnalyzerQueue, &dummy, 0);
      xQueueSend(FrequencyAnalyzerQueue, &frequencyAnalysis, 0);
    }

    // Send to Display
    display->updateAnalysis(frequencyAnalysis);

    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(ANALYSIS_INTERVAL_MS));
  }
}

// FrequencyInterpreterTask ---
void FrequencyInterpreterTask(void *pvParameters){
    FrequencyAnalysis frequencyAnalysis;
  
    for (;;) {
        // Wait up to 1s for new data
        if (xQueueReceive(FrequencyAnalyzerQueue, &frequencyAnalysis, pdMS_TO_TICKS(ANALYSIS_INTERVAL_MS)) == pdTRUE) {

            // Interpret the results and check for alerts
            FrequencyAlert alert = interpreter->interpret(frequencyAnalysis);
            if(alert.valid){
                 // Update display and handle alerts
                if (alert.hasAlert){
                    display->addAlert(alert);
                }

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

        } 
     }
}

// NetworkingTask ---
void NetworkingTask(void *pvParameters){
  TickType_t lastWake = xTaskGetTickCount();
  for (;;) {
    networking->loop();
  }
}

// DisplayTask
void DisplayTask(void *pvParameters){
  TickType_t lastWake = xTaskGetTickCount();
  for (;;) {
    display->loop();
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(DISPLAY_REFRESH_MS));
  }
}

void setup_timer(){
    // Configure high-precision timer for 1024Hz sampling
    uint32_t apb_freq = getApbFrequency();
    uint32_t prescaler = 2; // Small prescaler for better precision
    uint32_t timer_divider = apb_freq / (prescaler * SAMPLING_FREQUENCY);

    timer = timerBegin(0, prescaler, true); // Use timer 0 with minimal prescaler
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

    // Create queue
    FrequencyAnalyzerQueue = xQueueCreate(QUEUE_LENGTH, sizeof(FrequencyAnalysis));
    if (FrequencyAnalyzerQueue == NULL) {
        Serial.println("Error creating FrequencyAnalyzerQueue!");
        while (1);
    }

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

    // Create producer (higher priority)
    xTaskCreatePinnedToCore(
        FrequencyAnalyzerTask,
        "FrequencyAnalyzerTask",
        16384,  // Increased stack size for FFT calculations
        NULL,
        25, // Highest priority (max is 25)
        &FrequencyAnalyzerTaskHandle,
        1  // Run on Core 1
    );

    // Create consumer (lower priority)
    xTaskCreatePinnedToCore(
        FrequencyInterpreterTask,
        "FrequencyInterpreterTask",
        8192,
        NULL,
        2, // Higher than networking but lower than analyzer
        &FrequencyInterpreterTaskHandle,  // Fixed: was using wrong handle
        0  // Run on Core 0
    );

    // Create networking task on Core 0
    xTaskCreatePinnedToCore(
        NetworkingTask,
        "NetworkingTask",
        8192,
        NULL,
        1, // Lower priority
        &NetworkingTaskHandle,
        0  // Run on Core 0
    );

    // Create display task on Core 0
    xTaskCreatePinnedToCore(
        DisplayTask,
        "DisplayTask",
        4096,  // Display task needs less stack
        NULL,
        1, // Lower priority
        &DisplayTaskHandle,
        0  // Run on Core 0
    );

    // Initialize timer for sampling
    pinMode(ADC_PIN,INPUT_PULLDOWN);
    setup_timer();
}

void loop(){
  vTaskDelete(NULL); // Delete the loop task to free up resources
}
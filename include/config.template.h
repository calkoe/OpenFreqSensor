#ifndef CONFIG_H
#define CONFIG_H

// Helper macros to check if config is set
#define IS_PLACEHOLDER(str) ((strcmp(str, "your_ssid") == 0) || \
                           (strcmp(str, "your_password") == 0) || \
                           (strcmp(str, "your_mqtt_server") == 0) || \
                           (strcmp(str, "your_username") == 0) || \
                           (strcmp(str, "your_id") == 0))

// Device Configuration
#define SENSOR_ID "freqsensor/your_id"

// Network Configuration
#define WIFI_SSID "your_ssid"
#define WIFI_PASSWORD "your_password"
#define MQTT_TOPIC "public/freqsensor/your_id"
#define MQTT_SERVER "your_mqtt_server"
#define MQTT_PORT 1883
#define MQTT_USERNAME "your_username"
#define MQTT_PASSWORD "your_password"

// Configuration state flags
#define WIFI_CONFIGURED (!IS_PLACEHOLDER(WIFI_SSID) && !IS_PLACEHOLDER(WIFI_PASSWORD))
#define MQTT_CONFIGURED (!IS_PLACEHOLDER(MQTT_SERVER) && !IS_PLACEHOLDER(MQTT_USERNAME))

// NTP Configuration
#define NTP_SERVER "pool.ntp.org"
#define TIME_ZONE "CET-1CEST,M3.5.0,M10.5.0/3"

// Hardware Pin Configuration
#define ADC_PIN 34           // GPIO36 = ADC1_CH0 for frequency measurement
#define BUTTON_UP_PIN 23     // GPIO for up button
#define BUTTON_DOWN_PIN 19   // GPIO for down button
#define BUTTON_MUTE_PIN 18   // GPIO for mute button
#define LCD_I2C_ADDR 0x27    // I2C address for LCD display
#define LCD_SDA_PIN 21       // I2C SDA pin for LCD
#define LCD_SCL_PIN 22       // I2C SCL pin for LCD
#define BUZZER_PIN 25        // GPIO pin for the buzzer
#define BUTTON_DEBOUNCE_MS 500

// Frequency Analysis Configuration
#define SAMPLING_FREQUENCY 512    // Target sampling frequency in Hz
#define RING_BUFFER_SIZE 4096     // Ring buffer size
#define ANALYSIS_SIZE 512        // Size of data to analyze
#define ANALYSIS_INTERVAL_MS 250  // Analysis interval in milliseconds
#define AMPLITUDE_THRESHOLD 10000 // Minimum acceptable amplitude

// Frequency Interpretation Configuration
#define TARGET_FREQUENCY 50.000f          // Target grid frequency in Hz
#define STANDARD_RANGE_THRESHOLD 0.050f   // Standard range threshold (±50mHz)
#define ALERT_RANGE_THRESHOLD 0.200f      // Stage 1 alert threshold (±200mHz)
#define CRITICAL_RANGE_THRESHOLD 0.400f   // Stage 2 alert threshold (±400mHz)
#define RAMP_THRESHOLD 0.100f             // Rate of Change threshold (Hz/s)

// Display Configuration
#define LCD_I2C_ADDR    0x27
#define LCD_COLS 20                 // LCD display columns
#define LCD_ROWS 4                  // LCD display rows
#define LCD_PIXELS 100               // LCD display rows
#define ALARM_HISTORY_SIZE 256      // Number of alarm events to store
#define MAX_ALARMS      512         // Store up to 512 Alarms
#define MAX_ALARM_INTERVAL_MS 5000  // How Fast new Alarms will be added
#define ALARM_DURATION 3600000       // 1H
#define DISPLAY_REFRESH_MS 500

// Timer Configuration
#define TIMER_PRESCALER 2        // Timer prescaler for sampling
#define CPU_FREQUENCY_MHZ 240    // CPU frequency in MHz

#endif // CONFIG_H
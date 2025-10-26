#ifndef CONFIG_H
#define CONFIG_H

// Helper macros to check if config is set
// Validates that configuration values have been properly set and are not default placeholders
#define IS_PLACEHOLDER(str) ((strcmp(str, "your_ssid") == 0) || \
                           (strcmp(str, "your_password") == 0) || \
                           (strcmp(str, "your_mqtt_server") == 0) || \
                           (strcmp(str, "your_username") == 0) || \
                           (strcmp(str, "your_id") == 0))

// Device Configuration
#define SENSOR_ID "freqsensor/your_id"    // Unique identifier for this sensor instance in the MQTT network

// Network Configuration
// WiFi and MQTT settings for data transmission and remote monitoring
#define WIFI_SSID "your_ssid"             // Your WiFi network name
#define WIFI_PASSWORD "your_password"      // Your WiFi password
#define MQTT_TOPIC "public/freqsensor/your_id"  // MQTT topic for publishing frequency data
#define MQTT_SERVER "your_mqtt_server"     // MQTT broker address
#define MQTT_PORT 1883                     // Standard MQTT port (1883=unencrypted, 8883=encrypted)
#define MQTT_USERNAME "your_username"      // MQTT broker authentication username
#define MQTT_PASSWORD "your_password"      // MQTT broker authentication password

// Configuration state flags
// Runtime checks to ensure proper configuration
#define WIFI_CONFIGURED (!IS_PLACEHOLDER(WIFI_SSID) && !IS_PLACEHOLDER(WIFI_PASSWORD))
#define MQTT_CONFIGURED (!IS_PLACEHOLDER(MQTT_SERVER) && !IS_PLACEHOLDER(MQTT_USERNAME))

// NTP Configuration
// Time synchronization settings for accurate timestamping
#define NTP_SERVER "pool.ntp.org"          // NTP server pool for time synchronization
#define TIME_ZONE "CET-1CEST,M3.5.0,M10.5.0/3"  // Central European Time with automatic DST

// Hardware Pin Configuration
// ESP32 GPIO assignments for various components
#define ADC_PIN 34           // ADC input for grid voltage measurement (GPIO36 = ADC1_CH0)
#define BUTTON_UP_PIN 23     // Navigation button for menu/value increase
#define BUTTON_DOWN_PIN 19   // Navigation button for menu/value decrease
#define BUTTON_MUTE_PIN 18   // Silence alarm buzzer and acknowledge alerts
#define LCD_I2C_ADDR 0x27    // Default I2C address for PCF8574 LCD backpack
#define LCD_SDA_PIN 21       // I2C data line for LCD communication
#define LCD_SCL_PIN 22       // I2C clock line for LCD communication
#define BUZZER_PIN 25        // Alert buzzer output
#define BUTTON_DEBOUNCE_MS 500  // Button debounce delay to prevent multiple triggers

// Frequency Analysis Configuration
// Signal processing parameters for accurate frequency measurement
#define SAMPLING_FREQUENCY 512    // ADC sampling rate (Hz) - Nyquist frequency > 100Hz
#define RING_BUFFER_SIZE 4096    // Circular buffer for continuous sampling (8 seconds)
#define ANALYSIS_SIZE 512        // FFT window size (1 second of data)
#define ANALYSIS_INTERVAL_MS 250 // Update rate for frequency calculations (4 Hz)
#define AMPLITUDE_THRESHOLD 10000 // Minimum signal strength for valid measurement

// Frequency Interpretation Configuration
// All thresholds according to ENTSO-E Operation Handbook, Policy 1
#define TARGET_FREQUENCY 50.000f          // Nominal frequency of the Continental European power grid (Hz)
#define STANDARD_RANGE_THRESHOLD 0.050f   // Normal operation range threshold (±50mHz). If exceeded, monitoring required
#define ALERT_RANGE_THRESHOLD 0.200f      // Alert state threshold (±200mHz). Triggers system operator awareness
#define LEVEL1_EMERGENCY_THRESHOLD 0.800f // Level 1 Emergency threshold (±800mHz). System stressed, corrective actions needed
#define LEVEL2_EMERGENCY_THRESHOLD 2.500f // Level 2 Emergency threshold (±2500mHz). High risk of grid collapse
#define ROCOF_THRESHOLD 0.500f           // Rate of Change of Frequency threshold (±500mHz/s). Indicates dynamic stability issues

// Display Configuration
// LCD and alarm system parameters
#define LCD_I2C_ADDR    0x27          // I2C address for LCD controller (default for PCF8574)
#define LCD_COLS 20                   // Display width in characters (20x4 LCD)
#define LCD_ROWS 4                    // Display height in characters
#define LCD_PIXELS 100                // Display width in pixels (for graphics)
#define ALARM_HISTORY_SIZE 256        // Circular buffer size for alarm event history
#define MAX_ALARMS      512          // Maximum number of stored alarm events
#define MAX_ALARM_INTERVAL_MS 5000   // Minimum time between new alarms (prevent spam)
#define ALARM_DURATION 3600000       // How long an alarm remains active (1 hour)
#define DISPLAY_REFRESH_MS 500       // Screen update interval (2 Hz refresh rate)

// Timer Configuration
// ESP32 timer settings for precise sampling
#define CPU_FREQUENCY_MHZ 240       // ESP32 CPU clock speed (240MHz for optimal ADC)

#endif // CONFIG_H
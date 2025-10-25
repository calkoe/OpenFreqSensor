#include "display_handler.h"

DisplayHandler::DisplayHandler() 
    : lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS),
      hasAnalysis{0},
      currentAnalysis(FrequencyAnalysis{}),
      currentWifiStatus(false),
      currentMqttStatus(false),
      currentNTPStatus(false),
      needsUpdate(false),
      writeIndex(MAX_ALARMS),
      writeOverflow(0),
      numAlarms(0),
      lastAlarmAdded(0),
      scrollPosition(0)
      {
}


void DisplayHandler::begin() {
    
    // Init Display
    Wire.begin();
    Wire.setClock(400000); // 400 kHz Fast Mode I2C
    lcd.init();
    lcd.backlight();
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Freq Sensor");
    lcd.setCursor(0, 1);
    lcd.print("Starting...");
    lcd.setCursor(0, 2);
    lcd.print("Calvin Koecher");
    lcd.setCursor(0, 3);
    lcd.print("10.2025");

    // Create custom chars
    lcd.createChar(0, wifiIcon);
    lcd.createChar(1, mqttIcon);
    lcd.createChar(2, clockIcon);
    lcd.createChar(3, full);
    lcd.createChar(4, horzLine);

    // Init Pins
    pinMode(BUTTON_UP_PIN, INPUT_PULLUP);
    pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);
    pinMode(BUTTON_MUTE_PIN, INPUT_PULLUP);
    ledcSetup(0, 2000, 8);   // 2 kHz, 8-bit
    ledcWriteTone(0, 1000);
    delay(1000);
    ledcAttachPin(BUZZER_PIN, 0);
}


void DisplayHandler::updateAnalysis(const FrequencyAnalysis& analysis) {
    currentAnalysis = analysis;
    hasAnalysis = true;
    needsUpdate = true;
}

void DisplayHandler::updateWifiStatus(const bool status) {
    currentWifiStatus = status;
    needsUpdate = true;
}

void DisplayHandler::updateMqttStatus(const bool status) {
    currentMqttStatus = status;
    needsUpdate = true;
}

void DisplayHandler::updateNTPStatus(const bool status) {
    currentNTPStatus = status;
    needsUpdate = true;
}

void DisplayHandler::handleUpButton() {
    if(numAlarms){
        scrollPosition = (scrollPosition + numAlarms - 1) % numAlarms;
    }
}

void DisplayHandler::handleDownButton() {
    if(numAlarms){
        scrollPosition = (scrollPosition + 1) % numAlarms;
    }
}

void DisplayHandler::handleMuteButton() {
    numAlarms = 0;
    scrollPosition = 0;
    lcd.clear();
}

// Method to add an alarms
void DisplayHandler::addAlert(FrequencyAlert alarm) {

    // Limit the Rate an Alarm is Added
    if (millis() - lastAlarmAdded > MAX_ALARM_INTERVAL_MS) {

        // Add Alarm to Ringbuffer
        writeIndex = (writeIndex + 1) % MAX_ALARMS;
        if(numAlarms < MAX_ALARMS){
            numAlarms = numAlarms + 1;
        }
        alarmHistory[writeIndex] = alarm;

        // Scroll Helper - Let Scroll run if not 0
        if(scrollPosition) scrollPosition = (scrollPosition + 1) % numAlarms;
        needsUpdate = true;
        lastAlarmAdded = millis();

    }
}


// Draw centered frequency bar using only CGRAM slot 7 for partial fill
void DisplayHandler::drawFrequencyBar(uint8_t row, float value) {
    const float MIN = TARGET_FREQUENCY - ALERT_RANGE_THRESHOLD;
    const float MAX = TARGET_FREQUENCY + ALERT_RANGE_THRESHOLD;
    const int CENTER = LCD_PIXELS / 2; // 50

    // Clamp
    if (value < MIN) value = MIN;
    if (value > MAX) value = MAX;

    // Map to pixel 0-99
    int pixel = (int)round((value - MIN) / (MAX - MIN) * (LCD_PIXELS - 1));

    // Clear line buffer
    uint8_t chars[LCD_COLS];
    for (uint8_t i = 0; i < LCD_COLS; ++i) chars[i] = ' ';

    // Center marker
    int centerChar = CENTER / 5;
    chars[centerChar] = 3;

    // Compute deviation
    int delta = pixel - CENTER;
    int dir = (delta > 0) ? 1 : -1;
    int filled = abs(delta);
    int fullChars = filled / 5;
    int remainder = filled % 5;

    // Fill full blocks
    for (int i = 1; i <= fullChars; ++i) {
        int pos = centerChar + i * dir;
        if (pos >= 0 && pos < LCD_COLS) chars[pos] = 3; // slot 3 = solid block
    }

    // Partial fill in slot 7
    if (remainder > 0) {
        uint8_t mask = 0;
        for (int b = 0; b < remainder; ++b)
            mask |= (1 << (dir < 0 ? b : (4 - b)));

        uint8_t rows[8];
        for (uint8_t i = 0; i < 8; ++i) rows[i] = mask;
        rows[7] = B00000; // Always Empty
        lcd.createChar(7, rows);

        int pos = centerChar + (fullChars + 1) * dir;
        if (pos >= 0 && pos < LCD_COLS) chars[pos] = 7;
    }

    // Print the line
    lcd.setCursor(0, row);
    for (uint8_t i = 0; i < LCD_COLS; ++i) lcd.write(chars[i]);
}

void DisplayHandler::loop() {

    // Read Pins
    if(!digitalRead(BUTTON_UP_PIN)) handleUpButton();
    if(!digitalRead(BUTTON_DOWN_PIN)) handleDownButton();
    if(!digitalRead(BUTTON_MUTE_PIN)) handleMuteButton();

    // Drive Buzzer
    if (numAlarms && millis() - lastAlarmAdded <= ALARM_DURATION){ // Check if ALARM_DURATION have passed
        if(alarmHistory[writeIndex].alertType == "AMPL") ledcWriteTone(0, 200);
        if(alarmHistory[writeIndex].alertType == "RAMP") ledcWriteTone(0, 500);
        if(alarmHistory[writeIndex].alertType == "FREQ") ledcWriteTone(0, 1000);
    }else{
        ledcWriteTone(0, 0);  // 440 Hz for 500 ms (A4 note)
    }

    if (!needsUpdate) {
        return;
    }

    // Global Vars
    char message[LCD_COLS]{' '};
    uint16_t i = writeIndex - scrollPosition;
    
    // First line: Always show current frequency
    if (hasAnalysis && currentAnalysis.isValidSignal) {
        snprintf(message, LCD_COLS, "Freq: %.3f %-20s", currentAnalysis.frequency,"Hz");
    } else {
        snprintf(message, LCD_COLS, "%-20s","No Signal");
    }
    lcd.setCursor(0, 0);
    lcd.print(message);

    // First line: Show Connection Status
    lcd.setCursor(17, 0); lcd.write(currentWifiStatus ? byte(0) : ' '); // WiFi
    lcd.setCursor(18, 0); lcd.write(currentMqttStatus ? byte(1) : ' '); // MQTT
    lcd.setCursor(19, 0); lcd.write(currentNTPStatus  ? byte(2) : ' '); // Clock

    if (numAlarms) {

        // Second line: Show alarm numbers
        lcd.setCursor(0, 1);
        snprintf(message, LCD_COLS, "MSG %d/%d", scrollPosition + 1 , numAlarms);
        int textLen = strlen(message);
        int totalWidth = 20;
        int dashCount = (totalWidth - textLen) / 2;
        // Left padding
        for (int i = 0; i < dashCount; i++) lcd.print('-');
        // The message part
        lcd.print(message);
        // Right padding (if width is odd)
        while ((dashCount + textLen) < totalWidth) {
            lcd.print('-');
            dashCount++;
        }

        // Convert epoch time to local time and format with milliseconds
        time_t epoch = alarmHistory[i].frequencyAnalysis.time.tv_sec;
        struct tm timeinfo;
        localtime_r(&epoch, &timeinfo);
        
        snprintf(message, LCD_COLS, "%02d/%02d/%02d %02d:%02d:%02d",
            timeinfo.tm_mday,
            timeinfo.tm_mon + 1,
            timeinfo.tm_year % 100,  // Using 2-digit year to fit milliseconds
            timeinfo.tm_hour,
            timeinfo.tm_min,
            timeinfo.tm_sec);
        lcd.setCursor(0, 2);
        lcd.print(message);

        // Forth line: Show alarm if any
        snprintf(message, LCD_COLS, "%-20s", alarmHistory[i].message);
        lcd.setCursor(0, 3);
        lcd.print("                    ");
        lcd.setCursor(0, 3);
        lcd.print(message);

    }else if(hasAnalysis && currentAnalysis.isValidSignal){
        // Second line: Darw Line
        lcd.setCursor(0, 1);  // row = 0..3 for a 20x4 display
        for (int i = 0; i < 20; i++) {
            lcd.write(byte(4));  // write custom char slot 4 (0–7 valid)
        }

        // Third line: Draw Freq Bar
        drawFrequencyBar(2,currentAnalysis.frequency);

        //Forth line: Draw Scala
        lcd.setCursor(0, 3);
        lcd.print("|-200mHz  | +200mHz|");
    }else{
        // Clear everything
        lcd.setCursor(0, 1);
        lcd.print("                    ");
        lcd.setCursor(0, 2);
        lcd.print("                    ");
        lcd.setCursor(0, 3);
        lcd.print("                    ");
    }
    
    needsUpdate = false;
    vTaskDelay(pdMS_TO_TICKS(10)); // Add small delay to prevent task hogging CPU
}
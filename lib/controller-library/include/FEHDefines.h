#ifndef FEH_DEFINE_H
#define FEH_DEFINE_H

// Expected ESP firmware version
#define EXPECTED_ESP_FIRMWARE_VERSION_MAJOR 1
#define EXPECTED_ESP_FIRMWARE_VERSION_MINOR 0
#define EXPECTED_ESP_FIRMWARE_VERSION_PATCH 0

// OTA update configuration (for ESP32 firmware updates)
#define OTA_WIFI_SSID "ERC2-OTA"
#define OTA_WIFI_PASS "ota-pass-123"
#define OTA_WIFI_BSSID_BYTES {0xd8, 0xeb, 0x97, 0xea, 0x54, 0x5c}
#define OTA_WIFI_CHANNEL 6
#define FIRMWARE_URL "http://10.0.0.1/app.bin"

// RCS connectivity configuration
#define RCS_WIFI_SSID "ERC2-RCS"
#define RCS_WIFI_PASS "rcs-pass-123"
#define RCS_WIFI_BSSID_BYTES {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#define RCS_WIFI_CHANNEL 6
#define RCS_SERVER_IP_BYTES {10, 0, 0, 1}

// Number of motors and servos
#define NUM_SERVOS 8
#define NUM_MOTORS 4
#define NUM_STUDENT_GPIO 16

// LCD Dimensions
#define LCD_WIDTH 320
#define LCD_HEIGHT 240
#define DEFAULT_TEXT_SIZE 2

// Motor Pins
const PROGMEM uint8_t MOTOR_PWM_PINS[] = {3, 45, 44, 46};
const PROGMEM uint8_t MOTOR_DIRECTION_PINS[] = {24, 23, 26, 25};
#define MOTOR_nFAULT_01_PIN 28
#define MOTOR_nFAULT_23_PIN 38
#define MOTOR_nSLEEP_PIN 27

// IO Pins
const static PROGMEM uint8_t FEHIOPIN_TO_ARDUINOPIN[] = {
    A0,
    A1,
    A2,
    A3,
    A4,
    A5,
    A6,
    A7,
    A8,
    A9,
    A10,
    A11,
    A12,
    A13,
    A14,
    49,
};

const static PROGMEM bool FEHIOPIN_VALID_ANALOG_PINS[] = {
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    0,
};

const static PROGMEM bool FEHIOPIN_VALID_INTERRUPT_PINS[] = {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    0,
};

#define TOUCHSCREEN_IRQ_PIN 13
#define SD_DETECT_PIN 41
#define SD_CS_PIN 43
#define BATTERY_PIN A15
#define I2C_nFAULT_PIN 35
#define IO_nFAULT_PIN 37
#define BATT_LOW_LED_PIN 36
#define LOW_BATTERY_THRESHOLD 10.0

// SD Card
#define MAX_NUMBER_OF_OPEN_FILES 25
#define BUFFER_SIZE 256

#endif // FEH_DEFINE_H
#ifndef ESP32_H
#define ESP32_H

#include <Arduino.h>
#include <SPI.h>
#include <stdint.h>

// === Compile-time Configuration ===
#define ESP32_PIN_CS 40            ///< SPI Chip Select pin
#define ESP32_PIN_EN 22            ///< ESP32 Enable pin (power control)
#define ESP32_PIN_SPARE 39         ///< ESP32 Spare pin (factory reset trigger)
#define ESP32_SPI_CLOCK_HZ 1000000 ///< SPI clock speed in Hz
#define ESP32_POLL_INTERVAL_MS 100 ///< Polling interval in milliseconds
#define ESP32_TX_BUF_LEN 48        ///< Maximum packet size (SYNC:2 + CMD:1 + LEN:1 + DATA:41 + padding)

/**
 * @brief Callback function type for receiving messages from ESP32
 * @param msg Pointer to message data (without length byte)
 * @param len Length of message
 */
typedef void (*ESP32MessageCallback)(const uint8_t *msg, uint8_t len);

/**
 * @brief Static ESP32 SPI driver with zero runtime configuration storage
 *
 * All configuration is compile-time via #defines. Only callback pointer and
 * initialization flag stored in memory. Buffers are stack-allocated during
 * transactions only.
 */
namespace ESP32
{

    /**
     * @brief Initialize the ESP32 driver and reset to known state
     *
     * @param messageCallback Callback function for received messages (can be nullptr)
     *
     * Configuration is compile-time via #defines:
     *   - ESP32_PIN_CS, ESP32_PIN_EN, ESP32_PIN_SPARE
     *   - ESP32_SPI_CLOCK_HZ, ESP32_POLL_INTERVAL_MS, ESP32_TX_BUF_LEN
     */
    void init(ESP32MessageCallback messageCallback);

    /**
     * @brief Power off the ESP32
     */
    void powerOff();

    /**
     * @brief Power on the ESP32
     *
     * @param factoryReset If true, assert factory reset pin during power-on
     */
    void powerOn(bool factoryReset = false);

    /**
     * @brief Reset the ESP32 to known state
     *
     * @param factoryReset If true, trigger factory reset during power cycle
     */
    void reset(bool factoryReset = false);

    /**
     * @brief Poll for messages from ESP32
     * Automatically handles received messages via callback
     */
    void poll();

    /**
     * @brief Send a command packet to ESP32
     *
     * Packet format: [length] [cmd] [data...]
     *
     * @param cmd Command byte
     * @param data Optional data payload (nullptr if none)
     * @param dataLen Length of data payload (max 30 bytes)
     * @return true if transaction successful, false otherwise
     */
    bool sendCommand(uint8_t cmd, const uint8_t *data = nullptr, uint8_t dataLen = 0);

    /**
     * @brief Perform periodic polling task
     * Call this regularly (e.g., in a timer or loop) to handle incoming messages
     */
    void update();

} // namespace ESP32

#endif // ESP32_H

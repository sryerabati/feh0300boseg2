#ifndef FEHLOG_H
#define FEHLOG_H

#include <stdarg.h>
#include <stdint.h>

class FEHLog
{
public:
    /**
     * @brief Enable Serial (UART) output.
     *        Call after Serial.begin() if you want Serial output.
     */
    static void enableSerial();

    /**
     * @brief Disable Serial output.
     */
    static void disableSerial();

    /**
     * @brief Enable BLE logging.
     *        Calls FEHESP32::startBLELog with device name "FEH-XXX" where XXX
     *        is the zero-padded controller number, then waits for the ack.
     *
     * @param controllerNumber  Controller number (e.g. 7 -> "FEH-007")
     * @param ackTimeoutMs      How long to wait for the BLE start ack (default 2000 ms)
     * @return true  if BLE start was acknowledged within the timeout
     * @return false if the ack timed out
     */
    static bool enableBLE(int controllerNumber); //, uint32_t ackTimeoutMs = 2000);

    /**
     * @brief Disable BLE logging and stop the BLE log service on the ESP32.
     *
     * @return true  if the stop command was acknowledged
     * @return false if the ack timed out
     */
    static bool disableBLE();

    /**
     * @brief Check whether BLE logging is currently enabled.
     */
    static bool isBLEEnabled();

    /**
     * @brief Check whether Serial logging is currently enabled.
     */
    static bool isSerialEnabled();

    /**
     * @brief Print a formatted message (no trailing newline).
     *
     * @param fmt  printf-style format string
     * @param ...  Variable arguments
     */
    static void printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

    /**
     * @brief Print a plain string (no trailing newline).
     */
    static void print(const char *msg);

private:
    static bool s_serialEnabled;
    static bool s_bleEnabled;

    // Maximum length of a single log message (including null terminator)
    static const int LOG_BUF_SIZE = 256;

    static void _dispatch(bool newline, const char *fmt, va_list args);
    static void _send(const char *msg, bool newline);
};

#endif // FEHLOG_H

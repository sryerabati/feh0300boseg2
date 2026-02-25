/**
 * FEHLog.cpp
 *
 * Implementation of FEHLog static methods.
 */

#include <Arduino.h>
#include <stdio.h>
#include <FEHLog.h>
#include "../private_include/FEHESP32.h"
#include "../private_include/ApplicationProtocol.h"

// ---------------------------------------------------------------------------
// Static member definitions
// ---------------------------------------------------------------------------

bool FEHLog::s_serialEnabled = false;
bool FEHLog::s_bleEnabled    = false;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------

void FEHLog::enableSerial()
{
    s_serialEnabled = true;
}

void FEHLog::disableSerial()
{
    s_serialEnabled = false;
}

bool FEHLog::enableBLE(int controllerNumber)//, uint32_t ackTimeoutMs)
{
    char deviceName[16];
    snprintf(deviceName, sizeof(deviceName), "FEH-%03d", controllerNumber);
    FEHESP32::startBLELog(deviceName);
    // bool ok = FEHESP32::waitForAck(CMD_BLE_START, ackTimeoutMs);
    s_bleEnabled = true;
    return true;
}

bool FEHLog::disableBLE()
{
    bool ok = FEHESP32::stopBLELog();
    s_bleEnabled = false;
    return ok;
}

bool FEHLog::isBLEEnabled()
{
    return s_bleEnabled;
}

bool FEHLog::isSerialEnabled()
{
    return s_serialEnabled;
}

void FEHLog::printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    _dispatch(false, fmt, args);
    va_end(args);
}

void FEHLog::print(const char *msg)
{
    _send(msg, false);
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void FEHLog::_dispatch(bool newline, const char *fmt, va_list args)
{
    char buf[LOG_BUF_SIZE];
    vsnprintf(buf, sizeof(buf), fmt, args);
    _send(buf, newline);
}

void FEHLog::_send(const char *msg, bool newline)
{
    if (s_serialEnabled)
    {
        if (newline)
            Serial.println(msg);
        else
            Serial.print(msg);
    }

    if (s_bleEnabled)
    {
        if (newline)
        {
            // Append newline for BLE consumers that parse line-by-line
            char buf[LOG_BUF_SIZE];
            snprintf(buf, sizeof(buf), "%s\n", msg);
            FEHESP32::sendBLELog(buf);
        }
        else
        {
            FEHESP32::sendBLELog(msg);
        }
    }
}

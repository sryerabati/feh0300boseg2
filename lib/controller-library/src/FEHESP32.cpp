#include "../private_include/FEHESP32.h"
#include <string.h>
#include <Arduino.h>

ESP32Version FEHESP32::s_version = {0, 0, 0, 0xFF};
bool FEHESP32::s_connected = false;
bool FEHESP32::s_flashing = false;
bool FEHESP32::s_flashComplete = false;
bool FEHESP32::s_flashError = false;
uint8_t FEHESP32::s_flashErrorCode = 0;
uint32_t FEHESP32::s_flashBytes = 0;
uint32_t FEHESP32::s_flashTotal = 0;
bool FEHESP32::s_partitionValid = false;
uint8_t FEHESP32::s_validatedPartition = 0xFF;
uint8_t FEHESP32::s_lastAckedCmd = 0x00;
bool FEHESP32::s_wifiConnectResult = false;
bool FEHESP32::s_wifiConnectSuccess = false;
bool FEHESP32::s_rcsConnected = false;
ESP32RCSCallback FEHESP32::s_rcsCallback = nullptr;
uint8_t FEHESP32::s_bleState = BLE_STATE_OFF;

void FEHESP32::init()
{
    ESP32::init(FEHESP32::handleMessage);
}

void FEHESP32::begin()
{
    ESP32::powerOn();
}

void FEHESP32::poll()
{
    ESP32::poll();
}

extern volatile bool g_esp32PollPending;

void FEHESP32::servicePoll()
{
    if (g_esp32PollPending)
    {
        g_esp32PollPending = false;
        ESP32::poll();
    }
}

bool FEHESP32::ping()
{
    return ESP32::sendCommand(CMD_PING, nullptr, 0);
}

bool FEHESP32::connectWifi(const char *ssid, const char *password)
{
    uint8_t buf[34]; // Max size needed
    uint8_t ssidLen = strlen(ssid);
    uint8_t passLen = strlen(password);

    if (ssidLen > 16 || passLen > 16)
        return false;

    buf[0] = ssidLen;
    memcpy(&buf[1], ssid, ssidLen);
    buf[1 + ssidLen] = passLen;
    if (passLen > 0)
    {
        memcpy(&buf[2 + ssidLen], password, passLen);
    }

    return ESP32::sendCommand(CMD_WIFI_CONNECT, buf, 2 + ssidLen + passLen);
}

bool FEHESP32::connectWifiFast(const char *ssid, const char *password, const uint8_t *bssid, uint8_t channel)
{
    uint8_t buf[41]; // SSID(1) + 16 + PASS(1) + 16 + BSSID(6) + CHANNEL(1) = 41
    uint8_t ssidLen = strlen(ssid);
    uint8_t passLen = strlen(password);

    if (ssidLen > 16 || passLen > 16 || !bssid)
        return false;

    uint8_t pos = 0;
    buf[pos++] = ssidLen;
    memcpy(&buf[pos], ssid, ssidLen);
    pos += ssidLen;
    buf[pos++] = passLen;
    if (passLen > 0)
    {
        memcpy(&buf[pos], password, passLen);
        pos += passLen;
    }
    // Copy BSSID (6 bytes)
    memcpy(&buf[pos], bssid, 6);
    pos += 6;
    // Copy channel
    buf[pos++] = channel;

    Serial.print("FEHESP32::connectWifiFast called with SSID: ");
    Serial.println(ssid);
    return ESP32::sendCommand(CMD_WIFI_CONNECT_FAST, buf, pos);
}

bool FEHESP32::connectRCS(char region, const uint8_t *ip, const char *teamKey)
{
    uint8_t buf[15]; // 1 region + 4 ip + 1 key_len + max 9 key
    
    if (!ip)
        return false;
    
    uint8_t keyLen = strlen(teamKey);
    if (keyLen > 9)
        keyLen = 9;

    uint8_t pos = 0;
    buf[pos++] = (uint8_t)region;
    memcpy(&buf[pos], ip, 4);
    pos += 4;
    buf[pos++] = keyLen;
    memcpy(&buf[pos], teamKey, keyLen);
    pos += keyLen;

    return ESP32::sendCommand(CMD_RCS_CONNECT, buf, pos);
}

bool FEHESP32::disconnectRCS()
{
    return ESP32::sendCommand(CMD_RCS_DISCONNECT, nullptr, 0);
}

void FEHESP32::setRCSCallback(ESP32RCSCallback cb)
{
    s_rcsCallback = cb;
}

bool FEHESP32::isRCSConnected()
{
    return s_rcsConnected;
}

bool FEHESP32::downloadAndFlash(const char *url)
{
    uint8_t buf[33];
    uint8_t urlLen = strlen(url);

    if (urlLen > 32)
        return false;

    buf[0] = urlLen;
    memcpy(&buf[1], url, urlLen);

    s_flashing = true;
    s_flashComplete = false;
    s_flashError = false;
    s_flashBytes = 0;
    s_flashTotal = 0;

    return ESP32::sendCommand(CMD_DOWNLOAD_AND_FLASH, buf, 1 + urlLen);
}

bool FEHESP32::validatePartition()
{
    s_partitionValid = false;
    s_validatedPartition = 0xFF;
    return ESP32::sendCommand(CMD_VALIDATE_PARTITION, nullptr, 0);
}

bool FEHESP32::setBootPartition(uint8_t partition)
{
    return ESP32::sendCommand(CMD_SET_BOOT_PARTITION, &partition, 1);
}

void FEHESP32::reset(bool factoryReset)
{
    ESP32::reset(factoryReset);
    // Reset internal state
    s_version = {0, 0, 0, 0xFF};
    s_connected = false;
    s_flashing = false;
    s_flashComplete = false;
    s_flashError = false;
    s_partitionValid = false;
    s_validatedPartition = 0xFF;
    s_lastAckedCmd = 0x00;
    s_wifiConnectResult = false;
    s_wifiConnectSuccess = false;
    s_rcsConnected = false;
    s_bleState = BLE_STATE_OFF;
}

bool FEHESP32::startBLELog(const char *deviceName)
{
    // Set device name first if non-default
    if (deviceName != nullptr && strlen(deviceName) > 0) {
        uint8_t nameLen = strlen(deviceName);
        if (nameLen > 16) nameLen = 16;
        
        uint8_t buf[17]; // 1 byte length + 16 bytes name
        buf[0] = nameLen;
        memcpy(&buf[1], deviceName, nameLen);
        
        if (!ESP32::sendCommand(CMD_BLE_SET_NAME, buf, 1 + nameLen)) {
            return false;
        }

        // Wait for ESP32 to process the set-name command before sending start.
        if (!waitForAck(CMD_BLE_SET_NAME, 5000)) {
            return false;
        }
    }

    // Small delay to let ESP32 SPI slave task re-enter spi_slave_transmit()
    delay(20);
    
    return ESP32::sendCommand(CMD_BLE_START, nullptr, 0);
}

bool FEHESP32::stopBLELog()
{
    return ESP32::sendCommand(CMD_BLE_STOP, nullptr, 0);
}

bool FEHESP32::setDebugLevel(uint8_t level)
{
    return ESP32::sendCommand(CMD_SET_DEBUG_LEVEL, &level, 1);
}

bool FEHESP32::sendBLELog(const char *msg)
{
    if (msg == nullptr) return false;
    uint8_t len = strlen(msg);
    if (len == 0 || len > 32) return false;
    return ESP32::sendCommand(CMD_BLE_SEND_LOG, (const uint8_t *)msg, len);
}

bool FEHESP32::isBLEConnected()
{
    return s_bleState == BLE_STATE_CONNECTED;
}

uint8_t FEHESP32::getBLEState()
{
    return s_bleState;
}

ESP32Version FEHESP32::getVersion()
{
    return s_version;
}

bool FEHESP32::isConnected()
{
    return s_connected;
}

float FEHESP32::getFlashProgress()
{
    if (s_flashTotal == 0)
        return 0.0f;
    return (float)s_flashBytes / (float)s_flashTotal;
}

bool FEHESP32::isFlashing()
{
    return s_flashing;
}

bool FEHESP32::isFlashComplete()
{
    return s_flashComplete;
}

bool FEHESP32::hasFlashError()
{
    return s_flashError;
}

uint8_t FEHESP32::getFlashErrorCode()
{
    return s_flashErrorCode;
}

bool FEHESP32::isPartitionValid()
{
    return s_partitionValid;
}

uint8_t FEHESP32::getValidatedPartition()
{
    return s_validatedPartition;
}

bool FEHESP32::waitForAck(uint8_t cmdId, uint32_t timeoutMs)
{
    // Reset ACK state before waiting
    s_lastAckedCmd = 0x00;

    unsigned long startTime = millis();
    while (millis() - startTime < timeoutMs)
    {
        poll();
        if (s_lastAckedCmd == cmdId)
        {
            return true;
        }
        delay(10);
    }
    return false;
}

bool FEHESP32::waitForWifiConnect(uint32_t timeoutMs)
{
    // Reset WiFi connect result state before waiting
    s_wifiConnectResult = false;
    s_wifiConnectSuccess = false;

    unsigned long startTime = millis();
    while (millis() - startTime < timeoutMs)
    {
        poll();
        if (s_wifiConnectResult)
        {
            return s_wifiConnectSuccess;
        }
        delay(10);
    }
    return false;
}

void FEHESP32::handleMessage(const uint8_t *msg, uint8_t len)
{
    // msg includes header: [SYNC][SYNC][CMD][LENGTH][DATA...]
    if (len < 4)
        return;

    uint8_t cmd = msg[2];
    // uint8_t dataLen = msg[3];
    const uint8_t *data = &msg[4];

    switch (cmd)
    {
    case RSP_ACK:
        // RSP_ACK contains the command ID being acknowledged
        if (len >= 5)
        {
            s_lastAckedCmd = data[0];
        }
        break;

    case RSP_PONG:
        if (len >= 8)
        {
            s_version.major = data[0];
            s_version.minor = data[1];
            s_version.patch = data[2];
            s_version.partition = data[3];
        }
        break;

    case NOTIFY_DEBUG:
        // Log ESP32 debug message to Serial
        if (len > 4)
        {
            uint8_t dataLen = msg[3];
            Serial.print("ESP32 DEBUG: ");
            for (uint8_t i = 0; i < dataLen; ++i)
            {
                Serial.write(data[i]);
            }
            Serial.println();
        }
        break;

    case NOTIFY_WIFI_CONNECTED:
        s_connected = true;
        s_wifiConnectResult = true;
        s_wifiConnectSuccess = true;
        break;

    case NOTIFY_WIFI_DISCONNECTED:
    case NOTIFY_WIFI_FAILED:
        s_connected = false;
        s_wifiConnectResult = true;
        s_wifiConnectSuccess = false;
        break;

    case NOTIFY_RCS_CONNECTED:
        s_rcsConnected = true;
        break;

    case NOTIFY_RCS_DISCONNECTED:
        s_rcsConnected = false;
        break;

    case NOTIFY_RCS_DATA:
        if (s_rcsCallback && len > 4)
        {
            uint8_t dataLen = msg[3];
            s_rcsCallback(data, dataLen);
        }
        break;

    case NOTIFY_FLASH_PROGRESS:
        if (len >= 12)
        {
            memcpy(&s_flashBytes, &data[0], 4);
            memcpy(&s_flashTotal, &data[4], 4);
            s_flashing = true;
        }
        break;

    case NOTIFY_FLASH_COMPLETE:
        s_flashing = false;
        s_flashComplete = true;
        break;

    case NOTIFY_FLASH_FAILED:
        s_flashing = false;
        s_flashError = true;
        if (len >= 5)
        {
            s_flashErrorCode = data[0];
        }
        break;

    case RSP_PARTITION_VALID:
        // RSP_PARTITION_VALID itself means validation succeeded
        // data[0] = partition (0x01 for OTA_0)
        // data[1-4] = firmware size (uint32_t, little-endian)
        if (len >= 5)
        {
            s_partitionValid = true;
            s_validatedPartition = data[0]; // Partition that was validated
        }
        break;

    case RSP_BLE_STATUS:
        if (len >= 5)
        {
            s_bleState = data[0];
        }
        break;

    case NOTIFY_BLE_CLIENT_EVENT:
        if (len >= 5)
        {
            if (data[0] == BLE_EVENT_CLIENT_CONNECTED)
            {
                s_bleState = BLE_STATE_CONNECTED;
            }
            else if (data[0] == BLE_EVENT_CLIENT_DISCONNECTED)
            {
                s_bleState = BLE_STATE_ADVERTISING;
            }
        }
        break;
    }
}

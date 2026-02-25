#ifndef FEHESP32_H
#define FEHESP32_H

#include <stdint.h>
#include "esp32.h"
#include "UpdaterProtocol.h"
#include "ApplicationProtocol.h"

// Define partition constants if not already defined
#ifndef PARTITION_FACTORY
#define PARTITION_FACTORY 0x00
#endif
#ifndef PARTITION_OTA_0
#define PARTITION_OTA_0 0x01
#endif

struct ESP32Version
{
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
    uint8_t partition;
};

/**
 * @brief Callback function type for receiving RCS data from ESP32
 * @param data Pointer to RCS data payload (objective, lever, slider, time, kill, ...)
 * @param len Length of data
 */
typedef void (*ESP32RCSCallback)(const uint8_t *data, uint8_t len);

class FEHESP32
{
public:
    static void init();
    static void poll();
    static void begin();

    // Commands
    static bool ping();
    static bool connectWifi(const char *ssid, const char *password);
    static bool connectWifiFast(const char *ssid, const char *password, const uint8_t *bssid, uint8_t channel);
    static bool downloadAndFlash(const char *url);
    static bool validatePartition();
    static bool setBootPartition(uint8_t partition);
    static void reset(bool factoryReset);

    // RCS Commands
    static bool connectRCS(char region, const uint8_t *ip, const char *teamKey);
    static bool disconnectRCS();
    static void setRCSCallback(ESP32RCSCallback cb);
    static bool isRCSConnected();

    // Debugging
    static bool setDebugLevel(uint8_t level);

    // BLE Logging
    static bool startBLELog(const char *deviceName = "FEH-ESP32");
    static bool stopBLELog();
    static bool sendBLELog(const char *msg);
    static bool isBLEConnected();
    static uint8_t getBLEState();

    // Helpers
    static bool waitForAck(uint8_t cmdId, uint32_t timeoutMs = 1000);
    static bool waitForWifiConnect(uint32_t timeoutMs = 5000);

    // Status accessors
    static ESP32Version getVersion();
    static bool isConnected();
    static float getFlashProgress(); // 0.0 to 1.0
    static bool isFlashing();
    static bool isFlashComplete();
    static bool hasFlashError();
    static uint8_t getFlashErrorCode();
    static bool isPartitionValid();
    static uint8_t getValidatedPartition();

    // Drain deferred poll flag from main-thread context (safe to call anytime)
    static void servicePoll();

    // Internal callback
    static void handleMessage(const uint8_t *msg, uint8_t len);

private:
    static ESP32Version s_version;
    static bool s_connected;
    static bool s_flashing;
    static bool s_flashComplete;
    static bool s_flashError;
    static uint8_t s_flashErrorCode;
    static uint32_t s_flashBytes;
    static uint32_t s_flashTotal;
    static bool s_partitionValid;
    static uint8_t s_validatedPartition;
    static uint8_t s_lastAckedCmd;
    static bool s_wifiConnectResult;
    static bool s_wifiConnectSuccess;
    static bool s_rcsConnected;
    static ESP32RCSCallback s_rcsCallback;
    static uint8_t s_bleState;
};

#endif // FEHESP32_H

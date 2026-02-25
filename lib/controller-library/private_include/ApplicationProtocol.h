/**
 * @file application_protocol_h
 * @brief ESP32 <-> Mega Application Protocol
 *
 * Full-featured protocol for WiFi control, HTTP operations, and RCS data forwarding. Can be updated via OTA.
 *
 * PACKET FORMAT: [SYNC:2][CMD:1][LENGTH:1][DATA:0-41]
 *
 * Packet Fields:
 *   - SYNC: 2 bytes (0xAA 0x55)
 *     Packet synchronization marker
 *   - CMD: 1 byte (0x01-0xFF)
 *     Command identifier
 *   - LENGTH: 1 byte (0-41)
 *     Payload length
 *   - DATA: 0-41 bytes (varies)
 *     Command payload
 *
 * @warning This protocol definition is auto-generated. Do not edit manually.
 */

#ifndef APPLICATION_PROTOCOL_H
#define APPLICATION_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ============================================================================
 * Packet Structure
 * ============================================================================
 */

#define APPLICATION_PROTOCOL_SYNC_BYTE_1     0XAA
#define APPLICATION_PROTOCOL_SYNC_BYTE_2     0X55

#define APPLICATION_PROTOCOL_SYNC_SIZE       2
#define APPLICATION_PROTOCOL_CMD_SIZE        1
#define APPLICATION_PROTOCOL_LENGTH_SIZE     1
#define APPLICATION_PROTOCOL_DATA_SIZE       41

#define APPLICATION_PROTOCOL_HEADER_SIZE     4
#define APPLICATION_PROTOCOL_MAX_PACKET_SIZE 45

/*
 * ============================================================================
 * System
 * ============================================================================
 * Core system commands for device health checks, version information, and partition management.
 *
 */

/*
 * Commands (Requests)
 */

/** @brief Check if esp32 is alive and responsive. */
#define CMD_PING                             0x01

/** @brief Configure debug message verbosity. */
#define CMD_SET_DEBUG_LEVEL                  0x02
/* Debug level */
#define DEBUG_LEVEL_OFF                      0x00
#define DEBUG_LEVEL_ERROR                    0x01
#define DEBUG_LEVEL_WARNING                  0x02
#define DEBUG_LEVEL_INFO                     0x03
#define DEBUG_LEVEL_VERBOSE                  0x04


/*
 * Responses & Notifications
 */

/** @brief Command accepted and will be executed. */
#define RSP_ACK                              0x80

/** @brief Command failed with error code. */
#define RSP_ERROR                            0x81
/* Error code */
#define ERROR_INVALID_PARAMETERS             0x01
#define ERROR_NOT_CONNECTED_TO_WIFI          0x02
#define ERROR_HTTP_CONNECTION_FAILED         0x03
#define ERROR_DOWNLOAD_FAILED                0x04
#define ERROR_FLASH_FAILED                   0x05
#define ERROR_INVALID_PARTITION              0x06
#define ERROR_OUT_OF_MEMORY                  0x07
#define ERROR_TIMEOUT                        0x08
#define ERROR_UNKNOWN                        0xFE

/** @brief Response to CMD_PING. Indicates esp32 is alive and ready. */
#define RSP_PONG                             0x82
/* Current partition */
#define PARTITION_FACTORY                    0x00
#define PARTITION_OTA_0                      0x01

/** @brief ESP32 debug message sent over SPI. Messages are queued and sent asynchronously. */
#define NOTIFY_DEBUG                         0x83

/*
 * ============================================================================
 * WiFi
 * ============================================================================
 * WiFi connectivity commands including connection, disconnection, and status reporting.
 *
 */

/*
 * Commands (Requests)
 */

/** @brief Connect to WiFi network with SSID and password. Standard connection (scans all channels). */
#define CMD_WIFI_CONNECT                     0x10

/** @brief Fast WiFi connection using known BSSID and channel (skips scanning). Use this for faster connection. */
#define CMD_WIFI_CONNECT_FAST                0x11

/** @brief Disconnect from current WiFi network. */
#define CMD_WIFI_DISCONNECT                  0x12

/** @brief Get current WiFi connection status and network information. */
#define CMD_WIFI_STATUS                      0x13

/** @brief Scan for available WiFi networks. */
#define CMD_WIFI_SCAN                        0x14
/* Scan options */
#define WIFI_SCAN_OPTION_NONE                0x00
#define WIFI_SCAN_OPTION_SHOW_HIDDEN         0x01
#define WIFI_SCAN_OPTION_ACTIVE_SCAN         0x02


/*
 * Responses & Notifications
 */

/** @brief WiFi connection status and network info. */
#define RSP_WIFI_STATUS                      0x90
/* Connection state */
#define WIFI_STATE_DISCONNECTED              0x00
#define WIFI_STATE_CONNECTED                 0x01
#define WIFI_STATE_CONNECTING                0x02

/** @brief WiFi connection successful. */
#define NOTIFY_WIFI_CONNECTED                0x91

/** @brief WiFi connection failed. */
#define NOTIFY_WIFI_FAILED                   0x92
/* Failure reason */
#define WIFI_FAIL_NETWORK_NOT_FOUND          0x01
#define WIFI_FAIL_WRONG_PASSWORD             0x02
#define WIFI_FAIL_TIMEOUT                    0x03
#define WIFI_FAIL_AP_REJECTED                0x04
#define WIFI_FAIL_UNKNOWN                    0xFF

/** @brief Disconnected from WiFi (either by CMD_WIFI_DISCONNECT or unexpected). */
#define NOTIFY_WIFI_DISCONNECTED             0x93
/* Reason for disconnection */
#define WIFI_DISCONNECT_REASON_USER          0x00
#define WIFI_DISCONNECT_REASON_LOST          0x01
#define WIFI_DISCONNECT_REASON_AP_GONE       0x02

/** @brief WiFi network found during scan. */
#define NOTIFY_WIFI_SCAN_RESULT              0x94
/* Encryption type */
#define WIFI_ENCRYPTION_OPEN                 0x00
#define WIFI_ENCRYPTION_WEP                  0x01
#define WIFI_ENCRYPTION_WPA_PSK              0x02
#define WIFI_ENCRYPTION_WPA2_PSK             0x03
#define WIFI_ENCRYPTION_WPA_WPA2_PSK         0x04

/** @brief WiFi scan complete. */
#define NOTIFY_WIFI_SCAN_DONE                0x95

/*
 * ============================================================================
 * HTTP
 * ============================================================================
 * HTTP client commands for fetching small metadata files and server communication.
 *
 */

/*
 * Commands (Requests)
 */

/** @brief Fetch a small metadata file from HTTP server (max 32 bytes response). Useful for getting version.txt or other small metadata. Also verifies server is reachable. */
#define CMD_HTTP_GET                         0x20


/*
 * Responses & Notifications
 */

/** @brief HTTP fetch successful, contains the file data (max 32 bytes). */
#define RSP_HTTP_DATA                        0xA0

/*
 * ============================================================================
 * RCS
 * ============================================================================
 * Robot Communication System commands for connecting to course regions and receiving objective data. The ESP32 sends RobotIdentPacket (region + team key) to the RCS server via UDP and forwards received RobotPacket data as notifications.
 *
 */

/*
 * Commands (Requests)
 */

/** @brief Connect to RCS and start sending/receiving data. ESP32 will continuously send identification packets to the RCS server at the specified IP address (port 5000) and forward received data as notifications. WiFi must be connected first. */
#define CMD_RCS_CONNECT                      0x30

/** @brief Stop RCS communication and close UDP socket. */
#define CMD_RCS_DISCONNECT                   0x31


/*
 * Responses & Notifications
 */

/** @brief RCS communication started successfully. ESP32 is now sending robot identification packets and listening for data. */
#define NOTIFY_RCS_CONNECTED                 0xC0

/** @brief RCS communication stopped. */
#define NOTIFY_RCS_DISCONNECTED              0xC1

/** @brief RCS data update received from server. Contains objective, lever state, slider state, time remaining, and kill switch status. Optionally includes RPS position data (floats are little-endian IEEE 754). */
#define NOTIFY_RCS_DATA                      0xC2

/*
 * ============================================================================
 * Bluetooth
 * ============================================================================
 * BLE logging service for wireless debug output to a laptop.
 *
 */

/*
 * Commands (Requests)
 */

/** @brief Start BLE advertising and log service. Uses the previously set device name, or default. */
#define CMD_BLE_START                        0x40

/** @brief Stop BLE advertising, disconnect any client, and shut down BLE. */
#define CMD_BLE_STOP                         0x41

/** @brief Send a log message to the BLE ring buffer. If a client is subscribed, it will be sent immediately via notify. */
#define CMD_BLE_SEND_LOG                     0x42

/** @brief Set the BLE advertising device name. Max 16 characters. Must be called before CMD_BLE_START. */
#define CMD_BLE_SET_NAME                     0x43


/*
 * Responses & Notifications
 */

/** @brief Current BLE service state. */
#define RSP_BLE_STATUS                       0xC3
/* BLE state */
#define BLE_STATE_OFF                        0x00
#define BLE_STATE_ADVERTISING                0x01
#define BLE_STATE_CONNECTED                  0x02

/** @brief A BLE client connected or disconnected. */
#define NOTIFY_BLE_CLIENT_EVENT              0xC4
/* Event type */
#define BLE_EVENT_CLIENT_CONNECTED           0x01
#define BLE_EVENT_CLIENT_DISCONNECTED        0x02


#ifdef __cplusplus
}
#endif

#endif // APPLICATION_PROTOCOL_H
/**
 * @file updater_protocol_h
 * @brief ESP32 <-> Mega Updater Protocol
 *
 * Minimal, stable protocol for OTA updates only. Lives in factory partition and never changes after deployment.
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

#ifndef UPDATER_PROTOCOL_H
#define UPDATER_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ============================================================================
 * Packet Structure
 * ============================================================================
 */

#define UPDATER_PROTOCOL_SYNC_BYTE_1       0XAA
#define UPDATER_PROTOCOL_SYNC_BYTE_2       0X55

#define UPDATER_PROTOCOL_SYNC_SIZE         2
#define UPDATER_PROTOCOL_CMD_SIZE          1
#define UPDATER_PROTOCOL_LENGTH_SIZE       1
#define UPDATER_PROTOCOL_DATA_SIZE         41

#define UPDATER_PROTOCOL_HEADER_SIZE       4
#define UPDATER_PROTOCOL_MAX_PACKET_SIZE   45

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
#define CMD_PING                           0x01


/*
 * Responses & Notifications
 */

/** @brief Command accepted and will be executed. */
#define RSP_ACK                            0x80

/** @brief Command failed with error code. */
#define RSP_ERROR                          0x81
/* Error code */
#define ERROR_INVALID_PARAMETERS           0x01
#define ERROR_NOT_CONNECTED_TO_WIFI        0x02
#define ERROR_HTTP_CONNECTION_FAILED       0x03
#define ERROR_DOWNLOAD_FAILED              0x04
#define ERROR_FLASH_FAILED                 0x05
#define ERROR_INVALID_PARTITION            0x06
#define ERROR_OUT_OF_MEMORY                0x07
#define ERROR_TIMEOUT                      0x08
#define ERROR_UNKNOWN                      0xFE

/** @brief Response to CMD_PING. Indicates esp32 is alive and ready. */
#define RSP_PONG                           0x82
/* Current partition */
#define PARTITION_FACTORY                  0x00
#define PARTITION_OTA_0                    0x01

/** @brief ESP32 debug message sent over SPI. Messages are queued and sent asynchronously. */
#define NOTIFY_DEBUG                       0x83

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
#define CMD_WIFI_CONNECT                   0x10

/** @brief Fast WiFi connection using known BSSID and channel (skips scanning). Use this for faster connection. */
#define CMD_WIFI_CONNECT_FAST              0x11

/** @brief Disconnect from current WiFi network. */
#define CMD_WIFI_DISCONNECT                0x12

/** @brief Get current WiFi connection status and network information. */
#define CMD_WIFI_STATUS                    0x13


/*
 * Responses & Notifications
 */

/** @brief WiFi connection status and network info. */
#define RSP_WIFI_STATUS                    0x90
/* Connection state */
#define WIFI_STATE_DISCONNECTED            0x00
#define WIFI_STATE_CONNECTED               0x01
#define WIFI_STATE_CONNECTING              0x02

/** @brief WiFi connection successful. */
#define NOTIFY_WIFI_CONNECTED              0x91

/** @brief WiFi connection failed. */
#define NOTIFY_WIFI_FAILED                 0x92
/* Failure reason */
#define WIFI_FAIL_NETWORK_NOT_FOUND        0x01
#define WIFI_FAIL_WRONG_PASSWORD           0x02
#define WIFI_FAIL_TIMEOUT                  0x03
#define WIFI_FAIL_AP_REJECTED              0x04
#define WIFI_FAIL_UNKNOWN                  0xFF

/** @brief Disconnected from WiFi (either by CMD_WIFI_DISCONNECT or unexpected). */
#define NOTIFY_WIFI_DISCONNECTED           0x93
/* Reason for disconnection */
#define WIFI_DISCONNECT_REASON_USER        0x00
#define WIFI_DISCONNECT_REASON_LOST        0x01
#define WIFI_DISCONNECT_REASON_AP_GONE     0x02

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
#define CMD_HTTP_GET                       0x20


/*
 * Responses & Notifications
 */

/** @brief HTTP fetch successful, contains the file data (max 32 bytes). */
#define RSP_HTTP_DATA                      0xA0

/*
 * ============================================================================
 * OTA Update Commands
 * ============================================================================
 * Commands for downloading and flashing new firmware to the OTA partition.
 *
 */

/*
 * Commands (Requests)
 */

/** @brief Download firmware from HTTP server and write directly to OTA partition. Takes the firmware URL as parameter. ESP32 streams data from HTTP and writes to flash simultaneously. */
#define CMD_DOWNLOAD_AND_FLASH             0x30

/** @brief Set which partition to boot from on next reboot. Does not reboot automatically. */
#define CMD_SET_BOOT_PARTITION             0x31
/* Partition to boot */
#define PARTITION_FACTORY                  0x00
#define PARTITION_OTA_0                    0x01

/** @brief Validate OTA partition integrity (check if firmware is valid and bootable). */
#define CMD_VALIDATE_PARTITION             0x32


/*
 * Responses & Notifications
 */

/** @brief Partition validation result. */
#define RSP_PARTITION_VALID                0xB0
/* Partition */
#define PARTITION_OTA_0                    0x01

/** @brief Download and flash progress update (sent every 64KB or every 2 seconds). */
#define NOTIFY_FLASH_PROGRESS              0xB1

/** @brief Firmware downloaded and flashed successfully to OTA_0 partition. */
#define NOTIFY_FLASH_COMPLETE              0xB2

/** @brief Firmware download or flash operation failed. */
#define NOTIFY_FLASH_FAILED                0xB3
/* Error code */
#define FLASH_ERROR_WRITE_ERROR            0x01
#define FLASH_ERROR_VERIFICATION_FAILED    0x02
#define FLASH_ERROR_INVALID_FIRMWARE_IMAGE 0x03
#define FLASH_ERROR_OUT_OF_SPACE           0x04
#define FLASH_ERROR_CONNECTION_LOST        0x05
#define FLASH_ERROR_SERVER_ERROR           0x06
#define FLASH_ERROR_UNKNOWN                0xFF


#ifdef __cplusplus
}
#endif

#endif // UPDATER_PROTOCOL_H
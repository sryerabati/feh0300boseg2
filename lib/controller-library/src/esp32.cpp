#include "../private_include/esp32.h"
#include "../private_include/UpdaterProtocol.h"

// Minimal static state (~5 bytes of RAM only)
static ESP32MessageCallback s_rxCallback = nullptr;
static uint32_t s_lastPollTime = 0;
static bool s_isInitialized = false;

// Forward declarations
static void spiTransfer(uint8_t *tx, uint8_t *rx, uint8_t len);
static void handleReceivedMessage(const uint8_t *rxBuf);

void ESP32::init(ESP32MessageCallback messageCallback) {
	s_rxCallback = messageCallback;
	s_lastPollTime = 0;

	// Initialize pins
	pinMode(ESP32_PIN_CS, OUTPUT);
	pinMode(ESP32_PIN_EN, OUTPUT);
	pinMode(ESP32_PIN_SPARE, OUTPUT);

	// Start SPI
	SPI.begin();

	// Start ESP32 powered off
    powerOff();

	s_isInitialized = true;
}

void ESP32::powerOff() {
	digitalWrite(ESP32_PIN_EN, LOW);
	digitalWrite(ESP32_PIN_SPARE, LOW);
}

void ESP32::powerOn(bool factoryReset) {
	if (factoryReset) {
		digitalWrite(ESP32_PIN_SPARE, HIGH);
	}
	digitalWrite(ESP32_PIN_EN, HIGH);
	delay(1500);  // Wait for ESP32 to boot
	if (factoryReset) {
		digitalWrite(ESP32_PIN_SPARE, LOW);
	}
}

void ESP32::reset(bool factoryReset) {
	powerOff();
	delay(100);
	powerOn(factoryReset);
}

static void spiTransfer(uint8_t *tx, uint8_t *rx, uint8_t len) {
	SPI.beginTransaction(SPISettings(ESP32_SPI_CLOCK_HZ, MSBFIRST, SPI_MODE0));
	digitalWrite(ESP32_PIN_CS, LOW);

	for (uint8_t i = 0; i < len; ++i) {
		rx[i] = SPI.transfer(tx[i]);
	}

	digitalWrite(ESP32_PIN_CS, HIGH);
	SPI.endTransaction();
}

bool ESP32::sendCommand(uint8_t cmd, const uint8_t *data, uint8_t dataLen) {
	if (!s_isInitialized) return false;

	uint8_t tx[ESP32_TX_BUF_LEN];
	uint8_t rx[ESP32_TX_BUF_LEN];

	memset(tx, 0, sizeof(tx));
	memset(rx, 0, sizeof(rx));

	// Check if data already has sync bytes
	if (data && dataLen >= 2 && data[0] == UPDATER_PROTOCOL_SYNC_BYTE_1 && data[1] == UPDATER_PROTOCOL_SYNC_BYTE_2) {
		// Data is already a complete packet, copy directly
		memcpy(tx, data, min(dataLen, (uint8_t)ESP32_TX_BUF_LEN));
	} else {
		// Build packet with sync bytes: [SYNC][SYNC][CMD][LENGTH][DATA...]
		tx[0] = UPDATER_PROTOCOL_SYNC_BYTE_1;
		tx[1] = UPDATER_PROTOCOL_SYNC_BYTE_2;
		tx[2] = cmd;
		tx[3] = dataLen;
		
		if (dataLen > 0 && data) {
			memcpy(&tx[4], data, min(dataLen, (uint8_t)(ESP32_TX_BUF_LEN - 4)));
		}
	}

	spiTransfer(tx, rx, ESP32_TX_BUF_LEN);
	handleReceivedMessage(rx);

	return true;
}

void ESP32::poll() {
	if (!s_isInitialized) return;

	uint8_t tx[ESP32_TX_BUF_LEN];
	uint8_t rx[ESP32_TX_BUF_LEN];

	memset(tx, 0, sizeof(tx));
	memset(rx, 0, sizeof(rx));

	// Send empty poll packet (all zeros - no sync bytes needed for poll)
	// ESP32 will respond with queued message if any

	spiTransfer(tx, rx, ESP32_TX_BUF_LEN);
	handleReceivedMessage(rx);
}

void ESP32::update() {
	if (!s_isInitialized) return;

	uint32_t now = millis();
	if (now - s_lastPollTime >= ESP32_POLL_INTERVAL_MS) {
		s_lastPollTime = now;
		poll();
	}
}

static void handleReceivedMessage(const uint8_t *rxBuf) {
	// Check for sync bytes to identify valid response
	// Response format: [SYNC:0xAA][SYNC:0x55][CMD][LENGTH][DATA...]

    // Serial.println("ESP32::handleReceivedMessage called");
	
	if (rxBuf[0] != UPDATER_PROTOCOL_SYNC_BYTE_1 || rxBuf[1] != UPDATER_PROTOCOL_SYNC_BYTE_2) {
		// No valid sync pattern - empty response or garbage
		return;
	}
	
	uint8_t cmd = rxBuf[2];
	uint8_t dataLen = rxBuf[3];
	
	// Validate data length
	if (dataLen > (ESP32_TX_BUF_LEN - UPDATER_PROTOCOL_HEADER_SIZE)) {
		dataLen = ESP32_TX_BUF_LEN - UPDATER_PROTOCOL_HEADER_SIZE;
	}
	
	// Calculate total message length (including header)
	uint8_t totalLen = UPDATER_PROTOCOL_HEADER_SIZE + dataLen;
	
	// Call callback with the full packet (so Updater can parse it)
	if (s_rxCallback) {
		s_rxCallback(rxBuf, totalLen);
	}
}


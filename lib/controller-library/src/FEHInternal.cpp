/**
 * @file FEHInternal.cpp
 * @authors Brian Jia, Adam Exley
 * @brief Internal helper functions implementation for FEH library
 *
 * This file implements the core internal functionality of the FEH library including:
 * - Arduino setup() and loop() wrappers
 * - Hardware initialization sequence
 * - ESP32 firmware version checking and factory reset logic
 * - Health monitoring and fault detection
 * - Error handling and robot kill mechanisms
 * - Splash screen display during initialization
 * - Hardware monitoring (battery, I2C, I/O faults)
 */

#include <FEH.h>
#include "../private_include/FEHInternal.h"
#include "../private_include/scheduler.h"
#include "../private_include/FEHESP32.h"
#include <avr/wdt.h>

//=============================================================================
// FORWARD DECLARATIONS
//=============================================================================

/// @brief Initialize and display the startup splash screen with OSU logo
void initSplashScreen();

/// @brief Update the status message on the splash screen
/// @param status Status text to display (centered)
void updateSplashScreenWithStatus(const char *status);

/// @brief Wait for the ESP32 to be ready by polling for a successful ping response
/// @param timeout_ms
bool waitForESP32Ready(unsigned long timeout_ms);

//=============================================================================
// SYSTEM UTILITIES
//=============================================================================

/**
 * @brief Perform software reset using watchdog timer
 *
 * Forces an ATmega2560 reset by enabling the watchdog timer with a short timeout
 * and then waiting for it to expire. This is used to recover from certain fault
 * conditions (e.g., shield power-off while robot is running).
 *
 * @note This function does not return - system will reset
 */
static void _softwareReset()
{
    // Disable watchdog first to ensure clean state
    wdt_disable();

    // Enable watchdog with 250ms timeout
    wdt_enable(WDTO_250MS);

    // Wait for watchdog to trigger reset
    while (1)
    {
        // Infinite loop - watchdog will reset us
    }
}

//=============================================================================
// HEALTH MONITORING
//=============================================================================

/**
 * @brief Periodic health check callback
 *
 * Scheduled task that runs approximately every 100ms to monitor robot health:
 * - Checks for I2C bus faults
 * - Checks for I/O expander faults
 * - Monitors battery voltage
 * - Controls low battery LED
 * - Triggers software reset if shield is powered off
 * - Kills robot on hardware faults
 *
 * @note Called by scheduler every ~1563 ticks (~100ms at 64µs/tick)
 * @note Automatically reschedules itself after each execution
 */
// Flag set by ISR; actual SPI poll happens from main-thread context in servicePoll()
volatile bool g_esp32PollPending = false;

static void eventESP32Poll()
{
    // Never do SPI from an ISR - the LCD may have its CS asserted mid-draw.
    // Just set a flag; servicePoll() will drain it from the main thread.
    g_esp32PollPending = true;
    // 781 ticks = ~50ms at 64µs/tick
    scheduleEvent(eventESP32Poll, 781);
}

static void eventHealthCheck()
{
    // TODO: Make health checks write to LCD, with concurrency and reentrancy in mind
    // TODO: - Reading motor status lines FAULT_12, FAULT_34

    // Check hardware fault lines
    bool i2cFault = _I2CFault();
    bool ioFault = _IOFault();

    if (i2cFault && ioFault)
    {
        // Both faults indicate shield is likely powered off
        // Perform software reset to recover cleanly
        _softwareReset();
    }
    else if (i2cFault)
    {
        // I2C fault only - this is a critical error
        _kill("I2C fault");
    }
    else if (ioFault)
    {
        // I/O fault only - this is a critical error
        _kill("IO fault");
    }

    // Monitor battery voltage and control warning LED
    float voltage = _batteryVoltage();
    if (voltage < LOW_BATTERY_THRESHOLD)
    {
        digitalWrite(BATT_LOW_LED_PIN, HIGH); // Turn on low battery LED
    }
    else
    {
        digitalWrite(BATT_LOW_LED_PIN, LOW); // Turn off low battery LED
    }

    // Reschedule for next health check
    // 1563 ticks = ~100ms at 64µs/tick
    scheduleEvent(eventHealthCheck, 1563);
}

//=============================================================================
// ARDUINO SETUP AND LOOP
//=============================================================================

/**
 * Arduino setup() and loop() are wrapped here to perform library initialization
 * before calling the student's ERCMain() function.
 *
 * Initialization sequence:
 * 1. Configure hardware pins and power management
 * 2. Initialize LCD and display splash screen
 * 3. Boot and verify ESP32 firmware
 * 4. Initialize touchscreen, serial, and SD card
 * 5. Start health monitoring
 * 6. Enable motors and play startup tone
 * 7. Call student's ERCMain()
 */

// Exclude setup() and loop() during unit testing (PlatformIO defines PIO_UNIT_TESTING)
#ifndef PIO_UNIT_TESTING

/// @brief Student code entry point (defined by user)
void ERCMain(void);

/**
 * @brief Arduino setup function - library and hardware initialization
 *
 * This function is called once at startup before the student's ERCMain().
 * It initializes all hardware subsystems, performs ESP32 firmware verification,
 * and sets up the robot for operation.
 *
 * Initialization steps:
 * 1. Pin configuration (motors, battery, faults, LEDs)
 * 2. LCD initialization and splash screen display
 * 3. ESP32 boot and firmware version verification
 * 4. Touchscreen initialization
 * 5. Serial communication setup
 * 6. SD card detection and mounting
 * 7. Motor driver enable
 * 8. Health monitoring startup
 * 9. Startup sound and random seed
 * 10. Call ERCMain()
 */
void setup()
{
    Serial.begin(115200);

    Serial.println("FEH Library initializing...");

    //-------------------------------------------------------------------------
    // Phase 1: Pin Configuration
    //-------------------------------------------------------------------------

    // Initialize ESP32 control pins (power, reset, etc.)
    pinMode(ESP32_PIN_CS, OUTPUT);
    pinMode(ESP32_PIN_EN, OUTPUT);
    pinMode(ESP32_PIN_SPARE, OUTPUT);

    // Start with ESP32 powered off
    digitalWrite(ESP32_PIN_EN, LOW);
    digitalWrite(ESP32_PIN_SPARE, LOW);

    // Configure monitoring and status pins
    pinMode(BATTERY_PIN, INPUT);       // Battery voltage ADC
    pinMode(I2C_nFAULT_PIN, INPUT);    // I2C bus fault indicator
    pinMode(IO_nFAULT_PIN, INPUT);     // I/O expander fault indicator
    pinMode(BATT_LOW_LED_PIN, OUTPUT); // Low battery warning LED

    // Configure LCD-related pins as high-impedance inputs
    // These pins must be set before LCD initialization
    pinMode(TOUCHSCREEN_IRQ_PIN, INPUT);
    pinMode(SD_DETECT_PIN, INPUT);
    pinMode(SD_CS_PIN, OUTPUT); // Set SD_CS as OUTPUT and HIGH to deselect SD card

    // Pull LCD-related pins LOW to prevent floating inputs
    // CRITICAL: Touchscreen IRQ pin must be LOW or touchscreen will malfunction
    digitalWrite(TOUCHSCREEN_IRQ_PIN, LOW);
    digitalWrite(SD_DETECT_PIN, LOW);
    // CRITICAL: Keep SD_CS HIGH to prevent SPI bus interference with ESP32
    // SD card shares the SPI bus, and pulling CS low interferes with ESP32 communication
    digitalWrite(SD_CS_PIN, HIGH);

    //-------------------------------------------------------------------------
    // Phase 2: Motor Safety
    //-------------------------------------------------------------------------

    // Put motors in sleep mode and stop all PWM outputs
    // This ensures motors don't move during initialization
    FEHMotor::SetAllSleep(true);
    FEHMotor::StopAll();

    //-------------------------------------------------------------------------
    // Phase 3: Wait for Power
    //-------------------------------------------------------------------------

    // Block until shield is powered on (battery voltage above threshold)
    // Subtract 1V to allow some margin below threshold
    while (_batteryVoltage() < LOW_BATTERY_THRESHOLD - 1)
    {
        // Busy wait for power
    }

    //-------------------------------------------------------------------------
    // Phase 4: LCD Initialization and Splash Screen
    //-------------------------------------------------------------------------

    // Initialize LCD display controller
    // Must be done early so we can show status during initialization
    ILI9341.begin();
    LCD.SetOrientation(FEHLCD::South);
    LCD.SetFontColor(BLACK);
    LCD.SetFontSize(2);

    // Display Ohio State splash screen
    initSplashScreen();
    Serial.println("Initializing splash screen...");
    updateSplashScreenWithStatus("Starting ESP32...");

    //-------------------------------------------------------------------------
    // Phase 5: ESP32 Boot and Firmware Verification
    //-------------------------------------------------------------------------

    Serial.println("Starting ESP32 firmware verification...");
    updateSplashScreenWithStatus("Connecting to ESP32...");

    FEHESP32::init();
    FEHESP32::begin();

    bool ready = waitForESP32Ready(1000);
    ESP32Version ver = FEHESP32::getVersion();

    bool needUpdate = false;
    bool runningAppPartition = true;

    // Check if ESP32 is running the expected firmware version
    if (!ready)
    {
        needUpdate = true;
    }
    else
    {
        bool outdated = (ver.major < EXPECTED_ESP_FIRMWARE_VERSION_MAJOR ||
                         (ver.major == EXPECTED_ESP_FIRMWARE_VERSION_MAJOR && ver.minor < EXPECTED_ESP_FIRMWARE_VERSION_MINOR) ||
                         (ver.major == EXPECTED_ESP_FIRMWARE_VERSION_MAJOR && ver.minor == EXPECTED_ESP_FIRMWARE_VERSION_MINOR && ver.patch < EXPECTED_ESP_FIRMWARE_VERSION_PATCH));

        if (ver.partition != PARTITION_OTA_0)
        {
            // If running partition is not the app partition, we need to update
            needUpdate = true;
            runningAppPartition = false;
        }
        else if (outdated)
        {
            // If the firmware version is older than expected, we need to update
            needUpdate = true;
        }
    }

    bool networkAvailable = false;
    // If we are running the app partition, first check if the OTA update host wifi network is available
    if (needUpdate)
    {
        updateSplashScreenWithStatus("ESP32 should update, checking network...");

        uint8_t bssid[] = OTA_WIFI_BSSID_BYTES;
        FEHESP32::connectWifiFast(OTA_WIFI_SSID, OTA_WIFI_PASS, bssid, OTA_WIFI_CHANNEL);
        // FEHESP32::connectWifi(OTA_WIFI_SSID, OTA_WIFI_PASS);

        // Wait for WiFi connection result (timeout 5s)
        bool connected = FEHESP32::waitForWifiConnect(5000);
        networkAvailable = connected;
    }

    // Proceed with the update process if we need to update and the network is available
    if (needUpdate && networkAvailable)
    {
        // If we are running in the app, we need to factory reset to enter the updater partition
        if (runningAppPartition)
        {
            // Factory reset ESP32
            updateSplashScreenWithStatus("Updating ESP32...");
            FEHESP32::reset(true);

            // Wait for esp32 to be ready (timeout 2s)
            waitForESP32Ready(2000);

            // Display updater version on splash screen
            ver = FEHESP32::getVersion();
            char statusBuf[64];
            snprintf(statusBuf, sizeof(statusBuf), "Updater v%d.%d.%d", ver.major, ver.minor, ver.patch);
            updateSplashScreenWithStatus(statusBuf);
            delay(250);

            // Connect to WiFi (fast connect using known BSSID and channel)
            updateSplashScreenWithStatus("Connecting to WiFi...");
            uint8_t bssid[] = OTA_WIFI_BSSID_BYTES;
            FEHESP32::connectWifiFast(OTA_WIFI_SSID, OTA_WIFI_PASS, bssid, OTA_WIFI_CHANNEL);
            // FEHESP32::connectWifi(OTA_WIFI_SSID, OTA_WIFI_PASS);

            // Wait for WiFi connection result (timeout 5s)
            FEHESP32::waitForWifiConnect(5000);
            if (!FEHESP32::isConnected())
            {
                Serial.println("WiFi Connection Failed");
                updateSplashScreenWithStatus("WiFi Connection Failed");
                delay(1000);
            }
        }
        else
        {
            // Display updater version on splash screen
            char statusBuf[64];
            snprintf(statusBuf, sizeof(statusBuf), "Updater v%d.%d.%d", ver.major, ver.minor, ver.patch);
            updateSplashScreenWithStatus(statusBuf);
            delay(250);
        }

        // Download and Flash
        updateSplashScreenWithStatus("Downloading firmware update...");
        FEHESP32::downloadAndFlash(FIRMWARE_URL);
        // wait for flash progress, 10 second timeout for flash to start
        unsigned long t0 = millis();
        while (FEHESP32::getFlashProgress() == 0.0)
        {
            FEHESP32::poll();
            delay(50);
            if (millis() - t0 > 10000)
            {
                updateSplashScreenWithStatus("Flash timeout");
                delay(500);
                break;
            }
        }

        while (FEHESP32::isFlashing())
        {
            FEHESP32::poll();
            char buf[64];
            snprintf(buf, sizeof(buf), "Flashing: %d%%", (int)(FEHESP32::getFlashProgress() * 100));
            updateSplashScreenWithStatus(buf);
            delay(50);
        }

        if (FEHESP32::hasFlashError())
        {
            Serial.println("Flash Error");
            updateSplashScreenWithStatus("Flash Error");
            delay(500);
        }

        if (!FEHESP32::isFlashComplete())
        {
            Serial.println("Flash Incomplete");
            updateSplashScreenWithStatus("Flash Incomplete");
            delay(500);
        }

        // Validate
        updateSplashScreenWithStatus("Validating...");
        FEHESP32::validatePartition();

        // Wait for validation response (timeout 5s)
        t0 = millis();
        bool validated = false;
        while (millis() - t0 < 5000)
        {
            FEHESP32::poll();
            if (FEHESP32::isPartitionValid())
            {
                validated = true;
                break;
            }
            delay(50);
        }

        if (!validated || FEHESP32::getValidatedPartition() != PARTITION_OTA_0)
        {
            Serial.println("Partition Validation Failed");
            updateSplashScreenWithStatus("Partition Validation Failed");
            delay(500);
        }

        // Set Boot Partition
        FEHESP32::setBootPartition(PARTITION_OTA_0);
        if (!FEHESP32::waitForAck(CMD_SET_BOOT_PARTITION, 1000))
        {
            Serial.println("Set boot partition failed");
            updateSplashScreenWithStatus("Set boot partition failed");
            delay(500);
        }

        // Reset
        updateSplashScreenWithStatus("Rebooting ESP32...");
        FEHESP32::reset(false);
    }
    else if (needUpdate && !networkAvailable)
    {
        updateSplashScreenWithStatus("Network Unavailable, continuing with existing firmware...");
        delay(500);
    }

    waitForESP32Ready(2000);

    ver = FEHESP32::getVersion();
    char statusBuf[64];
    snprintf(statusBuf, sizeof(statusBuf), "ESP32 Ready v%d.%d.%d", ver.major, ver.minor, ver.patch);
    updateSplashScreenWithStatus(statusBuf);
    delay(500);

    //-------------------------------------------------------------------------
    // Phase 6: Health Monitoring Setup
    //-------------------------------------------------------------------------

    // Start automated health monitoring (battery, faults, etc.)
    // Runs every ~100ms in background via scheduler
    scheduleEvent(eventHealthCheck, 0);

    //-------------------------------------------------------------------------
    // Phase 7: Touchscreen Initialization
    //-------------------------------------------------------------------------

    updateSplashScreenWithStatus("Initializing touchscreen...");

    // Initialize FT6206 capacitive touchscreen controller
    // 128 = touch sensitivity threshold
    FT6206.begin(128, &Wire);

    // Set LCD defaults to match Proteus simulator
    LCD.SetOrientation(FEHLCD::South);
    LCD.SetFontColor(FEHLCD::White);
    LCD.SetFontSize(2);

    //-------------------------------------------------------------------------
    // Phase 8: Serial Communication
    //-------------------------------------------------------------------------

    updateSplashScreenWithStatus("Starting serial communication...");
    // Serial.begin(115200);

    //-------------------------------------------------------------------------
    // Phase 9: SD Card Detection and Initialization
    //-------------------------------------------------------------------------

    // updateSplashScreenWithStatus("Checking for SD card...");

    // // Configure SD card detect pin with pull-up
    // // Pin is LOW when SD card is inserted
    // pinMode(SD_DETECT_PIN, INPUT_PULLUP);

    // if (digitalRead(SD_DETECT_PIN) == LOW)
    // {
    //     // SD card is present - attempt to mount filesystem
    //     updateSplashScreenWithStatus("Initializing SD card...");
    //     pinMode(SD_CS_PIN, OUTPUT);

    //     if (FAT.begin(SD_CS_PIN))
    //     {
    //         // SD card mounted successfully
    //         updateSplashScreenWithStatus("SD card ready");
    //     }
    //     else
    //     {
    //         // SD card present but mount failed
    //         updateSplashScreenWithStatus("SD card initialization failed");

    //         // Return pin to high-impedance to prevent interference
    //         pinMode(SD_CS_PIN, INPUT);
    //         digitalWrite(SD_CS_PIN, LOW);
    //     }
    // }
    // else
    // {
    //     // No SD card detected
    //     updateSplashScreenWithStatus("No SD card detected");

    //     // Set pins to high-impedance
    //     pinMode(SD_CS_PIN, INPUT);
    //     digitalWrite(SD_CS_PIN, LOW);
    //     digitalWrite(SD_DETECT_PIN, LOW);

    //     Serial.println("No SD Card detected.");
    // }

    // // Brief pause to allow user to see SD card status
    // delay(200);

    //-------------------------------------------------------------------------
    // Phase 10: Motor Driver Enable
    //-------------------------------------------------------------------------

    updateSplashScreenWithStatus("Enabling motors...");

    // Wake motor drivers by asserting nMSLEEP
    // This was originally in motor.cpp but moved here to prevent individual
    // motor instances from overriding a full-system kill state
    pinMode(MOTOR_nSLEEP_PIN, OUTPUT);
    digitalWrite(MOTOR_nSLEEP_PIN, HIGH);

    //-------------------------------------------------------------------------
    // Phase 11: Startup Complete
    //-------------------------------------------------------------------------

    updateSplashScreenWithStatus("Playing startup sound...");
    Serial.println("FEH Library initialized successfully.");

    // Play ascending tone sequence to indicate successful initialization
    Buzzer.Tone(Buzzer.NOTE_C5);
    Sleep(20);
    Buzzer.Tone(Buzzer.NOTE_D5);
    Sleep(20);
    Buzzer.Tone(Buzzer.NOTE_E5);
    Sleep(20);
    Buzzer.Tone(Buzzer.NOTE_G5);
    Sleep(20);
    Buzzer.Tone(Buzzer.NOTE_A5);
    Sleep(20);
    Buzzer.Tone(Buzzer.NOTE_C6);
    Sleep(20);
    Buzzer.Off();

    // Seed random number generator with current time
    randomSeed(TimeNow());

    // Wake motors from sleep (ready for use)
    FEHMotor::SetAllSleep(false);

    // Clear splash screen and prepare for user code
    LCD.Clear();
    LCD.SetFontColor(FEHLCD::White);
    LCD.SetFontSize(2);

    //-------------------------------------------------------------------------
    // Call Student Code
    //-------------------------------------------------------------------------

    // Start automatic ESP32 polling every ~50ms via scheduler
    // Deferred until here so it doesn't fire during hardware initialization
    scheduleEvent(eventESP32Poll, 781);

    // Transfer control to student's main function
    // This function should never return
    ERCMain();
}

/**
 * @brief Arduino loop function - not used
 *
 * The FEH library uses ERCMain() as the entry point for student code instead
 * of the traditional Arduino loop() function. This allows setup() to call
 * ERCMain() directly without returning, giving students a more traditional
 * main() function structure.
 *
 * @note This function should never be called
 */
void loop()
{
    // Empty - ERCMain() is used instead
}

#endif // PIO_UNIT_TESTING

//=============================================================================
// ERROR HANDLING IMPLEMENTATION
//=============================================================================

void _lcdErrorPrelude()
{
    // Safe to call multiple times - reinitializes LCD if needed
    ILI9341.begin();

    // Set landscape orientation for error display
    ILI9341.setRotation(1);

    // Fill screen with red background for visibility
    ILI9341.fillScreen(FEHLCD::Red);

    // Position cursor at top-left
    ILI9341.setCursor(0, 0);

    // Large white "ERROR!" heading
    ILI9341.setTextColor(FEHLCD::White);
    ILI9341.setTextSize(8);
    ILI9341.println("ERROR!");

    // Switch to smaller text for error details
    ILI9341.setTextSize(2);
    ILI9341.println(); // Add blank line after heading
}

bool _checkRange(const char *funcName, const char *valName, int val, int min, int max)
{
    // Check if value is within valid range [min, max]
    if (val < min || val > max)
    {
        // Print detailed error message to serial console
        Serial.print("ERROR! ");
        Serial.print(funcName);
        Serial.print("(): ");
        Serial.print(valName);
        Serial.print(" out of range. Minimum is ");
        Serial.print(min);
        Serial.print(", maximum is ");
        Serial.print(max);
        Serial.print(". Value of ");
        Serial.print(val);
        Serial.println(" provided.");

        return false;
    }

    return true;
}

bool _checkRangeFatal(const char *funcName, const char *valName, int val, int min, int max)
{
    // First perform standard range check (prints to serial)
    bool check = _checkRange(funcName, valName, val, min, max);

    if (!check)
    {
        // Range check failed - display error on LCD and kill robot
        _lcdErrorPrelude();

        // Display function name
        LCD.Write(funcName);
        LCD.WriteLine("(): ");

        // Display parameter name
        LCD.Write(valName);
        LCD.WriteLine(" out of range.");
        LCD.WriteLine();

        // Display valid range
        LCD.Write("Minimum is ");
        LCD.Write(min);
        LCD.WriteLine(".");
        LCD.Write("Maximum is ");
        LCD.Write(max);
        LCD.WriteLine(".");

        // Display invalid value that was provided
        LCD.Write("Value of ");
        LCD.Write(val);
        LCD.WriteLine(" provided.");

        // Note: Caller typically calls _killNoScreen() after this
    }

    return check;
}

void _fatalError(const char *msg)
{
    // Ensure Arduino core is fully initialized
    // This is critical because _fatalError() can be called from global constructors
    // which run BEFORE Arduino's main() function (not ERCMain, but the hidden
    // Arduino main() that calls setup() and loop())
    init();

    // Display error on LCD with red background
    _lcdErrorPrelude();
    ILI9341.print(msg);

    // Kill robot (plays tone, stops motors, loops forever)
    _killNoScreen();
}

void _fatalError()
{
    // Call _fatalError() with generic error message
    _fatalError("Unspecified fatal error.");
}

//=============================================================================
// ROBOT KILL / SAFETY IMPLEMENTATION
//=============================================================================

void _kill()
{
    // Call _kill() with generic message
    _kill("Unspecified kill.");
}

void _kill_esp()
{
    // ESP32 has pulled BOOT_SEL low - potential kill signal
    // Need to verify shield is actually on before killing

    // Brief delay to allow voltage reading to stabilize
    delay(10);

    // Check if shield is powered on
    bool shield_on = (_batteryVoltage() > LOW_BATTERY_THRESHOLD - 1);

    if (shield_on)
    {
        // Shield is on - this is a legitimate RCS kill command
        _kill("RCS kill signal");
    }
    else
    {
        // Shield is off - BOOT_SEL went low due to power-down, not a kill
        // Perform software reset to recover cleanly
        _softwareReset();
    }
}

void _kill(const char *reason)
{
    // First stop motors and disable interrupts without LCD output
    // Pass false to loop parameter so we return and can display kill screen
    _killNoScreen(false);

    // Display kill screen with reason
    ILI9341.begin();
    ILI9341.setRotation(1); // Landscape orientation

    // Fill screen with scarlet background
    ILI9341.fillScreen(FEHLCD::Scarlet);
    ILI9341.setCursor(0, 0);
    ILI9341.setTextColor(FEHLCD::White);

    // Large "KILLED" heading
    ILI9341.setTextSize(8);
    ILI9341.println("KILLED");

    // Display instructions and reason
    ILI9341.setTextSize(2);
    ILI9341.println();
    ILI9341.println("Power cycle to reset.");
    ILI9341.println();
    ILI9341.println("Source:");
    ILI9341.println(reason);

    // Loop forever with interrupts disabled
    while (true)
    {
        cli(); // Continuously disable interrupts to prevent any execution
    }
}

void _killNoScreen(bool loop, bool tone)
{
    // Optionally play descending warning tone
    if (tone)
    {
        // Two-tone warning (takes 20ms total)
        Buzzer.Tone(Buzzer.NOTE_A6);
        Sleep(10);
        Buzzer.Tone(Buzzer.NOTE_FS6);
        Sleep(10);
        Buzzer.Off();
    }

    // Put motor drivers to sleep and stop all PWM
    FEHMotor::SetAllSleep(true);
    FEHMotor::StopAll();

    // Disable all interrupts to halt execution
    cli();

    // Optionally loop forever (if called from non-returnable context)
    while (loop)
    {
        cli(); // Continuously disable interrupts
    }
}

//=============================================================================
// HARDWARE MONITORING IMPLEMENTATION
//=============================================================================

float _batteryVoltage()
{
    // Read battery voltage through ADC
    // Hardware: 12-bit ADC (0-1023), 5V reference voltage
    // Circuit: 3:1 voltage divider on battery input
    // Formula: ADC_value * (5V / 1023) * 3 = ADC_value * (15 / 1023)
    return analogRead(BATTERY_PIN) * (15.0 / 1023.0);
}

bool _I2CFault()
{
    // I2C_nFAULT pin is active-low (pulled LOW on fault)
    // Return true if fault is present (pin is LOW)
    return !digitalRead(I2C_nFAULT_PIN);
}

bool _IOFault()
{
    // IO_nFAULT pin is active-low (pulled LOW on fault)
    // Return true if fault is present (pin is LOW)
    return !digitalRead(IO_nFAULT_PIN);
}

//=============================================================================
// SPLASH SCREEN IMPLEMENTATION
//=============================================================================

/**
 * @brief Initialize and display Ohio State splash screen
 *
 * Displays the Ohio State block "O" logo in scarlet along with the system
 * title "EED Robot Controller 2" on a white background. This is shown during
 * system initialization.
 */
/**
 * @brief Helper to convert RGB to 16-bit color value
 */
static uint16_t convertRGBTo16Bit(unsigned char r, unsigned char g, unsigned char b)
{
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

/**
 * @brief Draw the Ohio State Block O image
 * @param x X coordinate (top-left corner)
 * @param y Y coordinate (top-left corner)
 */
static void drawOhioStateImage(int x, int y)
{
    unsigned char image[] = {23, 0, 52, 2, 45, 0, 54, 2, 43, 0, 56, 2, 41, 0, 58, 2, 39, 0, 6, 2, 48, 0, 6, 2, 37, 0, 6, 2, 50, 0, 6, 2, 35, 0, 6, 2, 52, 0, 6, 2, 33, 0, 6, 2, 4, 0, 46, 1, 4, 0, 6, 2, 31, 0, 6, 2, 4, 0, 48, 1, 4, 0, 6, 2, 29, 0, 6, 2, 4, 0, 50, 1, 4, 0, 6, 2, 27, 0, 6, 2, 4, 0, 52, 1, 4, 0, 6, 2, 25, 0, 6, 2, 4, 0, 54, 1, 4, 0, 6, 2, 23, 0, 6, 2, 4, 0, 56, 1, 4, 0, 6, 2, 21, 0, 6, 2, 4, 0, 58, 1, 4, 0, 6, 2, 19, 0, 6, 2, 4, 0, 60, 1, 4, 0, 6, 2, 17, 0, 6, 2, 4, 0, 62, 1, 4, 0, 6, 2, 15, 0, 6, 2, 4, 0, 64, 1, 4, 0, 6, 2, 13, 0, 6, 2, 4, 0, 66, 1, 4, 0, 6, 2, 11, 0, 6, 2, 4, 0, 68, 1, 4, 0, 6, 2, 9, 0, 6, 2, 4, 0, 70, 1, 4, 0, 6, 2, 7, 0, 6, 2, 4, 0, 72, 1, 4, 0, 6, 2, 5, 0, 6, 2, 4, 0, 74, 1, 4, 0, 6, 2, 3, 0, 6, 2, 4, 0, 76, 1, 4, 0, 6, 2, 1, 0, 6, 2, 4, 0, 78, 1, 4, 0, 11, 2, 4, 0, 80, 1, 4, 0, 9, 2, 4, 0, 82, 1, 4, 0, 8, 2, 3, 0, 84, 1, 3, 0, 8, 2, 3, 0, 84, 1, 3, 0, 8, 2, 3, 0, 29, 1, 26, 0, 29, 1, 3, 0, 8, 2, 3, 0, 28, 1, 28, 0, 28, 1, 3, 0, 8, 2, 3, 0, 27, 1, 30, 0, 27, 1, 3, 0, 8, 2, 3, 0, 26, 1, 4, 0, 24, 2, 4, 0, 26, 1, 3, 0, 8, 2, 3, 0, 25, 1, 4, 0, 26, 2, 4, 0, 25, 1, 3, 0, 8, 2, 3, 0, 24, 1, 4, 0, 28, 2, 4, 0, 24, 1, 3, 0, 8, 2, 3, 0, 23, 1, 4, 0, 30, 2, 4, 0, 23, 1, 3, 0, 8, 2, 3, 0, 22, 1, 4, 0, 6, 2, 21, 0, 5, 2, 4, 0, 22, 1, 3, 0, 8, 2, 3, 0, 21, 1, 4, 0, 6, 2, 23, 0, 5, 2, 4, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 6, 2, 25, 0, 5, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 5, 2, 27, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 4, 2, 28, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 5, 2, 27, 0, 4, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 3, 0, 6, 2, 25, 0, 5, 2, 3, 0, 21, 1, 3, 0, 8, 2, 3, 0, 21, 1, 4, 0, 6, 2, 23, 0, 5, 2, 4, 0, 21, 1, 3, 0, 8, 2, 3, 0, 22, 1, 4, 0, 6, 2, 21, 0, 5, 2, 4, 0, 22, 1, 3, 0, 8, 2, 3, 0, 23, 1, 4, 0, 30, 2, 4, 0, 23, 1, 3, 0, 8, 2, 3, 0, 24, 1, 4, 0, 28, 2, 4, 0, 24, 1, 3, 0, 8, 2, 3, 0, 25, 1, 4, 0, 26, 2, 4, 0, 25, 1, 3, 0, 8, 2, 3, 0, 26, 1, 4, 0, 24, 2, 4, 0, 26, 1, 3, 0, 8, 2, 3, 0, 27, 1, 30, 0, 27, 1, 3, 0, 8, 2, 3, 0, 28, 1, 28, 0, 28, 1, 3, 0, 8, 2, 3, 0, 29, 1, 26, 0, 29, 1, 3, 0, 8, 2, 3, 0, 84, 1, 3, 0, 8, 2, 3, 0, 84, 1, 3, 0, 8, 2, 4, 0, 82, 1, 4, 0, 9, 2, 4, 0, 80, 1, 4, 0, 11, 2, 4, 0, 78, 1, 4, 0, 6, 2, 1, 0, 6, 2, 4, 0, 76, 1, 4, 0, 6, 2, 3, 0, 6, 2, 4, 0, 74, 1, 4, 0, 6, 2, 5, 0, 6, 2, 4, 0, 72, 1, 4, 0, 6, 2, 7, 0, 6, 2, 4, 0, 70, 1, 4, 0, 6, 2, 9, 0, 6, 2, 4, 0, 68, 1, 4, 0, 6, 2, 11, 0, 6, 2, 4, 0, 66, 1, 4, 0, 6, 2, 13, 0, 6, 2, 4, 0, 64, 1, 4, 0, 6, 2, 15, 0, 6, 2, 4, 0, 62, 1, 4, 0, 6, 2, 17, 0, 6, 2, 4, 0, 60, 1, 4, 0, 6, 2, 19, 0, 6, 2, 4, 0, 58, 1, 4, 0, 6, 2, 21, 0, 6, 2, 4, 0, 56, 1, 4, 0, 6, 2, 23, 0, 6, 2, 4, 0, 54, 1, 4, 0, 6, 2, 25, 0, 6, 2, 4, 0, 52, 1, 4, 0, 6, 2, 27, 0, 6, 2, 4, 0, 50, 1, 4, 0, 6, 2, 29, 0, 6, 2, 4, 0, 48, 1, 4, 0, 6, 2, 31, 0, 6, 2, 4, 0, 46, 1, 4, 0, 6, 2, 33, 0, 6, 2, 52, 0, 6, 2, 35, 0, 6, 2, 50, 0, 6, 2, 37, 0, 6, 2, 48, 0, 6, 2, 39, 0, 58, 2, 41, 0, 56, 2, 43, 0, 54, 2, 45, 0, 52, 2, 23, 0};

    // Image dimensions: 98x126
    int image_length = sizeof(image) / sizeof(image[0]);
    int currentX = x;
    int currentY = y;

    for (int i = 0; i < image_length; i += 2)
    {
        // image[i] = count of pixels
        // image[i+1] = color code (0=white, 1=scarlet, 2=gray)
        unsigned char r, g, b;

        if (image[i + 1] == 0)
        {
            r = 255;
            g = 255;
            b = 255; // White
        }
        else if (image[i + 1] == 1)
        {
            r = 212;
            g = 0;
            b = 38; // Scarlet
        }
        else
        {
            r = 181;
            g = 186;
            b = 176; // Gray
        }

        uint16_t color = convertRGBTo16Bit(r, g, b);

        // Draw 'image[i]' pixels of this color
        for (int j = 0; j < image[i]; j++)
        {
            ILI9341.drawPixel(currentX, currentY, color);
            currentX++;
            if (currentX >= x + 98) // Wrap to next line after 98 pixels
            {
                currentX = x;
                currentY++;
            }
        }
    }
}

void initSplashScreen()
{
    // Clear screen to white background
    LCD.Clear(FEHLCD::White);

    //-------------------------------------------------------------------------
    // Draw Ohio State Block "O" Image
    //-------------------------------------------------------------------------

    // Image is 98x126 pixels
    // Center horizontally on 320px wide screen: (320 - 98) / 2 = 111
    int image_x = 111;
    int image_y = 20; // Position near top
    drawOhioStateImage(image_x, image_y);

    //-------------------------------------------------------------------------
    // Draw System Title
    //-------------------------------------------------------------------------

    LCD.SetFontColor(BLACK);
    LCD.SetFontSize(2);
    LCD.WriteAt("EED Robot Controller 2", 30, 160);

    // Reset to smaller font for status messages
    LCD.SetFontSize(1);
}

/**
 * @brief Update status message on splash screen
 *
 * Clears the status area at the bottom of the splash screen and displays
 * a new centered status message. Used during initialization to show progress.
 *
 * @param status Status message to display (null-terminated string)
 */
void updateSplashScreenWithStatus(const char *status)
{
    //-------------------------------------------------------------------------
    // Clear Status + Battery Area
    //-------------------------------------------------------------------------

    // Make room for the status line and an additional battery voltage line
    LCD.SetFontColor(FEHLCD::White);
    LCD.FillRectangle(0, 180, 320, 40); // Clear full width, 40px tall strip

    //-------------------------------------------------------------------------
    // Center Status Text
    //-------------------------------------------------------------------------

    // Calculate text centering
    // Font size 1 is approximately 6 pixels wide per character
    int textLength = strlen(status);
    int textWidth = textLength * 6;      // Approximate pixel width
    int centerX = (320 - textWidth) / 2; // Center horizontally

    // Enforce minimum left margin to prevent text from going off-screen
    if (centerX < 10)
        centerX = 10;

    //-------------------------------------------------------------------------
    // Display Status Text
    //-------------------------------------------------------------------------

    LCD.SetFontColor(FEHLCD::Black);
    LCD.SetFontSize(1);
    LCD.WriteAt(status, centerX, 185);

    //-------------------------------------------------------------------------
    // Read and display averaged battery voltage beneath the status text
    //-------------------------------------------------------------------------

    // Take 3 samples and average them to reduce noise
    float sum = 0.0f;
    const int samples = 3;
    for (int i = 0; i < samples; i++)
    {
        sum += _batteryVoltage();
        // small delay between ADC reads to let sample capacitor settle
        delay(5);
    }
    float avg = sum / (float)samples;

    // Format voltage as X.YY using integer math to avoid floating-point printf
    int volts = (int)avg;
    int hundredths = (int)(avg * 100.0f + 0.5f) - volts * 100;
    if (hundredths < 0)
        hundredths = 0;

    char battBuf[32];
    snprintf(battBuf, sizeof(battBuf), "Battery: %d.%02d V", volts, hundredths);

    int battTextLength = strlen(battBuf);
    int battTextWidth = battTextLength * 6; // approx 6 px per char for font size 1
    int battCenterX = (320 - battTextWidth) / 2;
    if (battCenterX < 10)
        battCenterX = 10;

    // Draw battery text slightly below the status line
    LCD.WriteAt(battBuf, battCenterX, 205);
}

bool waitForESP32Ready(unsigned long timeout_ms)
{
    unsigned long t0 = millis();
    ESP32Version ver;

    while (millis() - t0 < timeout_ms)
    {
        // Repeatedly ping and poll the ESP32 to check if it's ready
        FEHESP32::poll();
        FEHESP32::ping();

        // Check if the ESP32 has responded with a valid version number
        ver = FEHESP32::getVersion();
        if (ver.major != 0 || ver.minor != 0 || ver.patch != 0)
        {
            return true; // ESP32 is ready
            break;
        }

        delay(10);
    }

    return false; // Timeout expired without ESP32 becoming ready
}
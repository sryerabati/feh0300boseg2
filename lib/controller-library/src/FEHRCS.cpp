/**
 * @file FEHRCS.cpp
 * @author Jayson Clark
 * @brief Support for the Robot Communication System (RCS) on the FEH Shield.
 *
 */

#include <FEH.h>
#include "FEHDefines.h"

#include "../private_include/FEHInternal.h"
#include "../private_include/scheduler.h"
#include "../private_include/esp32.h"

#define REGION_COUNT 8

// Global instance of FEHRCS
FEHRCS RCS;

/**
 * @brief Initializes the touch menu and starts the FEHRCS system.
 *
 * This function is intended to use FEHLCD to allow the user to select the region.
 * Currently, it defaults to region 'A'.
 *
 * @param team_key The team key used for authentication or identification.
 */
void FEHRCS::InitializeTouchMenu(const char *team_key)
{
    int cancel = 1;
    int c = 0, d = 0, n;
    int x, y;
    char region;

    FEHIcon::Icon regions_title[1];
    char regions_title_label[1][20] = {"Select RCS Region"};

    FEHIcon::Icon regions[REGION_COUNT];
    char regions_labels[12][20] = {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L"};

    FEHIcon::Icon confirm_title[1];
    char confirm_title_label[1][20] = {""};

    FEHIcon::Icon confirm[2];
    char confirm_labels[2][20] = {"Ok", "Cancel"};

    while (cancel)
    {
        c = 0;
        d = 0;
        LCD.Clear(BLACK);

        int regionLabelRowCount = REGION_COUNT / 4;

        FEHIcon::DrawIconArray(regions_title, 1, 1, 1, 201, 1, 1, regions_title_label, BLACK, WHITE);
        FEHIcon::DrawIconArray(regions, regionLabelRowCount, 4, 40, 2, 1, 1, regions_labels, WHITE, WHITE);

        // Wait for region selection
        while (!c)
        {
            if (LCD.Touch(&x, &y))
            {
                for (n = 0; n < REGION_COUNT; n++)
                {
                    if (regions[n].Pressed(x, y, 0))
                    {
                        regions[n].WhilePressed(x, y);
                        c = n + 1; // c now holds the index (1-indexed)
                    }
                }
            }
        }

        // Compute region character and update the confirm title without switch-case
        region = 'A' + c - 1;
        sprintf(confirm_title_label[0], "Choice: %c", region);

        LCD.Clear(BLACK);
        FEHIcon::DrawIconArray(confirm_title, 1, 1, 60, 201, 1, 1, confirm_title_label, BLACK, WHITE);
        FEHIcon::DrawIconArray(confirm, 1, 2, 60, 60, 1, 1, confirm_labels, WHITE, WHITE);

        // Wait for confirmation selection
        while (!d)
        {
            if (LCD.Touch(&x, &y))
            {
                for (n = 0; n < 2; n++)
                {
                    if (confirm[n].Pressed(x, y, 0))
                    {
                        confirm[n].WhilePressed(x, y);
                        d = n + 1;
                    }
                }
            }
        }

        // Set cancel based on selection: Ok (d==1) ends the loop, Cancel (d==2) restarts it.
        cancel = (d == 1) ? 0 : 1;
    }

    Initialize(region, team_key);
}

/**
 * @brief Initializes the FEHRCS system with the specified region and team key.
 *
 * Sends CMD_RCS_CONNECT to the ESP32, which opens a UDP socket and begins
 * continuously sending RobotIdentPacket to the RCS server (gateway IP, port 5000).
 * The ESP32 forwards received RobotPacket data as NOTIFY_RCS_DATA notifications.
 *
 * @param region The selected course region to connect to ('A'-'H').
 * @param team_key The team key used for identification (up to 9 characters).
 */
void FEHRCS::Initialize(char region, const char *team_key)
{
    // Check that region is in the range A-H
    if (region < 'A' || region > 'H')
    {
        _fatalError("Invalid region selected: " + region);
    }

    LCD.Clear();

    // Connect to RCS wifi network (separate from OTA wifi network)
    LCD.WriteLine("Connecting to RCS WiFi...");
    FEHESP32::connectWifi(RCS_WIFI_SSID, RCS_WIFI_PASS);
    bool connected = FEHESP32::waitForWifiConnect(10000);
    if (!connected)
    {
        _fatalError("Failed to connect to RCS.");
    }

    // Set internal region variable
    _region = region;

    // Register RCS data callback with ESP32 driver
    FEHESP32::setRCSCallback(handleRCSData);

    LCD.Write("Connecting to RCS region ");
    LCD.Write(region);
    LCD.WriteLine("...");

    // Send RCS connect command to ESP32
    uint8_t rcs_server_ip[] = RCS_SERVER_IP_BYTES;
    FEHESP32::connectRCS(region, rcs_server_ip, team_key);

    // Wait for ACK from ESP32
    if (!FEHESP32::waitForAck(CMD_RCS_CONNECT, 3000))
    {
        _fatalError("ESP32 did not acknowledge RCS connect.");
    }

    LCD.Write("RCS Region ");
    LCD.Write(region);
    LCD.WriteLine(" connected!");

    initialized = true;
}

int FEHRCS::CurrentCourse()
{
    if (!initialized)
    {
        _fatalError("FEHRCS not initialized and FEHRCS::CurrentCourse() called.");
    }
    return (int)(_region - 'A');
}

char FEHRCS::CurrentRegionLetter()
{
    if (!initialized)
    {
        _fatalError("FEHRCS not initialized and FEHRCS::CurrentRegionLetter() called.");
    }
    return _region;
}

void FEHRCS::handleRCSData(const uint8_t *data, uint8_t len)
{
    // RobotPacket format from RCS server:
    //   [0] Objective (correct lever: 0=left, 1=middle, 2=right)
    //   [1] Lever up/down state
    //   [2] Dual slider state
    //   [3] Time remaining (seconds)
    //   [4] Kill switch (0=off, 1=on)
    //   Optional position data (13 bytes) follows if RPS has a sighting

    if (len >= 5)
    {
        RCS._correctLever = data[0];
        RCS.hasLever = true;

        RCS._leverFlipped = data[1];
        RCS._dualSliderStatus = data[2];
        RCS._time = data[3];
        // data[4] = kill switch (not currently exposed via FEHRCS API)
    }
}

int FEHRCS::GetLever()
{
    if (!initialized)
    {
        _fatalError("FEHRCS not initialized and FEHRCS::GetLever() called.");
    }
    return _correctLever;
}

int FEHRCS::isLeverFlipped()
{
    if (!initialized)
    {
        _fatalError("FEHRCS not initialized and FEHRCS::isLeverFlipped() called.");
    }
    if (RCS._leverFlipped == 1)
    {
        return 1;
    }
    return 0;
}

int FEHRCS::isWindowOpen()
{
    if (!initialized)
    {
        _fatalError("FEHRCS not initialized and FEHRCS::isWindowOpen() called.");
    }
    if (RCS._dualSliderStatus == 2)
    {
        return 1;
    }
    return 0;
}

// returns the match time in seconds
int FEHRCS::Time()
{
    if (!initialized)
    {
        _fatalError("FEHRCS not initialized and FEHRCS::Time() called.");
    }
    return (int)RCS._time;
}
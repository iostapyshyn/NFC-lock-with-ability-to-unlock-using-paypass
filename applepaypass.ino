#include <SPI.h>
#include <PN532_SPI.h>
#include "PN532.h"

#define LOCK_PIN 5
#define LOCK_OPEN HIGH

PN532_SPI pn532spi(SPI, 10);
PN532 nfc(pn532spi);

bool check_response(uint8_t *response, uint8_t response_length) {
    uint8_t responses[][4] = { { 0xC6, 0x80, 0xC0, 0xF3 }, /* White card. */
                               { 0xA3, 0x5F, 0xC3, 0xD0 }, /* Key tag. */
                               { 0x6A, 0x82 } /* PayPass. */ };

    // Compare response to known responses, return true if values match.
    for (int i = 0; i < sizeof(responses)/sizeof(*responses); i++) {
        if (memcmp(responses[i], response, response_length) == 0)
            return true;
    }

    return false;
}

void setup() {
    pinMode(LOCK_PIN, OUTPUT);
    digitalWrite(LOCK_PIN, !LOCK_OPEN); // Close the switch on startup.

    Serial.begin(115200);
    nfc.begin();

    uint32_t versiondata = nfc.getFirmwareVersion();
    if (!versiondata) {
        Serial.print("Didn't find PN53x board");
        while (1); // Halt.
    }

    // Got connection, print out version of the board.
    Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
    Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
    Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);

    // Configure board to read RFID tags.
    nfc.SAMConfig();
}

void loop() {
    Serial.println("Waiting for an ISO14443A card...");

    if (nfc.inListPassiveTarget()) {
        Serial.println("Found something...");

        uint8_t select_apdu[] = { 0x00, /* CLA */
                                  0xA4, /* INS */
                                  0x04, /* P1 */
                                  0x00, /* P2 */
                                  0x07, /* Length of AID */
                                  0xF0, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, /* AID defined on Android App. */
                                  0x00  /* Le */ };

        uint8_t response_length = 32;
        uint8_t response[32];

        // Search for devices...
        if (nfc.inDataExchange(select_apdu, sizeof(select_apdu), response, &response_length) || // Look for active device.
            nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, response, &response_length)) { // Passive target, e.g card.

            Serial.print("Response length: ");
            Serial.println(response_length);
            nfc.PrintHexChar(response, response_length);

            if (check_response(response, response_length)) {
                Serial.println("Responses match. Unlocking the switch...");
                digitalWrite(LOCK_PIN, LOCK_OPEN);
                while(1); // Halt.
            }
        } else Serial.println("Unknown device.");
    } else Serial.println("Didn't find anything.");

    delay(1000);
}

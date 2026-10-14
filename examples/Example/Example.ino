#include <Radio_GemTek.h>

// Arduino Nano:
// CK = D3
// DA = D4
// CE = D2
// MT = D5
// NS = D6
GemtekRadio radio(3, 4, 2, 5, 6);

void setup() {
    Serial.begin(9600);
    Serial.println(F("GemTek Radio"));

    radio.begin();

    // Frequency is in kHz: 100800 = 100.8 MHz
    if (radio.setFrequency(100800UL)) {
        Serial.println(F("Frequency set to 100.8 MHz"));
    } else {
        Serial.println(F("Error: invalid frequency"));
    }

    radio.setMute(false);
    Serial.println("Mute Released");
}

void loop() {
    if (radio.hasStereoStatus()) {
        if (radio.getMono()) {
            Serial.println(F("S: MONO"));
        } else {
            Serial.println(F("S: STEREO"));
        }
    }

    delay(5000);
}

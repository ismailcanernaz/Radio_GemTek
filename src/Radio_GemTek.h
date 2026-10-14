/*
 *  GemTek radio module library
 *  Copyright 2026 İsmail Caner Naz <ismailcanernaz@aol.com>
 *  GemTek hasn't released any specs on the card,
 *  so the protocol had to be reverse engineered.
 *  Fully tested with the USB FM Transmitter.
 *
 */

#ifndef RADIO_GEMTEK_H
#define RADIO_GEMTEK_H

#include <Arduino.h>

// BU2614 serial interface signals
#define GEMTEK_CK  0x01
#define GEMTEK_DA  0x02
#define GEMTEK_CE  0x04
#define GEMTEK_NS  0x08
#define GEMTEK_MT  0x10

// Frequency synthesizer (BU2614) configuration constants
#define GEMTEK_STDF_3_125_KHZ  0x01
#define GEMTEK_PLL_OFF         0x07

#define BU2614_BUS_SIZE 32

class GemtekRadio {
public:
    // Arduino GPIO pin numbers are used for all pin arguments
    // Set pinNS to -1 to disable mono/stereo status reading
    GemtekRadio(uint8_t pinCK,
                uint8_t pinDA,
                uint8_t pinCE,
                uint8_t pinMT,
                int8_t pinNS = -1);

    void begin();

    // Frequency unit: kHz
    // Example: 99500 = 99.5 MHz
    bool setFrequency(uint32_t frequencyKHz);

    uint32_t getFrequency() const;

    void setMute(bool mute);
    bool isMuted() const;

    // Hard-mute behavior based on the Arduino library
    // the PLL is shut down when hard mute is enabled
    void setHardMute(bool enabled);
    bool isHardMute() const;

    // Reads mono/stereo status when the NS input is connected
    // NS HIGH = mono, LOW = stereo
    bool getMono() const;
    bool hasStereoStatus() const;

private:
    uint8_t _pinCK;
    uint8_t _pinDA;
    uint8_t _pinCE;
    uint8_t _pinMT;
    int8_t  _pinNS;

    uint32_t _frequencyKHz;
    uint32_t _bu2614Data;

    bool _muted;
    bool _hardMute;
    bool _initialized;

    // Private helper methods
    void transmit();
    void setField(uint32_t mask,
                  uint8_t shift,
                  uint32_t value);

    uint32_t convertFrequency(uint32_t frequencyKHz) const;
    void shutdownPLL();
};

#endif // RADIO_GEMTEK_H

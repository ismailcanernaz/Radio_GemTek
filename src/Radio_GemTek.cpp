/*
 *  GemTek radio module library
 *  Copyright 2026 İsmail Caner Naz <ismailcanernaz@aol.com>
 *  GemTek hasn't released any specs on the card,
 *  so the protocol had to be reverse engineered.
 *  Fully tested with the USB FM Transmitter.
 *
 */

#include <Radio_GemTek.h>

// Frequency calculation constants
#define FSCALE 8

#define IF_OFFSET \
    ((uint32_t)(10.52 * 16000.0 * (1UL << FSCALE)))

#define REF_FREQ \
    ((uint32_t)(6.39 * 16.0 * (1UL << FSCALE)))

// Exact delays required by the BU2614 chip spec
#define SHORT_DELAY 5
#define LONG_DELAY  75

// BU2614 data fields: bit widths
#define BU2614_FREQ_BITS   16
#define BU2614_PORT_BITS    3
#define BU2614_VOID_BITS    4
#define BU2614_FMES_BITS    1
#define BU2614_STDF_BITS    3
#define BU2614_SWIN_BITS    1
#define BU2614_SWAL_BITS    1
#define BU2614_VOID2_BITS   1
#define BU2614_FMUN_BITS    1
#define BU2614_TEST_BITS    1

// BU2614 bit shifting positions
#define BU2614_FREQ_SHIFT   0
#define BU2614_PORT_SHIFT   (BU2614_FREQ_BITS + BU2614_FREQ_SHIFT)
#define BU2614_VOID_SHIFT   (BU2614_PORT_BITS + BU2614_PORT_SHIFT)
#define BU2614_FMES_SHIFT   (BU2614_VOID_BITS + BU2614_VOID_SHIFT)
#define BU2614_STDF_SHIFT   (BU2614_FMES_BITS + BU2614_FMES_SHIFT)
#define BU2614_SWIN_SHIFT   (BU2614_STDF_BITS + BU2614_STDF_SHIFT)
#define BU2614_SWAL_SHIFT   (BU2614_SWIN_BITS + BU2614_SWIN_SHIFT)
#define BU2614_VOID2_SHIFT  (BU2614_SWAL_BITS + BU2614_SWAL_SHIFT)
#define BU2614_FMUN_SHIFT   (BU2614_VOID2_BITS + BU2614_VOID2_SHIFT)
#define BU2614_TEST_SHIFT   (BU2614_FMUN_BITS + BU2614_FMUN_SHIFT)

// Macro to generate specific bitmasks
#define MKMASK(field) \
    (((1UL << BU2614_##field##_BITS) - 1UL) \
     << BU2614_##field##_SHIFT)

#define BU2614_FREQ_MASK   MKMASK(FREQ)
#define BU2614_PORT_MASK   MKMASK(PORT)
#define BU2614_VOID_MASK   MKMASK(VOID)
#define BU2614_FMES_MASK   MKMASK(FMES)
#define BU2614_STDF_MASK   MKMASK(STDF)
#define BU2614_SWIN_MASK   MKMASK(SWIN)
#define BU2614_SWAL_MASK   MKMASK(SWAL)
#define BU2614_VOID2_MASK  MKMASK(VOID2)
#define BU2614_FMUN_MASK   MKMASK(FMUN)
#define BU2614_TEST_MASK   MKMASK(TEST)

GemtekRadio::GemtekRadio(uint8_t pinCK,
                         uint8_t pinDA,
                         uint8_t pinCE,
                         uint8_t pinMT,
                         int8_t pinNS)
    : _pinCK(pinCK),
      _pinDA(pinDA),
      _pinCE(pinCE),
      _pinMT(pinMT),
      _pinNS(pinNS),
      _frequencyKHz(99500UL), // Default initial frequency: 99.5 MHz
      _bu2614Data(0),
      _muted(true),
      _hardMute(false),
      _initialized(false) {
}

void GemtekRadio::begin() {
    pinMode(_pinCK, OUTPUT);
    pinMode(_pinDA, OUTPUT);
    pinMode(_pinCE, OUTPUT);
    pinMode(_pinMT, OUTPUT);

    if (_pinNS >= 0) {
        pinMode(_pinNS, INPUT);
    }

    // Default line states before transmission
    digitalWrite(_pinCK, HIGH);
    digitalWrite(_pinDA, HIGH);
    digitalWrite(_pinCE, HIGH);
    digitalWrite(_pinMT, HIGH); // Start hardware muted

    _initialized = true;

    // Initialize in the muted state
    setMute(true);
    setFrequency(_frequencyKHz);
}

void GemtekRadio::setField(uint32_t mask,
                           uint8_t shift,
                           uint32_t value) {
    _bu2614Data =
        (_bu2614Data & ~mask) |
        ((value << shift) & mask);
}

// Transfer sequence
// 1. Set CE, DA, and CK HIGH
// 2. Wait 75 us
// 3. Transmit 32 bits, least significant bit first
// 4. Toggle CK LOW and HIGH for each bit
// 5. Set CE LOW to complete the transfer
void GemtekRadio::transmit() {
    if (!_initialized) {
        return;
    }

    // Direct hardware mute feedback during transmission to avoid pop sounds
    digitalWrite(_pinMT, _muted ? HIGH : LOW);

    // Start of the exact Linux Kernel pulse structure
    digitalWrite(_pinCE, HIGH);
    digitalWrite(_pinDA, HIGH);
    digitalWrite(_pinCK, HIGH);

    delayMicroseconds(LONG_DELAY);

    uint32_t q = _bu2614Data;

    // Clock out 32 bits, LSB first
    for (uint8_t i = 0; i < BU2614_BUS_SIZE; ++i, q >>= 1) {
        const uint8_t bitValue = (q & 1UL) ? HIGH : LOW;

        digitalWrite(_pinCE, HIGH);
        digitalWrite(_pinCK, LOW);
        digitalWrite(_pinDA, bitValue);

        delayMicroseconds(SHORT_DELAY);

        digitalWrite(_pinCE, HIGH);
        digitalWrite(_pinCK, HIGH);
        digitalWrite(_pinDA, bitValue);

        delayMicroseconds(SHORT_DELAY);
    }

    // Terminate transmission
    digitalWrite(_pinCE, LOW);
    digitalWrite(_pinDA, HIGH);
    digitalWrite(_pinCK, HIGH);

    delayMicroseconds(SHORT_DELAY);
}

// Frequency conversion algorithm
// represents frequency in units of 1/16 kHz
// this Arduino API accepts frequency in kHz
uint32_t GemtekRadio::convertFrequency(
    uint32_t frequencyKHz) const {

    const uint64_t frequencyUnits =
        (uint64_t)frequencyKHz * 16ULL;

    const uint64_t numerator =
        (frequencyUnits << FSCALE) +
        IF_OFFSET +
        REF_FREQ / 2UL;

    return (uint32_t)(numerator / REF_FREQ);
}

bool GemtekRadio::setFrequency(uint32_t frequencyKHz) {
    // Standard FM Band limits (87.5 MHz to 108.0 MHz)
    if (frequencyKHz < 87500UL ||
        frequencyKHz > 108000UL) {
        return false;
    }

    _frequencyKHz = frequencyKHz;

    // Under hard-mute, we don't wake up the hardware PLL unnecessarily
    if (_hardMute && _muted) {
        return true;
    }

    // Reconstruct the exact register map sequence
    setField(BU2614_PORT_MASK, BU2614_PORT_SHIFT, 0);
    setField(BU2614_FMES_MASK, BU2614_FMES_SHIFT, 0);
    setField(BU2614_SWIN_MASK, BU2614_SWIN_SHIFT, 0);
    setField(BU2614_SWAL_MASK, BU2614_SWAL_SHIFT, 0);
    setField(BU2614_FMUN_MASK, BU2614_FMUN_SHIFT, 1);
    setField(BU2614_TEST_MASK, BU2614_TEST_SHIFT, 0);

    setField(BU2614_STDF_MASK,
             BU2614_STDF_SHIFT,
             GEMTEK_STDF_3_125_KHZ);

    setField(BU2614_FREQ_MASK,
             BU2614_FREQ_SHIFT,
             convertFrequency(frequencyKHz));

    transmit();
    return true;
}

uint32_t GemtekRadio::getFrequency() const {
    return _frequencyKHz;
}

void GemtekRadio::shutdownPLL() {
    setField(BU2614_PORT_MASK, BU2614_PORT_SHIFT, 0);
    setField(BU2614_FMES_MASK, BU2614_FMES_SHIFT, 0);
    setField(BU2614_SWIN_MASK, BU2614_SWIN_SHIFT, 0);
    setField(BU2614_SWAL_MASK, BU2614_SWAL_SHIFT, 0);
    setField(BU2614_FMUN_MASK, BU2614_FMUN_SHIFT, 0);
    setField(BU2614_TEST_MASK, BU2614_TEST_SHIFT, 0);

    setField(BU2614_STDF_MASK,
             BU2614_STDF_SHIFT,
             GEMTEK_PLL_OFF);

    setField(BU2614_FREQ_MASK, BU2614_FREQ_SHIFT, 0);

    transmit();
}

void GemtekRadio::setMute(bool mute) {
    _muted = mute;

    if (!_initialized) {
        return;
    }

    if (_hardMute) {
        if (!_muted) {
            // Restore the current frequency when mute is released
            setFrequency(_frequencyKHz);
        } else {
            // Shut down the PLL and keep the MT line muted
            shutdownPLL();
        }
    } else {
        // Soft mute: the tuner remains active
        // The Arduino implementation uses a dedicated MT GPIO
        digitalWrite(_pinMT, _muted ? HIGH : LOW);
    }
}

bool GemtekRadio::isMuted() const {
    return _muted;
}

void GemtekRadio::setHardMute(bool enabled) {
    if (_hardMute == enabled) {
        return;
    }

    _hardMute = enabled;

    if (!_initialized) {
        return;
    }

    if (_hardMute) {
        if (_muted) {
            shutdownPLL();
        }
    } else {
        // Reprogram the PLL when hard mute is disabled
        // Preserve the current mute state
        setFrequency(_frequencyKHz);
        digitalWrite(_pinMT, _muted ? HIGH : LOW);
    }
}

bool GemtekRadio::isHardMute() const {
    return _hardMute;
}

bool GemtekRadio::hasStereoStatus() const {
    return _pinNS >= 0;
}

bool GemtekRadio::getMono() const {
    if (_pinNS < 0) {
        return true;  // Assume mono when no status pin is connected
    }

    // GEMTEK_NS HIGH as mono
    return digitalRead(_pinNS) == HIGH;
}

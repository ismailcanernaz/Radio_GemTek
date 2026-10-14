# Radio_GemTek

Arduino library for controlling a GemTek FM radio module through its BU2614 frequency-synthesizer interface.

The module's protocol was reverse engineered because GemTek did not publish specifications for the card. The implementation has been tested with a USB FM transmitter.

## Features

- Set FM frequency from 87.5 MHz to 108.0 MHz
- Read back the configured frequency
- Control the module's MT mute pin
- Optional hard-mute mode that shuts down the PLL while muted
- Read mono/stereo status through the NS pin, when connected

## Hardware and wiring

The constructor takes Arduino pin numbers in this order:

`GemtekRadio(pinCK, pinDA, pinCE, pinMT, pinNS)`

Example wiring for an Arduino Nano:

| GemTek signal | Arduino Nano |
| --- | --- |
| CK | D3 |
| DA | D4 |
| CE | D2 |
| MT | D5 |
| NS | D6 |

Connect the module's ground and power according to the module's electrical requirements. Confirm logic levels and supply voltage before wiring; this library documentation does not specify them.

If the NS status output is not connected, pass `-1` as the final constructor argument. `hasStereoStatus()` will then return `false`.

## Basic example

```cpp
#include <Radio_GemTek.h>

// Arduino Nano: CK=D3, DA=D4, CE=D2, MT=D5, NS=D6
GemtekRadio radio(3, 4, 2, 5, 6);

void setup() {
    Serial.begin(9600);
    Serial.println(F("GemTek Radio"));

    radio.begin();

    // Frequency is expressed in kHz: 100800 = 100.8 MHz
    if (radio.setFrequency(100800UL)) {
        Serial.println(F("Frequency set to 100.8 MHz"));
    } else {
        Serial.println(F("Invalid frequency"));
    }

    radio.setMute(false);
}

void loop() {
    if (radio.hasStereoStatus()) {
        Serial.println(radio.getMono() ? F("MONO") : F("STEREO"));
    }

    delay(5000);
}
```

## API

| Method | Description |
| --- | --- |
| `begin()` | Configures the GPIO pins and initializes the module in muted state. |
| `setFrequency(uint32_t frequencyKHz)` | Sets a frequency in kHz. Returns `false` outside 87500–108000 kHz. |
| `getFrequency()` | Returns the configured frequency in kHz. |
| `setMute(bool mute)` | Enables or releases mute. |
| `isMuted()` | Returns the software-tracked mute state. |
| `setHardMute(bool enabled)` | Enables/disables hard-mute behavior. When enabled and muted, the library shuts down the PLL. |
| `isHardMute()` | Returns whether hard-mute mode is enabled. |
| `hasStereoStatus()` | Returns `true` if an NS pin was configured. |
| `getMono()` | Returns `true` for mono (NS HIGH), `false` for stereo (NS LOW). If NS is omitted, it assumes mono. |

### Frequency units

The frequency argument and return value use kHz, not MHz:

- `87500` = 87.5 MHz
- `99500` = 99.5 MHz
- `100800` = 100.8 MHz
- `108000` = 108.0 MHz

## Installation

### Arduino IDE

1. Close the Arduino IDE if it is open.
2. Copy the `Radio_GemTek` folder into your sketchbook's `libraries` directory.
3. Reopen Arduino IDE.
4. Open **File → Examples → Radio_GemTek → Example**.

The library should have this structure:

```text
Radio_GemTek/
├── src/
│   ├── Radio_GemTek.h
│   └── Radio_GemTek.cpp
├── library.properties
├── keywords.txt
├── README.md
├── LICENSE
└── examples/
    └── Example/
        └── Example.ino
```

## Notes

- The library uses Arduino GPIO functions and `delayMicroseconds()`.
- The `getFrequency()` method reports the frequency last configured by the library; it does not measure the received RF frequency.
- Mono/stereo status depends on the module's NS output and the wiring/polarity described above.

## License

See `LICENSE`. Confirm the license choice before publishing or redistributing the library.

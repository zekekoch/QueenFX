# QueenFX

LED effects for the Queen art car. A Teensy 3.2 drives 16 LED tubes
with FastLED. Effects are selected from the dashboard's 8 lit
buttons (but supports 16), which are read through a 4067 multiplexer and lit by
TLC59711 LED drivers.

## Files

- `QueenFX.ino`: the main sketch. It holds the tube layout, the
  effects, and the button-to-effect mapping in `loop()`.
- `Buttons.h`: dashboard buttons. It reads them through the mux and
  sets their colors through the TLC59711 boards.
- `Mux.h`: reads the 4067 multiplexer that the buttons and dials are
  wired to.

## Setup

### 1. Arduino IDE

Install [Arduino IDE 2.x](https://www.arduino.cc/en/software).

### 2. Teensyduino (Teensy board support)

1. Open **Arduino IDE → Settings** (Preferences on Windows/Linux).
2. Add this URL to **Additional boards manager URLs**:

   ```
   https://www.pjrc.com/teensy/package_teensy_index.json
   ```

3. Open **Tools → Board → Boards Manager**, search for **Teensy**, and
   install **Teensy (for Arduino IDE 2.0.4 or later)**.

PJRC's instructions are at
<https://www.pjrc.com/teensy/td_download.html>.

### 3. Libraries

Install these from **Tools → Manage Libraries**:

| Library | Used by | Notes |
|---|---|---|
| FastLED | `QueenFX.ino`, `Buttons.h` | Teensyduino also bundles a copy |
| Adafruit TLC59711 | `Buttons.h` | Install its dependencies when prompted |
| Adafruit BusIO | Adafruit TLC59711 | Usually installed as a dependency |

### 4. Board settings

Under **Tools**, set:

- **Board:** Teensy 3.2 / 3.1
- **USB Type:** Serial
- **Port:** the Teensy's port

Then compile and upload. The Teensy Loader app opens automatically
during upload. If it doesn't program the board, press the button on
the Teensy.

## Hardware notes

- **LED outputs:** FastLED's parallel output on port D and port C
  gives 8 + 8 = 16 outputs (`addLeds<WS2811_PORTD,8>` and
  `addLeds<WS2811_PORTC,8>` in `setup()`).
- **TLC59711 button lights:** 4 chained boards, data on pin 4, clock
  on pin 3.
- **4067 mux:** select pins A=17, B=18, C=A11, D=19; signal pin Z=A2.
- **Serial monitor:** 57600 baud.

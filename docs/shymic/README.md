# ShyMic — M5StampS3 sound-reactive Dynamixel mic stand

An M5StampS3 reads an analog microphone (envelope follower) and a ToF4M
distance sensor (VL53L1X), and drives an AX-12A Dynamixel that moves the
microphone closer to / farther from the sound source. A built-in web UI
(WiFi AP mode) is used for calibration and mode switching.

Pin/bus conventions follow the MisBKit firmware
(`MisBkit_firmware/include/Pins.h`, `src/motors/motorManager.*`,
`src/sensors/units/i2c/UnitToF4M.h`).

## Hardware

| Function            | GPIO      | Notes                                      |
| ------------------- | --------- | ------------------------------------------ |
| Analog mic output   | 5         | 12-bit ADC, 11 dB attenuation (~3.3 V)     |
| Dynamixel RX / TX   | 44 / 43   | `Serial0` @ 1 Mbaud, protocol 1.0          |
| Dynamixel direction | 41        | half-duplex dir (use 1 on plain StampS3)   |
| ToF4M (VL53L1X)     | I2C 13/15 | address 0x29, short mode, 20 ms continuous |
| Button              | 0 (BtnA)  | toggles invert in SHY_MIC                  |

## Web UI

1. Power the board — it starts a WiFi AP: **SSID `ShyMic`, password `shymic123`**
2. Connect with a phone/laptop and open **http://192.168.4.1**

Controls are grouped in titled boxes. Mode-specific controls only appear
when that mode is selected; everything else is always visible. The right
~20% of the page is a touch-free band for scrolling.

## Modes

| Mode      | Behavior                                                                                                                                                                                                                   |
| --------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| SHY_MIC   | Loud sound → mic runs away (position follows level, fast attack / slow release). With **presence detection on**: person far → arm slowly bows; person close + silent → stands still; person close + speaking → shy escape. |
| ALERT_DOG | Motor slowly sweeps between FAR and NEAR "searching"; freezes when the level crosses the dog threshold, resumes after the sound stops.                                                                                     |
| MANUAL    | The Manual position slider drives the motor directly.                                                                                                                                                                      |
| EXPLORE   | **Torque off.** Move the horn by hand; the position is read back live. Use the FAR/NEAR capture buttons to record safe endpoints.                                                                                          |

All goals pass through a low-pass filter (τ ≈ 120 ms) so motion glides
instead of wiggling.

## GUI groups and parameters

### LIVE (always visible)

- **Mic level scope** — scrolling history of the normalized mic level.
  The **red dashed line** is the ALERT_DOG sound trigger; drag it to set
  the threshold directly.
- **Status line** — `goal` (commanded position), `pos` (actual position
  read from the motor), motor temperature, `dist` (ToF reading when
  presence detection is on), `FROZEN` (ALERT_DOG triggered),
  `!!HOT - TORQUE CUT` (overheat protection active), `NO MOTOR`.
- **Motor position** — slider drives the goal in MANUAL; follows your
  hand in EXPLORE. Clamped to the FAR/NEAR range.
- **FAR: xxx / NEAR: xxx buttons** — capture the motor's _current_
  position as the FAR/NEAR endpoint (the stored value is shown on the
  button). Writes to RAM — press SAVE DEFAULTS to persist.

### SAFE TRAVEL RANGE (all modes)

Hard limits, also written to the motor's own angle-limit registers.

- **POS far / POS near** — travel endpoints in AX-12A joint units (0–1023).
  Level 0 → FAR, level 1 → NEAR (unless inverted). Position sliders
  clamp to this range automatically.
- **Torque limit** — caps stall current (0–1023, ~300 recommended).
  Lower = cooler and weaker. The main protection against the motor
  heating when the mechanism jams.

### SOUND MAPPING (all modes)

Raw mic amplitude (mV peak-to-peak, 30 ms window) is mapped to a 0–1
level using these two bounds:

- **Quiet floor (mV pp)** — maps to level 0. Set just above resting room
  noise (watch the live mV readout).
- **Loud ceiling (mV pp)** — maps to level 1. Set at your loudest
  expected sound (singer close to the mic).

### MODE: SHY_MIC

- **Presence detection** (checkbox) — enables the ToF sensor gate:
  far → bowing, close → still, speaking → escape.
- **Presence threshold (mm)** — distance below which a person counts as
  "close" (100–3000).
- **Release** — envelope decay per 30 ms window (0.80–0.99). Higher =
  slower, dreamier falloff after a peak. Attack is always instant.
- **Toggle direction** — flips the response (loud → NEAR instead of FAR).

### MODE: ALERT_DOG

- **Dog threshold** — level (0–1) that freezes the search. Same as
  dragging the red line on the scope.
- **Dog step** — sweep speed while searching (fraction of the range per
  30 ms window; small = slow creep).
- **Dog freeze (ms)** — how long the motor stays frozen after the sound
  stops before resuming the search.

### MODE: MANUAL / EXPLORE

- MANUAL: **Manual position** slider (clamped to FAR/NEAR).
- EXPLORE: torque is off — move the horn by hand and use the capture
  buttons in LIVE.

### MOTOR FEEL (all modes)

- **Motor speed** — AX-12A moving speed (10–1023), applied immediately.

### Persistence

- **SAVE DEFAULTS to flash** — stores everything in NVS: quiet/loud
  floors, FAR/NEAR, speed, torque limit, release, dog threshold/step/
  freeze, presence on/off + threshold, invert. Restored at boot.
- **LOAD DEFAULTS** — re-reads flash and updates all sliders (undo
  unsaved experiments).

## Motor protection

1. **Torque limit** — caps stall current (slider above).
2. **Hardware angle limits** — the motor's own CW/CCW angle-limit
   registers are set to FAR/NEAR at boot and on every change; the motor
   refuses goals outside the safe range even if firmware misbehaves.
3. **Overheat cut** — motor temperature is polled every 500 ms. At
   ≥ 65 °C torque is cut and goals stop; motion resumes automatically
   once the motor cools below 55 °C.

## Calibration workflow

1. Select **EXPLORE** — the horn goes limp. Move it by hand to the safe
   extremes and press **FAR** / **NEAR** capture buttons.
2. Select **SHY_MIC**, watch the scope while making quiet/loud sounds,
   and set **Quiet floor** / **Loud ceiling**.
3. Optional: enable **Presence detection** and set the threshold by
   watching the live `dist=` readout while walking toward the mic.
4. In **ALERT_DOG**, drag the red threshold line just above ambient noise.
5. Tune **Motor speed**, **Torque limit** and **Release** for the feel
   you want.
6. Press **SAVE DEFAULTS to flash**.

## Serial monitor

115200 baud prints `pp` (raw mV peak-to-peak), normalized `level`, and
the current `goal` ~5×/s — handy when calibrating without the web page.

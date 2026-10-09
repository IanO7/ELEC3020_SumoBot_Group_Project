# ELEC3020_SumoBot_Group_Project

Sumobot firmware for the **LILYGO T-Display-S3** (ESP32-S3), built with PlatformIO.

## Current status (8 Oct 2026)

| Part | Status |
|---|---|
| Upload / flashing | ✅ Works (use BOOT + RST if no COM port appears). Disconnect the battery while uploading: GPIO44 (right motor) is HIGH during reset/upload and spins the right wheel |
| Motors | ✅ Both directions both wheels. Left = MDD3A M1 (GPIO17/18, `LEFT_MOTOR_INVERTED = true`), right = M2 (GPIO44/3). Loose signal wire fixed 7 Oct |
| Sonars | ✅ All 3 working. Middle had a loose contact (fixed 8 Oct). Ranges for the 100 cm ring: middle 70 cm, sides 35 cm |
| IR edge sensors | ✅ All 4 enabled (rear-right fixed and re-enabled 9 Oct) |
| Edge escape | ✅ Works well at full speed on the practice mat (white ring, black border) |
| Search / attack | ✅ Finds and pushes objects out. 8 Oct changes **not yet tested** (battery ran out): sumo-style spin-and-hop search, full-power charge, push-through escapes even after crossing the line, side sonars only steer (middle must confirm), 3 readings per target |
| Power | ✅ 3S LiPo, full motor power |

All settings in `include/config.h` are at competition values.

### Next session (9 Oct)

1. **Charge the LiPo** on the balance charger (don't run it below ~3.5V/cell = 10.5V).
2. **Test the 8 Oct changes** on the mat (keep people ~1 m back - sonar range is 70 cm):
   - Empty ring: spin, hop, edge escapes, never leaves the ring.
   - Box (>= 10 cm tall) in 4-5 positions: finds it and pushes it out each time.
   - Box at the edge: pushes it out and does **not** follow it out.
3. **Tune:** `SEARCH_SPIN_MS` = about one full turn; lower `SEARCH_TURN_SPEED` if it spins past the box without seeing it; `EDGE_PUSH_THROUGH_MS` (0-250) if it still follows the object out or backs off too early.
4. Before week 12: confirm starting on power-on (no start button) is allowed in the competition.

### Test modes (`include/config.h`)

| Setting | Effect |
|---|---|
| `TEST_EDGE_ONLY = true` | Drives forward, edge escapes only (no search/attack) |
| `TEST_ATTACK_ONLY = true` | No edge, no search: sits still until a sonar sees something, then attacks. **Test on the floor** |
| `USE_MID_SONAR` / `USE_SIDE_SONARS = false` | Ignore a broken sonar |
| `MOTOR_SPEED_LIMIT = 160` | Slow everything down for testing |
| `SONAR_MAX_CM = 25`, `SIDE_MAX_CM = 15` | Short range when people are near the ring |
| `START_DELAY_MS = 0` | No countdown |
| `DISPLAY_LIVE_IN_MATCH = false` | Freeze the sonar/edge boxes during the match (on by default) |

Screen: large state name in the middle (`START IN 5`, `SEARCH`, `ATTACK`, `EDGE!`); three sonar boxes **L / M / R** with the distance inside, turning **red when that zone sees the opponent**; four edge boxes **FL / FR / RL / RR**, red on the border; battery voltage top-left, reset reason top-right.

Single-motor tests (runs on battery, step + GPIO shown on screen): `pio run -e left_motor_test -t upload` / `right_motor_test`.

### Motor power (`include/config.h`)

Now on the 3S LiPo: `MOTOR_SPEED_LIMIT = 255`, `MOTOR_SPEED_MIN = 0`, `MOTOR_RAMP_MS = 0` (full power). If the ESP32 ever resets when the motors start (`reset: BROWNOUT` top right), set `MOTOR_RAMP_MS = 50`. For a weak battery (e.g. 9V PP3, which can't drive these motors): `110` / `70` / `400`.

## Project layout

| File | What it is |
|---|---|
| `include/pins.h` | Every GPIO assignment. Change wiring here only. |
| `include/config.h` | All tuning values (speeds, timings, ranges, motor direction flags). |
| `src/main.cpp` | Sumobot logic (state machine + display). |
| `src/motors.cpp` | Motor driver (2 PWM inputs per motor). |
| `src/sensors.cpp` | Ultrasonic (interrupt-timed, non-blocking) + IR edge sensors. |
| `src/tests/sensor_test.cpp` | Prints all sensor readings to the serial monitor. |
| `src/tests/motor_test.cpp` | Runs each motor in each direction to check the wiring. |

## Build environments

The project contains three separate programs. In the VS Code PlatformIO sidebar, choose the environment, then **Upload**. Or from a terminal:

| Environment | Command | Use it for |
|---|---|---|
| `sumo` (default) | `pio run -e sumo -t upload` | The competition firmware |
| `sensor_test` | `pio run -e sensor_test -t upload` then `pio device monitor` | Rough sensor check: distances + IR states every 0.5 s |
| `motor_test` | `pio run -e motor_test -t upload` then `pio device monitor` | Checking motor wiring/direction |
| `left_motor_test` / `right_motor_test` | `pio run -e left_motor_test -t upload` | One motor at rising speeds, forward then backward. Step and GPIO shown on screen (runs on battery) |

## Wiring

Source of truth is `include/pins.h`. If this table and that file disagree, the file wins.

| Function | GPIO | Notes |
|---|---|---|
| Ultrasonic TRIG (shared) | 2 | |
| Ultrasonic ECHO left | 10 | **5V → use a divider to 3.3V** |
| Ultrasonic ECHO middle | 11 | **5V → use a divider to 3.3V** |
| Ultrasonic ECHO right | 13 | **5V → use a divider to 3.3V** |
| IR front-left | 1 | |
| IR front-right | 12 | |
| IR rear-right | 16 | |
| IR rear-left | 43 | U0TXD, may glitch briefly during boot |
| Left motor (MDD3A M1A / M1B) | 17 / 18 | M1B = forward, so `LEFT_MOTOR_INVERTED = true` |
| Right motor (MDD3A M2A / M2B) | 44 / 3 | M2A = forward |
| KEY button | 14 | On-board, right-hand side button. Not used by `sumo` (starts on power-on); used to start `motor_test` / single motor tests |
| Spare | 21 | Free GPIO (was the optional start module; removed) |

**Hardware:** Cytron **MDD3A** motor driver (4–16V, 3A continuous per channel, PWM up to 20 kHz) and two **12V 200RPM 25D high-power gearmotors**. Connect the motor driver GND to the ESP32 GND.

The motors can draw several amps each when starting or stalled (pushing). The battery must handle that. A 9V PP3 can't.

Do not use GPIO 0, 4–9, 14, 15, 19, 20, 38–42, 45–48. The board uses them for the LCD, battery, USB and buttons.

## Flashing and running on the robot

> ⚠️ **Never have USB and the robot battery connected at the same time.** Flash over USB with the battery disconnected. Run on battery with USB unplugged.

1. **Flash.** Battery disconnected, TTGO plugged into the PC over USB. Select the environment (`sumo`, `sensor_test` or `motor_test`) and click **Upload**.
   - If no COM port shows up: hold **BOOT**, tap **RST**, release BOOT, then upload again. Press RST after the upload finishes.
   - If that still fails, try another USB-C cable. Charge-only cables power the board (red LED on) but can't carry data.
2. **Unplug USB** from the TTGO.
3. **Mount/plug the TTGO into the robot** (if it was removed).
4. **Place the robot in the ring, then connect the battery.** The 5 s countdown starts immediately and the match runs until the battery is disconnected. There is no start/stop button.
5. **To stop:** pick the robot up and disconnect the battery. Disconnect it **before** plugging USB back in.

**Test programs:**
- `motor_test`: flash, unplug USB, connect battery, press KEY, then watch the wheels. Serial output isn't needed because the step order is fixed (see checklist below).
- `sensor_test`: needs the serial monitor, so run it on **USB only** (battery disconnected). The sensors must be powered from the TTGO's 5V/3V3 pins for this to work.

## How the robot behaves

1. Power on → 5 s countdown on screen → match starts and runs until the battery is disconnected.
2. Every loop, in priority order:
   - **Edge:** an IR sensor sees the black border → back off and turn away (rear sensor → drive forward).
   - **Attack:** the middle sonar sees the opponent → full-power charge, side sonars steer to keep it centred.
   - **Search:** spin on the spot (about one full turn) toward where the opponent was last seen, then a short forward hop, repeat. (`SEARCH_ARC = true` switches to a continuous curve instead.)

---

## Test checklist (fill in as you go)

Tester: ________ Date: ________

### 1. Sensor test (`sensor_test`)

**IR edge sensors.** In the serial output, `1` should mean the sensor is over the **black border**. (Our ring is white inside with a black border.)

| Sensor | Reads 1 on black border? | Reads 0 on white inside? | Notes |
|---|---|---|---|
| Front-left (1) | ☐ | ☐ | |
| Front-right (12) | ☐ | ☐ | |
| Rear-right (16) | ☐ | ☐ | |
| Rear-left (43) | ☐ | ☐ | |

- [ ] If they are all backwards, flip `LINE_ACTIVE_LEVEL` in `config.h` (currently `HIGH`).
- [ ] Sensor height / trim pot adjusted so it triggers reliably on the real ring border.

**Ultrasonic.** Put a box in front of each sensor.

| Sensor | Real distance (cm) | Reading (cm) | Max reliable range (cm) | Notes |
|---|---|---|---|---|
| Left (10) | | | | |
| Middle (11) | | | | |
| Right (13) | | | | |

- [ ] An empty ring shows no false targets (all `--`).
- [ ] Ring diameter: ______ cm → `SONAR_MAX_CM` (middle, currently `25`, temporary for testing; was `60`) and `SIDE_MAX_CM` (sides, currently `15`; was `35`) limit attacks to targets inside the ~1 m ring.

### 2. Motor test (`motor_test`) — wheels off the ground!

| Step | Correct? | Fix applied |
|---|---|---|
| Left wheel forward | ☑ | |
| Left wheel backward | ☑ | |
| Right wheel forward | ☑ | |
| Right wheel backward | ☑ | |
| Both forward | ☑ | |
| Spin right (clockwise) | ☑ | |

- Wheel spins the wrong way → set `LEFT_MOTOR_INVERTED` / `RIGHT_MOTOR_INVERTED` to `true`.
- Wrong wheel moves → swap the motor connectors (or swap pins in `pins.h`).
- Motor driver type: ________ → if L298N, set `MOTOR_PWM_FREQ_HZ = 1000`.

### 3. Sumo tuning (`sumo`, on the real ring)

| Test | Pass? | Value tried → final | Notes |
|---|---|---|---|
| Countdown is 5 s and the robot stays still during it | ☐ | | |
| Disconnecting the battery stops the robot | ☐ | | |
| Drives at the front edge → backs off before falling (`ESCAPE_REVERSE_MS` = 250) | ☐ | | |
| Turns away from the edge far enough (`ESCAPE_TURN_MS` = 220) | ☐ | | |
| Both front sensors on the edge → turns about 180° (`ESCAPE_TURN_180_MS` = 400) | ☐ | | |
| Pushed backwards to the edge → drives forward out of it (`ESCAPE_FORWARD_MS` = 300) | ☐ | | |
| Search spin ≈ one full turn (`SEARCH_SPIN_MS` = 1200) | ☐ | | |
| Finds a still opponent placed anywhere in the ring | ☐ | | |
| Turns toward an opponent seen only by a side sonar | ☐ | | |
| Keeps pushing when touching the opponent (`CONTACT_HOLD_MS` = 350) | ☐ | | |
| Does not spin too fast to detect targets (`SEARCH_TURN_SPEED` = 150) | ☐ | | |
| Battery voltage on screen when fully charged: ______ V | — | | |

### 4. Match log

| Match | Opponent | Result | What went wrong / what to change |
|---|---|---|---|
| 1 | | | |
| 2 | | | |
| 3 | | | |

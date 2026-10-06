# ELEC3020_SumoBot_Group_Project

Sumobot firmware for the **LILYGO T-Display-S3** (ESP32-S3), built with PlatformIO.

## Current status (6 Oct 2026)

| Part | Status |
|---|---|
| Upload / flashing | ✅ Works (use BOOT + RST if no COM port appears) |
| Motor wiring (`motor_test`) | ✅ All steps correct |
| Sensors (`sensor_test`) | ✅ Ultrasonics and front IR correct. ⚠️ **Rear IR disabled** (`USE_REAR_EDGE_SENSORS = false`): mounted too high, always read "border" |
| Edge detection (front) | ✅ Stays inside the black border on the practice mat |
| Attack / search | ⚠️ **Jerky, switches between ATTACK / SEARCH / EDGE too often.** Doesn't push objects out yet. See below |
| Power | ⚠️ **9V PP3 too weak.** Resets the ESP32 at full motor power; running in weak-battery mode |

### Jerking: what we know

- **Confirmed (video, 6 Oct):** in weak-battery mode on the 9V the motors get ~2–3V. The robot barely moves and twitches in place at the border for 15+ s. Timed moves (escape reverse/turn, search spin) don't physically move it, so it keeps re-finding the same edge. It can't push anything at this power.
- **Stand test (6 Oct):** with the wheels free, states behave correctly with only slight oscillation → the logic is mostly fine. Some jerk is built into the design (search = spin then forward, escape = reverse then turn, attack = fixed straight/curve/pivot moves, instant reversals) and will remain, faster, at full power.
- **Already added in code:** edge sensors must read the border for 10 ms (`EDGE_CONFIRM_MS`), sonar median-of-3 + range hysteresis, 0.25 s hold when the target drops out, side sonars limited to 35 cm, close-only escape interrupt, 0.5 s post-escape cooldown, push-through at the edge when in contact.
- **Planned after the stand test:** smoother motion (search as a continuous arc, proportional attack steering, short ramp on reversals).
- **Test target:** use a box at least 10 cm tall. A 9V battery is too small/low for the sonars.

### Test log

| # | Test | Status | Result |
|---|---|---|---|
| 1 | **Stand test, 9V** (wheels off the ground, watch the state on screen, white card under the front IR sensors). Nothing within 1 m → should stay in SEARCH | ✅ 6 Oct | Mostly SEARCH, slight oscillation to ATTACK |
| 2 | Stand test: box ~30 cm in front of middle sonar → should stay in ATTACK, wheels forward | ✅ 6 Oct | Stays in ATTACK; back to SEARCH when removed (slight oscillation on removal) |
| 3 | Stand test: move box slowly left/right → wheels steer toward it without snapping | ⏳ Not checked | |
| 4 | Stand test: black card under one front IR → EDGE once (reverse, turn), then SEARCH | ✅ 6 Oct | Edge OK. Note: on a stand with nothing under it, the IR sees "black" (no reflection) → constant EDGE, so put white card under for tests 1–3 |
| 5 | **Fit 3S LiPo**, weak-battery mode off, repeat tests 1–4 on the stand | ⏳ To do | |
| 6 | Mat, no object: moves around smoothly, never leaves the ring | ⏳ To do | |
| 7 | Mat, cardboard box: finds it and pushes it out | ⏳ To do | |

If 1–4 fail → code/sensor issue. If 1–4 pass but the robot still jerks on the mat with the 9V → power.

**Result (6 Oct, confirmed on a second stand video):** robot held in the air, white card under the front IR sensors. Edge triggers correctly when the card is removed, and starts in SEARCH. It switches straight to ATTACK when a hand or object is put in front, with very little oscillation. Stand tests 1, 2, 4 pass with only slight oscillation → the logic works; the heavy jerking on the mat is mainly the 9V. Remaining small oscillation will be smoothed after the LiPo test (planned smoother motion above).

**To do**
1. Get a proper battery: **3S LiPo (11.1V)**, e.g. CNHL Ministar 650mAh 3S XT30 ($15.95, Buzz FPV Wangara) + XT30 pigtail. Needs a LiPo balance charger. Must stay under the MDD3A's 16V max.
2. Run the test log above.
3. Lower the rear IR sensors until they read correctly in `sensor_test`, then set `USE_REAR_EDGE_SENSORS = true`.
4. Switch weak-battery mode off (see below), and set `DISPLAY_LIVE_IN_MATCH = false` for competition.
5. Re-tune the timed moves (`ESCAPE_*_MS`, `SEARCH_SPIN_MS`) at full power. Fill in checklist section 3.

### Weak-battery mode

In `include/config.h`:

| Setting | Now (9V battery) | Proper battery |
|---|---|---|
| `MOTOR_SPEED_LIMIT` | `110` (~43% max power) | `255` |
| `MOTOR_SPEED_MIN` | `70` (slowest speed that still moves) | `0` |
| `MOTOR_RAMP_MS` | `400` (gradual start) | `0` (or `50` if it still resets occasionally) |

If the robot still resets on the 9V, lower `MOTOR_SPEED_LIMIT` (90) or raise `MOTOR_RAMP_MS` (600). If a wheel hums but doesn't turn, raise `MOTOR_SPEED_MIN`. The screen shows `reset: BROWNOUT` (top right) after a power-related reset.

Timings tuned in weak-battery mode will be off at full power, because they are times, not angles.

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
| Left motor IN1 / IN2 (M1A / M1B) | 17 / 18 | |
| Right motor IN1 / IN2 (M2A / M2B) | 44 / 3 | |
| Start/stop button | 14 | On-board **KEY**, the right-hand side button (the other side button is BOOT) |
| Start module (optional) | 21 | Only if `USE_START_MODULE = true` |

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
4. **Connect the robot battery.** The screen should turn on and show the sensor view (`sumo` firmware).
5. **Place the robot** in the ring.
6. **Press KEY** (right-hand side button). 5 s countdown, then the match starts. Press KEY again to stop.
7. **When finished:** press KEY to stop, then disconnect the battery **before** plugging USB back in.

**Test programs:**
- `motor_test`: flash, unplug USB, connect battery, press KEY, then watch the wheels. Serial output isn't needed because the step order is fixed (see checklist below).
- `sensor_test`: needs the serial monitor, so run it on **USB only** (battery disconnected). The sensors must be powered from the TTGO's 5V/3V3 pins for this to work.

## How the robot behaves

1. Press **KEY** → 5 s countdown on screen → match starts. Press KEY again to stop; press once more to re-arm.
2. Every loop, in priority order:
   - **Edge:** an IR sensor sees the black border → back off and turn away (rear sensor → drive forward).
   - **Attack:** a sonar sees the opponent → steer at it and push (full power when close).
   - **Search:** spin toward where the opponent was last seen, then move forward a bit and repeat.

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
- [ ] Ring diameter: ______ cm → `SONAR_MAX_CM` (middle, currently `60`) and `SIDE_MAX_CM` (sides, currently `35`) limit attacks to targets inside the ~1 m ring.

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
| KEY stops the robot immediately | ☐ | | |
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

# ELEC3020_SumoBot_Group_Project

Sumobot firmware for the **LILYGO T-Display-S3** (ESP32-S3), built with PlatformIO.

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

| Function | GPIO | Notes |
|---|---|---|
| Ultrasonic TRIG (shared) | 1 | |
| Ultrasonic ECHO left | 44 | **5V → use a divider to 3.3V** |
| Ultrasonic ECHO middle | 43 | **5V → use a divider to 3.3V** |
| Ultrasonic ECHO right | 18 | **5V → use a divider to 3.3V** |
| IR front-left | 16 | |
| IR front-right | 21 | |
| IR rear-right | 17 | |
| IR rear-left | 2 | |
| Left motor IN1 / IN2 | 10 / 11 | |
| Right motor IN1 / IN2 | 12 / 13 | |
| Start/stop button | 14 | On-board KEY button |
| Start module (optional) | 3 | Only if `USE_START_MODULE = true` |

The motor driver needs 2 inputs per motor (DRV8833, MX1508, or L298N with the ENA/ENB jumpers **on**). Connect the motor driver GND to the ESP32 GND.

Do not use GPIO 0, 4–9, 14, 15, 19, 20, 38–48. The board uses them for the LCD, battery, USB and buttons.

## How the robot behaves

1. Press **KEY** → 5 s countdown on screen → match starts. Press KEY again to stop; press once more to re-arm.
2. Every loop, in priority order:
   - **Edge:** an IR sensor sees white → back off and turn away (rear sensor → drive forward).
   - **Attack:** a sonar sees the opponent → steer at it and push (full power when close).
   - **Search:** spin toward where the opponent was last seen, then move forward a bit and repeat.

---

## Test checklist (fill in as you go)

Tester: ________ Date: ________

### 1. Sensor test (`sensor_test`)

**IR edge sensors.** In the serial output, `1` should mean the sensor is over **white**.

| Sensor | Reads 1 on white? | Reads 0 on black? | Notes |
|---|---|---|---|
| Front-left (16) | ☐ | ☐ | |
| Front-right (21) | ☐ | ☐ | |
| Rear-right (17) | ☐ | ☐ | |
| Rear-left (2) | ☐ | ☐ | |

- [ ] If they are all backwards, flip `LINE_ACTIVE_LEVEL` in `config.h` (currently `LOW`).
- [ ] Sensor height / trim pot adjusted so it triggers reliably on the real ring border.

**Ultrasonic.** Put a box in front of each sensor.

| Sensor | Real distance (cm) | Reading (cm) | Max reliable range (cm) | Notes |
|---|---|---|---|---|
| Left (44) | | | | |
| Middle (43) | | | | |
| Right (18) | | | | |

- [ ] An empty ring shows no false targets (all `--`).
- [ ] Ring diameter: ______ cm → set `SONAR_MAX_CM` to about the ring diameter (currently `70`).

### 2. Motor test (`motor_test`) — wheels off the ground!

| Step | Correct? | Fix applied |
|---|---|---|
| Left wheel forward | ☐ | |
| Left wheel backward | ☐ | |
| Right wheel forward | ☐ | |
| Right wheel backward | ☐ | |
| Both forward | ☐ | |
| Spin right (clockwise) | ☐ | |

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

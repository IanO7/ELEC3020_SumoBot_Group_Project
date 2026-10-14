# ELEC3020_SumoBot_Group_Project

Autonomous sumobot firmware for the **LILYGO T-Display-S3** (ESP32-S3), built with PlatformIO. Group project for ELEC3020 (UWA): find the opponent in a ~100 cm ring and push it out without leaving the ring.

## Status (10 Oct 2026)

Everything works on the practice mat (white ring, black border): starts straight away on power-on, edge escape at full speed, sumo-style search, and full-power attack that pushes objects out. All 4 IR sensors and all 3 sonars are enabled; `include/config.h` is at competition values (start delay 0 - see to-do).

Still to do:
- Tune `SEARCH_SPIN_MS` (about one full turn) and `SEARCH_TURN_SPEED` (lower if it spins past a target), and `EDGE_PUSH_THROUGH_MS` (0-250) for pushing objects out without following them.
- Confirm the competition allows starting immediately on power-on (no button, no countdown). The brief's rules may require a 5 s delay: set `START_DELAY_MS = 5000`.
- Loose jumper contacts have caused faults more than once (motor signal, middle sonar). Solder or hot-glue connectors before the competition.

## Hardware

- **Controller:** LILYGO T-Display-S3 (ESP32-S3, 320x170 LCD)
- **Motor driver:** Cytron **MDD3A** (4-16V, 3A per channel, PWM up to 20 kHz)
- **Motors:** two 12V 200RPM 25D high-power gearmotors
- **Sensors:** 3 x HC-SR04 ultrasonic (left / middle / right, shared trigger), 4 x Maker Reflect IR edge sensors
- **Battery:** 3S LiPo (11.1V). A 9V PP3 can't supply the motor current - the ESP32 browns out.

## Project layout

| File | What it is |
|---|---|
| `include/pins.h` | Every GPIO assignment. Change wiring here only. |
| `include/config.h` | All tuning values: speeds, timings, sonar ranges, motor directions, test modes. |
| `src/main.cpp` | Sumobot logic (state machine) and display. |
| `src/motors.cpp` | Motor driver (2 PWM inputs per motor, speed limit/ramp). |
| `src/sensors.cpp` | Ultrasonic (interrupt-timed, median-filtered) and IR edge sensors (debounced). |
| `src/tests/sensor_test.cpp` | Prints all sensor readings to the serial monitor. |
| `src/tests/motor_test.cpp` | Runs each motor in each direction. |
| `src/tests/single_motor_test.cpp` | One motor at rising speeds, step and GPIO shown on screen. |
| `Manual.pdf` | Group project manual. |

## Build environments

Choose the environment in the PlatformIO sidebar, then **Upload**, or from a terminal:

| Environment | Command | Use it for |
|---|---|---|
| `sumo` (default) | `pio run -e sumo -t upload` | The competition firmware |
| `sensor_test` | `pio run -e sensor_test -t upload`, then `pio device monitor` | Sonar distances and IR states every 0.5 s (USB only) |
| `motor_test` | `pio run -e motor_test -t upload` | Each wheel forward/backward, both forward, spin right. Press KEY to start (wheels off the ground) |
| `left_motor_test` / `right_motor_test` | `pio run -e left_motor_test -t upload` | One motor forward then backward at rising speeds; shows the step and the GPIO being driven. Press KEY to start |

If the PlatformIO Serial Monitor button errors ("Cannot read properties of undefined"), use the terminal: `pio device monitor -b 115200`, or reload VS Code (`Ctrl+Shift+P` → Developer: Reload Window).

## Wiring

`include/pins.h` is the source of truth.

| Function | GPIO | Notes |
|---|---|---|
| Ultrasonic TRIG (shared) | 2 | All three sonars ping together every 30 ms |
| Ultrasonic ECHO left / middle / right | 10 / 11 / 13 | **5V → divider to 3.3V** |
| IR front-left / front-right | 1 / 12 | |
| IR rear-left / rear-right | 43 / 16 | GPIO43 is U0TXD, may glitch briefly at boot |
| Left motor (MDD3A M1A / M1B) | 17 / 18 | M1B = forward, so `LEFT_MOTOR_INVERTED = true` |
| Right motor (MDD3A M2A / M2B) | 44 / 3 | M2A = forward. GPIO44 is HIGH during reset/upload, so the right wheel can twitch then |
| KEY button | 14 | Only used by the motor test programs |
| Spare | 21 | Free GPIO |

Connect the motor driver GND to the ESP32 GND. Don't use GPIO 0, 4-9, 14, 15, 19, 20, 38-42, 45-48 (LCD, battery, USB, buttons).

## Flashing and running

> ⚠️ **Never connect USB and the robot battery at the same time.** Flash with the battery disconnected; run on battery with USB unplugged.

1. **Flash:** battery off, USB on, upload the environment.
   - No COM port? Hold **BOOT**, tap **RST**, release BOOT, upload again, then press RST.
   - Still nothing? Try another USB-C cable - charge-only cables power the board but carry no data.
2. **Unplug USB.**
3. **Place the robot in the ring, then connect the battery.** It starts the match immediately - also after any reset or brownout.
4. **To stop:** pick the robot up and disconnect the battery. There is no start/stop button.

LiPo care: charge only on a balance charger, don't run below ~3.5V/cell (10.5V), store at ~3.8V/cell. The voltage on the screen is the TTGO's battery pin, not the LiPo.

## How the robot behaves

Power on → match starts immediately and runs until the battery is disconnected (with `START_DELAY_MS` > 0 the screen counts down `START IN n` first). Every loop, in priority order:

1. **Edge** - an IR sensor sees the black border (confirmed for 10 ms):
   - front sensor → reverse, then turn away (both front → turn ~180°)
   - rear sensor → drive forward, angled away
   - while pushing an opponent (in contact), keep pushing for `EDGE_PUSH_THROUGH_MS` to shove it over the line, then escape - even if the sensors have already crossed the line.
2. **Attack** - the middle sonar sees a target within 70 cm (3 readings in a row) → **full-power charge**; side sonars steer to keep it centred. A side sonar alone doesn't start an attack, it turns the search toward that side.
3. **Search** - spin on the spot (about one full turn) toward where the opponent was last seen, then a short forward hop, repeat.

**Screen:** large state name (`SEARCH` cyan, `ATTACK` red, `EDGE!` yellow); three sonar boxes **L / M / R** with the distance inside, **red when that zone sees the opponent**; four edge boxes **FL / FR / RL / RR**, red on the border, grey if disabled; voltage top-left; reset reason top-right (`BROWNOUT` in red = battery sagged).

## Test modes and useful settings (`include/config.h`)

| Setting | Effect |
|---|---|
| `TEST_EDGE_ONLY = true` | Drives forward and only does edge escapes (no search/attack) |
| `TEST_ATTACK_ONLY = true` | No edge, no search: waits until a sonar sees something, then attacks. **Test on the floor** |
| `USE_MID_SONAR` / `USE_SIDE_SONARS = false` | Ignore a faulty sonar |
| `USE_REAR_EDGE_SENSORS` / `USE_REAR_RIGHT_SENSOR = false` | Ignore faulty rear IR sensors |
| `MOTOR_SPEED_LIMIT = 160` | Slow everything down proportionally |
| `SONAR_MAX_CM = 25`, `SIDE_MAX_CM = 15` | Short sonar range when people stand near the ring (competition: 70 / 35) |
| `START_DELAY_MS = 5000` | 5 s countdown before the match (currently 0 = start immediately) |
| `LINE_ACTIVE_LEVEL` | `HIGH` = black border (practice mat). Use `LOW` for a black ring with a white line |
| `LEFT/RIGHT_MOTOR_INVERTED` | Flip a wheel that runs backwards in `motor_test` |
| `MOTOR_RAMP_MS = 50` | Soften motor starts if the ESP32 resets (`BROWNOUT`) |

## License

See [LICENSE](LICENSE).

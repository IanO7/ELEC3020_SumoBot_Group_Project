#pragma once
#include <Arduino.h>

// =====================================================================
//  Sumobot tuning. All speeds are 0..255 PWM. Tune on the real dohyo.
// =====================================================================

// --- Wiring fixes (flip if a motor spins the wrong way in motor_test) -
constexpr bool LEFT_MOTOR_INVERTED  = false;
constexpr bool RIGHT_MOTOR_INVERTED = false;

// --- Motor PWM --------------------------------------------------------
// 20 kHz is silent and suits DRV8833/MX1508/TB6612. Use ~1000 for L298N.
constexpr uint32_t MOTOR_PWM_FREQ_HZ = 20000;
constexpr uint8_t  MOTOR_PWM_BITS    = 8;

// --- Weak-battery protection -----------------------------------------
// A 9V PP3 battery can't supply motor start-up current: the voltage
// collapses and the ESP32 resets. These cap and soften motor current.
// Every non-zero speed (1..255) is squeezed into MIN..LIMIT, so slow moves
// still turn the wheels while fast ones stay gentle on the battery.
// Proper battery (3S LiPo): LIMIT 255, MIN 0, RAMP 0.
constexpr uint8_t  MOTOR_SPEED_LIMIT = 110;  // fastest any motor runs (255 = no limit)
constexpr uint8_t  MOTOR_SPEED_MIN   = 70;   // slowest non-zero speed; raise if wheels hum but don't turn
constexpr uint16_t MOTOR_RAMP_MS     = 400;  // time to ramp 0 -> full speed (0 = instant)

// --- IR edge sensors --------------------------------------------------
// Level a sensor outputs when it sees the ring BORDER.
// Our ring is WHITE inside with a BLACK border. TCRT5000 modules output
// HIGH on black (no reflection) -> HIGH. A standard black ring with a
// white border would need LOW.
constexpr uint8_t LINE_ACTIVE_LEVEL = HIGH;
// TEMPORARY: rear sensors sit too high and always read "border".
// false = the sumo code ignores them (sensor_test still shows them).
constexpr bool USE_REAR_EDGE_SENSORS = false;
// A sensor must see the border continuously this long before it counts.
// Filters out motor-noise spikes (a few us-ms) that caused phantom EDGEs.
constexpr uint32_t EDGE_CONFIRM_MS = 10;

// --- Ultrasonic -------------------------------------------------------
// Ring is ~1 m diameter: from the centre the opponent is always < ~60 cm.
// Anything further is probably outside the ring (people, walls).
constexpr uint16_t SONAR_MAX_CM      = 60;   // middle sonar: ignore anything beyond this
constexpr uint16_t SIDE_MAX_CM       = 35;   // side sonars: only react to close targets
// DEBUG: false = sumo code ignores the left/right sonars (middle only).
constexpr bool     USE_SIDE_SONARS   = true;
constexpr uint16_t SONAR_INTERVAL_MS = 30;   // ping period (>= ~25 ms to avoid ghost echoes)
// Each sonar uses the median of its last 3 readings, so one bad echo
// (or one missed echo) is ignored. Then:
constexpr uint8_t  SONAR_HITS_TO_SEE  = 2;   // consecutive (median) hits before a target counts
constexpr uint8_t  SONAR_MISSES_TO_LOSE = 4; // consecutive (median) misses before it is dropped
constexpr uint16_t SONAR_HYST_CM      = 10;  // once seen, keep it until this much past the max range

// --- Match ------------------------------------------------------------
constexpr uint32_t START_DELAY_MS   = 5000;  // mandatory 5 s after pressing start
constexpr bool     USE_START_MODULE = false; // true = use IR start module on PIN_START_MODULE
constexpr int8_t   START_SEARCH_DIR = +1;    // first search spin: +1 right, -1 left

// --- Attack -----------------------------------------------------------
constexpr uint8_t  ATTACK_SPEED     = 255;   // target close in front: full push
constexpr uint8_t  APPROACH_SPEED   = 210;   // target ahead but far
constexpr uint8_t  TRACK_TURN_SPEED = 170;   // pivot toward a side-sensor target
constexpr float    STEER_RATIO      = 0.6f;  // inner wheel ratio when target is mid + one side
constexpr uint16_t CLOSE_CM         = 25;    // below this -> ATTACK_SPEED
constexpr uint16_t CONTACT_CM       = 10;    // closer than this = we're touching
constexpr uint32_t CONTACT_HOLD_MS  = 350;   // keep pushing this long after sonar drops out at contact
// Pushing an opponent and our FRONT reaches the border: they're already
// over it, so keep pushing this long before escaping. 0 = always escape.
constexpr uint32_t EDGE_PUSH_THROUGH_MS = 300;

// --- Anti-jerk --------------------------------------------------------
// Target briefly lost: keep doing the last attack move this long before
// falling back to search (stops ATTACK <-> SEARCH flicker).
constexpr uint32_t TARGET_LOST_HOLD_MS  = 250;
// After an edge escape, ignore targets further than CLOSE_CM for this long
// (stops chasing things outside the ring straight back into the edge).
constexpr uint32_t ESCAPE_COOLDOWN_MS   = 500;

// --- Search -----------------------------------------------------------
constexpr uint8_t  SEARCH_TURN_SPEED = 150;
constexpr uint8_t  SEARCH_FWD_SPEED  = 140;
constexpr uint32_t SEARCH_SPIN_MS    = 1200; // ~1 full turn, then reposition
constexpr uint32_t SEARCH_ADVANCE_MS = 350;

// --- Edge escape ------------------------------------------------------
constexpr uint8_t  ESCAPE_REVERSE_SPEED = 220;
constexpr uint8_t  ESCAPE_TURN_SPEED    = 210;
constexpr uint8_t  ESCAPE_FORWARD_SPEED = 230;
constexpr uint16_t ESCAPE_REVERSE_MS    = 250;
constexpr uint16_t ESCAPE_TURN_MS       = 220;  // ~100-135 deg away from edge
constexpr uint16_t ESCAPE_TURN_180_MS   = 400;  // both front sensors on edge
constexpr uint16_t ESCAPE_FORWARD_MS    = 300;  // pushed back onto the edge

// --- Display ----------------------------------------------------------
constexpr bool     ENABLE_DISPLAY     = true;
constexpr uint32_t DISPLAY_REFRESH_MS = 150;  // live sensor view refresh period
// Testing: keep the live sensor view updating during a match. Each redraw
// pauses the control loop for a few ms - set false for competition.
constexpr bool     DISPLAY_LIVE_IN_MATCH = true;

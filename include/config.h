#pragma once
#include <Arduino.h>

// =====================================================================
//  Sumobot tuning. All speeds are 0..255 PWM. Tune on the real dohyo.
// =====================================================================

// --- Wiring fixes (flip if a motor spins the wrong way in motor_test) -
constexpr bool LEFT_MOTOR_INVERTED  = true;   // left wheel forward = M1B (button test)
constexpr bool RIGHT_MOTOR_INVERTED = false;

// --- Motor PWM --------------------------------------------------------
// 20 kHz is silent and suits DRV8833/MX1508/TB6612. Use ~1000 for L298N.
constexpr uint32_t MOTOR_PWM_FREQ_HZ = 20000;
constexpr uint8_t  MOTOR_PWM_BITS    = 8;

// --- Motor power limits -----------------------------------------------
// Running on a 3S LiPo: full power, no ramp. (Weak-battery mode for a 9V
// PP3 was LIMIT 110, MIN 70, RAMP 400: every non-zero speed squeezed into
// MIN..LIMIT and acceleration ramped so the battery didn't brown out.)
// Lower (e.g. 160) to slow everything down proportionally for testing.
constexpr uint8_t  MOTOR_SPEED_LIMIT = 255;  // fastest any motor runs (255 = no limit)
constexpr uint8_t  MOTOR_SPEED_MIN   = 0;    // slowest non-zero speed (0 = no minimum)
constexpr uint16_t MOTOR_RAMP_MS     = 0;    // ramp 0 -> full speed (0 = instant; try 50 if it resets)

// --- IR edge sensors --------------------------------------------------
// Level a sensor outputs when it sees the ring BORDER.
// Our ring is WHITE inside with a BLACK border. Our Maker Reflect IR
// sensors read HIGH over black (tested in sensor_test) -> HIGH. A standard
// black ring with a white border would need LOW.
constexpr uint8_t LINE_ACTIVE_LEVEL = HIGH;
// false = the sumo code ignores the rear sensors (sensor_test still shows
// them). Use if they're mounted too high and always read "border".
constexpr bool USE_REAR_EDGE_SENSORS = true;
// false = ignore just the rear-right sensor (e.g. if it misreads).
constexpr bool USE_REAR_RIGHT_SENSOR = true;
// A sensor must see the border continuously this long before it counts.
// Filters out motor-noise spikes (a few us-ms) that caused phantom EDGEs.
constexpr uint32_t EDGE_CONFIRM_MS = 10;

// --- Ultrasonic -------------------------------------------------------
// Ring is ~1 m diameter: from the centre the opponent is always < ~60 cm.
// Anything further is probably outside the ring (people, walls).
// Ring measured 100 cm. Robots start ~50-60 cm apart, so 70 sees the
// opponent from the start; spectators stay 3 ft (~90 cm) back, so < ~85 cm
// won't pick them up. For testing with people close by, use 25 / 15.
constexpr uint16_t SONAR_MAX_CM      = 70;   // middle sonar: ignore anything beyond this
constexpr uint16_t SIDE_MAX_CM       = 35;   // side sonars: only react to close targets
// TEMPORARY (debugging attack rotations): false = sumo code ignores the
// left/right sonars, so ATTACK only charges straight at middle-sonar targets.
constexpr bool     USE_SIDE_SONARS   = true;
// false = ignore the middle sonar (attack then uses the side sonars only:
// both see it = straight, one sees it = swing toward that side).
constexpr bool     USE_MID_SONAR     = true;
constexpr uint16_t SONAR_INTERVAL_MS = 30;   // ping period (>= ~25 ms to avoid ghost echoes)
// Each sonar uses the median of its last 3 readings, so one bad echo
// (or one missed echo) is ignored. Then:
constexpr uint8_t  SONAR_HITS_TO_SEE  = 3;   // consecutive (median) hits before a target counts (2 let noise through)
constexpr uint8_t  SONAR_MISSES_TO_LOSE = 4; // consecutive (median) misses before it is dropped
constexpr uint16_t SONAR_HYST_CM      = 10;  // once seen, keep it until this much past the max range

// --- Match ------------------------------------------------------------
// Power on -> this delay -> fight. No start/stop button.
// 0 = straight into the match on power-on (also straight back in after any
// reset/brownout). Set 5000 for a 5 s countdown if the rules require it.
constexpr uint32_t START_DELAY_MS   = 0;
// TEMPORARY TEST MODES - set both false for normal use.
// TEST_ATTACK_ONLY: edge ignored, no search. Sits still until a sonar sees
//   something, then attacks it. WILL DRIVE OFF THE RING - test on the floor.
// TEST_EDGE_ONLY: no attack, no search. Drives straight forward at
//   EDGE_TEST_SPEED and does the normal edge escape at the border.
constexpr bool     TEST_ATTACK_ONLY = false;
constexpr bool     TEST_EDGE_ONLY   = false;
constexpr uint8_t  EDGE_TEST_SPEED  = 140;
constexpr int8_t   START_SEARCH_DIR = +1;    // first search spin: +1 right, -1 left

// --- Attack -----------------------------------------------------------
constexpr uint8_t  ATTACK_SPEED     = 255;   // target close in front: full push
constexpr uint8_t  APPROACH_SPEED   = 255;   // target ahead but far (full power: sumo-style charge as soon as it is seen)
constexpr uint8_t  TRACK_TURN_SPEED = 220;   // outer wheel when swinging toward a side target (150 stalled the left motor at the 160 speed limit)
// Inner wheel when turning toward a side target. 0 = inner wheel stopped:
// the robot swings toward the target AND moves forward, so it still reaches
// it if the middle sonar never confirms. Negative = spin in place (old way).
constexpr int16_t  TRACK_INNER_SPEED = 0;
constexpr float    STEER_RATIO      = 0.6f;  // inner wheel ratio when target is mid + one side
constexpr uint16_t CLOSE_CM         = 25;    // below this -> ATTACK_SPEED
constexpr uint16_t CONTACT_CM       = 10;    // closer than this = we're touching
constexpr uint32_t CONTACT_HOLD_MS  = 350;   // keep pushing this long after sonar drops out at contact
// Pushing an opponent and our FRONT reaches the border: they're already
// over it, so keep pushing this long before escaping. 0 = always escape.
constexpr uint32_t EDGE_PUSH_THROUGH_MS = 150;  // was 300: robot followed the object out
// false = a side sonar alone can't start an attack (noise / crosstalk made
// the robot drive circles at nothing); it just curves the search toward that
// side. The middle sonar must confirm. Ignored when USE_MID_SONAR is false.
constexpr bool     SIDE_ONLY_ATTACK     = false;

// --- Anti-jerk --------------------------------------------------------
// Target briefly lost: keep doing the last attack move this long before
// falling back to search (stops ATTACK <-> SEARCH flicker).
constexpr uint32_t TARGET_LOST_HOLD_MS  = 400;
// After an edge escape, ignore targets further than CLOSE_CM for this long
// (stops chasing things outside the ring straight back into the edge).
constexpr uint32_t ESCAPE_COOLDOWN_MS   = 500;
// Pivoting toward a side-sonar target this long without the middle sonar
// confirming it -> it's a false reading (wire, chassis, floor, crosstalk):
// ignore that side sonar for SIDE_IGNORE_MS. Stops endless 360 spins.
constexpr uint32_t TRACK_TIMEOUT_MS     = 1500;
constexpr uint32_t SIDE_IGNORE_MS       = 2000;

// --- Search -----------------------------------------------------------
// Sumo-style search: spin on the spot to sweep the sonar round the ring,
// then a short forward hop to a new spot, repeat. The spin must be slow
// enough for the sonar (~0.1 s to confirm a target, ~30 deg beam).
constexpr uint8_t  SEARCH_TURN_SPEED = 150;  // lower if it spins past targets without seeing them
constexpr uint8_t  SEARCH_FWD_SPEED  = 200;  // forward hop between spins
constexpr uint32_t SEARCH_SPIN_MS    = 1200; // TUNE to about one full turn at SEARCH_TURN_SPEED
// Don't spin again if we already did within this time, and never spin right
// after an edge escape (the escape already turned us). Stops repeated spins.
constexpr uint32_t SEARCH_RESPIN_MS  = 3000;
constexpr uint32_t SEARCH_ADVANCE_MS = 350;
// true  = spin once, then drive a continuous curve (smooth, no stop-start).
// false = keep alternating spin SEARCH_SPIN_MS / forward SEARCH_ADVANCE_MS.
constexpr bool     SEARCH_ARC        = false;
constexpr uint8_t  SEARCH_ARC_OUTER  = 200;  // outer wheel speed while curving
constexpr uint8_t  SEARCH_ARC_INNER  = 40;   // inner wheel (bigger = wider curve)

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
// Keep the sonar/edge boxes updating during the match (shows where the
// opponent is detected - required by the brief). Only boxes that changed are
// redrawn, so the cost to the control loop is small.
constexpr bool     DISPLAY_LIVE_IN_MATCH = true;

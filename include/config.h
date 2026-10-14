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
// false = the sumo code ignores the rear sensors. Setting false eliminates
// phantom edge triggers and escape jitter caused by GPIO 43 (UART0 TX).
constexpr bool USE_REAR_EDGE_SENSORS = false;
// false = ignore just the rear-right sensor (e.g. if it misreads).
constexpr bool USE_REAR_RIGHT_SENSOR = false;
// Leaky integrator filter thresholds:
// Samples every 1 ms. Pin == border increments confidence (+2), else decrements (-1).
// Requires CONFIRM_COUNT to activate, drops below RELEASE_COUNT to clear.
// Completely filters out motor EMI brush spikes and border boundary chatter.
constexpr int16_t  EDGE_CONFIRM_COUNT = 14;  // ~10-12 ms of predominantly black to trigger
constexpr int16_t  EDGE_RELEASE_COUNT = 4;   // ~12-16 ms of predominantly white to clear
// Cooldown after an edge escape finishes: prevents immediate re-triggering / jitter
// and allows the robot to drive safely back into the ring interior.
constexpr uint32_t EDGE_ESCAPE_COOLDOWN_MS = 300;

// --- Ultrasonic -------------------------------------------------------
// Ring is ~1 m diameter: from the centre the opponent is always < ~60 cm.
// Anything further is probably outside the ring (people, walls).
// Ring measured 100 cm. Robots start ~50-60 cm apart, so 70 sees the
// opponent from the start; spectators stay 3 ft (~90 cm) back, so < ~85 cm
// won't pick them up. For testing with people close by, use 25 / 15.
constexpr uint16_t SONAR_MAX_CM      = 70;   // middle sonar: ignore anything beyond this
constexpr uint16_t SIDE_MAX_CM       = 30;   // side sonars: only react to close targets (reduces crosstalk)
constexpr bool     USE_SIDE_SONARS   = true; // enabled with forward turning (no spin-in-place)
constexpr bool     USE_MID_SONAR     = true;
constexpr uint16_t SONAR_INTERVAL_MS = 30;   // ping period (>= ~25 ms to avoid ghost echoes)
// 2 hits confirms in ~60ms so the robot does not spin past targets during search.
constexpr uint8_t  SONAR_HITS_TO_SEE  = 2;
constexpr uint8_t  SONAR_MISSES_TO_LOSE = 3;
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
constexpr uint8_t  TRACK_TURN_SPEED = 220;   // outer wheel when swinging toward a side target
// Inner wheel when turning toward a side target. 120 keeps the robot driving
// forward while swinging into the target. 0 stopped the inner wheel and caused
// spinning in circles.
constexpr int16_t  TRACK_INNER_SPEED = 120;
// Steering ratio when mid + one side sees target. 0.8 is a smooth curve that keeps
// heading towards target without swinging away (0.6 was too aggressive and circled).
constexpr float    STEER_RATIO      = 0.8f;
constexpr uint16_t CLOSE_CM         = 25;    // below this -> pure straight charge (ATTACK_SPEED)
constexpr uint16_t CONTACT_CM       = 15;    // closer than this = in contact / touching (marks contact hold)
// Keep pushing straight at full power this long after sonar drops out in contact.
// 1800 ms gives enough time to push an opponent across the ring to the edge.
constexpr uint32_t CONTACT_HOLD_MS  = 1800;
// Pushing an opponent and our FRONT reaches the border: keep pushing to shove them out.
constexpr uint32_t EDGE_PUSH_THROUGH_MS = 250;
// true = allows swinging toward side-sonar targets while maintaining forward motion.
constexpr bool     SIDE_ONLY_ATTACK     = true;

// --- Anti-jerk --------------------------------------------------------
// Target briefly lost: keep doing the last attack move this long before
// falling back to search (stops ATTACK <-> SEARCH flicker).
constexpr uint32_t TARGET_LOST_HOLD_MS  = 400;
// After an edge escape, ignore targets further than CLOSE_CM for this long
// (stops chasing things outside the ring straight back into the edge).
constexpr uint32_t ESCAPE_COOLDOWN_MS   = 400;
// Pivoting toward a side-sonar target this long without the middle sonar
// confirming it -> false reading: mute side sonars for SIDE_IGNORE_MS.
constexpr uint32_t TRACK_TIMEOUT_MS     = 800;
constexpr uint32_t SIDE_IGNORE_MS       = 1500;

// --- Search -----------------------------------------------------------
// Sumo-style search: spin on the spot to sweep the sonar round the ring,
// then a short forward hop to a new spot, repeat.
constexpr uint8_t  SEARCH_TURN_SPEED = 140;  // 140 is reliable for ultrasonic echo acquisition
constexpr uint8_t  SEARCH_FWD_SPEED  = 200;  // forward hop between spins
constexpr uint32_t SEARCH_SPIN_MS    = 1000; // tuned for full turn
// Don't spin again if we already did within this time, and never spin right
// after an edge escape (the escape already turned us). Stops repeated spins.
constexpr uint32_t SEARCH_RESPIN_MS  = 2500;
constexpr uint32_t SEARCH_ADVANCE_MS = 350;
// true  = spin once, then drive a continuous curve (smooth, no stop-start).
// false = keep alternating spin SEARCH_SPIN_MS / forward SEARCH_ADVANCE_MS.
constexpr bool     SEARCH_ARC        = false;
constexpr uint8_t  SEARCH_ARC_OUTER  = 200;  // outer wheel speed while curving
constexpr uint8_t  SEARCH_ARC_INNER  = 40;   // inner wheel (bigger = wider curve)

// --- Edge escape ------------------------------------------------------
constexpr uint8_t  ESCAPE_REVERSE_SPEED = 220;
constexpr uint8_t  ESCAPE_TURN_SPEED    = 210;
constexpr uint8_t  ESCAPE_FORWARD_SPEED = 200;
constexpr uint16_t ESCAPE_REVERSE_MS    = 220;
constexpr uint16_t ESCAPE_TURN_MS       = 200;  // ~90-120 deg away from edge
constexpr uint16_t ESCAPE_TURN_180_MS   = 350;  // both front sensors on edge
constexpr uint16_t ESCAPE_FORWARD_MS    = 150;  // forward hop into ring away from border

// --- Display ----------------------------------------------------------
constexpr bool     ENABLE_DISPLAY     = true;
constexpr uint32_t DISPLAY_REFRESH_MS = 150;  // live sensor view refresh period
// Keep the sonar/edge boxes updating during the match (shows where the
// opponent is detected - required by the brief). Only boxes that changed are
// redrawn, so the cost to the control loop is small.
constexpr bool     DISPLAY_LIVE_IN_MATCH = true;

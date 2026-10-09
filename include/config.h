#pragma once
#include <Arduino.h>

// =====================================================================
//  Sumobot settings. All speeds are 0..255 PWM.
// =====================================================================

// --- Motor direction (flip if a wheel runs backwards in motor_test) ---
constexpr bool LEFT_MOTOR_INVERTED  = true;   // left wheel forward = MDD3A M1B
constexpr bool RIGHT_MOTOR_INVERTED = false;

// --- Motor PWM (MDD3A supports up to 20 kHz; 20 kHz is silent) --------
constexpr uint32_t MOTOR_PWM_FREQ_HZ = 20000;
constexpr uint8_t  MOTOR_PWM_BITS    = 8;

// --- Motor power ------------------------------------------------------
// Every non-zero speed is scaled into MIN..LIMIT. Lower LIMIT (e.g. 160)
// to slow everything down for testing. For a weak battery: 110 / 70 / 400.
constexpr uint8_t  MOTOR_SPEED_LIMIT = 255;  // fastest any motor runs (255 = full power)
constexpr uint8_t  MOTOR_SPEED_MIN   = 0;    // slowest non-zero speed (0 = no minimum)
constexpr uint16_t MOTOR_RAMP_MS     = 0;    // ramp time 0 -> full speed (0 = instant; try 50 if it browns out)

// --- IR edge sensors (Maker Reflect) ----------------------------------
// Pin level that means "on the border". Practice mat = white ring, black
// border, and the sensors read HIGH on black -> HIGH. Black ring with a
// white line -> LOW.
constexpr uint8_t LINE_ACTIVE_LEVEL = HIGH;
constexpr bool USE_REAR_EDGE_SENSORS = true;  // false = ignore both rear sensors
constexpr bool USE_REAR_RIGHT_SENSOR = true;  // false = ignore just rear-right
// Border must be seen continuously this long to count (filters motor noise).
constexpr uint32_t EDGE_CONFIRM_MS = 10;

// --- Ultrasonic -------------------------------------------------------
// Ring is 100 cm: robots start ~50-60 cm apart and spectators stay ~90 cm
// back, so 70 cm sees the opponent without seeing people. Use 25 / 15 when
// testing with people near the ring.
constexpr uint16_t SONAR_MAX_CM      = 70;   // middle sonar range
constexpr uint16_t SIDE_MAX_CM       = 35;   // side sonar range
constexpr bool     USE_SIDE_SONARS   = true; // false = ignore left/right sonars
constexpr bool     USE_MID_SONAR     = true; // false = ignore middle (sides then attack alone)
constexpr uint16_t SONAR_INTERVAL_MS = 30;   // ping period (>= ~25 ms to avoid ghost echoes)
// Each reading is the median of the last 3, then:
constexpr uint8_t  SONAR_HITS_TO_SEE    = 3;  // readings in a row before a target counts
constexpr uint8_t  SONAR_MISSES_TO_LOSE = 4;  // misses in a row before it is dropped
constexpr uint16_t SONAR_HYST_CM        = 10; // a tracked target is kept this far past the range

// --- Match ------------------------------------------------------------
// Power on -> START_DELAY_MS -> fight until power off (no button).
// 0 = start immediately (also after a reset). 5000 = 5 s countdown.
constexpr uint32_t START_DELAY_MS   = 0;
constexpr int8_t   START_SEARCH_DIR = +1;    // first search spin: +1 right, -1 left

// --- Test modes (both false for normal use) ---------------------------
// TEST_ATTACK_ONLY: no edge, no search - waits for a sonar target, then
//   attacks. Will drive off the ring, so test on the floor.
// TEST_EDGE_ONLY: drives forward at EDGE_TEST_SPEED, edge escapes only.
constexpr bool     TEST_ATTACK_ONLY = false;
constexpr bool     TEST_EDGE_ONLY   = false;
constexpr uint8_t  EDGE_TEST_SPEED  = 140;

// --- Attack -----------------------------------------------------------
constexpr uint8_t  ATTACK_SPEED     = 255;   // target closer than CLOSE_CM
constexpr uint8_t  APPROACH_SPEED   = 255;   // target further away (full-power charge)
constexpr uint8_t  TRACK_TURN_SPEED = 220;   // outer wheel when turning toward a side target
constexpr int16_t  TRACK_INNER_SPEED = 0;    // inner wheel: 0 = swing forward, negative = spin in place
constexpr float    STEER_RATIO      = 0.6f;  // inner/outer wheel ratio when target is mid + one side
constexpr uint16_t CLOSE_CM         = 25;    // closer than this -> ATTACK_SPEED
constexpr uint16_t CONTACT_CM       = 10;    // closer than this = touching the opponent
constexpr uint32_t CONTACT_HOLD_MS  = 350;   // keep pushing this long after the sonar drops out at contact
// In contact and our front reaches the border: the opponent is already
// over it, so keep pushing this long before escaping (0 = escape at once).
constexpr uint32_t EDGE_PUSH_THROUGH_MS = 150;
// false = a side sonar alone can't start an attack (avoids chasing noise);
// it only turns the search toward that side. Ignored if USE_MID_SONAR = false.
constexpr bool     SIDE_ONLY_ATTACK     = false;

// --- Anti-jerk --------------------------------------------------------
constexpr uint32_t TARGET_LOST_HOLD_MS  = 400;   // keep the last attack move if the target drops out briefly
constexpr uint32_t ESCAPE_COOLDOWN_MS   = 500;   // after an escape, only attack targets closer than CLOSE_CM
// Turning toward a side target this long without the middle sonar confirming
// it = false reading: ignore the side sonars for SIDE_IGNORE_MS.
constexpr uint32_t TRACK_TIMEOUT_MS     = 1500;
constexpr uint32_t SIDE_IGNORE_MS       = 2000;

// --- Search -----------------------------------------------------------
// Spin on the spot to sweep the sonar round the ring, then hop forward,
// repeat. Spin slowly enough for the sonar (~0.1 s to confirm, ~30 deg beam).
constexpr uint8_t  SEARCH_TURN_SPEED = 150;  // lower if it spins past targets without seeing them
constexpr uint8_t  SEARCH_FWD_SPEED  = 200;  // forward hop speed
constexpr uint32_t SEARCH_SPIN_MS    = 1200; // tune to about one full turn
constexpr uint32_t SEARCH_ADVANCE_MS = 350;  // forward hop time
// No new spin within this time, and none right after an edge escape.
constexpr uint32_t SEARCH_RESPIN_MS  = 3000;
// true = spin once, then drive a continuous curve instead of spin + hop.
constexpr bool     SEARCH_ARC        = false;
constexpr uint8_t  SEARCH_ARC_OUTER  = 200;  // outer wheel speed while curving
constexpr uint8_t  SEARCH_ARC_INNER  = 40;   // inner wheel (bigger = wider curve)

// --- Edge escape ------------------------------------------------------
constexpr uint8_t  ESCAPE_REVERSE_SPEED = 220;
constexpr uint8_t  ESCAPE_TURN_SPEED    = 210;
constexpr uint8_t  ESCAPE_FORWARD_SPEED = 230;
constexpr uint16_t ESCAPE_REVERSE_MS    = 250;
constexpr uint16_t ESCAPE_TURN_MS       = 220;  // turn away from the edge
constexpr uint16_t ESCAPE_TURN_180_MS   = 400;  // both front sensors on the edge
constexpr uint16_t ESCAPE_FORWARD_MS    = 300;  // rear sensor on the edge (being pushed)

// --- Display ----------------------------------------------------------
constexpr uint8_t  GROUP_NUMBER       = 5;    // shown top-centre (required by the brief)
constexpr bool     ENABLE_DISPLAY     = true;
constexpr uint32_t DISPLAY_REFRESH_MS = 150;  // sensor box refresh period
// Keep the sensor boxes live during the match (shows where the opponent is
// detected). Only changed boxes are redrawn, so it barely slows the loop.
constexpr bool     DISPLAY_LIVE_IN_MATCH = true;

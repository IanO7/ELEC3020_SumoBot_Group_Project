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
// With a proper battery (2x18650 / 2S LiPo / 6xAA) set 255 and 0.
constexpr uint8_t  MOTOR_SPEED_LIMIT = 140;  // every speed is scaled to at most this (255 = no limit)
constexpr uint16_t MOTOR_RAMP_MS     = 200;  // time to ramp 0 -> full speed (0 = instant)

// --- IR edge sensors --------------------------------------------------
// Level a sensor outputs when it sees the WHITE border.
// TCRT5000 modules pull LOW on white -> LOW.
constexpr uint8_t LINE_ACTIVE_LEVEL = LOW;

// --- Ultrasonic -------------------------------------------------------
constexpr uint16_t SONAR_MAX_CM      = 70;   // ignore anything beyond this (ring diameter-ish)
constexpr uint16_t SONAR_INTERVAL_MS = 30;   // ping period (>= ~25 ms to avoid ghost echoes)
constexpr uint8_t  SONAR_HITS_TO_SEE  = 2;   // consecutive hits before a target counts
constexpr uint8_t  SONAR_MISSES_TO_LOSE = 3; // consecutive misses before it is dropped

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
constexpr uint32_t DISPLAY_REFRESH_MS = 150;  // live sensor view (only while not fighting)

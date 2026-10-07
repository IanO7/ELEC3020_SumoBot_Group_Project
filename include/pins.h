#pragma once
#include <Arduino.h>

// =====================================================================
//  LILYGO T-Display-S3 pin map for the sumobot
// =====================================================================
//  Reserved by the board - DO NOT wire anything to these:
//    5-9, 38-42, 45-48  LCD (8-bit parallel)
//    15                 LCD power enable (must be HIGH on battery)
//    4                  Battery voltage ADC (via on-board divider)
//    19, 20             USB D-/D+
//    0, 14              On-board buttons (BOOT / KEY)
//  Free header GPIOs: 1, 2, 3, 10, 11, 12, 13, 16, 17, 18, 21, 43, 44
//  Final wiring uses all of them except 21.
// =====================================================================

// --- Ultrasonic (HC-SR04 style), one shared trigger -------------------
// NOTE: HC-SR04 ECHO is 5V - use a divider (e.g. 1k/2k) to 3.3V.
constexpr uint8_t PIN_US_TRIG       = 2;
constexpr uint8_t PIN_US_ECHO_LEFT  = 10;
constexpr uint8_t PIN_US_ECHO_MID   = 11;
constexpr uint8_t PIN_US_ECHO_RIGHT = 13;

// --- IR line / edge sensors ("top" = front of the robot) --------------
constexpr uint8_t PIN_IR_FRONT_LEFT  = 1;   // IR top left
constexpr uint8_t PIN_IR_FRONT_RIGHT = 12;  // IR top right
constexpr uint8_t PIN_IR_REAR_RIGHT  = 16;  // IR bottom right
constexpr uint8_t PIN_IR_REAR_LEFT   = 43;  // IR bottom left (U0TXD - may glitch during boot)

// --- Motor driver: 2 inputs per motor, PWM on the inputs --------------
// Works with DRV8833, MX1508/TB67H450, or L298N with ENA/ENB jumpers ON.
// (TB6612: tie PWMA/PWMB and STBY to 3.3V, use AIN/BIN as below.)
// MDD3A button test (7 Oct):
//   LEFT wheel  = M1: M1A = backward, M1B = forward -> LEFT_MOTOR_INVERTED = true
//   RIGHT wheel = M2: M2A = forward,  M2B = backward
// If a wheel spins the wrong way in motor_test, flip *_MOTOR_INVERTED in
// config.h (don't swap pins).
constexpr uint8_t PIN_MOTOR_R_IN1 = 44;  // M2A (idles HIGH at boot: brief twitch)
constexpr uint8_t PIN_MOTOR_R_IN2 = 3;   // M2B (strapping pin, fine as output)
constexpr uint8_t PIN_MOTOR_L_IN1 = 17;  // M1A
constexpr uint8_t PIN_MOTOR_L_IN2 = 18;  // M1B

// --- Start / stop -----------------------------------------------------
constexpr uint8_t PIN_BTN_START    = 14;  // on-board KEY button (active LOW)
constexpr uint8_t PIN_START_MODULE = 21;  // optional IR start module signal (HIGH = go) - only free GPIO left

// --- Board ------------------------------------------------------------
constexpr uint8_t PIN_LCD_POWER = 15;
constexpr uint8_t PIN_BATTERY   = 4;

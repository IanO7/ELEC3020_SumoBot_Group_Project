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
//  All are used except 21 (spare).
// =====================================================================

// --- Ultrasonic (HC-SR04), one shared trigger -------------------------
// ECHO is 5V - use a divider (e.g. 1k/2k) to 3.3V.
constexpr uint8_t PIN_US_TRIG       = 2;
constexpr uint8_t PIN_US_ECHO_LEFT  = 10;
constexpr uint8_t PIN_US_ECHO_MID   = 11;
constexpr uint8_t PIN_US_ECHO_RIGHT = 13;

// --- IR edge sensors (Maker Reflect) ----------------------------------
constexpr uint8_t PIN_IR_FRONT_LEFT  = 1;
constexpr uint8_t PIN_IR_FRONT_RIGHT = 12;
constexpr uint8_t PIN_IR_REAR_RIGHT  = 16;
constexpr uint8_t PIN_IR_REAR_LEFT   = 43;  // U0TXD - may glitch briefly at boot

// --- Motor driver: Cytron MDD3A, PWM on both inputs of each motor -----
//   LEFT wheel  = M1: M1A = backward, M1B = forward (LEFT_MOTOR_INVERTED)
//   RIGHT wheel = M2: M2A = forward,  M2B = backward
// Wrong direction in motor_test -> flip *_MOTOR_INVERTED, don't swap pins.
constexpr uint8_t PIN_MOTOR_R_IN1 = 44;  // M2A - HIGH during boot/upload: right wheel may twitch
constexpr uint8_t PIN_MOTOR_R_IN2 = 3;   // M2B - strapping pin, fine as output
constexpr uint8_t PIN_MOTOR_L_IN1 = 17;  // M1A
constexpr uint8_t PIN_MOTOR_L_IN2 = 18;  // M1B

// --- Buttons ----------------------------------------------------------
constexpr uint8_t PIN_BTN_START = 14;  // on-board KEY (active LOW) - test programs only

// --- Board ------------------------------------------------------------
constexpr uint8_t PIN_LCD_POWER = 15;
constexpr uint8_t PIN_BATTERY   = 4;

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
// =====================================================================

// --- Ultrasonic (HC-SR04 style), one shared trigger -------------------
// NOTE: HC-SR04 ECHO is 5V - use a divider (e.g. 1k/2k) to 3.3V.
constexpr uint8_t PIN_US_TRIG       = 1;
constexpr uint8_t PIN_US_ECHO_LEFT  = 44;
constexpr uint8_t PIN_US_ECHO_MID   = 43;
constexpr uint8_t PIN_US_ECHO_RIGHT = 18;

// --- IR line / edge sensors ("top" = front of the robot) --------------
constexpr uint8_t PIN_IR_FRONT_LEFT  = 16;  // was irTopLeft
constexpr uint8_t PIN_IR_FRONT_RIGHT = 21;  // was irTopRight
constexpr uint8_t PIN_IR_REAR_RIGHT  = 17;  // was irBottomRight
constexpr uint8_t PIN_IR_REAR_LEFT   = 2;   // was irBottomLeft

// --- Motor driver: 2 inputs per motor, PWM on the inputs --------------
// Works with DRV8833, MX1508/TB67H450, or L298N with ENA/ENB jumpers ON.
// (TB6612: tie PWMA/PWMB and STBY to 3.3V, use AIN/BIN as below.)
constexpr uint8_t PIN_MOTOR_L_IN1 = 10;
constexpr uint8_t PIN_MOTOR_L_IN2 = 11;
constexpr uint8_t PIN_MOTOR_R_IN1 = 12;
constexpr uint8_t PIN_MOTOR_R_IN2 = 13;

// --- Start / stop -----------------------------------------------------
constexpr uint8_t PIN_BTN_START    = 14;  // on-board KEY button (active LOW)
constexpr uint8_t PIN_START_MODULE = 3;   // optional IR start module signal (HIGH = go)

// --- Board ------------------------------------------------------------
constexpr uint8_t PIN_LCD_POWER = 15;
constexpr uint8_t PIN_BATTERY   = 4;

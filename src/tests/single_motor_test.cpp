// --- SINGLE MOTOR TEST (left or right) ---
// Build/upload with:  pio run -e left_motor_test -t upload
//                 or: pio run -e right_motor_test -t upload
// Runs on battery (no USB needed): the current step is shown on screen.
// Wheels off the ground. Press KEY (GPIO14) to start. Drives ONE motor
// forward then backward at rising speeds. Forward = PWM on IN1, backward =
// PWM on IN2, so a direction that does nothing means that pin's wire/input
// is dead. Speeds are before MOTOR_SPEED_LIMIT (config.h); "actual" is after.
#include <Arduino.h>
#include <TFT_eSPI.h>

#include "config.h"
#include "motors.h"
#include "pins.h"

#ifdef TEST_RIGHT_MOTOR
static const char *SIDE = "RIGHT";
static const uint8_t IN1 = PIN_MOTOR_R_IN1, IN2 = PIN_MOTOR_R_IN2;
static const bool INVERTED = RIGHT_MOTOR_INVERTED;
static void driveOne(int s) { Motors::drive(0, s); }
#else
static const char *SIDE = "LEFT";
static const uint8_t IN1 = PIN_MOTOR_L_IN1, IN2 = PIN_MOTOR_L_IN2;
static const bool INVERTED = LEFT_MOTOR_INVERTED;
static void driveOne(int s) { Motors::drive(s, 0); }
#endif

static TFT_eSPI tft;
static const int speeds[] = {80, 120, 160, 200, 255};

static void show(const char *line1, const char *line2, const char *line3, uint16_t color) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(color, TFT_BLACK);
  tft.drawString(line1, 4, 10, 4);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString(line2, 4, 60, 4);
  tft.drawString(line3, 4, 110, 4);
  Serial.printf("%s | %s | %s\n", line1, line2, line3);
}

void setup() {
  Motors::begin();
  Motors::coast();
  Serial.begin(115200);
  Serial.setTxTimeoutMs(0);
  pinMode(PIN_BTN_START, INPUT_PULLUP);

  pinMode(PIN_LCD_POWER, OUTPUT);
  digitalWrite(PIN_LCD_POWER, HIGH);
  tft.init();
  tft.setRotation(1);
  char a[32], c[32];
  snprintf(a, sizeof a, "%s MOTOR TEST", SIDE);
  snprintf(c, sizeof c, "IN1 GPIO%d  IN2 GPIO%d", IN1, IN2);
  show(a, "Press KEY", c, TFT_GREEN);
}

void loop() {
  if (digitalRead(PIN_BTN_START) == HIGH) return;
  delay(500);

  char a[32], b[32], c[32];
  for (int dir = 1; dir >= -1; dir -= 2) {
    for (int s : speeds) {
      int actual = MOTOR_SPEED_MIN + (s - 1) * (MOTOR_SPEED_LIMIT - MOTOR_SPEED_MIN) / 254;
      snprintf(a, sizeof a, "%s %s", SIDE, dir > 0 ? "FORWARD" : "BACKWARD");
      snprintf(b, sizeof b, "speed %d (actual %d)", s, actual);
      // Forward PWMs IN1, backward IN2 - swapped if *_MOTOR_INVERTED.
      bool useIn1 = (dir > 0) != INVERTED;
      snprintf(c, sizeof c, "PWM on GPIO%d", useIn1 ? IN1 : IN2);
      show(a, b, c, dir > 0 ? TFT_CYAN : TFT_ORANGE);
      driveOne(dir * s);  // the other motor stays off
      delay(2000);
      Motors::coast();
      delay(700);
    }
  }
  show("DONE", "Press KEY to repeat", "", TFT_GREEN);
}

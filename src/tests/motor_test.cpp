// --- MOTOR WIRING TEST ---
// Build/upload with:  pio run -e motor_test -t upload
// Put the robot on a stand (wheels off the ground), open the serial
// monitor, press KEY (GPIO14). Each step says what SHOULD happen; if a
// wheel spins backwards, flip LEFT/RIGHT_MOTOR_INVERTED in config.h.
// If the wrong wheel moves, swap the motor connectors (or the pins).
#include <Arduino.h>

#include "motors.h"
#include "pins.h"

struct TestStep {
  const char *expect;
  int left, right;
};

static const TestStep steps[] = {
    {"LEFT wheel FORWARD", 180, 0},
    {"LEFT wheel BACKWARD", -180, 0},
    {"RIGHT wheel FORWARD", 0, 180},
    {"RIGHT wheel BACKWARD", 0, -180},
    {"BOTH FORWARD (robot drives forward)", 180, 180},
    {"SPIN RIGHT / clockwise from above", 180, -180},
};

void setup() {
  Motors::begin();
  Serial.begin(115200);
  pinMode(PIN_BTN_START, INPUT_PULLUP);
  delay(1000);
  Serial.println("\n--- Motor test: press KEY (GPIO14) to run ---");
}

void loop() {
  if (digitalRead(PIN_BTN_START) == HIGH) return;
  delay(500);

  for (const TestStep &s : steps) {
    Serial.printf("Expect: %s\n", s.expect);
    Motors::drive(s.left, s.right);
    delay(1500);
    Motors::brake();
    delay(700);
  }
  Motors::coast();
  Serial.println("Done. Press KEY to run again.");
}

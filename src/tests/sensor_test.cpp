// --- SENSOR FUNCTIONALITY TEST CODE ---
// Build/upload with:  pio run -e sensor_test -t upload
// Uses the same Sonar/Line drivers as the sumobot, so what you see here
// is exactly what the robot sees.
#include <Arduino.h>

#include "motors.h"
#include "pins.h"
#include "sensors.h"

static void printDistance(const char *name, uint8_t pin, Sonar::Side s) {
  Serial.printf("  [%-6s (Pin %d)]: ", name, pin);
  int16_t raw = Sonar::rawCm(s);
  if (raw < 0) Serial.print("  --  ");
  else Serial.printf("%4d cm", raw);
  Serial.println(Sonar::sees(s) ? "   <- TARGET" : "");
}

void setup() {
  // Hold the motors off: GPIO44 (right motor IN1) idles HIGH as UART0 RX,
  // which would otherwise spin the right wheel.
  Motors::begin();
  Motors::coast();

  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- Starting Sensor Diagnostics ---");
  Sonar::begin();
  Line::begin();
}

void loop() {
  Sonar::update();  // must run continuously; it is non-blocking

  static uint32_t lastPrint = 0;
  if (millis() - lastPrint < 500) return;
  lastPrint = millis();

  LineState line = Line::read();

  Serial.println("========================================");
  Serial.println("ULTRASONIC SENSORS (blank = nothing within SONAR_MAX_CM):");
  printDistance("Left", PIN_US_ECHO_LEFT, Sonar::LEFT);
  printDistance("Middle", PIN_US_ECHO_MID, Sonar::MID);
  printDistance("Right", PIN_US_ECHO_RIGHT, Sonar::RIGHT);

  Serial.println("\nIR EDGE SENSORS (1 = sees WHITE border):");
  Serial.printf("  Front-Left  (Pin %d): %d   raw %d\n", PIN_IR_FRONT_LEFT, line.frontLeft, digitalRead(PIN_IR_FRONT_LEFT));
  Serial.printf("  Front-Right (Pin %d): %d   raw %d\n", PIN_IR_FRONT_RIGHT, line.frontRight, digitalRead(PIN_IR_FRONT_RIGHT));
  Serial.printf("  Rear-Right  (Pin %d): %d   raw %d\n", PIN_IR_REAR_RIGHT, line.rearRight, digitalRead(PIN_IR_REAR_RIGHT));
  Serial.printf("  Rear-Left   (Pin %d):  %d   raw %d\n", PIN_IR_REAR_LEFT, line.rearLeft, digitalRead(PIN_IR_REAR_LEFT));
}

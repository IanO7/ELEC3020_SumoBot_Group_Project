#pragma once
#include <Arduino.h>

// ---------------------------------------------------------------------
//  Ultrasonic: one shared trigger, all three echoes timed in parallel
//  by interrupts, so update() never blocks (unlike pulseIn).
// ---------------------------------------------------------------------
namespace Sonar {

enum Side : uint8_t { LEFT = 0, MID = 1, RIGHT = 2, COUNT = 3 };

void begin();
void update();             // call every loop; fires pings and collects echoes
bool sees(Side s);         // filtered: target present within SONAR_MAX_CM
int16_t distanceCm(Side s);  // last good distance while seen, else -1
int16_t rawCm(Side s);     // latest unfiltered reading, -1 = nothing in range

}  // namespace Sonar

// ---------------------------------------------------------------------
//  IR edge sensors: true = that sensor is over the white border.
// ---------------------------------------------------------------------
struct LineState {
  bool frontLeft, frontRight, rearLeft, rearRight;
  bool front() const { return frontLeft || frontRight; }
  bool rear() const { return rearLeft || rearRight; }
  bool any() const { return front() || rear(); }
};

namespace Line {
void begin();
LineState read();
}  // namespace Line

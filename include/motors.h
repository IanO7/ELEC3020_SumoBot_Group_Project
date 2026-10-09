#pragma once
#include <Arduino.h>

class Motor {
 public:
  Motor(uint8_t in1, uint8_t in2, uint8_t ch1, uint8_t ch2, bool inverted);
  void begin();
  void set(int speed);  // -255 (full reverse) .. 255 (full forward)
  void brake();         // both inputs high: short brake
  void coast();         // both inputs low: free-wheel

 private:
  void write(uint8_t pin, uint8_t ch, uint32_t duty);
  uint8_t in1_, in2_, ch1_, ch2_;
  bool inverted_;
  float current_ = 0;     // speed actually applied (after ramping)
  uint32_t lastMs_ = 0;
};

namespace Motors {
void begin();
void drive(int left, int right);  // each -255..255
int lastLeft();                   // last commanded speeds (0 after brake/coast)
int lastRight();
void brake();
void coast();
}  // namespace Motors

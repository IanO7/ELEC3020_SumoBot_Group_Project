#include "motors.h"

#include "config.h"
#include "pins.h"

static constexpr uint32_t PWM_MAX = (1u << MOTOR_PWM_BITS) - 1;

Motor::Motor(uint8_t in1, uint8_t in2, uint8_t ch1, uint8_t ch2, bool inverted)
    : in1_(in1), in2_(in2), ch1_(ch1), ch2_(ch2), inverted_(inverted) {}

void Motor::begin() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttachChannel(in1_, MOTOR_PWM_FREQ_HZ, MOTOR_PWM_BITS, ch1_);
  ledcAttachChannel(in2_, MOTOR_PWM_FREQ_HZ, MOTOR_PWM_BITS, ch2_);
#else
  ledcSetup(ch1_, MOTOR_PWM_FREQ_HZ, MOTOR_PWM_BITS);
  ledcSetup(ch2_, MOTOR_PWM_FREQ_HZ, MOTOR_PWM_BITS);
  ledcAttachPin(in1_, ch1_);
  ledcAttachPin(in2_, ch2_);
#endif
  coast();
}

void Motor::write(uint8_t pin, uint8_t ch, uint32_t duty) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  (void)ch;
  ledcWrite(pin, duty);
#else
  (void)pin;
  ledcWrite(ch, duty);
#endif
}

void Motor::set(int speed) {
  speed = constrain(speed, -255, 255);
  speed = speed * MOTOR_SPEED_LIMIT / 255;

  // Ramp toward the target so the motors never get a sudden current surge.
  uint32_t now = millis();
  if (MOTOR_RAMP_MS > 0) {
    float maxStep = 255.0f * (now - lastMs_) / MOTOR_RAMP_MS;
    current_ += constrain(speed - current_, -maxStep, maxStep);
  } else {
    current_ = speed;
  }
  lastMs_ = now;
  speed = (int)current_;

  if (inverted_) speed = -speed;
  uint32_t duty = (uint32_t)abs(speed) * PWM_MAX / 255;

  // Fast-decay drive: PWM one input, hold the other low.
  if (speed > 0) {
    write(in2_, ch2_, 0);
    write(in1_, ch1_, duty);
  } else if (speed < 0) {
    write(in1_, ch1_, 0);
    write(in2_, ch2_, duty);
  } else {
    coast();
  }
}

void Motor::brake() {
  current_ = 0;
  lastMs_ = millis();
  write(in1_, ch1_, PWM_MAX);
  write(in2_, ch2_, PWM_MAX);
}

void Motor::coast() {
  current_ = 0;
  lastMs_ = millis();
  write(in1_, ch1_, 0);
  write(in2_, ch2_, 0);
}

// ---------------------------------------------------------------------

static Motor leftMotor(PIN_MOTOR_L_IN1, PIN_MOTOR_L_IN2, 0, 1, LEFT_MOTOR_INVERTED);
static Motor rightMotor(PIN_MOTOR_R_IN1, PIN_MOTOR_R_IN2, 2, 3, RIGHT_MOTOR_INVERTED);

namespace Motors {

void begin() {
  leftMotor.begin();
  rightMotor.begin();
}

void drive(int left, int right) {
  leftMotor.set(left);
  rightMotor.set(right);
}

void brake() {
  leftMotor.brake();
  rightMotor.brake();
}

void coast() {
  leftMotor.coast();
  rightMotor.coast();
}

}  // namespace Motors

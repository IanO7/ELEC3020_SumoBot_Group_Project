#include "sensors.h"

#include <driver/gpio.h>

#include "config.h"
#include "pins.h"

namespace Sonar {
namespace {

struct Echo {
  uint8_t pin;
  // Written by the ISR
  volatile uint32_t riseUs;
  volatile uint32_t widthUs;
  volatile bool high;
  volatile bool done;
  // Owned by update()
  bool resolved;
  int16_t rawCm;
  int16_t distCm;
  uint8_t hits;
  uint8_t misses;
  bool seen;
};

Echo echoes[COUNT] = {
    {PIN_US_ECHO_LEFT}, {PIN_US_ECHO_MID}, {PIN_US_ECHO_RIGHT}};

uint32_t pingUs = 0;
uint32_t lastPingMs = 0;
bool waiting = false;

// Echo goes high ~0.5-2 ms after the trigger, then stays high for the round trip.
constexpr uint32_t ECHO_TIMEOUT_US = (uint32_t)SONAR_MAX_CM * 58 + 2500;

void IRAM_ATTR echoIsr(void *arg) {
  Echo *e = static_cast<Echo *>(arg);
  uint32_t t = micros();
  if (gpio_get_level((gpio_num_t)e->pin)) {
    e->riseUs = t;
    e->high = true;
  } else if (e->high) {
    e->widthUs = t - e->riseUs;
    e->high = false;
    e->done = true;
  }
}

void record(Echo &e, int16_t cm) {
  e.rawCm = cm;
  if (cm > 0) {
    e.misses = 0;
    if (e.hits < 255) e.hits++;
    if (e.hits >= SONAR_HITS_TO_SEE) {
      e.seen = true;
      e.distCm = cm;
    }
  } else {
    e.hits = 0;
    if (e.misses < 255) e.misses++;
    if (e.misses >= SONAR_MISSES_TO_LOSE) {
      e.seen = false;
      e.distCm = -1;
    }
  }
}

void ping() {
  noInterrupts();
  for (Echo &e : echoes) {
    e.high = false;
    e.done = false;
    e.resolved = false;
  }
  interrupts();

  digitalWrite(PIN_US_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_US_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_US_TRIG, LOW);

  pingUs = micros();
  lastPingMs = millis();
  waiting = true;
}

}  // namespace

void begin() {
  pinMode(PIN_US_TRIG, OUTPUT);
  digitalWrite(PIN_US_TRIG, LOW);
  for (Echo &e : echoes) {
    e.rawCm = -1;
    e.distCm = -1;
    pinMode(e.pin, INPUT);
    attachInterruptArg(digitalPinToInterrupt(e.pin), echoIsr, &e, CHANGE);
  }
}

void update() {
  if (waiting) {
    bool allResolved = true;
    uint32_t elapsed = micros() - pingUs;
    for (Echo &e : echoes) {
      if (e.resolved) continue;
      if (e.done) {
        int16_t cm = e.widthUs / 58;
        record(e, (cm >= 1 && cm <= SONAR_MAX_CM) ? cm : -1);
        e.resolved = true;
      } else if (elapsed > ECHO_TIMEOUT_US) {
        record(e, -1);  // nothing within range (sensor may still be high)
        e.resolved = true;
      } else {
        allResolved = false;
      }
    }
    if (allResolved) waiting = false;
  }

  // A sensor still busy from a long/no echo just ignores the trigger and
  // times out as "nothing", which is the right answer anyway.
  if (!waiting && millis() - lastPingMs >= SONAR_INTERVAL_MS) ping();
}

bool sees(Side s) { return echoes[s].seen; }
int16_t distanceCm(Side s) { return echoes[s].distCm; }
int16_t rawCm(Side s) { return echoes[s].rawCm; }

}  // namespace Sonar

// ---------------------------------------------------------------------

namespace Line {

void begin() {
  pinMode(PIN_IR_FRONT_LEFT, INPUT_PULLUP);
  pinMode(PIN_IR_FRONT_RIGHT, INPUT_PULLUP);
  pinMode(PIN_IR_REAR_LEFT, INPUT_PULLUP);
  pinMode(PIN_IR_REAR_RIGHT, INPUT_PULLUP);
}

LineState read() {
  LineState s;
  s.frontLeft = digitalRead(PIN_IR_FRONT_LEFT) == LINE_ACTIVE_LEVEL;
  s.frontRight = digitalRead(PIN_IR_FRONT_RIGHT) == LINE_ACTIVE_LEVEL;
  s.rearLeft = digitalRead(PIN_IR_REAR_LEFT) == LINE_ACTIVE_LEVEL;
  s.rearRight = digitalRead(PIN_IR_REAR_RIGHT) == LINE_ACTIVE_LEVEL;
  return s;
}

}  // namespace Line

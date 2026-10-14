#include "sensors.h"

#include <driver/gpio.h>

#include "config.h"
#include "motors.h"
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
  int16_t history[3];  // last 3 raw readings (-1 = none) for the median
  uint8_t histIdx;
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
// Always listen out to 150 cm (timeout ~11 ms, well inside the 30 ms ping
// period) - some HC-SR04 clones take 2-3 ms just to start the echo, so a
// short timeout drops close readings. SONAR_MAX_CM is applied in record().
constexpr uint16_t MEASURE_MAX_CM = 150;
constexpr uint32_t ECHO_TIMEOUT_US = (uint32_t)MEASURE_MAX_CM * 58 + 2500;

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

int16_t median3(int16_t a, int16_t b, int16_t c) {
  return max(min(a, b), min(max(a, b), c));
}

void record(Echo &e, int16_t raw, uint16_t maxCm) {
  e.rawCm = raw;
  e.history[e.histIdx] = raw > 0 ? raw : 999;  // "nothing" sorts as far away
  e.histIdx = (e.histIdx + 1) % 3;
  int16_t med = median3(e.history[0], e.history[1], e.history[2]);

  // Hysteresis: a target we already track may drift a bit past the range.
  uint16_t limit = e.seen ? maxCm + SONAR_HYST_CM : maxCm;
  int16_t cm = (med >= 1 && med <= (int16_t)limit) ? med : -1;
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
    e.history[0] = e.history[1] = e.history[2] = 999;
    e.histIdx = 0;
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
      uint16_t maxLimit = (&e == &echoes[MID]) ? SONAR_MAX_CM : SIDE_MAX_CM;
      if (e.done) {
        int16_t cm = e.widthUs / 58;
        record(e, (cm >= 1 && cm <= MEASURE_MAX_CM) ? cm : -1, maxLimit);
        e.resolved = true;
      } else if (elapsed > ECHO_TIMEOUT_US) {
        record(e, -1, maxLimit);  // nothing within range (sensor may still be high)
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

// Edge filter: the border counts only once the sensor has travelled
// EDGE_CONFIRM_MS (in "ms at full speed") over continuous black, or has
// read black for EDGE_CONFIRM_MAX_MS. Any white longer than EDGE_GAP_MS ends
// the run, so narrow scratches / marks are ignored whether the robot is
// charging, spinning slowly or just starting to move.
struct EdgeFilter {
  uint32_t blackSinceMs = 0;  // start of the current black run (0 = none)
  uint32_t lastBlackMs = 0;   // last time the sensor read black
  float travelled = 0;        // distance over black, in ms at full speed

  void reset() {
    blackSinceMs = lastBlackMs = 0;
    travelled = 0;
  }

  // dtFull = distance moved since the last call, in ms at full speed
  bool update(uint8_t pin, uint32_t now, float dtFull) {
    if (digitalRead(pin) == LINE_ACTIVE_LEVEL) {
      if (!blackSinceMs) {
        blackSinceMs = now ? now : 1;
        travelled = 0;
      } else {
        travelled += dtFull;
      }
      lastBlackMs = now;
    } else if (blackSinceMs && now - lastBlackMs > EDGE_GAP_MS) {
      blackSinceMs = 0;  // back on white: it was a mark, not the border
    }
    return blackSinceMs &&
           (travelled >= EDGE_CONFIRM_MS || now - blackSinceMs >= EDGE_CONFIRM_MAX_MS);
  }
};

static EdgeFilter debouncers[4];
static float speedEst = 0;      // estimated robot speed, 0..1 of full speed
static uint32_t lastReadMs = 0;

void begin() {
  pinMode(PIN_IR_FRONT_LEFT, INPUT_PULLUP);
  pinMode(PIN_IR_FRONT_RIGHT, INPUT_PULLUP);
  pinMode(PIN_IR_REAR_LEFT, INPUT_PULLUP);
  pinMode(PIN_IR_REAR_RIGHT, INPUT_PULLUP);
  for (auto &d : debouncers) d.reset();
}

void reset() {
  for (auto &d : debouncers) d.reset();
}

LineState read() {
  uint32_t now = millis();
  float dt = lastReadMs ? (float)(now - lastReadMs) : 0;
  lastReadMs = now;

  // Commanded speed (average wheel speed, either direction), then a simple
  // lag so the estimate takes ~MOTOR_SPINUP_MS to follow a sudden change.
  float target = (abs(Motors::lastLeft()) + abs(Motors::lastRight())) / (2 * 255.0f);
  speedEst += (target - speedEst) * min(1.0f, dt / MOTOR_SPINUP_MS);
  float dtFull = dt * speedEst;

  LineState s;
  s.frontLeft  = debouncers[0].update(PIN_IR_FRONT_LEFT, now, dtFull);
  s.frontRight = debouncers[1].update(PIN_IR_FRONT_RIGHT, now, dtFull);
  s.rearLeft   = USE_REAR_EDGE_SENSORS ? debouncers[2].update(PIN_IR_REAR_LEFT, now, dtFull) : false;
  s.rearRight  = (USE_REAR_EDGE_SENSORS && USE_REAR_RIGHT_SENSOR)
                     ? debouncers[3].update(PIN_IR_REAR_RIGHT, now, dtFull)
                     : false;
  return s;
}

}  // namespace Line

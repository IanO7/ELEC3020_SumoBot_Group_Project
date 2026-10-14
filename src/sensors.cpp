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

// Edge filter: each reading adds or removes "border evidence".
//   black -> + distance moved toward it (in ms at full speed) and + time
//   white -> - EDGE_WHITE_WEIGHT x time from both
// The border counts once the distance reaches EDGE_CONFIRM_MS, or the time
// reaches EDGE_CONFIRM_MAX_MS (stopped / being pushed). The real border is
// solid black and builds up fast; a narrow mark is crossed before it gets
// there; motor noise flickering black/white is mostly white so it never
// builds up. One reading never counts for more than MAX_SAMPLE_MS, so a
// stalled loop (display redraw) can't turn one black blip into a long run.
struct EdgeFilter {
  float dist = 0;  // evidence in ms of travel at full speed
  float time = 0;  // evidence in plain ms

  void reset() { dist = time = 0; }

  bool update(uint8_t pin, float dt, float speed) {
    if (digitalRead(pin) == LINE_ACTIVE_LEVEL) {
      dist = min(dist + dt * speed, 2.0f * EDGE_CONFIRM_MS);
      time = min(time + dt, 2.0f * EDGE_CONFIRM_MAX_MS);
    } else {
      dist = max(0.0f, dist - EDGE_WHITE_WEIGHT * dt);
      time = max(0.0f, time - EDGE_WHITE_WEIGHT * dt);
    }
    return dist >= EDGE_CONFIRM_MS || time >= EDGE_CONFIRM_MAX_MS;
  }
};

constexpr float MAX_SAMPLE_MS = 2.0f;

static EdgeFilter debouncers[4];
// Estimated robot speed forward / backward, 0..1 of full speed.
static float fwdEst = 0, backEst = 0;
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
  float elapsed = lastReadMs ? (float)(now - lastReadMs) : 0;
  lastReadMs = now;

  // Commanded forward speed (average of the wheels: spinning on the spot = 0),
  // then a simple lag so the estimate takes ~MOTOR_SPINUP_MS to follow a
  // sudden change. Front sensors only build edge evidence while driving
  // FORWARD (toward what they see), rear sensors only while reversing:
  // turning on the spot over a scuff can't carry us out of the ring.
  float fwd = (Motors::lastLeft() + Motors::lastRight()) / (2 * 255.0f);
  float k = min(1.0f, elapsed / MOTOR_SPINUP_MS);
  fwdEst += (max(fwd, 0.0f) - fwdEst) * k;
  backEst += (max(-fwd, 0.0f) - backEst) * k;
  float dt = min(elapsed, MAX_SAMPLE_MS);  // one reading = at most 2 ms of evidence

  LineState s;
  s.frontLeft  = debouncers[0].update(PIN_IR_FRONT_LEFT, dt, fwdEst);
  s.frontRight = debouncers[1].update(PIN_IR_FRONT_RIGHT, dt, fwdEst);
  s.rearLeft   = USE_REAR_EDGE_SENSORS ? debouncers[2].update(PIN_IR_REAR_LEFT, dt, backEst) : false;
  s.rearRight  = (USE_REAR_EDGE_SENSORS && USE_REAR_RIGHT_SENSOR)
                     ? debouncers[3].update(PIN_IR_REAR_RIGHT, dt, backEst)
                     : false;
  return s;
}

}  // namespace Line

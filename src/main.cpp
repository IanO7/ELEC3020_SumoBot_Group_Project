// =====================================================================
//  ELEC3020 Sumobot - LILYGO T-Display-S3
// ---------------------------------------------------------------------
//  Priority every loop (loop is non-blocking, runs at kHz rates):
//    1. EDGE   - an IR sensor sees the white border -> timed escape
//    2. ATTACK - a sonar sees the opponent -> steer at it and push
//    3. SEARCH - spin toward where it was last seen, then reposition
//
//  Start: press the KEY button (GPIO14) -> 5 s countdown -> fight.
//  Press KEY again at any time to stop. Press once more to re-arm.
// =====================================================================
#include <Arduino.h>
#include <TFT_eSPI.h>

#include "config.h"
#include "motors.h"
#include "pins.h"
#include "sensors.h"

// --------------------------------------------------------------------
//  State
// --------------------------------------------------------------------
enum class State : uint8_t { Idle, Countdown, Search, Attack, Escape, Stopped };

static const char *stateName(State s) {
  switch (s) {
    case State::Idle:      return "READY";
    case State::Countdown: return "COUNTDOWN";
    case State::Search:    return "SEARCH";
    case State::Attack:    return "ATTACK";
    case State::Escape:    return "EDGE!";
    case State::Stopped:   return "STOPPED";
  }
  return "?";
}

static State state = State::Idle;
static uint32_t stateSinceMs = 0;
static int8_t lastSeenDir = START_SEARCH_DIR;  // -1 left, +1 right
static uint32_t lastContactMs = 0;             // last time mid sonar saw it touching us
static uint32_t searchPhaseSinceMs = 0;
static bool searchSpinning = true;

static bool inMatch() {
  return state == State::Search || state == State::Attack || state == State::Escape;
}

static void setState(State s) {
  if (s == state) return;
  state = s;
  stateSinceMs = millis();
  if (s == State::Search) {
    searchSpinning = true;
    searchPhaseSinceMs = stateSinceMs;
  }
  Serial.printf("[%lu] -> %s\n", (unsigned long)stateSinceMs, stateName(s));
}

// --------------------------------------------------------------------
//  Escape manoeuvres: a short list of timed motor steps
// --------------------------------------------------------------------
struct Step {
  int16_t left, right;
  uint16_t ms;
  bool interruptible;  // may be cut short to attack a target straight ahead
};

static Step escapeSteps[3];
static uint8_t escapeCount = 0;
static uint8_t escapeIndex = 0;
static uint32_t escapeStepSinceMs = 0;

static void addStep(int16_t l, int16_t r, uint16_t ms, bool interruptible = false) {
  if (escapeCount < 3) escapeSteps[escapeCount++] = {l, r, ms, interruptible};
}

// Spin in place: dir +1 = clockwise (right), -1 = anticlockwise (left)
static void addSpin(int8_t dir, uint16_t ms, bool interruptible) {
  addStep(dir * ESCAPE_TURN_SPEED, -dir * ESCAPE_TURN_SPEED, ms, interruptible);
}

static void planEscape(const LineState &line) {
  escapeCount = 0;
  escapeIndex = 0;
  escapeStepSinceMs = millis();

  const bool leftSide = line.frontLeft || line.rearLeft;
  const bool rightSide = line.frontRight || line.rearRight;

  if (line.front() && line.rear()) {
    // Straddling the border sideways: turn away from it, then drive in.
    int8_t dir = (leftSide && !rightSide) ? +1 : (rightSide && !leftSide) ? -1 : lastSeenDir;
    addSpin(dir, ESCAPE_TURN_MS, false);
    addStep(ESCAPE_FORWARD_SPEED, ESCAPE_FORWARD_SPEED, ESCAPE_FORWARD_MS, true);
  } else if (line.front()) {
    addStep(-ESCAPE_REVERSE_SPEED, -ESCAPE_REVERSE_SPEED, ESCAPE_REVERSE_MS);
    if (line.frontLeft && line.frontRight) {
      addSpin(lastSeenDir, ESCAPE_TURN_180_MS, true);
    } else {
      addSpin(line.frontLeft ? +1 : -1, ESCAPE_TURN_MS, true);  // turn away from the edge
    }
  } else {
    // Rear on the border (we're being pushed): drive forward, angled away.
    int16_t fast = ESCAPE_FORWARD_SPEED;
    int16_t slow = ESCAPE_FORWARD_SPEED * 3 / 4;
    if (line.rearLeft && !line.rearRight) addStep(fast, slow, ESCAPE_FORWARD_MS, true);
    else if (line.rearRight && !line.rearLeft) addStep(slow, fast, ESCAPE_FORWARD_MS, true);
    else addStep(fast, fast, ESCAPE_FORWARD_MS, true);
  }

  setState(State::Escape);
}

// True if the current step is driving us further onto the border we see.
static bool escapeConflicts(const LineState &line) {
  const Step &s = escapeSteps[escapeIndex];
  bool forward = s.left > 0 && s.right > 0;
  bool reverse = s.left < 0 && s.right < 0;
  return (forward && line.front()) || (reverse && line.rear());
}

static void runEscape() {
  uint32_t now = millis();
  if (now - escapeStepSinceMs >= escapeSteps[escapeIndex].ms) {
    escapeIndex++;
    escapeStepSinceMs = now;
  }
  if (escapeIndex >= escapeCount) {
    setState(State::Search);
    return;
  }

  const Step &s = escapeSteps[escapeIndex];
  if (s.interruptible && Sonar::sees(Sonar::MID)) {
    setState(State::Attack);
    return;
  }
  Motors::drive(s.left, s.right);
}

// --------------------------------------------------------------------
//  Attack / search
// --------------------------------------------------------------------
static bool runAttack() {
  const bool l = Sonar::sees(Sonar::LEFT);
  const bool m = Sonar::sees(Sonar::MID);
  const bool r = Sonar::sees(Sonar::RIGHT);
  const uint32_t now = millis();

  if (m) {
    int16_t d = Sonar::distanceCm(Sonar::MID);
    if (d > 0 && d <= (int16_t)CONTACT_CM) lastContactMs = now;
    int16_t speed = (d > 0 && d <= (int16_t)CLOSE_CM) ? ATTACK_SPEED : APPROACH_SPEED;
    int16_t inner = speed * STEER_RATIO;

    if (l && !r) {
      lastSeenDir = -1;
      Motors::drive(inner, speed);
    } else if (r && !l) {
      lastSeenDir = +1;
      Motors::drive(speed, inner);
    } else {
      Motors::drive(speed, speed);
    }
  } else if (l && r) {
    Motors::drive(APPROACH_SPEED, APPROACH_SPEED);  // wide/close target dead ahead
  } else if (l) {
    lastSeenDir = -1;
    Motors::drive(-TRACK_TURN_SPEED, TRACK_TURN_SPEED);
  } else if (r) {
    lastSeenDir = +1;
    Motors::drive(TRACK_TURN_SPEED, -TRACK_TURN_SPEED);
  } else if (now - lastContactMs < CONTACT_HOLD_MS) {
    // Sonar often reads nothing when pressed against the opponent: keep pushing.
    Motors::drive(ATTACK_SPEED, ATTACK_SPEED);
  } else {
    return false;
  }

  setState(State::Attack);
  return true;
}

static void runSearch() {
  setState(State::Search);
  uint32_t now = millis();
  uint32_t phaseMs = searchSpinning ? SEARCH_SPIN_MS : SEARCH_ADVANCE_MS;
  if (now - searchPhaseSinceMs >= phaseMs) {
    searchSpinning = !searchSpinning;
    searchPhaseSinceMs = now;
  }

  if (searchSpinning) {
    Motors::drive(lastSeenDir * SEARCH_TURN_SPEED, -lastSeenDir * SEARCH_TURN_SPEED);
  } else {
    Motors::drive(SEARCH_FWD_SPEED, SEARCH_FWD_SPEED);  // reposition; edge sensors guard us
  }
}

static void runMatch() {
  LineState line = Line::read();

  // 1. Edge always wins.
  if (line.any() && (state != State::Escape || escapeConflicts(line))) {
    planEscape(line);
  }
  if (state == State::Escape) {
    runEscape();
    if (state == State::Escape) return;
  }

  // 2. Opponent, 3. search.
  if (!runAttack()) runSearch();
}

// --------------------------------------------------------------------
//  Start / stop input
// --------------------------------------------------------------------
static bool buttonPressed() {
  static bool lastLevel = HIGH;
  static uint32_t lastChangeMs = 0;
  bool level = digitalRead(PIN_BTN_START);
  uint32_t now = millis();
  if (level != lastLevel && now - lastChangeMs > 40) {
    lastChangeMs = now;
    lastLevel = level;
    return level == LOW;  // falling edge = press
  }
  return false;
}

static void handleStartStop() {
  if (USE_START_MODULE) {
    bool go = digitalRead(PIN_START_MODULE) == HIGH;
    if (go && state == State::Idle) setState(State::Search);  // module handles the delay
    if (!go && inMatch()) setState(State::Stopped);
  }

  if (!buttonPressed()) return;
  switch (state) {
    case State::Idle:    setState(State::Countdown); break;
    case State::Stopped: setState(State::Idle); break;
    default:             setState(State::Stopped); break;  // countdown or fighting
  }
}

// --------------------------------------------------------------------
//  Display (only redraws small regions; never blocks the fight)
// --------------------------------------------------------------------
static TFT_eSPI tft;

static uint16_t stateColor(State s) {
  switch (s) {
    case State::Attack:    return TFT_RED;
    case State::Escape:    return TFT_YELLOW;
    case State::Search:    return TFT_CYAN;
    case State::Countdown: return TFT_ORANGE;
    case State::Stopped:   return TFT_DARKGREY;
    default:               return TFT_GREEN;
  }
}

static void drawStatic() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.drawString("ELEC3020 SUMO", 4, 4, 2);
  tft.drawString("KEY: start/stop", 200, 4, 2);
  tft.drawString("SONAR L / M / R (cm)", 4, 96, 2);
  tft.drawString("EDGE", 4, 136, 2);
}

static void drawEdgeBox(int x, int y, const char *label, bool active) {
  tft.fillRoundRect(x, y, 44, 22, 4, active ? TFT_WHITE : TFT_NAVY);
  tft.setTextColor(active ? TFT_BLACK : TFT_LIGHTGREY);
  tft.drawCentreString(label, x + 22, y + 3, 2);
}

static void drawLive() {
  char buf[32];
  tft.setTextPadding(312);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  auto cm = [](Sonar::Side s) { return (int)Sonar::rawCm(s); };
  snprintf(buf, sizeof buf, "%4d %4d %4d", cm(Sonar::LEFT), cm(Sonar::MID), cm(Sonar::RIGHT));
  tft.drawString(buf, 4, 112, 2);

  LineState line = Line::read();
  drawEdgeBox(50, 132, "FL", line.frontLeft);
  drawEdgeBox(98, 132, "FR", line.frontRight);
  drawEdgeBox(146, 132, "RL", line.rearLeft);
  drawEdgeBox(194, 132, "RR", line.rearRight);

  float vbat = analogReadMilliVolts(PIN_BATTERY) * 2 / 1000.0f;
  snprintf(buf, sizeof buf, "%.2fV", vbat);
  tft.setTextPadding(60);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString(buf, 252, 136, 2);
}

static void updateDisplay() {
  if (!ENABLE_DISPLAY) return;
  static State shown = State::Stopped;
  static int lastSecs = -1;
  static uint32_t lastLiveMs = 0;
  uint32_t now = millis();

  if (state != shown) {
    shown = state;
    lastSecs = -1;
    tft.setTextPadding(312);
    tft.setTextColor(stateColor(state), TFT_BLACK);
    tft.drawString(stateName(state), 4, 28, 4);
    if (state != State::Countdown) tft.drawString("", 4, 56, 4);
  }

  if (state == State::Countdown) {
    int secs = (START_DELAY_MS - (now - stateSinceMs) + 999) / 1000;
    if (secs != lastSecs) {
      lastSecs = secs;
      char buf[8];
      snprintf(buf, sizeof buf, "%d", secs);
      tft.setTextPadding(312);
      tft.setTextColor(TFT_ORANGE, TFT_BLACK);
      tft.drawString(buf, 4, 56, 4);
    }
  }

  // Live sensor view is for setup/calibration; skip it while fighting.
  if (!inMatch() && now - lastLiveMs >= DISPLAY_REFRESH_MS) {
    lastLiveMs = now;
    drawLive();
  }
}

// --------------------------------------------------------------------

void setup() {
  // Motors first so the bot can never twitch on boot.
  Motors::begin();
  Motors::coast();

  Serial.begin(115200);
  Serial.setTxTimeoutMs(0);  // never stall the control loop when USB is unplugged
  pinMode(PIN_BTN_START, INPUT_PULLUP);
  if (USE_START_MODULE) pinMode(PIN_START_MODULE, INPUT_PULLDOWN);

  Sonar::begin();
  Line::begin();

  if (ENABLE_DISPLAY) {
    pinMode(PIN_LCD_POWER, OUTPUT);
    digitalWrite(PIN_LCD_POWER, HIGH);
    tft.init();
    tft.setRotation(1);
    drawStatic();
  }

  stateSinceMs = millis();
  Serial.println("Sumobot ready - press KEY (GPIO14) to start");
}

void loop() {
  Sonar::update();
  handleStartStop();

  switch (state) {
    case State::Idle:
    case State::Stopped:
      Motors::brake();
      break;
    case State::Countdown:
      Motors::brake();
      if (millis() - stateSinceMs >= START_DELAY_MS) setState(State::Search);
      break;
    default:
      runMatch();
      break;
  }

  updateDisplay();
}

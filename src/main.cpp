// =====================================================================
//  ELEC3020 Sumobot - LILYGO T-Display-S3
// ---------------------------------------------------------------------
//  Priority every loop (loop is non-blocking, runs at kHz rates):
//    1. EDGE   - an IR sensor sees the (black) border -> timed escape
//    2. ATTACK - a sonar sees the opponent -> steer at it and push
//    3. SEARCH - spin toward where it was last seen, then reposition
//
//  Start: press the KEY button (GPIO14) -> 5 s countdown -> fight.
//  Press KEY again at any time to stop. Press once more to re-arm.
// =====================================================================
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <esp_system.h>

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
static uint32_t lastTargetMs = 0;              // last time runAttack had a real target
static int16_t lastAttackL = 0, lastAttackR = 0;  // motor command used for it
static uint32_t escapeEndMs = 0;               // when the last edge escape finished
static uint32_t searchPhaseSinceMs = 0;
static bool searchSpinning = true;
static uint32_t lastSearchSpinMs = 0;  // when search last started a spin

static bool inMatch() {
  return state == State::Search || state == State::Attack || state == State::Escape;
}

// Last few state changes + why, shown on screen after STOP (debugging).
struct LogEntry {
  uint32_t ms;
  State state;
  char why[16];
};
static constexpr uint8_t LOG_SIZE = 7;
static LogEntry stateLog[LOG_SIZE];
static uint32_t logCount = 0;  // entries this match (wraps around the buffer)
static uint32_t matchStartMs = 0;

static void setState(State s, const char *why = "") {
  if (s == state) return;
  const State prev = state;
  state = s;
  stateSinceMs = millis();
  if (s == State::Search) {
    // Spin only if it's been a while and we didn't just escape an edge.
    bool recentSpin = lastSearchSpinMs && stateSinceMs - lastSearchSpinMs < SEARCH_RESPIN_MS;
    searchSpinning = prev != State::Escape && !recentSpin;
    if (searchSpinning) lastSearchSpinMs = stateSinceMs;
    searchPhaseSinceMs = stateSinceMs;
  }
  if (s == State::Countdown) lastSearchSpinMs = 0;  // new match: first search spins
  if (s == State::Countdown) logCount = 0;
  LogEntry &e = stateLog[logCount++ % LOG_SIZE];
  e.ms = stateSinceMs - matchStartMs;
  e.state = s;
  strncpy(e.why, why, sizeof e.why - 1);
  e.why[sizeof e.why - 1] = 0;
  Serial.printf("[%lu] -> %s %s\n", (unsigned long)stateSinceMs, stateName(s), why);
}

// "L0 M45 R0" summary of what the sonars currently see (0 = nothing).
static const char *sonarWhy() {
  static char buf[16];
  auto d = [](Sonar::Side s) { return Sonar::sees(s) ? (int)Sonar::distanceCm(s) : 0; };
  snprintf(buf, sizeof buf, "L%d M%d R%d", d(Sonar::LEFT), d(Sonar::MID), d(Sonar::RIGHT));
  return buf;
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
  // Forget attack/contact memory so we don't resume driving into the edge.
  lastTargetMs = 0;
  lastContactMs = 0;

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

  char why[16];
  snprintf(why, sizeof why, "%s%s%s%s", line.frontLeft ? "FL " : "", line.frontRight ? "FR " : "",
           line.rearLeft ? "RL " : "", line.rearRight ? "RR" : "");
  setState(State::Escape, why);
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
    escapeEndMs = now;
    setState(State::Search, "escape done");
    return;
  }

  // Only cut the escape short for an opponent close in front - not for
  // something far away (likely outside the ring).
  const Step &s = escapeSteps[escapeIndex];
  int16_t d = Sonar::distanceCm(Sonar::MID);
  if (s.interruptible && !TEST_EDGE_ONLY && USE_MID_SONAR && Sonar::sees(Sonar::MID) && d > 0 &&
      d <= (int16_t)CLOSE_CM) {
    escapeEndMs = now;
    setState(State::Attack, sonarWhy());
    return;
  }
  Motors::drive(s.left, s.right);
}

// --------------------------------------------------------------------
//  Attack / search
// --------------------------------------------------------------------
static bool runAttack() {
  const uint32_t now = millis();
  // Just escaped an edge: only react to close targets for a moment.
  const bool cooldown = now - escapeEndMs < ESCAPE_COOLDOWN_MS;
  // Side sonars that caused an unconfirmed pivot are muted for a while.
  static uint32_t pivotSinceMs = 0, sideMutedUntilMs = 0;
  if (state != State::Attack) pivotSinceMs = 0;  // fresh timing each attack
  const bool sidesMuted = (int32_t)(sideMutedUntilMs - now) > 0;
  auto seen = [&](Sonar::Side s) {
    if (!Sonar::sees(s)) return false;
    if (s == Sonar::MID && !USE_MID_SONAR) return false;
    if (s != Sonar::MID && sidesMuted) return false;
    int16_t d = Sonar::distanceCm(s);
    if (s != Sonar::MID && (!USE_SIDE_SONARS || d > (int16_t)SIDE_MAX_CM)) return false;
    return !cooldown || (d > 0 && d <= (int16_t)CLOSE_CM);
  };
  // Middle sonar had the target a moment ago -> a side-only reading is the
  // same target drifting off-centre: curve after it, don't pivot hard.
  static uint32_t lastMidMs = 0;
  const bool midRecent = lastMidMs && now - lastMidMs < TARGET_LOST_HOLD_MS;
  const bool l = seen(Sonar::LEFT);
  const bool m = seen(Sonar::MID);
  const bool r = seen(Sonar::RIGHT);
  auto drive = [&](int16_t left, int16_t right) {
    Motors::drive(left, right);
    lastAttackL = left;
    lastAttackR = right;
    lastTargetMs = now;
  };

  if (m) {
    lastMidMs = now;
    int16_t d = Sonar::distanceCm(Sonar::MID);
    if (d > 0 && d <= (int16_t)CONTACT_CM) lastContactMs = now;
    int16_t speed = (d > 0 && d <= (int16_t)CLOSE_CM) ? ATTACK_SPEED : APPROACH_SPEED;
    int16_t inner = speed * STEER_RATIO;

    if (l && !r) {
      lastSeenDir = -1;
      drive(inner, speed);
    } else if (r && !l) {
      lastSeenDir = +1;
      drive(speed, inner);
    } else {
      drive(speed, speed);
    }
  } else if (l && r) {
    drive(APPROACH_SPEED, APPROACH_SPEED);  // wide/close target dead ahead
  } else if (USE_MID_SONAR && (l || r) && !midRecent && pivotSinceMs &&
             now - pivotSinceMs > TRACK_TIMEOUT_MS) {
    // (Only possible with a working middle sonar to confirm targets.)
    // Spun toward a side reading for too long and the middle sonar never
    // saw anything: false reading. Mute the sides and go back to search.
    sideMutedUntilMs = now + SIDE_IGNORE_MS;
    pivotSinceMs = 0;
    return false;
  } else if (l) {
    lastSeenDir = -1;
    if (midRecent) drive(APPROACH_SPEED * STEER_RATIO, APPROACH_SPEED);
    else drive(TRACK_INNER_SPEED, TRACK_TURN_SPEED);
  } else if (r) {
    lastSeenDir = +1;
    if (midRecent) drive(APPROACH_SPEED, APPROACH_SPEED * STEER_RATIO);
    else drive(TRACK_TURN_SPEED, TRACK_INNER_SPEED);
  } else if (lastContactMs && now - lastContactMs < CONTACT_HOLD_MS) {
    // Sonar often reads nothing when pressed against the opponent: keep pushing.
    Motors::drive(ATTACK_SPEED, ATTACK_SPEED);
  } else if (lastTargetMs && now - lastTargetMs < TARGET_LOST_HOLD_MS &&
             lastAttackL > 0 && lastAttackR > 0) {
    // Target dropped out for a moment: keep driving at it. (Pivots aren't
    // held - that just overshoots and swings the other way.)
    Motors::drive(lastAttackL, lastAttackR);
  } else {
    return false;
  }

  // Time how long we've been pivoting on a side-only reading.
  const bool pivoting = !m && (l != r) && !midRecent;
  if (!pivoting) pivotSinceMs = 0;
  else if (!pivotSinceMs) pivotSinceMs = now;

  if (state != State::Attack) setState(State::Attack, sonarWhy());
  return true;
}

static void runSearch() {
  setState(State::Search, "lost target");
  uint32_t now = millis();
  uint32_t phaseMs = searchSpinning ? SEARCH_SPIN_MS : SEARCH_ADVANCE_MS;
  if (now - searchPhaseSinceMs >= phaseMs) {
    // Arc mode: one spin, then curve until something happens (no stop-start).
    searchSpinning = SEARCH_ARC ? false : !searchSpinning;
    searchPhaseSinceMs = now;
  }

  if (searchSpinning) {
    Motors::drive(lastSeenDir * SEARCH_TURN_SPEED, -lastSeenDir * SEARCH_TURN_SPEED);
  } else if (SEARCH_ARC) {
    // Curve toward where the target was last seen; edge sensors guard us.
    int16_t outer = SEARCH_ARC_OUTER, inner = SEARCH_ARC_INNER;
    if (lastSeenDir > 0) Motors::drive(outer, inner);
    else Motors::drive(inner, outer);
  } else {
    Motors::drive(SEARCH_FWD_SPEED, SEARCH_FWD_SPEED);  // reposition; edge sensors guard us
  }
}

static void runMatch() {
  LineState line = Line::read();
  if (!USE_REAR_EDGE_SENSORS) line.rearLeft = line.rearRight = false;
  if (!USE_REAR_RIGHT_SENSOR) line.rearRight = false;
  if (TEST_ATTACK_ONLY) line = LineState{};  // test mode: ignore the edge
  const uint32_t now = millis();

  // Push-through: in contact with the opponent and only our front is on the
  // border -> they're further out than us, so keep pushing for a moment.
  static uint32_t pushEdgeSinceMs = 0;
  static bool pushingAtEdge = false;
  bool inContact = state == State::Attack && lastContactMs && now - lastContactMs < CONTACT_HOLD_MS;
  if (line.front() && !line.rear() && inContact) {
    if (!pushingAtEdge) {
      pushingAtEdge = true;
      pushEdgeSinceMs = now;
    }
    if (now - pushEdgeSinceMs < EDGE_PUSH_THROUGH_MS) line = LineState{};
  } else {
    pushingAtEdge = false;
  }

  // 1. Edge wins (except during a push-through).
  if (line.any() && (state != State::Escape || escapeConflicts(line))) {
    planEscape(line);
  }
  if (state == State::Escape) {
    runEscape();
    if (state == State::Escape) return;
  }

  if (TEST_EDGE_ONLY) {
    setState(State::Search, "edge test");  // test mode: just drive forward
    Motors::drive(EDGE_TEST_SPEED, EDGE_TEST_SPEED);
    return;
  }

  // 2. Opponent, 3. search.
  if (runAttack()) return;
  if (TEST_ATTACK_ONLY) {
    setState(State::Search, "attack test");  // test mode: wait still for a target
    Motors::coast();
    return;
  }
  runSearch();
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
    if (go && state == State::Idle) {
      matchStartMs = millis();
      setState(State::Search, "start module");
    }  // module handles the delay
    if (!go && inMatch()) setState(State::Stopped, "start module");
  }

  if (!buttonPressed()) return;
  switch (state) {
    case State::Idle:    matchStartMs = millis(); setState(State::Countdown, "KEY"); break;
    case State::Stopped: setState(State::Idle, "KEY"); break;
    default:             setState(State::Stopped, "KEY"); break;  // countdown or fighting
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

  // Why did we (re)boot? BROWNOUT = battery sagged when the motors started.
  const char *why = "?";
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:  why = "power on"; break;
    case ESP_RST_BROWNOUT: why = "BROWNOUT"; break;
    case ESP_RST_PANIC:    why = "CRASH"; break;
    case ESP_RST_SW:       why = "software"; break;
    case ESP_RST_EXT:      why = "RST button"; break;
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT:      why = "WATCHDOG"; break;
    default: break;
  }
  char buf[32];
  snprintf(buf, sizeof buf, "reset: %s", why);
  tft.setTextColor(esp_reset_reason() == ESP_RST_BROWNOUT ? TFT_RED : TFT_DARKGREY, TFT_BLACK);
  tft.drawString(buf, 200, 28, 2);
}

static void drawEdgeBox(int x, int y, const char *label, bool active, bool enabled = true) {
  tft.fillRoundRect(x, y, 44, 22, 4, !enabled ? TFT_DARKGREY : active ? TFT_RED : TFT_NAVY);
  tft.setTextColor(active || !enabled ? TFT_WHITE : TFT_LIGHTGREY);
  tft.drawCentreString(label, x + 22, y + 3, 2);
}

static void drawLive() {
  char buf[32];

  // Small line under the state: where the opponent is detected. Skipped in
  // the countdown, which uses this area for its digits.
  if (state != State::Countdown) {
    char t[48];
    int n = snprintf(t, sizeof t, "TARGET:");
    const char *names[3] = {"L", "M", "R"};
    const Sonar::Side ss[3] = {Sonar::LEFT, Sonar::MID, Sonar::RIGHT};
    bool any = false;
    for (int i = 0; i < 3; i++) {
      if (!Sonar::sees(ss[i])) continue;
      any = true;
      n += snprintf(t + n, sizeof t - n, " %s %dcm", names[i], (int)Sonar::distanceCm(ss[i]));
    }
    if (!any) snprintf(t, sizeof t, "TARGET: none (range %d cm)", SONAR_MAX_CM);
    tft.setTextPadding(312);
    tft.setTextColor(any ? TFT_RED : TFT_DARKGREY, TFT_BLACK);
    tft.drawString(t, 4, 76, 2);

    // What the code is telling the motors (-255..255, before the speed limit).
    snprintf(t, sizeof t, "MOTORS  L %4d   R %4d", Motors::lastLeft(), Motors::lastRight());
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString(t, 4, 58, 2);
  }

  tft.setTextPadding(312);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  auto cm = [](Sonar::Side s) { return (int)Sonar::rawCm(s); };
  snprintf(buf, sizeof buf, "%4d %4d %4d", cm(Sonar::LEFT), cm(Sonar::MID), cm(Sonar::RIGHT));
  tft.drawString(buf, 4, 112, 2);

  LineState line = Line::read();
  drawEdgeBox(50, 132, "FL", line.frontLeft);
  drawEdgeBox(98, 132, "FR", line.frontRight);
  drawEdgeBox(146, 132, "RL", line.rearLeft, USE_REAR_EDGE_SENSORS);
  drawEdgeBox(194, 132, "RR", line.rearRight, USE_REAR_EDGE_SENSORS && USE_REAR_RIGHT_SENSOR);

  float vbat = analogReadMilliVolts(PIN_BATTERY) * 2 / 1000.0f;
  snprintf(buf, sizeof buf, "%.2fV", vbat);
  tft.setTextPadding(60);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString(buf, 252, 136, 2);
}

// After STOP: the last state changes of the match, oldest first.
static void drawLog() {
  tft.fillRect(0, 56, 320, 114, TFT_BLACK);
  tft.setTextPadding(0);
  uint8_t n = logCount < LOG_SIZE ? logCount : LOG_SIZE;
  for (uint8_t i = 0; i < n; i++) {
    const LogEntry &e = stateLog[(logCount - n + i) % LOG_SIZE];
    char buf[40];
    snprintf(buf, sizeof buf, "%6.2fs %-9s %s", e.ms / 1000.0f, stateName(e.state), e.why);
    tft.setTextColor(stateColor(e.state), TFT_BLACK);
    tft.drawString(buf, 4, 56 + i * 16, 2);
  }
}

static void updateDisplay() {
  if (!ENABLE_DISPLAY) return;
  static State shown = State::Stopped;
  static int lastSecs = -1;
  static uint32_t lastLiveMs = 0;
  uint32_t now = millis();

  if (state != shown) {
    if (shown == State::Stopped) drawStatic();  // leaving the log screen
    shown = state;
    lastSecs = -1;
    tft.setTextPadding(312);
    tft.setTextColor(stateColor(state), TFT_BLACK);
    tft.drawString(stateName(state), 4, 28, 4);
    if (state != State::Countdown) tft.drawString("", 4, 56, 4);
    if (state == State::Stopped) {
      drawLog();
      return;
    }
  }
  if (state == State::Stopped) return;  // keep the log on screen

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

  // Live sensor view: always before/after a match, during it only if enabled.
  if ((!inMatch() || DISPLAY_LIVE_IN_MATCH) && now - lastLiveMs >= DISPLAY_REFRESH_MS) {
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
  if (AUTO_START) {
    matchStartMs = millis();
    setState(State::Countdown, "power on");
  } else {
    Serial.println("Sumobot ready - press KEY (GPIO14) to start");
  }
}

void loop() {
  Sonar::update();
  handleStartStop();

  switch (state) {
    case State::Idle:
      Motors::coast();  // motors at 0 (both driver inputs off)
      break;
    case State::Stopped:
      // Brake briefly so it stops quickly, then motors at 0.
      if (millis() - stateSinceMs < 300) Motors::brake();
      else Motors::coast();
      break;
    case State::Countdown:
      Motors::coast();
      if (millis() - stateSinceMs >= START_DELAY_MS) setState(State::Search, "go");
      break;
    default:
      runMatch();
      break;
  }

  updateDisplay();
}

// --- IR SENSOR HEIGHT CALIBRATION (screen only, motors OFF) ---
// Build/upload with:  pio run -e ir_calibrate -t upload
// Runs on battery (no USB needed). Shows all 4 IR edge sensors live, raw
// (no filter) - including the rear ones even if USE_REAR_EDGE_SENSORS is
// false - so each sensor's height / sensitivity knob can be set:
//   GREEN  = white (ring)        RED = black (border)
//   YELLOW = flickering: the reading changed FLICKER_PER_S+ times in the
//            last second -> sensor is at the edge of its range (too high,
//            or knob on the limit). Hold the robot still when checking.
// Each box also shows how many times it changed in the last second.
// Sonar raw distances are shown along the bottom.
#include <Arduino.h>
#include <TFT_eSPI.h>

#include "config.h"
#include "motors.h"
#include "pins.h"
#include "sensors.h"

static constexpr uint16_t FLICKER_PER_S = 3;

static TFT_eSPI tft;

struct IrBox {
  const char *label;
  uint8_t pin;
  int16_t x, y;
  bool usedInSumo;     // false = ignored by the match code (config.h)
  bool black;          // latest raw reading
  uint16_t flips;      // changes counted this second
  uint16_t flipsShown; // changes in the last full second
  bool drawn;
  uint16_t lastColor;
  bool lastBlack;
  uint16_t lastFlips;
};

// Laid out like the robot from above: front on top.
static constexpr int16_t BOX_W = 150, BOX_H = 52;
static IrBox ir[4] = {
    {"FL", PIN_IR_FRONT_LEFT, 6, 22, true},
    {"FR", PIN_IR_FRONT_RIGHT, 164, 22, true},
    {"RL", PIN_IR_REAR_LEFT, 6, 80, USE_REAR_EDGE_SENSORS},
    {"RR", PIN_IR_REAR_RIGHT, 164, 80, USE_REAR_EDGE_SENSORS && USE_REAR_RIGHT_SENSOR},
};

static bool readBlack(uint8_t pin) { return digitalRead(pin) == LINE_ACTIVE_LEVEL; }

static void drawIr(IrBox &b) {
  uint16_t color = b.flipsShown >= FLICKER_PER_S ? TFT_YELLOW : b.black ? TFT_RED : TFT_DARKGREEN;
  if (b.drawn && color == b.lastColor && b.black == b.lastBlack && b.flipsShown == b.lastFlips) return;
  b.drawn = true;
  b.lastColor = color;
  b.lastBlack = b.black;
  b.lastFlips = b.flipsShown;

  char buf[24];
  tft.fillRoundRect(b.x, b.y, BOX_W, BOX_H, 6, color);
  tft.setTextColor(color == TFT_YELLOW ? TFT_BLACK : TFT_WHITE);
  tft.setTextDatum(TL_DATUM);
  tft.drawString(b.label, b.x + 6, b.y + 4, 2);
  snprintf(buf, sizeof buf, "IO%d%s", b.pin, b.usedInSumo ? "" : " off");
  tft.drawString(buf, b.x + 6, b.y + 32, 2);  // "off" = ignored by the match code
  tft.setTextDatum(MC_DATUM);
  tft.drawString(b.black ? "BLACK" : "WHITE", b.x + 95, b.y + 18, 4);
  snprintf(buf, sizeof buf, "%u flips/s", b.flipsShown);
  tft.drawString(buf, b.x + 95, b.y + 42, 2);
  tft.setTextDatum(TL_DATUM);
}

static void drawBottom() {
  char buf[40], v[3][6];
  const Sonar::Side sides[3] = {Sonar::LEFT, Sonar::MID, Sonar::RIGHT};
  for (int i = 0; i < 3; i++) {
    int cm = Sonar::rawCm(sides[i]);
    if (cm > 0) snprintf(v[i], sizeof v[i], "%d", cm);
    else snprintf(v[i], sizeof v[i], "--");
  }
  snprintf(buf, sizeof buf, "L %s   M %s   R %s cm", v[0], v[1], v[2]);
  tft.setTextPadding(308);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString(buf, 6, 140, 4);

  snprintf(buf, sizeof buf, "%.2fV", analogReadMilliVolts(PIN_BATTERY) * 2 / 1000.0f);
  tft.setTextPadding(60);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString(buf, 316, 2, 2);
  tft.setTextDatum(TL_DATUM);
  tft.setTextPadding(0);
}

void setup() {
  // Motors held off for the whole test (GPIO44 would otherwise idle HIGH).
  Motors::begin();
  Motors::coast();

  Sonar::begin();
  Line::begin();  // sets the IR pins up exactly as the match code does
  for (auto &b : ir) b.black = readBlack(b.pin);

  pinMode(PIN_LCD_POWER, OUTPUT);
  digitalWrite(PIN_LCD_POWER, HIGH);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("IR CALIBRATE - motors OFF", 6, 2, 2);
}

void loop() {
  Sonar::update();
  const uint32_t now = millis();

  // Sample every loop (fast) so short flickers are counted too.
  for (auto &b : ir) {
    bool black = readBlack(b.pin);
    if (black != b.black) {
      b.black = black;
      b.flips++;
    }
  }

  static uint32_t secondStartMs = 0;
  if (now - secondStartMs >= 1000) {
    secondStartMs = now;
    for (auto &b : ir) {
      b.flipsShown = b.flips;
      b.flips = 0;
    }
  }

  static uint32_t lastDrawMs = 0;
  if (now - lastDrawMs < 100) return;
  lastDrawMs = now;
  for (auto &b : ir) drawIr(b);
  drawBottom();
}

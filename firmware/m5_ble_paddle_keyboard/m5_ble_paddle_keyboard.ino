/*
  M5StickC Plus2 BLE paddle keyboard.

  The device appears as a Bluetooth keyboard:
  - dit paddle sends and holds 'a'
  - dah paddle sends and holds 's'

  Open the trainer on a phone:
  https://alekseigor.github.io/paddle-trainer/
*/

#include <M5Unified.h>
#include <BleKeyboard.h>

const int DIT_PIN = 33;
const int DAH_PIN = 32;

const char DIT_KEY = 'a';
const char DAH_KEY = 's';

const char *BLE_NAME = "CW Paddle BLE";
const char *TRAINER_HOST = "alekseigor.github.io";
const char *TRAINER_PATH = "/paddle-trainer/";

const unsigned long DEBOUNCE_MS = 8;
const unsigned long SCREEN_REFRESH_MS = 250;

BleKeyboard bleKeyboard(BLE_NAME, "PaddleTrainer", 100);
M5Canvas canvas(&M5.Display);

bool ditDown = false;
bool dahDown = false;
bool lastRawDit = false;
bool lastRawDah = false;
bool stableDit = false;
bool stableDah = false;
unsigned long rawDitChangedAt = 0;
unsigned long rawDahChangedAt = 0;
unsigned long lastScreenAt = 0;
uint32_t sentDitCount = 0;
uint32_t sentDahCount = 0;
uint32_t testSendCount = 0;

void setup() {
  pinMode(DIT_PIN, INPUT_PULLUP);
  pinMode(DAH_PIN, INPUT_PULLUP);

  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setRotation(1);
  M5.Display.fillScreen(TFT_BLACK);

  canvas.setColorDepth(8);
  canvas.createSprite(M5.Display.width(), M5.Display.height());
  canvas.setTextDatum(textdatum_t::top_left);

  bleKeyboard.begin();
  drawScreen(true);
}

void loop() {
  M5.update();
  updateInputs();
  updateBleKeys();
  updateButtons();

  if (millis() - lastScreenAt >= SCREEN_REFRESH_MS) {
    drawScreen(false);
  }

  delay(1);
}

void updateInputs() {
  const bool rawDit = digitalRead(DIT_PIN) == LOW;
  const bool rawDah = digitalRead(DAH_PIN) == LOW;

  if (rawDit != lastRawDit) {
    lastRawDit = rawDit;
    rawDitChangedAt = millis();
  }
  if (rawDah != lastRawDah) {
    lastRawDah = rawDah;
    rawDahChangedAt = millis();
  }

  if (millis() - rawDitChangedAt >= DEBOUNCE_MS) {
    stableDit = rawDit;
  }
  if (millis() - rawDahChangedAt >= DEBOUNCE_MS) {
    stableDah = rawDah;
  }
}

void updateBleKeys() {
  const bool connected = bleKeyboard.isConnected();

  if (!connected) {
    ditDown = false;
    dahDown = false;
    return;
  }

  if (stableDit != ditDown) {
    ditDown = stableDit;
    if (ditDown) {
      bleKeyboard.press(DIT_KEY);
      sentDitCount++;
    } else {
      bleKeyboard.release(DIT_KEY);
    }
  }

  if (stableDah != dahDown) {
    dahDown = stableDah;
    if (dahDown) {
      bleKeyboard.press(DAH_KEY);
      sentDahCount++;
    } else {
      bleKeyboard.release(DAH_KEY);
    }
  }
}

void updateButtons() {
  if (M5.BtnA.wasClicked() && bleKeyboard.isConnected()) {
    bleKeyboard.write('a');
    delay(20);
    bleKeyboard.write('s');
    testSendCount++;
  }
}

void drawScreen(bool force) {
  if (!force && millis() - lastScreenAt < SCREEN_REFRESH_MS) {
    return;
  }
  lastScreenAt = millis();

  const bool connected = bleKeyboard.isConnected();

  canvas.fillScreen(TFT_BLACK);
  drawHeader(connected);
  drawBody(connected);

  M5.Display.startWrite();
  canvas.pushSprite(0, 0);
  M5.Display.endWrite();
}

void drawHeader(bool connected) {
  canvas.fillRect(0, 0, 240, 24, connected ? TFT_DARKGREEN : TFT_DARKGREY);
  canvas.setTextFont(2);
  canvas.setTextColor(TFT_WHITE, connected ? TFT_DARKGREEN : TFT_DARKGREY);
  canvas.setCursor(6, 5);
  canvas.print("BLE Paddle");
  canvas.setCursor(138, 5);
  canvas.print(connected ? "CONNECTED" : "PAIR");
}

void drawBody(bool connected) {
  canvas.setTextFont(2);

  canvas.setTextColor(TFT_CYAN, TFT_BLACK);
  canvas.setCursor(8, 30);
  canvas.print("BT name:");
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.setCursor(82, 30);
  canvas.print(BLE_NAME);

  canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
  canvas.setCursor(8, 50);
  canvas.print(connected ? "Open on phone:" : "Pair in phone BT settings");

  canvas.setTextColor(TFT_GREEN, TFT_BLACK);
  canvas.setCursor(8, 68);
  canvas.print(TRAINER_HOST);
  canvas.setCursor(8, 84);
  canvas.print(TRAINER_PATH);

  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.setCursor(8, 104);
  canvas.printf("D G%d:%s %lu  H G%d:%s %lu", DIT_PIN, stableDit ? "ON" : "--", sentDitCount, DAH_PIN, stableDah ? "ON" : "--", sentDahCount);
  canvas.setCursor(8, 118);
  canvas.printf("BtnA test as: %lu", testSendCount);
}

/*
  USB paddle interface for VBand on Arduino Micro / Leonardo / Pro Micro.

  Wiring:
    DIT paddle -> pin 2 and GND
    DAH paddle -> pin 3 and GND

  The board appears as a USB keyboard. It holds one keyboard key while each
  paddle contact is closed, so VBand can measure the timing itself.

  Default VBand keys:
    DIT -> [
    DAH -> ]

  Hold both paddles while resetting the board to disable keyboard output for
  that boot. This is a safety lockout for wiring mistakes.
*/

#include <Keyboard.h>

const byte DIT_PIN = 2;
const byte DAH_PIN = 3;

const char DIT_KEY = '[';
const char DAH_KEY = ']';

const unsigned long DEBOUNCE_MS = 5;
const unsigned long STARTUP_GUARD_MS = 1200;

struct DebouncedInput {
  byte pin;
  bool stablePressed;
  bool lastRawPressed;
  bool sentPressed;
  unsigned long lastChangeMs;
};

DebouncedInput dit = { DIT_PIN, false, false, false, 0 };
DebouncedInput dah = { DAH_PIN, false, false, false, 0 };

bool keyboardEnabled = true;

void setup() {
  pinMode(DIT_PIN, INPUT_PULLUP);
  pinMode(DAH_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);

  delay(STARTUP_GUARD_MS);

  keyboardEnabled = !(readPressed(DIT_PIN) && readPressed(DAH_PIN));
  if (keyboardEnabled) {
    Keyboard.begin();
  } else {
    blinkLockout();
  }
}

void loop() {
  updateInput(dit);
  updateInput(dah);

  handlePaddle(dit, DIT_KEY);
  handlePaddle(dah, DAH_KEY);

  digitalWrite(LED_BUILTIN, dit.sentPressed || dah.sentPressed ? HIGH : LOW);
}

void handlePaddle(DebouncedInput &input, char key) {
  if (!keyboardEnabled) {
    return;
  }

  if (input.stablePressed && !input.sentPressed) {
    Keyboard.press(key);
    input.sentPressed = true;
  }

  if (!input.stablePressed && input.sentPressed) {
    Keyboard.release(key);
    input.sentPressed = false;
  }
}

void updateInput(DebouncedInput &input) {
  bool rawPressed = readPressed(input.pin);
  unsigned long now = millis();

  if (rawPressed != input.lastRawPressed) {
    input.lastRawPressed = rawPressed;
    input.lastChangeMs = now;
  }

  if ((now - input.lastChangeMs) >= DEBOUNCE_MS) {
    input.stablePressed = rawPressed;
  }
}

bool readPressed(byte pin) {
  return digitalRead(pin) == LOW;
}

void blinkLockout() {
  for (byte i = 0; i < 8; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(80);
    digitalWrite(LED_BUILTIN, LOW);
    delay(80);
  }
}

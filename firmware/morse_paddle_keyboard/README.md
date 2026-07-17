# Morse Paddle Keyboard

USB paddle interface for Arduino Micro, Leonardo, or Pro Micro on ATmega32U4.

The board appears to the computer as a USB keyboard. It holds `[` for dit and `]` for dah while the paddle contacts are closed.

## Wiring

| Paddle | Arduino Micro | Connection |
| --- | --- | --- |
| Dit, left paddle | D2 | paddle contact to D2, common to GND |
| Dah, right paddle | D3 | paddle contact to D3, common to GND |

External resistors are not required. The sketch uses internal `INPUT_PULLUP`.

## Sketch Settings

- `DIT_PIN` and `DAH_PIN` are the paddle input pins.
- `DIT_KEY` and `DAH_KEY` are the keyboard keys sent to the computer. Defaults are `[` and `]`.

## Paddle Trainer

Use this sketch with the local or hosted Paddle Trainer page.

1. Connect Arduino Micro to the computer.
2. Open the trainer page in a browser.
3. Click `Focus input`.
4. Use the paddle.

## VBand

1. Open https://hamradio.solutions/vband/.
2. Select a channel, for example Practice Channel.
3. Select `Iambic Keyer Mode A`, `Iambic Keyer Mode B`, `Straight Key/Cootie`, or another required mode.
4. Make sure the VBand page has keyboard focus.
5. The left paddle should work as dit and the right paddle as dah.

## Safety

There is a startup pause controlled by `STARTUP_GUARD_MS`.

Hold both paddles while resetting the Arduino to disable USB keyboard output for that boot. This helps recover from wiring mistakes or a stuck contact.

## Usage

1. Open `morse_paddle_keyboard.ino` in Arduino IDE.
2. Select board `Arduino Micro`.
3. Select the correct port, for example `COM3`.
4. Press `Verify` to compile.
5. Upload only when ready.

For a quick check after flashing, open Notepad. Holding the left paddle should type `[`, and holding the right paddle should type `]`.

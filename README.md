# Paddle Trainer

Local browser trainer for an Arduino Micro USB paddle interface.

## Hardware

Connect the paddle to Arduino Micro:

| Paddle jack | Arduino Micro |
| --- | --- |
| tip / dit | `D2` |
| ring / dah | `D3` |
| sleeve / common | `GND` |

Do not connect the paddle to `5V`.

Use the Arduino sketch from this repository:

`firmware/morse_paddle_keyboard/morse_paddle_keyboard.ino`

It sends:

- dit as held `[`
- dah as held `]`

For phone training over Bluetooth, use the M5StickC Plus2 firmware:

`firmware/m5_ble_paddle_keyboard/m5_ble_paddle_keyboard.ino`

The M5 BLE firmware sends:

- dit as held `a`
- dah as held `s`

## Trainer

Open:

`index.html`

The trainer has five modes:

- Single symbol: the next symbol is shown only after the current one is sent correctly. Wrong input keeps the same symbol and marks it red.
- Word: generates a word of the configured length and waits until the whole word is repeated correctly.
- Short sentence: generates several words and waits until all generated symbols are repeated correctly.
- Listen repeat: generates a hidden sequence from the selected symbols, plays it as Morse audio, and waits until the user repeats it correctly. The top `Play` button starts a game loop: after a correct answer, the answer is shown briefly and the next hidden sequence starts automatically.
- Free input: decodes and displays whatever you send with the paddle without checking against a target.

All paddle input is sounded by the trainer through Web Audio.

The right side of the page contains a full Morse reference table with letters, numbers, and common punctuation.

The session stats panel watches the log, reports accuracy, highlights weak symbols in the Symbols grid, and does not change target generation. A highlighted symbol clears after the last 5 attempts for that symbol are correct. Target generation always uses the full selected symbol set.

## Cloud Sync

Cloud sync is optional. Without `sync-config.js` values, the trainer keeps using local browser storage.

When sync is configured, the trainer stores these fields on the server under the shared `syncKey` from `sync-config.js`:

- selected symbols
- trainer settings
- full input log
- Coach statistics derived from the log

To enable sync:

1. Create a Supabase project.
2. Open the Supabase SQL editor.
3. Run `supabase-schema.sql`.
4. Copy the project URL and anon public key into `sync-config.js`.
5. Set one shared `syncKey` in `sync-config.js`.
6. Deploy the updated files.

Every device that opens the site uses the same shared server profile automatically.

The bottom input log shows every decoded key input:

- green entries are correct
- red entries are wrong
- blue entries are free-input decoded symbols
- each entry shows `actual/expected` and the Morse sequence that was decoded
- the log is kept for the whole browser tab session and is not cleared when a new target is generated

## Arduino Build

```powershell
& "$env:LOCALAPPDATA\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe" compile --fqbn arduino:avr:micro .\firmware\morse_paddle_keyboard
```

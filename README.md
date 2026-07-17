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

## Trainer

Open:

`index.html`

The trainer has four modes:

- Single symbol: the next symbol is shown only after the current one is sent correctly. Wrong input keeps the same symbol and marks it red.
- Word: generates a word of the configured length and waits until the whole word is repeated correctly.
- Short sentence: generates several words and waits until all generated symbols are repeated correctly.
- Free input: decodes and displays whatever you send with the paddle without checking against a target.

All paddle input is sounded by the trainer through Web Audio.

The right side of the page contains a full Morse reference table with letters, numbers, and common punctuation.

The session stats panel watches the log, reports accuracy, and highlights weak symbols. Target generation always uses the full selected symbol set.

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

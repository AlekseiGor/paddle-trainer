# M5 BLE Paddle Keyboard

Bluetooth HID paddle interface for M5StickC Plus2.

The device appears to a phone or computer as a Bluetooth keyboard named:

`CW Paddle BLE`

It sends:

- dit paddle as held `a`
- dah paddle as held `s`

## Wiring

| Paddle jack | M5StickC Plus2 |
| --- | --- |
| tip / dit | `G33` |
| ring / dah | `G32` |
| sleeve / common | `GND` |

Do not connect the paddle to `5V`.

## Screen

The M5StickC Plus2 screen shows:

- Bluetooth device name
- connection status
- trainer URL: `alekseigor.github.io/paddle-trainer/`
- current dit/dah input state

## Phone Use

1. Pair the phone with `CW Paddle BLE` in Bluetooth settings.
2. Open `https://alekseigor.github.io/paddle-trainer/`.
3. Tap `Focus input` or the input field in the trainer panel.
4. Use the paddle.

## Build

```powershell
& "$env:LOCALAPPDATA\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe" compile --fqbn esp32:esp32:m5stack_stickc_plus2 .\arduino\m5_ble_paddle_keyboard
```

## Upload

```powershell
& "$env:LOCALAPPDATA\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe" upload -p COM7 --fqbn esp32:esp32:m5stack_stickc_plus2 .\arduino\m5_ble_paddle_keyboard
```

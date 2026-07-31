# M5 Wi-Fi Paddle Trainer

Standalone M5StickC Plus2 trainer. The device creates a Wi-Fi access point and serves the same web app files used by GitHub Pages.

## Use

1. Connect the phone to Wi-Fi `CW-Paddle`.
2. Password: `morse12345`.
3. Open `http://192.168.4.1`.

The paddle is connected directly to the M5StickC Plus2:

| Paddle jack | M5StickC Plus2 |
| --- | --- |
| tip / dit | `G33` |
| ring / dah | `G32` |
| sleeve / common | `GND` |

## Local Storage

The stick stores data in SPIFFS:

- `/settings.json` contains trainer settings.
- `/attempts.jsonl` contains one JSON record per attempt.

## Build

```powershell
& "$env:LOCALAPPDATA\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe" compile --fqbn esp32:esp32:m5stack_stickc_plus2 .\arduino\m5_wifi_paddle_trainer
```

## Upload

Upload the sketch, then upload the SPIFFS image made from the `data` folder to offset `0x670000`.

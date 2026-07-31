/*
  M5StickC Plus2 standalone Wi-Fi paddle trainer.

  The device creates a Wi-Fi access point and serves the same web app files
  that are published on GitHub Pages. Paddle input is exposed through HTTP.
*/

#include <DNSServer.h>
#include <M5Unified.h>
#include <SPIFFS.h>
#include <WebServer.h>
#include <WiFi.h>

const int DIT_PIN = 33;
const int DAH_PIN = 32;

const char *AP_SSID = "CW-Paddle";
const char *AP_PASSWORD = "morse12345";
const IPAddress AP_IP(192, 168, 4, 1);
const IPAddress AP_GATEWAY(192, 168, 4, 1);
const IPAddress AP_SUBNET(255, 255, 255, 0);

const unsigned long DEBOUNCE_MS = 8;
const unsigned long SCREEN_REFRESH_MS = 250;
const uint16_t DNS_PORT = 53;
const size_t MAX_BODY_BYTES = 120000;
const size_t MAX_LOG_BYTES = 900000;

const char *SETTINGS_PATH = "/settings.json";
const char *LOG_PATH = "/attempts.jsonl";

struct PaddleEvent {
  uint32_t id;
  uint32_t at;
  char mark;
  bool down;
};

const size_t EVENT_BUFFER_SIZE = 64;
PaddleEvent eventBuffer[EVENT_BUFFER_SIZE];
size_t eventWriteIndex = 0;
size_t eventCount = 0;
uint32_t lastEventId = 0;

WebServer server(80);
DNSServer dnsServer;
M5Canvas canvas(&M5.Display);

bool lastRawDit = false;
bool lastRawDah = false;
bool stableDit = false;
bool stableDah = false;
bool lastStableDit = false;
bool lastStableDah = false;
unsigned long rawDitChangedAt = 0;
unsigned long rawDahChangedAt = 0;
unsigned long lastScreenAt = 0;
uint32_t ditCount = 0;
uint32_t dahCount = 0;
uint32_t savedAttemptCount = 0;
String lastDbStatus = "OK";

void setup() {
  Serial.begin(115200);
  pinMode(DIT_PIN, INPUT_PULLUP);
  pinMode(DAH_PIN, INPUT_PULLUP);

  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setRotation(1);
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextFont(2);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setCursor(8, 8);
  M5.Display.print("CW Paddle WiFi");
  M5.Display.setCursor(8, 28);
  M5.Display.print("Starting...");

  canvas.setColorDepth(8);
  canvas.createSprite(M5.Display.width(), M5.Display.height());
  canvas.setTextDatum(textdatum_t::top_left);

  if (!SPIFFS.begin(true)) {
    lastDbStatus = "FS error";
  } else {
    savedAttemptCount = countLogLines();
  }

  startAccessPoint();
  bindRoutes();
  server.begin();
  drawScreen(true);
}

void loop() {
  M5.update();
  dnsServer.processNextRequest();
  server.handleClient();
  updateInputs();

  if (millis() - lastScreenAt >= SCREEN_REFRESH_MS) {
    drawScreen(false);
  }

  delay(1);
}

void startAccessPoint() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  dnsServer.start(DNS_PORT, "*", AP_IP);
}

void bindRoutes() {
  server.on("/", HTTP_GET, []() { serveFile("/index.html"); });
  server.on("/index.html", HTTP_GET, []() { serveFile("/index.html"); });
  server.on("/app.js", HTTP_GET, []() { serveFile("/app.js"); });
  server.on("/styles.css", HTTP_GET, []() { serveFile("/styles.css"); });
  server.on("/sync-config.js", HTTP_GET, []() { serveFile("/sync-config.js"); });

  server.on("/api/events", HTTP_GET, handleEvents);
  server.on("/api/profile", HTTP_GET, handleProfile);
  server.on("/api/settings", HTTP_POST, handleSaveSettings);
  server.on("/api/attempt", HTTP_POST, handleAppendAttempt);
  server.on("/api/clear-log", HTTP_POST, handleClearLog);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.onNotFound(handleNotFound);
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

  if (stableDit != lastStableDit) {
    lastStableDit = stableDit;
    if (stableDit) {
      ditCount++;
    }
    addEvent('.', stableDit);
  }
  if (stableDah != lastStableDah) {
    lastStableDah = stableDah;
    if (stableDah) {
      dahCount++;
    }
    addEvent('-', stableDah);
  }
}

void addEvent(char mark, bool down) {
  PaddleEvent event = { ++lastEventId, millis(), mark, down };
  eventBuffer[eventWriteIndex] = event;
  eventWriteIndex = (eventWriteIndex + 1) % EVENT_BUFFER_SIZE;
  if (eventCount < EVENT_BUFFER_SIZE) {
    eventCount++;
  }
}

void sendCors() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
}

String readBody() {
  String body = server.arg("plain");
  if (body.length() > MAX_BODY_BYTES) {
    return "";
  }
  body.trim();
  return body;
}

void sendJson(int code, const String &body) {
  sendCors();
  server.send(code, "application/json", body);
}

void handleEvents() {
  const uint32_t after = server.hasArg("after") ? strtoul(server.arg("after").c_str(), nullptr, 10) : 0;
  String body = "{\"id\":";
  body += String(lastEventId);
  body += ",\"dit\":";
  body += stableDit ? "true" : "false";
  body += ",\"dah\":";
  body += stableDah ? "true" : "false";
  body += ",\"events\":[";

  bool first = true;
  for (size_t i = 0; i < eventCount; i++) {
    const size_t index = (eventWriteIndex + EVENT_BUFFER_SIZE - eventCount + i) % EVENT_BUFFER_SIZE;
    const PaddleEvent &event = eventBuffer[index];
    if (event.id <= after) {
      continue;
    }
    if (!first) {
      body += ",";
    }
    first = false;
    body += "{\"id\":";
    body += String(event.id);
    body += ",\"at\":";
    body += String(event.at);
    body += ",\"mark\":\"";
    body += event.mark;
    body += "\",\"down\":";
    body += event.down ? "true" : "false";
    body += "}";
  }
  body += "]}";
  sendJson(200, body);
}

void handleProfile() {
  sendCors();
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json", "");
  server.sendContent("{\"settings\":");
  streamFileContentOrNull(SETTINGS_PATH);
  server.sendContent(",\"attempt_log\":[");
  streamLogAsJsonArray();
  server.sendContent("]}");
}

void handleSaveSettings() {
  const String body = readBody();
  if (!body.length()) {
    sendJson(400, "{\"ok\":false}");
    return;
  }
  File file = SPIFFS.open(SETTINGS_PATH, "w");
  if (!file) {
    lastDbStatus = "Settings fail";
    sendJson(500, "{\"ok\":false}");
    return;
  }
  file.print(body);
  file.close();
  lastDbStatus = "OK";
  sendJson(200, "{\"ok\":true}");
}

void handleAppendAttempt() {
  const String body = readBody();
  if (!body.length()) {
    sendJson(400, "{\"ok\":false}");
    return;
  }
  if (SPIFFS.exists(LOG_PATH)) {
    File current = SPIFFS.open(LOG_PATH, "r");
    if (current && current.size() > MAX_LOG_BYTES) {
      current.close();
      lastDbStatus = "DB full";
      sendJson(507, "{\"ok\":false,\"error\":\"db_full\"}");
      return;
    }
    if (current) {
      current.close();
    }
  }
  File file = SPIFFS.open(LOG_PATH, "a");
  if (!file) {
    lastDbStatus = "Log fail";
    sendJson(500, "{\"ok\":false}");
    return;
  }
  file.println(body);
  file.close();
  savedAttemptCount++;
  lastDbStatus = "OK";
  sendJson(200, "{\"ok\":true}");
}

void handleClearLog() {
  SPIFFS.remove(LOG_PATH);
  savedAttemptCount = 0;
  lastDbStatus = "OK";
  sendJson(200, "{\"ok\":true}");
}

void handleStatus() {
  String body = "{\"ssid\":\"";
  body += AP_SSID;
  body += "\",\"ip\":\"";
  body += WiFi.softAPIP().toString();
  body += "\",\"clients\":";
  body += String(WiFi.softAPgetStationNum());
  body += ",\"attempts\":";
  body += String(savedAttemptCount);
  body += ",\"db\":\"";
  body += lastDbStatus;
  body += "\"}";
  sendJson(200, body);
}

void handleNotFound() {
  if (server.method() == HTTP_OPTIONS) {
    sendCors();
    server.send(204);
    return;
  }
  if (serveFile(server.uri())) {
    return;
  }
  server.sendHeader("Location", "http://192.168.4.1/", true);
  server.send(302, "text/plain", "");
}

bool serveFile(const String &path) {
  if (!SPIFFS.exists(path)) {
    server.send(404, "text/plain", "File not found. Upload SPIFFS data.");
    return false;
  }
  File file = SPIFFS.open(path, "r");
  if (!file) {
    server.send(500, "text/plain", "Cannot open file.");
    return false;
  }
  server.streamFile(file, contentType(path));
  file.close();
  return true;
}

String contentType(const String &path) {
  if (path.endsWith(".html")) {
    return "text/html";
  }
  if (path.endsWith(".css")) {
    return "text/css";
  }
  if (path.endsWith(".js")) {
    return "application/javascript";
  }
  if (path.endsWith(".json")) {
    return "application/json";
  }
  return "text/plain";
}

void streamFileContentOrNull(const char *path) {
  if (!SPIFFS.exists(path)) {
    server.sendContent("null");
    return;
  }
  File file = SPIFFS.open(path, "r");
  if (!file) {
    server.sendContent("null");
    return;
  }
  while (file.available()) {
    server.sendContent(file.readStringUntil('\n'));
  }
  file.close();
}

void streamLogAsJsonArray() {
  if (!SPIFFS.exists(LOG_PATH)) {
    return;
  }
  File file = SPIFFS.open(LOG_PATH, "r");
  if (!file) {
    return;
  }
  bool first = true;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (!line.length()) {
      continue;
    }
    if (!first) {
      server.sendContent(",");
    }
    first = false;
    server.sendContent(line);
  }
  file.close();
}

uint32_t countLogLines() {
  if (!SPIFFS.exists(LOG_PATH)) {
    return 0;
  }
  File file = SPIFFS.open(LOG_PATH, "r");
  if (!file) {
    return 0;
  }
  uint32_t count = 0;
  while (file.available()) {
    if (file.read() == '\n') {
      count++;
    }
  }
  file.close();
  return count;
}

void drawScreen(bool force) {
  if (!force && millis() - lastScreenAt < SCREEN_REFRESH_MS) {
    return;
  }
  lastScreenAt = millis();

  canvas.fillScreen(TFT_BLACK);
  canvas.fillRect(0, 0, 240, 24, TFT_DARKGREEN);
  canvas.setTextFont(2);
  canvas.setTextColor(TFT_WHITE, TFT_DARKGREEN);
  canvas.setCursor(6, 5);
  canvas.print("CW Paddle WiFi");
  canvas.setCursor(168, 5);
  canvas.printf("C:%d", WiFi.softAPgetStationNum());

  canvas.setTextColor(TFT_CYAN, TFT_BLACK);
  canvas.setCursor(8, 30);
  canvas.print("WiFi:");
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.setCursor(62, 30);
  canvas.print(AP_SSID);

  canvas.setTextColor(TFT_CYAN, TFT_BLACK);
  canvas.setCursor(8, 48);
  canvas.print("Pass:");
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.setCursor(62, 48);
  canvas.print(AP_PASSWORD);

  canvas.setTextColor(TFT_GREEN, TFT_BLACK);
  canvas.setCursor(8, 68);
  canvas.print("Open http://192.168.4.1");

  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.setCursor(8, 92);
  canvas.printf("Dit G%d:%s %lu", DIT_PIN, stableDit ? "ON" : "--", ditCount);
  canvas.setCursor(8, 110);
  canvas.printf("Dah G%d:%s %lu", DAH_PIN, stableDah ? "ON" : "--", dahCount);
  canvas.setCursor(8, 128);
  canvas.printf("DB:%s A:%lu", lastDbStatus.c_str(), savedAttemptCount);

  M5.Display.startWrite();
  canvas.pushSprite(0, 0);
  M5.Display.endWrite();
}

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <time.h>

const char* ssid = "";
const char* password = "";

WebServer server(80);

// GPIO Pins
const int pirPin = 13;
const int buzzerPin = 12;
const int alertPin = 27;

bool motionDetected = false;
int motionCount = 0;
bool alertTriggered = false;
bool countReset = false;

time_t startTime = 0;
time_t endTime = 0;

void handleRoot() {
  server.send(200, "text/html", R"rawliteral(
    <html>
      <head><title>MyGuard Dashboard</title></head>
      <body>
        <h2>Set Active Schedule</h2>
        <form action="/set">
          Start Time (HH:MM): <input type="text" name="start"><br>
          End Time (HH:MM): <input type="text" name="end"><br>
          <input type="submit" value="Submit">
        </form>
        <p><a href="/buzzer/on">Turn Buzzer ON</a></p>
        <p><a href="/buzzer/off">Turn Buzzer OFF</a></p>
        <p>Status: <a href="/status">Check</a></p>
      </body>
    </html>
  )rawliteral");
}

void handleSetTime() {
  if (server.hasArg("start") && server.hasArg("end")) {
    String startStr = server.arg("start");
    String endStr = server.arg("end");

    int startH = startStr.substring(0, 2).toInt();
    int startM = startStr.substring(3, 5).toInt();
    int endH = endStr.substring(0, 2).toInt();
    int endM = endStr.substring(3, 5).toInt();

    time_t now = time(nullptr);
    struct tm* nowTm = localtime(&now);

    struct tm startTm = *nowTm;
    startTm.tm_hour = startH;
    startTm.tm_min = startM;
    startTm.tm_sec = 0;
    startTime = mktime(&startTm);

    struct tm endTm = *nowTm;
    endTm.tm_hour = endH;
    endTm.tm_min = endM;
    endTm.tm_sec = 0;
    endTime = mktime(&endTm);

    alertTriggered = false;
    motionDetected = false;
    motionCount = 0;
    countReset = false;

    server.send(200, "text/html", "⏱️ Start and end time set. <a href='/'>Back</a>");
  } else {
    server.send(400, "text/plain", "Missing time arguments.");
  }
}

void handleStatus() {
  String status = "Motion count: " + String(motionCount) + "<br>";
  status += "Motion detected: " + String(motionDetected ? "Yes" : "No") + "<br>";
  status += "Alert triggered: " + String(alertTriggered ? "Yes" : "No") + "<br>";
  status += "Time now: " + String(time(nullptr)) + "<br>";
  server.send(200, "text/html", status);
}

void handleBuzzerOn() {
  digitalWrite(buzzerPin, HIGH);
  server.send(200, "text/html", "🔔 Buzzer ON. <a href='/'>Back</a>");
}

void handleBuzzerOff() {
  digitalWrite(buzzerPin, LOW);
  server.send(200, "text/html", "🔕 Buzzer OFF. <a href='/'>Back</a>");
}

void setupWiFiAndMDNS() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("MyGuard")) {
    Serial.println("✅ mDNS responder started: http://MyGuard.local");
  } else {
    Serial.println("❌ mDNS setup failed.");
  }
}

void setupServer() {
  server.on("/", handleRoot);
  server.on("/set", handleSetTime);
  server.on("/status", handleStatus);
  server.on("/buzzer/on", handleBuzzerOn);
  server.on("/buzzer/off", handleBuzzerOff);
  server.begin();
  Serial.println("✅ HTTP server started.");
}

void setupTime() {
  configTime(19800, 0, "pool.ntp.org");  // UTC+5:30 (IST)
  while (time(nullptr) < 100000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n🕒 Time synced.");
}

void setup() {
  Serial.begin(115200);
  pinMode(pirPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(alertPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);
  digitalWrite(alertPin, LOW);

  setupWiFiAndMDNS();
  setupTime();
  setupServer();
}

void loop() {
  server.handleClient();

  // Check for motion
  if (digitalRead(pirPin) == HIGH) {
    motionDetected = true;
    motionCount++;
    delay(1000);  // Simple debounce
  }

  time_t now = time(nullptr);

  // Reset count at start
  if (startTime != 0 && endTime != 0 && now >= startTime && now < endTime && !countReset) {
    motionCount = 0;
    motionDetected = false;
    countReset = true;
    Serial.println("🔄 Motion count reset at start time.");
  }

  // No motion in final 2 mins
  if (now >= endTime - 120 && now < endTime && motionCount == 0 && !alertTriggered) {
    unsigned long alertStart = millis();
    const unsigned long alertDuration = 2 * 60 * 1000;

    Serial.println("🚨 No motion. Starting buzzer alert...");

    while (millis() - alertStart < alertDuration) {
      digitalWrite(buzzerPin, HIGH);
      delay(300);

      if (digitalRead(pirPin) == HIGH) {
        Serial.println("✅ Motion detected. Stopping buzzer.");
        digitalWrite(buzzerPin, LOW);
        motionDetected = true;
        motionCount++;
        return;
      }

      digitalWrite(buzzerPin, LOW);
      delay(700);
    }

    alertTriggered = true;
    digitalWrite(alertPin, HIGH);
    Serial.println("❌ No motion entire period. Alert triggered.");
  }
}

#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

/* ---------- WiFi ---------- */
const char* ssid = "RobotArm";
const char* password = "12345678";

/* ---------- Server ---------- */
WebServer server(80);

/* ---------- PCA9685 ---------- */
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

#define SERVO_FREQ 50
#define SERVOMIN 120
#define SERVOMAX 520

/* ---------- Servo State ---------- */
const int SERVO_COUNT = 4;

int currentPos[SERVO_COUNT] = {0, 0, 0, 0};     // start from 0
int targetPos[SERVO_COUNT]  = {0, 0, 0, 0};

unsigned long lastMoveTime = 0;
const int moveInterval = 20; // ms → slower = smoother

/* ---------- Helpers ---------- */
int angleToPulse(int angle) {
  return map(angle, 0, 180, SERVOMIN, SERVOMAX);
}

/* ---------- Smooth Motion Engine ---------- */
void updateServos() {
  if (millis() - lastMoveTime < moveInterval) return;
  lastMoveTime = millis();

  for (int i = 0; i < SERVO_COUNT; i++) {
    if (currentPos[i] < targetPos[i]) {
      currentPos[i]++;
    } else if (currentPos[i] > targetPos[i]) {
      currentPos[i]--;
    } else {
      continue;
    }
    pwm.setPWM(i, 0, angleToPulse(currentPos[i]));
  }
}

/* ---------- Web ---------- */
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body { font-family: Arial; text-align:center; }
input { width:300px; }
</style>
</head>
<body>
<h2>4 DOF Robotic Arm</h2>

<p>Base</p>
<input type="range" min="0" max="180" value="0" oninput="fetch('/set?ch=0&val='+this.value)">

<p>Shoulder</p>
<input type="range" min="0" max="180" value="0" oninput="fetch('/set?ch=1&val='+this.value)">

<p>Elbow</p>
<input type="range" min="0" max="180" value="0" oninput="fetch('/set?ch=2&val='+this.value)">

<p>Wrist</p>
<input type="range" min="0" max="180" value="0" oninput="fetch('/set?ch=3&val='+this.value)">

</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

void handleSet() {
  int ch = server.arg("ch").toInt();
  int val = server.arg("val").toInt();

  if (ch >= 0 && ch < SERVO_COUNT) {
    targetPos[ch] = constrain(val, 0, 180);
  }
  server.send(200, "text/plain", "OK");
}

/* ---------- Setup ---------- */
void setup() {
  Serial.begin(115200);

  Wire.begin(21, 22);
  pwm.begin();
  pwm.setPWMFreq(SERVO_FREQ);

  // Move all servos gently from 0
  for (int i = 0; i < SERVO_COUNT; i++) {
    pwm.setPWM(i, 0, angleToPulse(0));
  }

  WiFi.softAP(ssid, password);

  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.begin();
}

/* ---------- Loop ---------- */
void loop() {
  server.handleClient();
  updateServos();   // 🔑 smooth motion engine
}

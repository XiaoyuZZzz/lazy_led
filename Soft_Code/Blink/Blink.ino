#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Servo.h>

#define SERVO_PIN D5
Servo myservo;

// 舵机控制参数
const int STOP_PULSE = 1500;  // 停止的脉宽（单位：微秒）
const int CW_PULSE = 1300;    // 逆时针（实际物理方向可能相反，需测试）
const int CCW_PULSE = 1700;   // 顺时针
const unsigned long ROTATE_TIME = 2000; // 旋转时间（毫秒），需实测调整

// 状态变量
enum State { STOPPED, ROTATING_CW, ROTATING_CCW };
State currentState = STOPPED;
unsigned long rotateStartTime = 0;
bool isLightOn = false; // 灯的状态

AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>连续舵机控制</title>
  <style>
    body { text-align: center; padding: 20px; }
    #btn { padding: 20px 40px; font-size: 24px; }
  </style>
</head>
<body>
  <h1>懒人开灯控制器</h1>
  <button id="btn" onclick="toggle()">%STATE%</button>
  <script>
    function toggle() {
      fetch('/toggle')
        .then(response => response.text())
        .then(state => {
          document.getElementById("btn").innerHTML = (state === "ON") ? "关灯" : "开灯";
        });
    }
  </script>
</body>
</html>
)rawliteral";

String processor(const String& var) {
  if (var == "STATE") return isLightOn ? "关灯" : "开灯";
  return String();
}

void setup() {
  Serial.begin(115200);
  myservo.attach(SERVO_PIN);
  myservo.writeMicroseconds(STOP_PULSE); // 初始停止

  WiFi.softAP("Servo_AP", "12345678");
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html; charset=utf-8", index_html, processor);
  });

  server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (currentState == STOPPED) {
      isLightOn = !isLightOn;
      // 根据状态选择旋转方向
      if (isLightOn) {
        myservo.writeMicroseconds(CCW_PULSE); // 顺时针旋转
        currentState = ROTATING_CCW;
      } else {
        myservo.writeMicroseconds(CW_PULSE);  // 逆时针旋转
        currentState = ROTATING_CW;
      }
      rotateStartTime = millis(); // 记录开始时间
    }
    request->send(200, "text/plain", isLightOn ? "ON" : "OFF");
  });

  server.begin();
}

void loop() {
  // 非阻塞式时间检查
  if (currentState != STOPPED && (millis() - rotateStartTime >= ROTATE_TIME)) {
    myservo.writeMicroseconds(STOP_PULSE); // 停止舵机
    currentState = STOPPED;
  }
}
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_MLX90614.h>
#include <DFRobot_BloodOxygen_S.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LIS3DH.h>
#include "driver/rtc_io.h"
#include "config.h"

#define I2C_ADDRESS_MAX30102 0x57
#define I2C_ADDRESS_LIS3DH 0x18
#define WAKEUP_PIN GPIO_NUM_3
#define BUTTON_PIN 3

const char* ssid = "TCHRKNQUXR"; //iPhone od: Lovro
const char* password = "BFYvKaT598ndFUhq2E"; //12341234

String currentMode = "Half";
String activeMode = "";

enum DeviceState { Initial, Full, Half, Low, Sleep };
DeviceState state;
DeviceState previousState;

struct PulseLog{
  unsigned long timestamp;
  int avgBpm;
};
struct MovementLog{
  unsigned long timestamp;
  float intensity;
};

bool wokeUp = false;
bool waitingForSecondPress = false;
unsigned long secondPressWindowStart = 0;

bool collectingPulse = false;
unsigned long pulseSampleStart = 0;
unsigned long lastPulseSampleTime = 0;
int pulseSampleCount = 0;
int pulseAllSampleCount = 0;
int pulseSampleSum = 0;
const unsigned long pulseSampleInterval = 4000;
const int pulseSampleTotal = 5;
const int pulseSampleMax = 7;
int previousMeasurement = 80;

int movementCount = 0;
unsigned long lastMovementCheck = 0;

const int MAX_PULSE_LOGS = 20;
PulseLog pulseLogs[MAX_PULSE_LOGS];
int pulseLogIndex = 0;
unsigned long lastPulseMeasurement = 0;
const unsigned long pulseMeasurementInterval = 100000;

unsigned long lastPollTime = 0;
unsigned long initialStartTime = 0;
const unsigned long pollInterval = 500;
bool waitingForUserCommand = true;
bool lowModeHandled = false;

unsigned long lastStepCheck = 0;
float alpha = 0.2;
static float filtered = 1.0;
static float prev1 = 1.0, prev2 = 1.0;
static unsigned long lastStepTime = 0;
static unsigned long lastMovement = 0;
int stepCount = 0;
String sleepState = "";

Adafruit_MLX90614 mlx = Adafruit_MLX90614();
DFRobot_BloodOxygen_S_I2C oximeter(&Wire, I2C_ADDRESS_MAX30102);
Adafruit_LIS3DH lis = Adafruit_LIS3DH();


void setup() {
  delay(2000);
  Serial.begin(115200);
  delay(1000);
  WiFi.begin(ssid, password);
  Wire.begin(5, 6);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  esp_sleep_wakeup_cause_t cause;
  cause =  esp_sleep_get_wakeup_cause();
  Serial.print("Wakeup cause: ");
  Serial.println(cause);
  if (cause == ESP_SLEEP_WAKEUP_EXT0) {
      rtc_gpio_deinit(WAKEUP_PIN);
      Serial.println("Woken up");
      state = Initial;
      initialStartTime = millis();
      lowModeHandled = false;
  }
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting...");
  }
  Serial.println("Connected!");

  while (!oximeter.begin()) {
    Serial.println("MAX not found");
    delay(1000);
  }

  if (!mlx.begin()) {
    Serial.println("MLX not found");
    while (1) {
      delay(1000);
    }
  }

  if (!lis.begin(I2C_ADDRESS_LIS3DH)) {
    Serial.println("LIS3DH no found");
    while (1);
  }
  lis.setRange(LIS3DH_RANGE_4_G);
  
}

void fullyDisconnectWiFi() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.println("WiFi fully disconnected.");
}

void fullyConnectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");

  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    Serial.print(".");
    retry++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
  } else {
    Serial.println("\nWiFi failed to connect.");
  }
}

void sendPulseLogs() {
  if (pulseLogIndex == 0) return;

  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();

  for (int i = 0; i < pulseLogIndex; i++) {
    JsonObject obj = arr.add<JsonObject>();
    obj["timestamp"] = pulseLogs[i].timestamp;
    obj["avgBpm"] = pulseLogs[i].avgBpm;
  }

  String output;
  serializeJson(doc, output);
  Serial.println("Sending logs to server:");
  Serial.println(output);

  HTTPClient http;
  http.begin(API_BASE_URL "/pulselogs");
  http.addHeader("Content-Type", "application/json");
  int responseCode = http.POST(output);
  Serial.print("Pulse logs POST response: ");
  Serial.println(responseCode);
  http.end();

  HTTPClient moveHttp;
  moveHttp.begin(API_BASE_URL "/pulselogs/movement");
  moveHttp.addHeader("Content-Type", "application/json");
  String jsonMove = "{\"movementCount\":" + String(movementCount) + "}";
  int moveRes = moveHttp.POST(jsonMove);
  Serial.print("Movement count sent, response: ");
  Serial.println(moveRes);
  moveHttp.end();
}


void measureHeartbeat(){
  delay(1000);
  oximeter.setBautrate(DFRobot_BloodOxygen_S::BAUT_RATE_9600);
  oximeter.sensorStartCollect();
  Serial.println("Device measuring heartbeat");

  int heartbeatSamples[10];
  int validSamples = 0;

  for (int i = 0; i < 10; i++) {
    oximeter.getHeartbeatSPO2();
    int hb = oximeter._sHeartbeatSPO2.Heartbeat;
    Serial.println(hb);
    if (hb > 30 && hb < 220) {
      heartbeatSamples[validSamples++] = hb;
    }
    /*
    Serial.print("Heart Rate: ");
    Serial.print(hb);
    Serial.print(" bpm\tSpO₂: ");
    Serial.print(oximeter._sHeartbeatSPO2.SPO2);
    Serial.println(" %");
    */
    delay(4000);
  }
  oximeter.sensorEndCollect();

  if (validSamples > 0){
    int sum = 0;
    for (int i = 0; i < validSamples; i++){
      sum += heartbeatSamples[i];
    }
    int avg = sum / validSamples;
    avg = avg / 1.2;
    Serial.println(avg);

    HTTPClient resultHttp;
    resultHttp.begin(API_BASE_URL "/result");
    resultHttp.addHeader("Content-Type", "application/json");

    String jsonResult = "{\"result\":\"" + String(avg) + "\"}";
    int resCode = resultHttp.POST(jsonResult);
    Serial.print("Result POST response code: ");
    Serial.println(resCode);
    resultHttp.end();
  }

  HTTPClient resetHttp;
  resetHttp.begin(API_BASE_URL "/command");
  resetHttp.addHeader("Content-Type", "application/json");
  resetHttp.POST("{\"command\":\"\"}");
  resetHttp.end();
}

void measureTemperature(){
  Serial.println("Let's measure temperature");
  float object = mlx.readObjectTempC();
  object = round(object * 10.0) / 10.0;
  object = object + 2;

  String resultStr;
  if (fmod(object, 1.0) == 0.0) {
    resultStr = String((int)object);
  } else {
    resultStr = String(object, 1);
  }

  /*
  Serial.print("Ambient Temp: ");
  Serial.print(ambient);
  Serial.print(" °C\t");
  Serial.print("Object Temp: ");
  Serial.print(object);
  Serial.println(" °C");
  */

  HTTPClient resultHttp;
  resultHttp.begin(API_BASE_URL "/result");
  resultHttp.addHeader("Content-Type", "application/json");

  String jsonResult = "{\"result\":\"" + String(resultStr) + "\"}";
  int resCode = resultHttp.POST(jsonResult);
  Serial.print("Result POST response code: ");
  Serial.println(resCode);
  resultHttp.end();

  HTTPClient resetHttp;
  resetHttp.begin(API_BASE_URL "/command");
  resetHttp.addHeader("Content-Type", "application/json");
  resetHttp.POST("{\"command\":\"\"}");
  resetHttp.end();
}

bool detectStep() {
  lis.read();
  float x = lis.x_g;
  float y = lis.y_g;
  float z = lis.z_g;
  float length = sqrt(x*x + y*y + z*z);

  filtered = alpha * length + (1-alpha) * filtered;

  if (prev1 > prev2 && prev1 > filtered){
    if (prev1 > 1.2){
      unsigned long now = millis();
      if(now - lastMovement < 500 || now - lastMovement > 1400){
        lastMovement = now;
        lastStepTime = now;
      }
      else if(now - lastStepTime > 500){
        lastStepTime = now;
        lastMovement = now;
        return true;
      }
    }
    else if(prev1 > 1.0){
      unsigned long now = millis();
      lastMovement = now;
    }
  }
  prev2 = prev1;
  prev1 = filtered;
  return false; 
}

void loopSleepMode() {
  
  if(wokeUp){
    wokeUp = false;
    Serial.println("Woke from light sleep. Starting 10s window for second button press...");
    waitingForSecondPress = true;
    secondPressWindowStart = millis();
    return;
  }

  if(waitingForSecondPress){
    if (digitalRead(BUTTON_PIN) == LOW) {
      Serial.println("Button clicked");
      fullyConnectWiFi();

      HTTPClient checkinHttp;
      checkinHttp.begin(API_BASE_URL "/checkin");
      checkinHttp.POST("");
      checkinHttp.end();

      for (int i = 0; i < 10; i++) {
        HTTPClient sleepHttp;
        sleepHttp.begin(API_BASE_URL "/sleep");
        int sleepCode = sleepHttp.GET();
        if (sleepCode == 200) {
          String payload = sleepHttp.getString();
          JsonDocument doc;
          DeserializationError error = deserializeJson(doc, payload);
          if (!error) {
            sleepState = doc["sleep"].as<String>();
            if (sleepState == "OFF") {
              oximeter.sensorEndCollect();
              Serial.println("Sleep OFF received, returning to previous mode");
              HTTPClient moveHttp;
              moveHttp.begin(API_BASE_URL "/pulselogs/movement");
              moveHttp.addHeader("Content-Type", "application/json");
              String jsonMove = "{\"movementCount\":" + String(movementCount) + "}";
              int moveRes = moveHttp.POST(jsonMove);
              Serial.print("Movement count sent, response: ");
              Serial.println(moveRes);
              moveHttp.end();

              sendPulseLogs();
              pulseLogIndex = 0;
              memset(pulseLogs, 0, sizeof(pulseLogs));

              movementCount = 0;

              if(previousState == Half){
                WiFi.setSleep(true);
              }
              state = previousState;
              waitingForSecondPress = false;
              return;
            }
          }
        }
        sleepHttp.end();
        delay(500);
      }
      fullyDisconnectWiFi();
      waitingForSecondPress = false;
      return;
    }

    if (millis() - secondPressWindowStart >= 5000) {
      Serial.println("No second press received. Continuing normal operation.");
      waitingForSecondPress = false;
    }
  }

  /*unsigned long now = millis();
  if(now - lastMovementCheck >= 35){
    lastMovementCheck = now;
    lis.read();
    float x = lis.x_g;
    float y = lis.y_g;
    float z = lis.z_g;
    float length = sqrt(x*x + y*y + z*z);

    filtered = alpha * length + (1-alpha) * filtered;

    if (prev1 > prev2 && prev1 > filtered){
      if (prev1 > 1.2){
        if(now - lastMovement> 2000){
          lastMovement = now;
          movementCount++;
          Serial.println("Movement detected while sleeping");
        }
      }
    }
    prev2 = prev1;
    prev1 = filtered;
  }*/

  unsigned long now = millis();
  if (!collectingPulse && now - lastPulseMeasurement >= pulseMeasurementInterval) {
    collectingPulse = true;
    pulseSampleCount = 0;
    pulseAllSampleCount = 0;
    pulseSampleSum = 0;
    pulseSampleStart = now;
    lastPulseSampleTime = now - pulseSampleInterval;
    Serial.println("Starting pulse sampling...");
  }
  else if(!collectingPulse){
    Serial.println("Not time for pulse sampling. Entering light sleep...");

    esp_sleep_enable_timer_wakeup(101000 * 1000);
    esp_sleep_enable_ext0_wakeup(WAKEUP_PIN, 0);

    delay(100);
    esp_light_sleep_start();
    delay(100);
    wokeUp = true;
  }
  
  if (collectingPulse && now - lastPulseSampleTime >= pulseSampleInterval) {
    oximeter.sensorStartCollect();
    lastPulseSampleTime = now;
    oximeter.getHeartbeatSPO2();
    int hb = oximeter._sHeartbeatSPO2.Heartbeat;
    pulseAllSampleCount++;

    if (hb > 30 && hb < 220) {
      pulseSampleCount++;
      pulseSampleSum += hb;
      Serial.print("Sample ");
      Serial.print(pulseSampleCount);
      Serial.print(": ");
      Serial.println(hb);
    }

    if (pulseSampleCount >= pulseSampleTotal) {
      int avg = pulseSampleSum / pulseSampleCount;
      avg = avg / 1.2;
      if (pulseLogIndex < MAX_PULSE_LOGS) {
        pulseLogs[pulseLogIndex++] = { now, avg };
        previousMeasurement = avg;
        Serial.print("Logged avg pulse: ");
        Serial.println(avg);
      }
      collectingPulse = false;
      oximeter.sensorEndCollect();
      lastPulseMeasurement = now;
    }

    else if (pulseAllSampleCount >= pulseSampleMax) {
      int avg = previousMeasurement;
      if(pulseSampleSum != 0){
        avg = pulseSampleSum / pulseSampleCount;
        avg = avg / 1.2;
      }
      if (pulseLogIndex < MAX_PULSE_LOGS) {
        pulseLogs[pulseLogIndex++] = { now, avg };
        previousMeasurement = avg;
        Serial.print("Logged avg pulse: ");
        Serial.println(avg);
      }
      collectingPulse = false;
      oximeter.sensorEndCollect();
      lastPulseMeasurement = now;
    }

    if (pulseLogIndex >= MAX_PULSE_LOGS) {
      fullyConnectWiFi();
      while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Reconnecting...");
      }
      sendPulseLogs();
      pulseLogIndex = 0;
      memset(pulseLogs, 0, sizeof(pulseLogs));
      fullyDisconnectWiFi();
    }
  }

}


void loopFullMode(){
  if(currentMode != "Full"){
    return;
  }
  if(activeMode != "Full"){
    Serial.println("Change of mode, running FULL");
    activeMode = "Full";
  }
  if (digitalRead(BUTTON_PIN) == LOW) {
    Serial.println("Button pressed, switching to INITIAL");
    state = Initial;
    initialStartTime = millis();
    return;
  }

  if (millis() - lastStepCheck >= 35){
    lastStepCheck = millis();
    if (detectStep()) {
      stepCount++;
    }
  }

  if (stepCount >= 10) {
    HTTPClient stepHttp;
    stepHttp.begin(API_BASE_URL "/steps");
    stepHttp.addHeader("Content-Type", "application/json");

    String json = "{\"steps\":" + String(stepCount) + "}";
    int resCode = stepHttp.POST(json);
    Serial.print("Steps POST response code: ");
    Serial.println(resCode);

    stepHttp.end();
    stepCount = 0; 
  }

  unsigned long now = millis();

  if (now - lastPollTime >= pollInterval) {
    lastPollTime = now;

    HTTPClient checkinHttp;
    checkinHttp.begin(API_BASE_URL "/checkin");
    checkinHttp.POST("");
    checkinHttp.end();

    HTTPClient sleepHttp;
    sleepHttp.begin(API_BASE_URL "/sleep");
    int sleepCode = sleepHttp.GET();
    if(sleepCode == 200){
      String payload = sleepHttp.getString();
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, payload);
      if(!error){
        sleepState = doc["sleep"].as<String>();
      }
    }
    sleepHttp.end();

    HTTPClient httpMode;
    httpMode.begin(API_BASE_URL "/mode");
    int httpCode = httpMode.GET();

    if (httpCode == 200) {
      String payload = httpMode.getString();
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        String mode = doc["mode"];
        if (mode == "Full" || mode == "Half" || mode == "Low") {
          currentMode = mode;
        }
      }
    }
    httpMode.end();
    if (sleepState == "ON") {
      if (currentMode == "Full") previousState = Full;
      else if (currentMode == "Half") previousState = Half;
      else previousState = Low;
      fullyDisconnectWiFi();
      state = Sleep;
      return;
    }
    if (currentMode == "Full") {
      state = Full;
    }
    else if (currentMode == "Half") {
      WiFi.setSleep(true);
      state = Half;
    }
    else state = Low;

    HTTPClient httpCommand;
    httpCommand.begin(API_BASE_URL "/command");
    int CmdCode = httpCommand.GET();

    if (CmdCode == 200) {
      String command = httpCommand.getString();

      if (command == "measure_heartbeat") {
        measureHeartbeat();
      }
      else if(command == "measure_temperature"){
        measureTemperature();
      }
    }
    httpCommand.end();
  }
}

void loopHalfMode(){
  if(currentMode != "Half"){
    return;
  }
  if(activeMode != "Half"){
    Serial.println("Change of mode, running HALF");
    activeMode = "Half";
  }

  if (digitalRead(BUTTON_PIN) == LOW) {
    Serial.println("Button pressed, switching to INITIAL");
    WiFi.setSleep(false);
    state = Initial;
    initialStartTime = millis();
    return;
  }

  if (millis() - lastStepCheck >= 35){
    lastStepCheck = millis();
    if (detectStep()) {
      stepCount++;
      Serial.print("Step detected: ");
      Serial.println(stepCount);
    }
  }

  if (stepCount >= 500) {
    WiFi.setSleep(false);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.println("Connecting...");
    }
    HTTPClient stepHttp;
    stepHttp.begin(API_BASE_URL "/steps");
    stepHttp.addHeader("Content-Type", "application/json");

    String json = "{\"steps\":" + String(stepCount) + "}";
    int resCode = stepHttp.POST(json);
    Serial.print("Steps POST response code: ");
    Serial.println(resCode);

    stepHttp.end();
    stepCount = 0; 
    WiFi.setSleep(true);
  }
}

void loopLowMode() {
  if (currentMode != "Low") return;

  if (!lowModeHandled) {
    Serial.println("Switched to LOW mode");
    activeMode = "Low";
    lowModeHandled = true;

    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    esp_sleep_enable_ext0_wakeup(WAKEUP_PIN, 0); 

    Serial.println("Entering deep sleep. Press button to wake");
    delay(100);
    rtc_gpio_pullup_en(WAKEUP_PIN);
    esp_deep_sleep_start();
  }
}


void handleInitialState() {
  unsigned long now = millis();
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting...");
  }

  if (now - lastPollTime >= pollInterval) {
    lastPollTime = now;

    HTTPClient checkinHttp;
    checkinHttp.begin(API_BASE_URL "/checkin");
    checkinHttp.POST("");
    checkinHttp.end();

    HTTPClient sleepHttp;
    sleepHttp.begin(API_BASE_URL "/sleep");
    int sleepCode = sleepHttp.GET();
    if(sleepCode == 200){
      String payload = sleepHttp.getString();
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, payload);
      if(!error){
        sleepState = doc["sleep"].as<String>();
      }
    }
    sleepHttp.end();

    HTTPClient httpMode;
    httpMode.begin(API_BASE_URL "/mode");
    int httpCode = httpMode.GET();

    if (httpCode == 200) {
      String payload = httpMode.getString();
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        String mode = doc["mode"];
        if (mode == "Full" || mode == "Half" || mode == "Low") {
          currentMode = mode;
        }
      }
    }
    httpMode.end();

    HTTPClient httpCommand;
    httpCommand.begin(API_BASE_URL "/command");
    int CmdCode = httpCommand.GET();

    if (CmdCode == 200) {
      String command = httpCommand.getString();

      if (command == "measure_heartbeat") {
        measureHeartbeat();
        initialStartTime = millis();
      }
      else if(command == "measure_temperature"){
        measureTemperature();
        initialStartTime = millis();
      }
    }
    httpCommand.end();
  }

  if (millis() - initialStartTime > 5000) {
    Serial.println("Timeout, switching to working mode: " + currentMode);
    if (sleepState == "ON"){
      if(currentMode == "Full") previousState = Full;
      else if (currentMode == "Half") previousState = Half;
      else previousState = Low;
      fullyDisconnectWiFi();
      state = Sleep;
    }
    else{
      if (currentMode == "Full") {
        state = Full;
      }
      else if (currentMode == "Half") {
        WiFi.setSleep(true);
        state = Half;
      }
      else state = Low;
    }
  }
}


void loop() {
  if (state == Initial) {
    handleInitialState();
  }
  else if (state == Full) {
    loopFullMode();
  }
  else if (state == Half) {
    loopHalfMode();
  }
  else if (state == Low) {
    loopLowMode();
  }
  else if (state == Sleep){
    loopSleepMode();
  }
}
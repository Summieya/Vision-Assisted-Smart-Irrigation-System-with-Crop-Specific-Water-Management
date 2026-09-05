
#define BLYNK_TEMPLATE_ID "TMPL3P5JEJ_L9"
#define BLYNK_TEMPLATE_NAME "Smart Irrigation System"
#define BLYNK_AUTH_TOKEN "nWO5YpOEGin4XjzT5N7UqEu-U4zgATGj"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <Firebase_ESP_Client.h>

// =====================================================
// WIFI
// =====================================================

char ssid[] = "POCO M5";
char pass[] = "mrdj62uai5zwth9";

// =====================================================
// FIREBASE CREDENTIALS
// =====================================================

#define API_KEY "AIzaSyAJGvnZqoskfs8LmicZDLWPdChQ9t4neWE"
#define DATABASE_URL "crop-irrigation-6b5f2-default-rtdb.asia-southeast1.firebasedatabase.app"

#define USER_EMAIL "adityadas0306@gmail.com"
#define USER_PASSWORD "keanureeves30"

// =====================================================
// PIN DEFINITIONS
// =====================================================

#define SOIL_PIN       1
#define RAIN_PIN       2
#define DHT_PIN        4
#define PIR_PIN        5
#define RELAY_K1       6

#define DHT_TYPE DHT11

// =====================================================
// DHT11
// =====================================================

DHT dht(DHT_PIN, DHT_TYPE);

// =====================================================
// FIREBASE OBJECTS
// =====================================================

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ADD THIS
void tokenStatusCallback(TokenInfo info)
{
  Serial.println();
  Serial.println("========== FIREBASE TOKEN STATUS ==========");

  Serial.print("Status: ");
  Serial.println(info.status);

  Serial.print("Type: ");
  Serial.println(info.type);

  if (info.status == token_status_ready)
  {
    Serial.println("Firebase token is ready.");
  }
  else if (info.status == token_status_error)
  {
    Serial.println("Firebase token error.");
  }

  Serial.println("===========================================");
}

// =====================================================
// BLYNK TIMER
// =====================================================

BlynkTimer timer;

// =====================================================
// RELAY
// =====================================================

// K1 relay is ACTIVE LOW
#define RELAY_ON  LOW
#define RELAY_OFF HIGH

bool pumpState = false;
bool manualMode = false;  // true if controlled manually, false if auto from crop detection

// =====================================================
// SOIL CALIBRATION
// =====================================================

int drySoil = 3500;
int wetSoil = 1200;

// =====================================================
// CROP DETECTION
// =====================================================

String currentCrop = "none";
float lastConfidence = 0.0;
int moistureThreshold = 50;   // default fallback threshold (%)

unsigned long lastPollTime = 0;
const unsigned long POLL_INTERVAL = 8000;   // poll Firebase every 8 sec

// =====================================================
// READ SOIL
// =====================================================

int readSoil() {

  long total = 0;

  for (int i = 0; i < 10; i++) {

    total += analogRead(SOIL_PIN);

    delay(10);
  }

  return total / 10;
}

// =====================================================
// READ RAIN
// =====================================================

int readRain() {

  long total = 0;

  for (int i = 0; i < 10; i++) {

    total += analogRead(RAIN_PIN);

    delay(10);
  }

  return total / 10;
}

// =====================================================
// PUMP ON
// =====================================================

void pumpOn() {

  digitalWrite(RELAY_K1, RELAY_ON);

  pumpState = true;

  Serial.println();
  Serial.println(">>> K1 ON");
  Serial.println(">>> PUMP ON");

  // Update Blynk pump status
  Blynk.virtualWrite(V4, 1);

  // Update Blynk switch
  Blynk.virtualWrite(V5, 1);
}

// =====================================================
// PUMP OFF
// =====================================================

void pumpOff() {

  digitalWrite(RELAY_K1, RELAY_OFF);

  pumpState = false;

  Serial.println();
  Serial.println(">>> K1 OFF");
  Serial.println(">>> PUMP OFF");

  // Update Blynk pump status
  Blynk.virtualWrite(V4, 0);

  // Update Blynk switch
  Blynk.virtualWrite(V5, 0);
}

// =====================================================
// BLYNK PUMP SWITCH
// V5 = PUMP ON/OFF SWITCH (MANUAL CONTROL)
// =====================================================

BLYNK_WRITE(V5) {

  int value = param.asInt();

  Serial.println();

  Serial.print("Blynk Pump Switch: ");

  manualMode = true;  // User switched to manual mode

  if (value == 1) {

    Serial.println("ON (MANUAL)");

    pumpOn();

  }

  else {

    Serial.println("OFF (MANUAL)");

    pumpOff();

  }
}

// =====================================================
// BLYNK CONNECTED
// =====================================================

BLYNK_CONNECTED() {

  Serial.println();
  Serial.println(">>> BLYNK CONNECTED");

  // Make Blynk display the actual pump state
  Blynk.virtualWrite(V4, pumpState ? 1 : 0);

  Blynk.virtualWrite(V5, pumpState ? 1 : 0);
}

// =====================================================
// POLL FIREBASE FOR CROP RESULT
// =================================================

void pollCropResult()
{
  Serial.println();
  Serial.println("========== FIREBASE POLL ==========");

  Serial.print("Firebase ready: ");
  Serial.println(Firebase.ready() ? "TRUE" : "FALSE");

  if (!Firebase.ready())
  {
    Serial.println("Firebase is NOT ready.");
    Serial.println("Waiting for Firebase authentication...");
    Serial.println("==================================");
    return;
  }

  Serial.println("Firebase is ready.");
  Serial.println("Reading /crop_result ...");

  if (Firebase.RTDB.getJSON(&fbdo, "/crop_result"))
  {
    Serial.println("Firebase read SUCCESS!");

    FirebaseJson *json = fbdo.jsonObjectPtr();
    FirebaseJsonData result;

    json->get(result, "crop");
    String crop = result.stringValue;

    json->get(result, "confidence");
    float confidence = result.floatValue;

    Serial.print("Crop received: ");
    Serial.println(crop);

    Serial.print("Confidence received: ");
    Serial.println(confidence);

    if (confidence > 0.70 && crop != "none" && crop.length() > 0)
    {
      currentCrop = crop;
      lastConfidence = confidence;

      updateMoistureThreshold(currentCrop);
      manualMode = false;

      Serial.println(">>> CROP UPDATED SUCCESSFULLY <<<");
    }
  }
  else
  {
    Serial.print("Firebase read FAILED: ");
    Serial.println(fbdo.errorReason());
  }

  Serial.println("==================================");
}

// =====================================================
// MAP CROP TO MOISTURE THRESHOLD
// =====================================================

void updateMoistureThreshold(String crop) {

  if (crop == "paddy") {
    moistureThreshold = 70;
  } else if (crop == "wheat") {
    moistureThreshold = 40;
  } else if (crop == "maize") {
    moistureThreshold = 50;
  } else if (crop == "sugarcane") {
    moistureThreshold = 60;
  } else {
    moistureThreshold = 50; // fallback
  }

  Serial.println("Updated threshold for " + crop + ": " + String(moistureThreshold) + "%");
}

// =====================================================
// READ ALL SENSORS & CONTROL PUMP AUTOMATICALLY
// =====================================================

void readAllSensors() {

  // -------------------------
  // SOIL
  // -------------------------

  int soilValue = readSoil();

  int moisturePercent = map(
    soilValue,
    drySoil,
    wetSoil,
    0,
    100
  );

  moisturePercent = constrain(
    moisturePercent,
    0,
    100
  );

  // -------------------------
  // RAIN
  // -------------------------

  int rainValue = readRain();

  // -------------------------
  // PIR
  // -------------------------

  int pirValue = digitalRead(PIR_PIN);

  // -------------------------
  // DHT11
  // -------------------------

  float temperature = dht.readTemperature();

  float humidity = dht.readHumidity();

  // =================================================
  // AUTOMATIC PUMP CONTROL (if not in manual mode)
  // =================================================

  if (!manualMode) {

    if (moisturePercent < moistureThreshold) {
      pumpOn();
    } else {
      pumpOff();
    }

  }

  // =================================================
  // SERIAL MONITOR
  // =================================================

  Serial.println();
  Serial.println("------------------------------------------");

  Serial.print("Crop Detection : ");
  Serial.print(currentCrop);
  Serial.print(" (conf: ");
  Serial.print(lastConfidence * 100);
  Serial.println("%)");

  Serial.print("Soil ADC       : ");
  Serial.println(soilValue);

  Serial.print("Soil Moisture  : ");
  Serial.print(moisturePercent);
  Serial.println(" %");

  Serial.print("Threshold      : ");
  Serial.print(moistureThreshold);
  Serial.println(" %");

  Serial.print("Rain ADC       : ");
  Serial.println(rainValue);

  Serial.print("PIR            : ");

  if (pirValue == HIGH) {

    Serial.println("MOTION");

  }

  else {

    Serial.println("NO MOTION");

  }

  // -------------------------
  // TEMPERATURE
  // -------------------------

  if (isnan(temperature)) {

    Serial.println("Temperature    : DHT11 ERROR");

  }

  else {

    Serial.print("Temperature    : ");
    Serial.print(temperature);
    Serial.println(" C");

  }

  // -------------------------
  // HUMIDITY
  // -------------------------

  if (isnan(humidity)) {

    Serial.println("Humidity       : DHT11 ERROR");

  }

  else {

    Serial.print("Humidity       : ");
    Serial.print(humidity);
    Serial.println(" %");

  }

  // -------------------------
  // PUMP & MODE
  // -------------------------

  Serial.print("Pump           : ");

  if (pumpState) {
    Serial.println("ON");
  } else {
    Serial.println("OFF");
  }

  Serial.print("Mode           : ");
  Serial.println(manualMode ? "MANUAL" : "AUTOMATIC");

  Serial.println("------------------------------------------");

  // =================================================
  // SEND DATA TO BLYNK
  // =================================================

  // Soil moisture
  Blynk.virtualWrite(V0, moisturePercent);

  // Temperature
  if (!isnan(temperature)) {
    Blynk.virtualWrite(V1, temperature);
  }

  // Humidity
  if (!isnan(humidity)) {
    Blynk.virtualWrite(V2, humidity);
  }

  // PIR
  Blynk.virtualWrite(V3, pirValue);

  // Pump status
  Blynk.virtualWrite(V4, pumpState ? 1 : 0);
}

// =====================================================
// SERIAL COMMANDS
// =====================================================

void checkSerialCommand() {

  if (Serial.available()) {

    char command = Serial.read();

    // Convert lowercase to uppercase
    if (command >= 'a' && command <= 'z') {
      command = command - 32;
    }

    // ================================================
    // P = PUMP ON (MANUAL)
    // ================================================

    if (command == 'P') {

      Serial.println();
      Serial.println("Serial Command: P");

      manualMode = true;
      pumpOn();

    }

    // ================================================
    // O = PUMP OFF (MANUAL)
    // ================================================

    else if (command == 'O') {

      Serial.println();
      Serial.println("Serial Command: O");

      manualMode = true;
      pumpOff();

    }

    // ================================================
    // R = READ SENSORS
    // ================================================

    else if (command == 'R') {

      Serial.println();
      Serial.println("Serial Command: R");

      readAllSensors();

    }

    // ================================================
    // A = SWITCH TO AUTOMATIC MODE
    // ================================================

    else if (command == 'A') {

      Serial.println();
      Serial.println("Serial Command: A - Switched to AUTOMATIC mode");

      manualMode = false;

    }
  }
}

// =====================================================
// SETUP
// =====================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Pin Modes
  pinMode(PIR_PIN, INPUT);
  pinMode(RELAY_K1, OUTPUT);
  digitalWrite(RELAY_K1, RELAY_OFF);
  pumpState = false;

  // ADC & Sensors
  analogReadResolution(12);
  dht.begin();

  Serial.println("\n==========================================");
  Serial.println("   SMART IRRIGATION + CROP DETECTION");
  Serial.println("==========================================");

  // 1. CONNECT TO WIFI & BLYNK FIRST
  Serial.println("Connecting to WiFi & Blynk...");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // 2. SYNCHRONIZE ESP32 TIME (CRITICAL FOR FIREBASE TOKEN)
  Serial.println("Synchronizing NTP Time...");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  time_t now = time(nullptr);
  while (now < 24 * 3600) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("\nTime synchronized successfully!");

  // 3. FIREBASE SETUP
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign token response handler
  config.token_status_callback = tokenStatusCallback;
  
  // Set buffer size and response timeouts for network stability
  config.timeout.serverResponse = 10 * 1000;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Firebase.setDoubleDigits(5);

  Serial.println("Firebase initialized.");

  // Timers
  timer.setInterval(3000L, readAllSensors);
}
// =====================================================
// MAIN LOOP
// =====================================================

void loop() {

  // Blynk communication
  Blynk.run();

  // Timers
  timer.run();

  // Poll Firebase for crop result (non-blocking)
  unsigned long now = millis();
  if (now - lastPollTime >= POLL_INTERVAL) {
    lastPollTime = now;
    
    // DEBUG: Check Firebase status
    Serial.print("Firebase.ready() = ");
    Serial.println(Firebase.ready() ? "TRUE" : "FALSE");
    
    pollCropResult();
  }

  // Serial commands
  checkSerialCommand();
}

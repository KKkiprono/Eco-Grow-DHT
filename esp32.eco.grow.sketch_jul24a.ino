#define BLYNK_TEMPLATE_ID "TMPL2eJc53B5n"
#define BLYNK_TEMPLATE_NAME "Temperature and Humidity Monitoring"

#include <WiFi.h> 
#include <BlynkSimpleEsp32.h> 
#include <DHT.h> // 
#include <HTTPClient.h>

// WiFi Credentials
char ssid[] = "TURBO SCHOOL";     
char pass[] = "@2024Turbo";

// Blynk Credentials
const char* AF_TALKING_USERNAME = "sandbox"; 
const char* AF_TALKING_API_KEY = "atsk_acd781eca29f781f2a2314b7f469f28c51818efac203e38aefbd123cb3b31ba81447d598";  
const char* RECIPIENT_PHONE_NUMBER = "+254746456965";
char auth[] = "-vKnrJNGdDm0EjvaVhYBSQ-e76cgwPHV";

// Sensor Pin Definitions 
#define DHTPIN 4        
#define DHTTYPE DHT11   
#define LDR_ANALOG_PIN 34

// Actuator Pin Definitions 
#define RED_LED_PIN 16    
#define YELLOW_LED_PIN 17
#define BLUE_LED_PIN 5   
#define FAN_RELAY_PIN 18 

// Sensor Objects 
DHT dht(DHTPIN, DHTTYPE);

// Global Variables for Sensor Readings 
float temperature = 0;
float humidity = 0;
int ldrValue = 0;

// Thresholds for Automation 
const float TEMP_HIGH_THRESHOLD = 30.0; 
const float HUMIDITY_HIGH_THRESHOLD = 80.0; 
const int LIGHT_LOW_THRESHOLD = 300;

// Blynk Virtual Pin Definitions 
#define VIRTUAL_PIN_TEMP V0
#define VIRTUAL_PIN_HUMIDITY V1
#define VIRTUAL_PIN_LIGHT V2
#define VIRTUAL_PIN_RED_LED V3
#define VIRTUAL_PIN_YELLOW_LED V4
#define VIRTUAL_PIN_BLUE_LED V5
#define VIRTUAL_PIN_FAN_STATUS V6
#define VIRTUAL_PIN_SMS_ALERT V7 // For triggering SMS from Blynk app (optional)

// Timers for periodic tasks
BlynkTimer timer;

// Function Prototypes
void readAndSendSensorData();
void applyAutomationLogic();
void controlFan(bool state);
void sendSmsAlert(String message);

void setup() {
  // Initialize Serial Communication for debugging
  Serial.begin(115200);

  // Initialize DHT sensor
  dht.begin();

  // Set LED pins as OUTPUT
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);
  pinMode(FAN_RELAY_PIN, OUTPUT);
  digitalWrite(FAN_RELAY_PIN, LOW);

  // Connect to Wi-Fi and Blynk
  Blynk.begin(auth, ssid, pass);
  delay(2000);
}

void loop() {
  Blynk.run(); 
  timer.run();
}

// Function to Read Sensor Data and Send to Blynk 
void readAndSendSensorData() {
  // Read Temperature and Humidity
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  // LDR values typically range from 0 (very bright) to 4095 (very dark) on ESP32 ADC
  ldrValue = analogRead(LDR_ANALOG_PIN);

  // Check if any reads failed and exit early (DHT returns NaN for failure)
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" °C, Humidity: ");
  Serial.print(humidity);
  Serial.print(" %, Light (LDR): ");
  Serial.println(ldrValue);

  // Send data to Blynk Virtual Pins
  Blynk.virtualWrite(VIRTUAL_PIN_TEMP, temperature);
  Blynk.virtualWrite(VIRTUAL_PIN_HUMIDITY, humidity);
  Blynk.virtualWrite(VIRTUAL_PIN_LIGHT, ldrValue);
}

// Function to Apply Automation Logic
void applyAutomationLogic() {
  String alertMessage = "";
  bool highTempAlert = false;
  bool highHumidityAlert = false;
  bool lowLightAlert = false;

  // Temperature and Red LED logic 
  if (temperature > TEMP_HIGH_THRESHOLD) {
    digitalWrite(RED_LED_PIN, HIGH); 
    Blynk.virtualWrite(VIRTUAL_PIN_RED_LED, 255);
    highTempAlert = true;
    if (digitalRead(FAN_RELAY_PIN) == LOW) { 
        controlFan(true); // Turn Fan ON
        alertMessage += "High Temp Alert! Fan activated. ";
    }
  } else {
    digitalWrite(RED_LED_PIN, LOW); 
    Blynk.virtualWrite(VIRTUAL_PIN_RED_LED, 0);
  }

  // Humidity and Blue LED logic 
  if (humidity > HUMIDITY_HIGH_THRESHOLD) {
    digitalWrite(BLUE_LED_PIN, HIGH);
    Blynk.virtualWrite(VIRTUAL_PIN_BLUE_LED, 255); 
    highHumidityAlert = true;
    if (digitalRead(FAN_RELAY_PIN) == LOW) { 
        controlFan(true); // Turn Fan ON
        alertMessage += "High Humidity Alert! Fan activated. ";
    }
  } else {
    digitalWrite(BLUE_LED_PIN, LOW); 
    Blynk.virtualWrite(VIRTUAL_PIN_BLUE_LED, 0); 
  }

  // Fan Control Logic
  if (!highTempAlert && !highHumidityAlert && digitalRead(FAN_RELAY_PIN) == HIGH) { // HIGH assumed ON
      controlFan(false); // Turn Fan OFF
      alertMessage += "Climate normalized. Fan deactivated. ";
  }

  // Light and Yellow LED logic (Grow Light) 
  if (ldrValue > LIGHT_LOW_THRESHOLD) { // LDR value is high, meaning light is low
    digitalWrite(YELLOW_LED_PIN, HIGH); 
    Blynk.virtualWrite(VIRTUAL_PIN_YELLOW_LED, 255);
    lowLightAlert = true;
    alertMessage += "Low Light Alert! Grow light activated. ";
  } else {
    digitalWrite(YELLOW_LED_PIN, LOW); 
    Blynk.virtualWrite(VIRTUAL_PIN_YELLOW_LED, 0);
  }

  // Send SMS Alert if 
  if (alertMessage.length() > 0) {
    sendSmsAlert(alertMessage);
  }
}

// Function to Control Fan 
void controlFan(bool state) {
  if (state) {
    digitalWrite(FAN_RELAY_PIN, HIGH); 
    Blynk.virtualWrite(VIRTUAL_PIN_FAN_STATUS, 1); 
    Serial.println("Fan ON");
  } else {
    digitalWrite(FAN_RELAY_PIN, LOW); 
    Blynk.virtualWrite(VIRTUAL_PIN_FAN_STATUS, 0); 
    Serial.println("Fan OFF");
  }
}

// Blynk Write Event for Fan Control from Dashboard 
BLYNK_WRITE(VIRTUAL_PIN_FAN_STATUS) {
  int pinValue = param.asInt(); // Get value from Blynk widget (0 or 1)
  if (pinValue == 1) {
    controlFan(true);
    Serial.println("Blynk: Fan ON command received.");
  } else {
    controlFan(false);
    Serial.println("Blynk: Fan OFF command received.");
  }
}

// Blynk Write Event for SMS Alert from Dashboard
BLYNK_WRITE(VIRTUAL_PIN_SMS_ALERT) {
  if (param.asInt() == 1) { // If button widget is pressed
    sendSmsAlert("Manual alert from Blynk dashboard: Check greenhouse conditions!");
    Blynk.virtualWrite(VIRTUAL_PIN_SMS_ALERT, 0);
  }
}
// Function to Send SMS Alert via Africa's Talking API 
void sendSmsAlert(String message) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, cannot send SMS.");
    return;
  }
  HTTPClient http;
  String url = "https://api.africastalking.com/version1/messaging";

  http.begin(url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.addHeader("Accept", "application/json");
  http.addHeader("apikey", AF_TALKING_API_KEY);

  String httpRequestData = "username=" + String(AF_TALKING_USERNAME) +
                           "&to=" + String(RECIPIENT_PHONE_NUMBER) +
                           "&message=" + message;

  Serial.print("Sending SMS: ");
  Serial.println(httpRequestData);

  int httpResponseCode = http.POST(httpRequestData);

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String response = http.getString();
    Serial.println(response);
    // parse the JSON response to confirm success
  } else {
    Serial.print("Error sending SMS. HTTP Error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

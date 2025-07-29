#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <TinyGPSPlus.h>
#include <LiquidCrystal.h> 
#include <DHT.h>           

// Define the pin for the DHT11 sensor
#define DHTPIN 7
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor

// Parallel LCD Display Configuration
// LiquidCrystal lcd(rs, enable, d4, d5, d6, d7);
// RS (Pin 4) -> Arduino Pin 12
// E (Pin 6)  -> Arduino Pin 11
// D4 (Pin 11) -> Arduino Pin 5
// D5 (Pin 12) -> Arduino Pin 4
// D6 (Pin 13) -> Arduino Pin 3
// D7 (Pin 14) -> Arduino Pin 2
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// LDR (Light Dependent Resistor)
const int ldrPin = A0; 

// Fan Control
const int fanPin = 9;

// LED Indicators for Temperature
const int redLED = 10;   
const int yellowLED = 8; 
const int blueLED = 6; 

// Gas Sensor
const int gasSensorPin = A1; 
const int gasAlertLED = 13;
 
// threshold for gas detection 
const int gasThreshold = 300;
                            
// Variables to manage SMS alert frequency
long lastSMSMillis = 0;      
const long smsInterval = 300000; // 5 minutes (300,000 milliseconds) delay between SMS alerts

void setup()
{
  Serial.begin(9600); 
  Serial.println("DIY Smart Environment Monitor with Arduino!");

  dht.begin();   
  lcd.begin(16, 2); //16 columns and 2 rows

  pinMode(fanPin, OUTPUT);
  pinMode(redLED, OUTPUT);
  pinMode(yellowLED, OUTPUT);
  pinMode(blueLED, OUTPUT);
  pinMode(gasAlertLED, OUTPUT);
  pinMode(ldrPin, INPUT);
  pinMode(gasSensorPin, INPUT);

  // all LEDs and the fan are off at startup
  digitalWrite(redLED, LOW);
  digitalWrite(yellowLED, LOW);
  digitalWrite(blueLED, LOW);
  digitalWrite(gasAlertLED, LOW);
  digitalWrite(fanPin, LOW); // Fan off initially

  // LCD 
  lcd.setCursor(0, 0); // column 0, row 0
  lcd.print("Environment");
  lcd.setCursor(0, 1); // cursor to column 0, row 1 
  lcd.print("Monitor Online!");
  delay(2000);   
  lcd.clear();
}

void loop()
{
  lcd.clear(); 
  delay(500);

  // Read Sensor Values
  float humidity = dht.readHumidity();      
  float temperature = dht.readTemperature(); 
  int ldrValue = analogRead(ldrPin);       
  int gasSensorValue = analogRead(gasSensorPin);

  // DHT Sensor Error Handling 
  if (isnan(humidity) || isnan(temperature))
  {
    Serial.println("Error: Failed to read from DHT sensor! Check your wiring.");
    // Display error message on LCD
    lcd.setCursor(0, 0);
    lcd.print("DHT Error!");
    lcd.setCursor(0, 1);
    lcd.print("Check Wiring.");
    delay(3000); 
    return;  
  }

  //  Print Readings to Serial Monitor 
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println("%");

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println("C");

  Serial.print("Light (LDR): ");
  Serial.print(ldrValue);
  Serial.println(" (0-1023)");

  Serial.print("Gas Sensor: ");
  Serial.print(gasSensorValue);
  Serial.println(" (0-1023)");

  //Display Readings on LCD 
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(temperature, 1); 
  lcd.print((char)223);     
  lcd.print("C H:");
  lcd.print(humidity, 1); 
  lcd.print("%");

  lcd.setCursor(0, 1); // Set cursor to bottom row
  lcd.print("L:");
  // Map LDR value (0-1023) to a percentage (0-100) for ease in understanding
  lcd.print(map(ldrValue, 0, 1023, 0, 100));
  lcd.print("% G:");
  lcd.print(gasSensorValue); // Display raw gas sensor value

  // Fan and LED Control based on Temperature 
  if (temperature >= 30.0)
  { // High temperature: 30 degrees Celsius or above
    digitalWrite(fanPin, HIGH);   
    digitalWrite(redLED, HIGH);   
    digitalWrite(yellowLED, LOW); 
    digitalWrite(blueLED, LOW);   
    Serial.println("Status: High Temp - Fan ON, Red LED ON");
    lcd.setCursor(14, 0); // Display 'F' (Fan) indicator on LCD
    lcd.print("F");
  }
  else if (temperature >= 15.0 && temperature < 30.0)
  { // Normal temperature: between 15 and 30 degrees Celsius
    digitalWrite(fanPin, LOW);    
    digitalWrite(redLED, LOW);   
    digitalWrite(yellowLED, HIGH); 
    digitalWrite(blueLED, LOW);   
    Serial.println("Status: Normal Temp - Fan OFF, Yellow LED ON");
    lcd.setCursor(14, 0); // Clear fan indicator on LCD
    lcd.print(" ");
  }
  else
  { // Cold temperature: less than 15 degrees Celsius
    digitalWrite(fanPin, LOW);    
    digitalWrite(redLED, LOW);    
    digitalWrite(yellowLED, LOW); 
    digitalWrite(blueLED, HIGH);  
    Serial.println("Status: Cold Temp - Fan OFF, Blue LED ON");
    lcd.setCursor(14, 0); // Clear fan indicator on LCD
    lcd.print(" ");
  }

  // Gas Sensor Detection and Alert 
  if (gasSensorValue > gasThreshold)
  {
    digitalWrite(gasAlertLED, HIGH); // Turn on gas alert LED
    Serial.println("!!! GAS DETECTED !!! - ALERT");
    lcd.setCursor(15, 1); // Display 'G' (Gas) alert indicator on LCD
    lcd.print("G");

    // Send SMS alert via Africa's Talking
    if (millis() - lastSMSMillis >= smsInterval)
    {
      sendSMSAlert("Gas detected! Please check your environment immediately.");
      lastSMSMillis = millis();
    }
  }
  else
  {
    digitalWrite(gasAlertLED, LOW); 
    lcd.setCursor(15, 1); 
    lcd.print(" ");
  }

  delay(2000);
}

void sendSMSAlert(String message) { 
  Serial.print("Attempting to send SMS: ");
  Serial.println(message);
 const char* AF_TALKING_USERNAME = "sandbox"; 
 const char* AF_TALKING_API_KEY = "atsk_acd781eca29f781f2a2314b7f469f28c51818efac203e38aefbd123cb3b31ba81447d598";  
 const char* RECIPIENT_PHONE_NUMBER = "+254746456965";

  SoftwareSerial gsmSerial(2, 3); // RX, TX pins for GSM module
  // gsmSerial.begin(9600); // Or the baud rate of your GSM module

  // In sendSMSAlert function:
  // gsmSerial.println("AT+CMGF=1"); // Set SMS to text mode
  // delay(100);
  // gsmSerial.print("AT+CMGS=\"+254746456965\"\r");
  // delay(100);
  // gsmSerial.print(message); // The actual message
  // delay(100);
  // gsmSerial.write(26); // ASCII code for CTRL+Z, signifies end of message
  // Serial.println("SMS command sent to GSM module.");
  

  Serial.println("SMS function called. Remember to implement actual GSM module code here.");
  Serial.println("You'll need to connect a GSM module and use its library to interact with Africa's Talking API.");
}

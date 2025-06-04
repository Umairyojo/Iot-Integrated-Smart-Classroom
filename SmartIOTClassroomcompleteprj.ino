#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define RST_PIN 2
#define SS_PIN 5

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C

#define BUZZER_PIN 4  // Define the buzzer pin
#define MQ135_PIN 32  // Define the MQ135 analog pin

// Wi-Fi credentials
const char* ssid = "ACT102692121332";
const char* password = "32615662";

// Google Apps Script Web App URL
const char* googleScriptUrl = "https://script.google.com/macros/s/AKfycbyZ6CjL9TuUVEjmNy8ZMGQAF8EzBiSZ0PPyBBQK8g6y69mTgOsM50ubeh6qhpNnO7wR/exec";

MFRC522 mfrc522(SS_PIN, RST_PIN);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

struct Card {
  String uid;
  String name;
};

Card cards[] = {
  {"7A D7 97 3F", "Srivani N Logged IN"},
  {"D3 88 2C DA", "Umair Ahmed Logged IN"}
};

void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  pinMode(BUZZER_PIN, OUTPUT);  // Initialize the buzzer pin as output
  digitalWrite(BUZZER_PIN, LOW);  // Ensure buzzer is off initially

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  display.display();
  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("RFID Attendance System");
  display.display();
}

void loop() {
  // Read MQ135 sensor value
  int mq135Value = analogRead(MQ135_PIN);
  String airQuality = (mq135Value < 2200) ? "Good Quality" : "Bad Quality";  // Adjust threshold as needed

  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String content= "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
      content.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    content.toUpperCase();
    content.trim();  // Trim any leading or trailing spaces

    Serial.println("Scanned UID: " + content);  // Print the scanned UID to Serial Monitor

    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Card UID:");
    display.println(content);

    String name = "Unknown";
    for (int i = 0; i < sizeof(cards) / sizeof(cards[0]); i++) {
      String storedUID = cards[i].uid;
      storedUID.trim();  // Trim any leading or trailing spaces

      if (content == storedUID) {
        name = cards[i].name;
        break;
      }
    }

    display.println("Name: " + name);
    display.println("Air Quality: " + airQuality);  // Display air quality status
    // Display MQ135 raw value at the bottom
    display.println("MQ135: " + String(mq135Value)); // Line 5: Raw sensor value
    display.display();

    // Activate buzzer for successful scan
    digitalWrite(BUZZER_PIN, HIGH);  // Turn on the buzzer
    delay(1000);  // Short beep (100ms)
    digitalWrite(BUZZER_PIN, LOW);  // Turn off the buzzer

    // Send data to Google Sheets
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(googleScriptUrl);
      http.addHeader("Content-Type", "application/json");

      // Create JSON payload
      String payload = "{\"uid\":\"" + content + "\",\"name\":\"" + name + "\",\"airQuality\":\"" + airQuality + "\"}";

      // Send POST request
      int httpResponseCode = http.POST(payload);
      if (httpResponseCode > 0) {
        Serial.println("Data sent to Google Sheets. Response code: " + String(httpResponseCode));
      } else {
        Serial.println("Error sending data to Google Sheets.");
      }
      http.end();
    }

    delay(2000);  // Delay to display the card details
    
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
  }
}
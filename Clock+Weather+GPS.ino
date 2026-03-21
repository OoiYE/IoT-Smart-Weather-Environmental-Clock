#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_SHT31.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TinyGPSPlus.h> 
#include <time.h> 

// ===== GPS Setup =====
#define GPS_RX_PIN 20 
#define GPS_TX_PIN 21 
HardwareSerial gpsSerial(1); 
TinyGPSPlus gps;

double currentLat = 0.0; 
double currentLon = 0.0;
bool hasGpsFix = false;
bool firstWeatherFetched = false;

// Custom 16x16 Weather Bitmaps
const unsigned char icon_sun [] PROGMEM = {
  0x00, 0x00, 0x01, 0x80, 0x21, 0x84, 0x41, 0x82, 0x03, 0xc0, 0x07, 0xe0, 0x0f, 0xf0, 0x0f, 0xf0, 
  0x0f, 0xf0, 0x0f, 0xf0, 0x07, 0xe0, 0x03, 0xc0, 0x41, 0x82, 0x21, 0x84, 0x01, 0x80, 0x00, 0x00
};

const unsigned char icon_cloud [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x1f, 0x80, 0x3f, 0xc0, 0x7f, 0xe0, 
  0xff, 0xf0, 0xff, 0xf0, 0xff, 0xf8, 0xff, 0xfc, 0xff, 0xfe, 0x7f, 0xfe, 0x00, 0x00, 0x00, 0x00
};

const unsigned char icon_rain [] PROGMEM = {
  0x00, 0x00, 0x06, 0x00, 0x1f, 0x80, 0x3f, 0xc0, 0x7f, 0xe0, 0xff, 0xf0, 0xff, 0xf0, 0xff, 0xf0, 
  0x7f, 0xe0, 0x00, 0x00, 0x24, 0x48, 0x24, 0x48, 0x12, 0x24, 0x12, 0x24, 0x00, 0x00, 0x00, 0x00
};

// ===== WiFi & API info =====
const char* ssid     = "ax6000";
const char* password = "weihanqi";
String apiKey = "54c7fd807ac4c2f5193467e9ca9520cd";
String city = "Searching GPS..."; 

// ===== Globals =====
String weatherDesc = "--";
float outdoorTemp = 0;
unsigned long lastWeatherUpdate = 0;
const long weatherInterval = 600000; 

// 3-Page logic variables
int currentPage = 0; // 0=Clock/Date, 1=Indoor, 2=Weather
unsigned long lastPageChange = 0;
const int pageInterval = 5000; // Switches every 5 seconds

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_SHT31 sht31 = Adafruit_SHT31();

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 8 * 3600, 60000); 

#define I2C_SDA 8
#define I2C_SCL 9

void getWeatherData() {
  if (WiFi.status() == WL_CONNECTED && hasGpsFix) {
    HTTPClient http;
    String url = "http://api.openweathermap.org/data/2.5/weather?lat=" + String(currentLat, 4) + "&lon=" + String(currentLon, 4) + "&appid=" + apiKey + "&units=metric";
    
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode > 0) {
      String payload = http.getString();
      StaticJsonDocument<1024> doc;
      deserializeJson(doc, payload);

      outdoorTemp = doc["main"]["temp"];
      const char* desc = doc["weather"][0]["main"]; 
      weatherDesc = String(desc);
      
      if (doc.containsKey("name")) {
        city = doc["name"].as<String>(); 
      }
    }
    http.end();
  }
}

void setup() {
  Serial.begin(9600);
  Wire.begin(I2C_SDA, I2C_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for (;;);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.println("Booting System...");
  display.display();

  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  if (!sht31.begin(0x44)) {
    display.println("SHT31 Error!"); display.display();
    delay(2000); 
  }

  display.println("Connecting WiFi..."); display.display();
  WiFi.begin(ssid, password);
  
  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 20) {
    delay(500);
    display.print("."); display.display();
    wifiAttempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    display.println("\nWiFi Connected!");
    timeClient.begin();
  } else {
    display.println("\nWiFi Timeout.");
  }
  display.display();
  delay(1500); 
}

void loop() {
  timeClient.update();

  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  if (gps.location.isUpdated() && gps.location.isValid()) {
    double newLat = gps.location.lat();
    double newLon = gps.location.lng();
    
    if (newLat != 0.0 && newLon != 0.0) {
      currentLat = newLat;
      currentLon = newLon;
      hasGpsFix = true;

      if (!firstWeatherFetched) {
        getWeatherData();
        firstWeatherFetched = true;
        lastWeatherUpdate = millis();
      }
    }
  }

  if (hasGpsFix && (millis() - lastWeatherUpdate > weatherInterval)) {
    getWeatherData();
    lastWeatherUpdate = millis();
  }

  // Cycles 0 -> 1 -> 2 -> 0 every 5 seconds
  if (millis() - lastPageChange > pageInterval) {
    currentPage++;
    if (currentPage > 2) { currentPage = 0; }
    lastPageChange = millis();
  }

  // ===== DRAWING THE UI =====
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  if (currentPage == 0) {
    // ==========================================
    // PAGE 1: TIME AND DATE
    // ==========================================
    int hours = timeClient.getHours();
    bool isPM = (hours >= 12);
    if (hours > 12) hours -= 12;
    if (hours == 0) hours = 12;

    // Increased memory size to prevent stack overflow crashes
    char timeStr[16]; 
    sprintf(timeStr, "%02d:%02d:%02d", hours, timeClient.getMinutes(), timeClient.getSeconds());

    display.setTextSize(2);
    display.setCursor(12, 12); 
    display.print(timeStr);

    display.setTextSize(1);
    display.setCursor(110, 12);
    display.print(isPM ? "PM" : "AM");

    display.drawLine(0, 32, 128, 32, SSD1306_WHITE);

    // Fixed memory corruption by using the proper time_t format
    time_t rawTime = timeClient.getEpochTime();
    char dateStr[32]; 

    // Only calculate if the timestamp is valid
    if (rawTime > 100000) {
      struct tm *ptm = gmtime(&rawTime);
      sprintf(dateStr, "%02d/%02d/%04d", ptm->tm_mday, ptm->tm_mon + 1, ptm->tm_year + 1900);
    } else {
      sprintf(dateStr, "Syncing Date...");
    }

    display.setTextSize(2);
    
    // Shifted slightly to center the date perfectly
    display.setCursor(5, 42); 
    display.print(dateStr);

  } else if (currentPage == 1) { 

    // ==========================================
    // PAGE 2: INDOOR TEMP & HUMIDITY
    // ==========================================
    display.setTextSize(1);
    display.setCursor(24, 0);
    display.print("INDOOR SENSOR");
    display.drawLine(0, 12, 128, 12, SSD1306_WHITE);

    // Left Side: Temperature
    display.setTextSize(2);
    display.setCursor(10, 25);
    display.print((int)sht31.readTemperature());
    display.setTextSize(2); display.print((char)247); display.print("C");
    display.setCursor(10, 48);
    display.print("Temp");

    // Right Side: Humidity
    display.setTextSize(2);
    display.setCursor(75, 25);
    display.print((int)sht31.readHumidity());
    display.setTextSize(2); display.print("%");
    display.setCursor(75, 48);
    display.print("Hum");

  } else if (currentPage == 2) {
    // ==========================================
    // PAGE 3: OUTDOOR WEATHER
    // ==========================================
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(city);
    
    if (!hasGpsFix && (millis() / 500) % 2 == 0) {
      display.print(" *");
    }

    if (weatherDesc == "Clear") {
      display.drawBitmap(10, 25, icon_sun, 16, 16, SSD1306_WHITE);
    } else if (weatherDesc == "Clouds") {
      display.drawBitmap(10, 25, icon_cloud, 16, 16, SSD1306_WHITE);
    } else if (weatherDesc == "Rain" || weatherDesc == "Drizzle") {
      display.drawBitmap(10, 25, icon_rain, 16, 16, SSD1306_WHITE);
    } else {
      display.setCursor(14, 25);
      display.print("?"); 
    }

    display.setTextSize(3);
    display.setCursor(45, 20);
    display.print((int)outdoorTemp);

    display.setTextSize(1);
    display.print((char)247); 
    display.setTextSize(2);
    display.print("C");

    display.setTextSize(2);
    display.setCursor(45, 50);
    display.print(weatherDesc);
  }
  
  display.display();
}

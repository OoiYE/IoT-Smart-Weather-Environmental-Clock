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

// ===== GPS Setup =====
#define GPS_RX_PIN 20 
#define GPS_TX_PIN 21 
HardwareSerial gpsSerial(1); // Use UART1 for GPS
TinyGPSPlus gps;

// Blank coordinates (No default location)
double currentLat = 0.0; 
double currentLon = 0.0;
bool hasGpsFix = false;
bool firstWeatherFetched = false; // Tracks if we've done our first API call

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
const char* ssid     = "YourWiFi";
const char* password = "YourPassword";
String apiKey = "YourAPIKey";
String city = "Waiting...";

// ===== Globals =====
String weatherDesc = "Loading...";
float outdoorTemp = 0;
unsigned long lastWeatherUpdate = 0;
const long weatherInterval = 600000; // Update every 10 mins

// For page cycling
bool showClockPage = true;
unsigned long lastPageChange = 0;
const int pageInterval = 10000; //Switch every 10 seconds

// ===== OLED setup =====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ===== SHT31 setup =====
Adafruit_SHT31 sht31 = Adafruit_SHT31();

// ===== NTP setup =====
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 8 * 3600, 60000); // GMT+8

// ===== Pins  =====
#define I2C_SDA 8
#define I2C_SCL 9

void getWeatherData() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    // API call uses lat and lon instead of city name
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
      
      // Extract the nearest city name from the coordinates to display on OLED
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

  // Initialize GPS Serial (9600 baud is standard for NEO-6M)
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  // WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");

  // Weather
  getWeatherData();

  // OLED init
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  display.setRotation(1);

  // SHT31 init
  if (!sht31.begin(0x44)) {
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }

  // NTP init
  timeClient.begin();

}

void loop() {
  timeClient.update();

  // Continuously feed GPS data into the TinyGPS++ parser
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  //Check for Satellite Lock
  if (gps.location.isUpdated() && gps.location.isValid()) {
    double newLat = gps.location.lat();
    double newLon = gps.location.lng();
    
    // SAFETY NET: Ignore the premature "0.0" coordinates (Null Island)
    if (newLat != 0.0 && newLon != 0.0) {
      currentLat = newLat;
      currentLon = newLon;
      hasGpsFix = true;

      // Fetch weather immediately the very first time we get a real lock
      if (!firstWeatherFetched) {
        getWeatherData();
        firstWeatherFetched = true;
        lastWeatherUpdate = millis();
      }
    }
  }

  // Update weather on interval
  if (millis() - lastWeatherUpdate > weatherInterval) {
    getWeatherData();
    lastWeatherUpdate = millis();
  }

  // Toggle Page every 5 seconds
  if (millis() - lastPageChange > pageInterval) {
    showClockPage = !showClockPage;
    lastPageChange = millis();
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  if (showClockPage) {
    // ===== PAGE 1: CLOCK & INDOOR SENSOR =====
    int hours = timeClient.getHours();
    bool isPM = (hours >= 12);
    if (hours > 12) hours -= 12;
    if (hours == 0) hours = 12;

    char hStr[3], mStr[3], sStr[3];
    sprintf(hStr, "%02d", hours);
    sprintf(mStr, "%02d", timeClient.getMinutes());
    sprintf(sStr, "%02d", timeClient.getSeconds());

    display.setTextSize(1);
    display.setCursor(2, 0);
    display.println("/////");
    display.setCursor(60, 10);
    display.println(isPM ? "PM" : "AM");
    display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

    display.setTextSize(2);
    display.setCursor(5, 30); display.println(hStr);
    display.setCursor(5, 52); display.println(mStr);
    display.setCursor(5, 74); display.println(sStr);

    display.drawLine(0, 96, 32, 96, SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(5, 103); display.print((int)sht31.readTemperature()); display.print((char)247); display.print("C");
    display.setCursor(5, 116); display.print((int)sht31.readHumidity()); display.print("%");

  } else {
    // ===== PAGE 2: OUTDOOR WEATHER =====
    display.setTextSize(1);
    display.setCursor(2, 5);
    // Show a small asterisk next to the city if it's using live GPS data
    display.println(city);
    if (hasGpsFix) display.print("*");

    display.setCursor(2, 27);
    display.println("OUT:");

    // Big Outdoor Temp
    display.setTextSize(2);
    display.setCursor(2, 45);
    display.print((int)outdoorTemp);
    display.setTextSize(2);
    display.print((char)247); // Degree symbol
    display.setTextSize(2);
    display.print("C");

    display.drawLine(0, 80, 32, 80, SSD1306_WHITE);

    // Weather Description
    display.setTextSize(1);
    display.setCursor(2, 85);
    display.println(weatherDesc);
    
    if (weatherDesc == "Clear") {
      display.drawBitmap(8, 100, icon_sun, 16, 16, WHITE);
    } else if (weatherDesc == "Clouds") {
      display.drawBitmap(8, 100, icon_cloud, 16, 16, WHITE);
    } else if (weatherDesc == "Rain" || weatherDesc == "Drizzle") {
      display.drawBitmap(8, 100, icon_rain, 16, 16, WHITE);
    } else {
      display.setCursor(12, 22);
      display.print("?"); // Fallback for unknown weather
    }
    
  }
  display.display();

}

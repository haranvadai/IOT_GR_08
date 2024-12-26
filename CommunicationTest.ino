#include <WiFi.h>
//#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Arduino_GFX_Library.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <time.h>

#define DHTPIN 4
#define DHTTYPE DHT11
DHT_Unified dht(DHTPIN, DHTTYPE);

#define w 320
#define h 240

// Replace with your Wi-Fi credentials
const char* ssid = "Drorbarak";
const char* password = "DrorBarak";

// API URLs
const char* time_api = "https://timeapi.io/api/time/current/zone?timeZone=Asia%2FJerusalem";
const char* weather_api = "https://goweather.herokuapp.com/weather/HAIFA";

// Define ILI9341 connections and rotation
#define TFT_DC 2
#define TFT_CS 15
#define TFT_RST 0
#define TFT_SCK 14
#define TFT_MOSI 13
#define TFT_MISO 12
#define ROTATION 1

#define TS_CS 21  //7
#define SD_CS 5
#define ROTATION 1

// Initialize the display
Arduino_ESP32SPI bus = Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI, TFT_MISO);
Arduino_ILI9341 tft = Arduino_ILI9341(&bus, TFT_RST);
//TFT_eSPI tft = TFT_eSPI();

int x = 0, y = 0, z = 0;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;

String outsideTemp = "0", insideTemp = "0", apiData = "";

int screen = 0;

#define MAIN 0
#define SECOND 1

void setup() {
  Serial.begin(115200);

  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  pinMode(21, OUTPUT);  //was 7
  digitalWrite(21, HIGH);  //was 7
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  //2 Lines below can be ignored, they're part of a bigger project
  pinMode(17, OUTPUT);
  digitalWrite(17, LOW);


  // Connect to Wi-Fi
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected.");


  // Initialize the display
  tft.begin();
  tft.setRotation(ROTATION);
  tft.fillScreen(0x0000); // Black background

  // Welcome message
  tft.setTextSize(2);
  tft.setTextColor(0xFFFF); // White color
  tft.setCursor(20, 50);
  tft.print("Fetching Data...");

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  xTaskCreate(mainLoop, "mainLoop", 8192, nullptr, 5, nullptr);
  xTaskCreate(updateTemp, "updateTemp", 8192, nullptr, 5, nullptr);
}

void loop() 
{

}

void mainLoop(void*)
{
  while(1)
  {
    if (WiFi.status() == WL_CONNECTED) 
    {
    //String timeData = fetchTime();
    //String tempData = fetchTemp();
    //String apiData = getAPI();
    //String dateData = fetchDate();
    
  
    // Format screen for display
    if(screen == MAIN)
    {
      tft.fillScreen(0x0000); // Black background
      printHeader();
      printComsScreen();
    }
    else if(screen == SECOND)
    {
      tft.fillScreen(0xFFFF);
    }
  } 
  else 
  {
    Serial.println("Wi-Fi disconnected!");
  }

  vTaskDelay(1000/ portTICK_PERIOD_MS); // Update every second
  }
}

void updateTemp(void *)
{
  while(1)
  {
    outsideTemp = readOutsideTemp();
    insideTemp = readInsideTemp();
    apiData = readAPI();
    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
}


float readInsideTemp()
{
  float t;
  // Get temperature event and print its value.
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature) || event.temperature >= 1000 || event.temperature <= -1000) {
    return t;
  }
  else 
  {
    t = event.temperature;
  }

  return t;
}

String readAPI() {

  HTTPClient http;
  String time = "Error";

  
  http.begin("https://haranvadai.github.io/IOT_GR_08/");
  int httpResponseCode = http.GET();
  String payload = "";
  if (httpResponseCode == 200) 
  {
    payload = http.getString();
  } else {
    return apiData;
  }

  http.end();
  return payload;
 }

// String getWeatherData() {
//   HTTPClient http;
//   String temperature = "Error";

//   http.begin(weather_api);
//   int httpResponseCode = http.GET();

//   if (httpResponseCode == 200) {
//     temperature = http.getString();
//     temperature.trim(); // Remove extra spaces or newlines
//     Serial.println(temperature);
//   } else {
//     Serial.println("Failed to fetch weather");
//     Serial.println(httpResponseCode);
//   }

//   http.end();
//   return temperature;
// }

// Draw header bar
void printHeader() {
  tft.fillRect(0, 0, w, 40, 0x07E0); // Green bar
  tft.setTextSize(2);
  tft.setTextColor(0xFFFF); // White text
  tft.setCursor(10, 10);
  tft.print("Haifa Weather");
}

String readOutsideTemp(){

  HTTPClient http;
  String temperature = "Error";

  http.begin(weather_api);
  int httpResponseCode = http.GET();

  if (httpResponseCode == 200) {
    String payload = http.getString();
    int tempStart = payload.indexOf("temperature") + 14;
    int tempEnd = payload.indexOf("°");
    temperature = payload.substring(tempStart, tempEnd) + "C";
    //Serial.println(payload);
  } 
  else 
  {
    return temperature;
  }

  http.end();
  return temperature;
}


void printLocalTime(){
  tft.setTextSize(2);
  tft.setTextColor(0x07FF); // Cyan text
  struct tm timeinfo;

  if(!getLocalTime(&timeinfo))
  {
    // add local time stamp for clock maybe for no wifi.
    Serial.println("Failed to obtain time");
    return;
  }
  tft.fillRect(10, 60, w, 80, 0x0000);

  tft.setCursor(10, 60);
  tft.print("Date: ");
  tft.print(&timeinfo, "%d"); //day
  tft.print(".");
  tft.print(timeinfo.tm_mon); //month
  tft.print(".");
  tft.println(&timeinfo, "%Y"); //year

  tft.setCursor(10, 100);
  tft.print("Time: ");
  tft.print(&timeinfo, "%H"); //hour
  tft.print(":");
  tft.print(&timeinfo, "%M"); //mins
  tft.print(":");
  tft.println(&timeinfo, "%S"); //secs
}

void printTemp()
{
  tft.setTextSize(2);
  tft.setTextColor(0xF800); // Red text

  // Display temperature
  tft.setCursor(10, 140);
  tft.print("Temp Outside: ");
  tft.print(outsideTemp);

  tft.setCursor(10, 180);
  tft.print("Temp inside: ");
  tft.print(insideTemp);
  tft.print(" C");
}

void printAPI()
{
  tft.setTextSize(2);
  tft.setTextColor(0xF800); // Red text

  tft.setCursor(10, 220);
  tft.print("API: ");
  tft.print(apiData);
}

void printComsScreen()
{
  printLocalTime();
  printTemp();
  printAPI();
}
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_BME680.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define SDA_PIN 21
#define SCL_PIN 22

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_BME680 bme;

void setup() {

  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED gagal");
    while (1);
  }

  // BME680
  if (!bme.begin(0x77)) {
    Serial.println("BME680 tidak ditemukan");
    while (1);
  }

  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150);

  // Splash Screen
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(10, 15);
  display.println("AIR");

  display.setCursor(10, 40);
  display.println("QUALITY");

  display.display();

  delay(2000);
}

void loop() {

  if (!bme.performReading()) {
    Serial.println("Gagal membaca sensor");
    return;
  }

  float suhu = bme.temperature;
  float hum  = bme.humidity;
  float gas  = bme.gas_resistance / 1000.0;

  String status;

  if (gas > 120)
    status = "BAIK";
  else if (gas > 60)
    status = "SEDANG";
  else
    status = "BURUK";

  // =====================
  // SERIAL MONITOR
  // =====================

  Serial.println("====================");

  Serial.print("Suhu : ");
  Serial.print(suhu, 1);
  Serial.println(" C");

  Serial.print("Humidity : ");
  Serial.print(hum, 1);
  Serial.println(" %");

  Serial.print("Gas : ");
  Serial.print(gas, 1);
  Serial.println(" kOhm");

  Serial.print("Status : ");
  Serial.println(status);

  // =====================
  // OLED
  // =====================

  display.clearDisplay();

  display.setTextSize(1);

  display.setCursor(0,0);
  display.println("AIR QUALITY MONITOR");

  display.drawLine(0,10,128,10,SSD1306_WHITE);

  display.setCursor(0,15);
  display.print("Temp : ");
  display.print(suhu,1);
  display.println(" C");

  display.setCursor(0,28);
  display.print("Hum  : ");
  display.print(hum,1);
  display.println(" %");

  display.setCursor(0,41);
  display.print("Gas  : ");
  display.print(gas,0);
  display.println(" kOhm");

  display.setCursor(0,54);
  display.print("Status : ");
  display.print(status);

  display.display();

  delay(3000);
}
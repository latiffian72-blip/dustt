#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Alamat I2C LCD biasanya 0x27 atau 0x3F
LiquidCrystal_I2C lcd(0x27, 20, 4);

void setup() {
  Wire.begin(21, 22); // SDA, SCL ESP32

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("ESP32 + LCD 20x4");

  lcd.setCursor(0, 1);
  lcd.print("Arduino IDE");

  lcd.setCursor(0, 2);
  lcd.print("I2C Address: 0x27");

  lcd.setCursor(0, 3);
  lcd.print("www.example.com");
}

void loop() {

}
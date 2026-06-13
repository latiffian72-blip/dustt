#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// =============================================
// KONFIGURASI LCD I2C 20x4
// =============================================
LiquidCrystal_I2C lcd(0x27, 20, 4);
#define SDA_PIN  21
#define SCL_PIN  22

// =============================================
// KONFIGURASI PIN SENSOR
// =============================================
#define TURBIDITY_PIN  35    // GPIO35 - ADC1_CH7
#define SAMPLES        30

// =============================================
// KALIBRASI
// Sesuaikan dengan hasil scan kamu sebelumnya
// ADC ~984 = air jernih (dari data sebelumnya)
// ADC ~400 = air keruh  (ukur sendiri)
// =============================================
#define ADC_JERNIH   984
#define ADC_KERUH    300

// =============================================
// VARIABEL GLOBAL
// =============================================
unsigned long lastRead  = 0;
unsigned long lastBlink = 0;
bool blinkState = false;

// =============================================
// FUNGSI: Baca ADC Averaging
// =============================================
int readADC() {
  // Buang sampel awal untuk stabilisasi
  for (int i = 0; i < 5; i++) {
    analogRead(TURBIDITY_PIN);
    delay(2);
  }
  long total = 0;
  for (int i = 0; i < SAMPLES; i++) {
    total += analogRead(TURBIDITY_PIN);
    delay(5);
  }
  return (int)(total / SAMPLES);
}

// =============================================
// FUNGSI: ADC ke Voltage
// =============================================
float adcToVoltage(int adc) {
  return adc * (3.3f / 4095.0f);
}

// =============================================
// FUNGSI: ADC ke NTU (mapping linear)
// Sensor turbidity: ADC tinggi = jernih (NTU rendah)
//                  ADC rendah = keruh  (NTU tinggi)
// =============================================
float adcToNTU(int adc) {
  if (adc >= ADC_JERNIH) return 0.0f;
  if (adc <= ADC_KERUH)  return 3000.0f;
  float ntu = map(adc, ADC_KERUH, ADC_JERNIH, 3000, 0);
  if (ntu < 0) ntu = 0;
  return ntu;
}

// =============================================
// FUNGSI: Label Kondisi
// =============================================
String getLabel(int adc) {
  if      (adc >= 900) return "Sangat Jernih";
  else if (adc >= 750) return "Jernih       ";
  else if (adc >= 600) return "Agak Keruh   ";
  else if (adc >= 450) return "Keruh        ";
  else                 return "Sangat Keruh ";
}

// =============================================
// FUNGSI: Level Bar Indikator (1-5)
// =============================================
int getBar(int adc) {
  if      (adc >= 900) return 1;
  else if (adc >= 750) return 2;
  else if (adc >= 600) return 3;
  else if (adc >= 450) return 4;
  else                 return 5;
}

// =============================================
// FUNGSI: Update LCD
// =============================================
void updateLCD(int adc, float voltage, float ntu) {
  String label = getLabel(adc);
  int    bar   = getBar(adc);

  // ----- Baris 0: Header -----
  lcd.setCursor(0, 0);
  lcd.print("==TURBIDITY METER==");

  // ----- Baris 1: ADC & Voltage -----
  lcd.setCursor(0, 1);
  lcd.print("ADC:");
  if (adc < 1000) lcd.print(" ");
  if (adc < 100)  lcd.print(" ");
  if (adc < 10)   lcd.print(" ");
  lcd.print(adc);
  lcd.print("  V:");
  lcd.print(voltage, 2);
  lcd.print("V   ");

  // ----- Baris 2: NTU + Bar -----
  lcd.setCursor(0, 2);
  lcd.print("NTU :");
  if (ntu < 10)        lcd.print("   ");
  else if (ntu < 100)  lcd.print("  ");
  else if (ntu < 1000) lcd.print(" ");
  lcd.print((int)ntu);
  lcd.print("  [");
  for (int i = 1; i <= 5; i++) {
    lcd.print(i <= bar ? "#" : "-");
  }
  lcd.print("]  ");

  // ----- Baris 3: Kondisi + Kedip -----
  lcd.setCursor(0, 3);
  lcd.print("> ");
  lcd.print(label);

  // Indikator kedip pojok kanan bawah
  if (millis() - lastBlink >= 500) {
    lastBlink  = millis();
    blinkState = !blinkState;
  }
  lcd.setCursor(19, 3);
  lcd.print(blinkState ? "*" : " ");
}

// =============================================
// SETUP
// =============================================
void setup() {
  Serial.begin(115200);
  delay(500);

  // Konfigurasi ADC ESP32
  analogReadResolution(12);        // 12-bit: 0 ~ 4095
  analogSetAttenuation(ADC_11db);  // Range: 0 ~ 3.3V

  // Inisialisasi I2C & LCD
  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.init();
  lcd.backlight();

  // Splash Screen
  lcd.clear();
  lcd.setCursor(2, 0); lcd.print("TURBIDITY METER");
  lcd.setCursor(2, 1); lcd.print("DFRobot SEN0189");
  lcd.setCursor(3, 2); lcd.print("ESP32 + 20x4");
  lcd.setCursor(4, 3); lcd.print("GPIO35 Ready");
  delay(2500);
  lcd.clear();

  // Info Serial Monitor
  Serial.println("=================================");
  Serial.println("  TURBIDITY SENSOR - ESP32");
  Serial.println("  DFRobot SEN0189 | GPIO35");
  Serial.println("=================================");
  Serial.print  ("  ADC Jernih (ref) : "); Serial.println(ADC_JERNIH);
  Serial.print  ("  ADC Keruh  (ref) : "); Serial.println(ADC_KERUH);
  Serial.println("=================================");
  Serial.println("ADC   | Volt    | NTU  | Kondisi");
  Serial.println("------|---------|------|---------------");
}

// =============================================
// LOOP
// =============================================
void loop() {
  if (millis() - lastRead >= 1000) {
    lastRead = millis();

    int   adc     = readADC();
    float voltage = adcToVoltage(adc);
    float ntu     = adcToNTU(adc);
    String label  = getLabel(adc);

    // Serial Monitor
    Serial.print(adc);
    Serial.print("  |  ");
    Serial.print(voltage, 3);
    Serial.print("V  |  ");
    Serial.print((int)ntu);
    Serial.print("  |  ");
    Serial.println(label);

    // LCD
    updateLCD(adc, voltage, ntu);
  }

  // Kedip non-blocking
  lcd.setCursor(19, 3);
  lcd.print(blinkState ? "*" : " ");
}
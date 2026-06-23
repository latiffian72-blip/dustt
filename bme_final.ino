#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>

Adafruit_BME680 bme;

// konstanta kalibrasi VOC
#define VOC_A 10007264417.0
#define VOC_B -6.1285

unsigned long startTime;
const unsigned long warmup = 180000; // 3 menit

float convertVOC(float R_kohm)
{
  float voc;

  voc = VOC_A * pow(R_kohm, VOC_B);

  // batas range
  if (voc < 0)
    voc = 0;

  if (voc > 5)
    voc = 5;

  return voc;
}


void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println("BME680 VOC Meter");

  if (!bme.begin())
  {
    Serial.println("BME680 tidak ditemukan!");
    while (1);
  }


  // setting BME680
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);

  bme.setGasHeater(320, 150);

  startTime = millis();

  Serial.println("Warmup 3 menit...");
}



void loop()
{

  if (!bme.performReading())
  {
    Serial.println("Gagal membaca sensor");
    return;
  }


  // resistansi gas dalam ohm
  float R_ohm = bme.gas_resistance;

  // ubah ke kilo ohm
  float R_kohm = R_ohm / 1000.0;


  // konversi VOC
  float VOC_mg = convertVOC(R_kohm);


  bool ready = millis() - startTime > warmup;



  Serial.println("====================");

  Serial.print("Gas Resistance : ");
  Serial.print(R_kohm);
  Serial.println(" kOhm");


  Serial.print("VOC            : ");

  if (ready)
  {
    Serial.print(VOC_mg,4);
    Serial.println(" mg/m3");
  }
  else
  {
    Serial.println("warming up");
  }


  // kategori udara
  if (ready)
  {
    if(VOC_mg < 0.05)
    {
      Serial.println("Kondisi : Indoor bersih");
    }
    else if(VOC_mg < 0.5)
    {
      Serial.println("Kondisi : Sedang");
    }
    else if(VOC_mg < 2)
    {
      Serial.println("Kondisi : Buruk");
    }
    else if(VOC_mg < 5)
    {
      Serial.println("Kondisi : Dekat debu/asap");
    }
    else
    {
      Serial.println("Kondisi : Sangat buruk");
    }
  }


  delay(2000);
}
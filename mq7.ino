#define MQ7_PIN 34

float RL = 10.0;
float Ro = 1.73;

void setup() {

  Serial.begin(115200);

}

void loop() {

  long total = 0;

  // averaging
  for(int i = 0; i < 50; i++) {

    total += analogRead(MQ7_PIN);
    delay(20);

  }

  float adc = total / 50.0;

  // konversi ADC ESP32
  float vout = adc * (3.3 / 4095.0);

  // kompensasi voltage divider
  float sensorVoltage = vout * 1.5;

  // hitung Rs
  float Rs = RL * ((5.0 - sensorVoltage) / sensorVoltage);

  // ratio
  float ratio = Rs / Ro;

  // hitung ppm
  float ppm = 99.042 * pow(ratio, -1.518);

  Serial.println("========== MQ-7 ==========");

  Serial.print("ADC            : ");
  Serial.println(adc);

  Serial.print("Sensor Voltage : ");
  Serial.print(sensorVoltage);
  Serial.println(" V");

  Serial.print("Rs             : ");
  Serial.print(Rs);
  Serial.println(" kOhm");

  Serial.print("Ro             : ");
  Serial.print(Ro);
  Serial.println(" kOhm");

  Serial.print("Rs/Ro          : ");
  Serial.println(ratio);

  Serial.print("CO             : ");
  Serial.print(ppm);
  Serial.println(" ppm");

  // status kualitas udara
  if(ppm < 10) {

    Serial.println("Status : Aman");

  }
  else if(ppm < 35) {

    Serial.println("Status : Waspada");

  }
  else {

    Serial.println("Status : Bahaya");

  }

  Serial.println("==========================");

  delay(2000);

}
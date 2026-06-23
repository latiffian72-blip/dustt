/*
 * =====================================================================
 * KONTROL GABUNGAN: BLOWER + LAMPU PILOT + POMPA DC
 * Mikrokontroller : ESP32 WROOM-32E
 * =====================================================================
 *
 * BAGIAN 1 — BLOWER (via SSR-10DA + Toggle Switch)
 * --------------------------------------------------
 * WIRING SSR:
 *   SSR Pin 3 (+) --> GPIO26 ESP32
 *   SSR Pin 4 (-) --> GND ESP32
 *   SSR Pin 1 (~) --> Output MCB C4 (Fasa/L)
 *   SSR Pin 2 (~) --> Kabel oranye blower
 *   Kabel hitam blower --> Busbar Netral (N) PLN
 *
 * WIRING TOGGLE SWITCH:
 *   Pin 1 / COM  --> GPIO18 ESP32
 *   Pin 3 / NO   --> GND ESP32
 *   Logika (INPUT_PULLUP):
 *     Toggle ON  --> GPIO18 = LOW  --> Blower ON
 *     Toggle OFF --> GPIO18 = HIGH --> Blower OFF
 *
 * BAGIAN 2 — LAMPU PILOT (3x Relay 5V SRD-05VDC, active LOW)
 * --------------------------------------------------
 *   Relay IN  --> GPIO ESP32
 *   Relay COM --> +12V dari PSU
 *   Relay NO  --> Terminal (+) lampu pilot
 *   Lampu (-) --> GND PSU
 *   Logika ACTIVE LOW:
 *     GPIO LOW  --> Relay ON  --> Lampu NYALA
 *     GPIO HIGH --> Relay OFF --> Lampu MATI
 *
 * BAGIAN 3 — POMPA DC 12V (via Dual MOSFET Module)
 * --------------------------------------------------
 * WIRING MOSFET:
 *   TRIG1 --> GPIO13 ESP32
 *   TRIG2 --> GPIO14 ESP32
 *   VCC   --> 12V PSU
 *   GND   --> GND bersama
 *   OUT+  --> Kabel (+) Pompa
 *   OUT-  --> Kabel (-) Pompa
 *
 * PIN YANG DIPAKAI:
 *   GPIO26 --> SSR Blower
 *   GPIO18 --> Toggle switch Blower
 *   GPIO2  --> LED built-in (indikator blower)
 *   GPIO25 --> Relay Lampu MERAH
 *   GPIO33 --> Relay Lampu KUNING
 *   GPIO32 --> Relay Lampu HIJAU
 *   GPIO13 --> TRIG1 MOSFET Pompa
 *   GPIO14 --> TRIG2 MOSFET Pompa
 *
 * SERIAL MONITOR (115200 baud, Line ending = Newline):
 *   ON / OFF          --> kontrol blower
 *   M / K / H         --> toggle lampu Merah / Kuning / Hijau
 *   ALL_ON / ALL_OFF  --> nyalakan / matikan semua lampu
 *   CYCLE             --> test urut lampu satu per satu
 *   P1 / P2 / P3      --> kecepatan pompa: 50% / 75% / 100%
 *   P0                --> pompa MATI
 *   STATUS            --> tampilkan status seluruh sistem
 * =====================================================================
 */

// ===== PIN BLOWER =====
#define PIN_SSR           26
#define PIN_TOGGLE        18
#define PIN_LED_STATUS     2

// ===== PIN LAMPU PILOT =====
#define PIN_MERAH         25
#define PIN_KUNING        33
#define PIN_HIJAU         32

// ===== PIN POMPA =====
#define PIN_TRIG1         13
#define PIN_TRIG2         14

// ===== PWM POMPA =====
const int PWM_FREQ = 5000;  // 5 kHz
const int PWM_RES  = 8;     // 8-bit (0–255)

// Nilai PWM untuk tiap tingkat kecepatan
const int PWM_OFF  = 0;
const int PWM_P1   = 128;   // 50%
const int PWM_P2   = 191;   // 75%
const int PWM_P3   = 255;   // 100%

// ===== STATUS BLOWER =====
bool blowerStatus    = false;
bool lastToggleState = HIGH;
bool toggleHandled   = false;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 80;

// ===== STATUS LAMPU =====
bool statusMerah  = false;
bool statusKuning = false;
bool statusHijau  = false;

// ===== STATUS POMPA =====
int pumpLevel = 0;   // 0 = OFF, 1 = 50%, 2 = 75%, 3 = 100%

// =====================================================================
// SETUP
// =====================================================================
void setup() {
  Serial.begin(115200);
  delay(500);

  // --- Pin blower ---
  pinMode(PIN_SSR,        OUTPUT);
  pinMode(PIN_LED_STATUS, OUTPUT);
  pinMode(PIN_TOGGLE,     INPUT_PULLUP);

  // --- Pin lampu ---
  pinMode(PIN_MERAH,  OUTPUT);
  pinMode(PIN_KUNING, OUTPUT);
  pinMode(PIN_HIJAU,  OUTPUT);

  // Semua lampu MATI saat startup (active LOW)
  digitalWrite(PIN_MERAH,  HIGH);
  digitalWrite(PIN_KUNING, HIGH);
  digitalWrite(PIN_HIJAU,  HIGH);

  // --- PWM Pompa (ESP32 Arduino v3.x API) ---
  ledcAttach(PIN_TRIG1, PWM_FREQ, PWM_RES);
  ledcAttach(PIN_TRIG2, PWM_FREQ, PWM_RES);
  setPumpSpeed(PWM_OFF);

  // --- Baca posisi awal toggle blower ---
  bool posisiAwal = digitalRead(PIN_TOGGLE);
  if (posisiAwal == LOW) {
    nyalakanBlower();
    Serial.println("[INIT] Toggle sudah ON saat startup");
  } else {
    matikanBlower();
    Serial.println("[INIT] Toggle OFF saat startup");
  }
  lastToggleState = posisiAwal;

  // --- Banner ---
  Serial.println("==============================================");
  Serial.println("  KONTROL GABUNGAN: BLOWER + LAMPU + POMPA");
  Serial.println("==============================================");
  Serial.println("BLOWER:");
  Serial.println("  ON / OFF      -> kontrol manual Serial");
  Serial.println("  Toggle GPIO18 -> kontrol fisik via switch");
  Serial.println("----------------------------------------------");
  Serial.println("LAMPU PILOT:");
  Serial.println("  M        -> toggle MERAH  (GPIO25)");
  Serial.println("  K        -> toggle KUNING (GPIO33)");
  Serial.println("  H        -> toggle HIJAU  (GPIO32)");
  Serial.println("  ALL_ON   -> nyalakan semua lampu");
  Serial.println("  ALL_OFF  -> matikan semua lampu");
  Serial.println("  CYCLE    -> test urut lampu satu per satu");
  Serial.println("----------------------------------------------");
  Serial.println("POMPA DC 12V:");
  Serial.println("  P0 -> MATI");
  Serial.println("  P1 -> Kecepatan  50% (PWM = 128)");
  Serial.println("  P2 -> Kecepatan  75% (PWM = 191)");
  Serial.println("  P3 -> Kecepatan 100% (PWM = 255)");
  Serial.println("----------------------------------------------");
  Serial.println("  STATUS -> tampilkan status seluruh sistem");
  Serial.println("==============================================");
  Serial.println();
}

// =====================================================================
// LOOP UTAMA
// =====================================================================
void loop() {
  bacaSerial();
  bacaToggleBlower();
}

// =====================================================================
// BAGIAN BLOWER
// =====================================================================

void bacaToggleBlower() {
  bool currentState = digitalRead(PIN_TOGGLE);

  if (currentState != lastToggleState) {
    lastDebounceTime = millis();
    toggleHandled = false;
  }

  if ((millis() - lastDebounceTime) > debounceDelay && !toggleHandled) {
    toggleHandled = true;

    if (currentState == LOW) {
      if (!blowerStatus) {
        nyalakanBlower();
        Serial.println("[TOGGLE] Switch -> ON");
      }
    } else {
      if (blowerStatus) {
        matikanBlower();
        Serial.println("[TOGGLE] Switch -> OFF");
      }
    }
  }

  lastToggleState = currentState;
}

void nyalakanBlower() {
  blowerStatus = true;
  digitalWrite(PIN_SSR,        HIGH);
  digitalWrite(PIN_LED_STATUS, HIGH);
  Serial.println("[ON]  Blower MENYALA");
}

void matikanBlower() {
  blowerStatus = false;
  digitalWrite(PIN_SSR,        LOW);
  digitalWrite(PIN_LED_STATUS, LOW);
  Serial.println("[OFF] Blower MATI");
}

// =====================================================================
// BAGIAN LAMPU PILOT (active LOW)
// =====================================================================

void nyalakanLampu(int pin, bool &status, String nama) {
  digitalWrite(pin, LOW);
  status = true;
  Serial.print("[ON]  Lampu ");
  Serial.print(nama);
  Serial.println(" NYALA");
}

void matikanLampu(int pin, bool &status, String nama) {
  digitalWrite(pin, HIGH);
  status = false;
  Serial.print("[OFF] Lampu ");
  Serial.print(nama);
  Serial.println(" MATI");
}

void toggleLampu(int pin, bool &status, String nama) {
  if (status) matikanLampu(pin, status, nama);
  else        nyalakanLampu(pin, status, nama);
}

void nyalakanSemuaLampu() {
  digitalWrite(PIN_MERAH,  LOW);
  digitalWrite(PIN_KUNING, LOW);
  digitalWrite(PIN_HIJAU,  LOW);
  statusMerah = statusKuning = statusHijau = true;
  Serial.println("[ALL_ON] Semua lampu NYALA");
}

void matikanSemuaLampu() {
  digitalWrite(PIN_MERAH,  HIGH);
  digitalWrite(PIN_KUNING, HIGH);
  digitalWrite(PIN_HIJAU,  HIGH);
  statusMerah = statusKuning = statusHijau = false;
  Serial.println("[ALL_OFF] Semua lampu MATI");
}

void jalankanCycleLampu() {
  Serial.println("[CYCLE] Mulai: MERAH -> KUNING -> HIJAU");
  matikanSemuaLampu();
  delay(300);

  digitalWrite(PIN_MERAH, LOW);
  Serial.println("  >> MERAH nyala");
  delay(1000);
  digitalWrite(PIN_MERAH, HIGH);
  delay(200);

  digitalWrite(PIN_KUNING, LOW);
  Serial.println("  >> KUNING nyala");
  delay(1000);
  digitalWrite(PIN_KUNING, HIGH);
  delay(200);

  digitalWrite(PIN_HIJAU, LOW);
  Serial.println("  >> HIJAU nyala");
  delay(1000);
  digitalWrite(PIN_HIJAU, HIGH);
  delay(200);

  statusMerah = statusKuning = statusHijau = false;
  Serial.println("[CYCLE] Selesai - semua lampu mati");
}

// =====================================================================
// BAGIAN POMPA DC
// =====================================================================

void setPumpSpeed(int pwmVal) {
  pwmVal = constrain(pwmVal, 0, 255);
  ledcWrite(PIN_TRIG1, pwmVal);
  ledcWrite(PIN_TRIG2, pwmVal);
}

void setPumpLevel(int level) {
  pumpLevel = level;
  switch (level) {
    case 0:
      setPumpSpeed(PWM_OFF);
      Serial.println("[POMPA] MATI (0%)");
      break;
    case 1:
      setPumpSpeed(PWM_P1);
      Serial.println("[POMPA] Kecepatan 1 ->  50% (PWM=128)");
      break;
    case 2:
      setPumpSpeed(PWM_P2);
      Serial.println("[POMPA] Kecepatan 2 ->  75% (PWM=191)");
      break;
    case 3:
      setPumpSpeed(PWM_P3);
      Serial.println("[POMPA] Kecepatan 3 -> 100% (PWM=255)");
      break;
    default:
      Serial.println("[!] Level pompa tidak valid (gunakan P0/P1/P2/P3)");
      break;
  }
}

// =====================================================================
// BAGIAN SERIAL (gabungan blower + lampu + pompa)
// =====================================================================

void bacaSerial() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();

    // --- Perintah blower ---
    if      (cmd == "ON")      nyalakanBlower();
    else if (cmd == "OFF")     matikanBlower();

    // --- Perintah lampu pilot ---
    else if (cmd == "M")       toggleLampu(PIN_MERAH,  statusMerah,  "MERAH");
    else if (cmd == "K")       toggleLampu(PIN_KUNING, statusKuning, "KUNING");
    else if (cmd == "H")       toggleLampu(PIN_HIJAU,  statusHijau,  "HIJAU");
    else if (cmd == "ALL_ON")  nyalakanSemuaLampu();
    else if (cmd == "ALL_OFF") matikanSemuaLampu();
    else if (cmd == "CYCLE")   jalankanCycleLampu();

    // --- Perintah pompa ---
    else if (cmd == "P0")      setPumpLevel(0);
    else if (cmd == "P1")      setPumpLevel(1);
    else if (cmd == "P2")      setPumpLevel(2);
    else if (cmd == "P3")      setPumpLevel(3);

    // --- Status gabungan ---
    else if (cmd == "STATUS")  tampilkanStatus();

    else {
      Serial.print("[!] Tidak dikenal: '");
      Serial.print(cmd);
      Serial.println("' -> gunakan ON/OFF/M/K/H/ALL_ON/ALL_OFF/CYCLE/P0/P1/P2/P3/STATUS");
    }
  }
}

// =====================================================================
// STATUS GABUNGAN
// =====================================================================

void tampilkanStatus() {
  Serial.println("==============================================");
  Serial.print  ("  BLOWER          : ");
  Serial.println(blowerStatus ? "MENYALA (ON)" : "MATI (OFF)");
  Serial.print  ("  Toggle (GPIO18) : ");
  Serial.println(digitalRead(PIN_TOGGLE) == LOW ? "ON (LOW)" : "OFF (HIGH)");
  Serial.println("----------------------------------------------");
  Serial.print  ("  MERAH  (GPIO25) : ");
  Serial.println(statusMerah  ? "NYALA" : "MATI");
  Serial.print  ("  KUNING (GPIO33) : ");
  Serial.println(statusKuning ? "NYALA" : "MATI");
  Serial.print  ("  HIJAU  (GPIO32) : ");
  Serial.println(statusHijau  ? "NYALA" : "MATI");
  Serial.println("----------------------------------------------");
  Serial.print  ("  POMPA  Level    : ");
  Serial.print  (pumpLevel);
  switch (pumpLevel) {
    case 0: Serial.println(" (MATI)");  break;
    case 1: Serial.println(" ( 50%)");  break;
    case 2: Serial.println(" ( 75%)");  break;
    case 3: Serial.println(" (100%)");  break;
  }
  Serial.println("==============================================");
}

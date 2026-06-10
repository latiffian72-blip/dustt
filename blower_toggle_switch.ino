/*
 * =====================================================
 * KONTROL BLOWER ON/OFF via SSR-10DA
 * Mikrokontroller : ESP32 WROOM-32E
 * SSR             : FOFER SSR-10DA
 * Switch          : TOGGLE SWITCH ON/OFF (DPDT/SPDT)
 * =====================================================
 *
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
 *   Pin 2,4,5,6  --> TIDAK DISAMBUNG
 *
 *   Logika (INPUT_PULLUP):
 *   Toggle ON  --> COM-NO terhubung --> GPIO18 = LOW  --> Blower ON
 *   Toggle OFF --> COM terputus     --> GPIO18 = HIGH --> Blower OFF
 *
 * SERIAL MONITOR (115200 baud):
 *   ON / OFF / STATUS / TOGGLE
 * =====================================================
 */

// ===== KONFIGURASI PIN =====
#define PIN_SSR         26    // GPIO ke SSR-10DA pin 3 (+)
#define PIN_TOGGLE      18    // Toggle switch (COM ke GPIO, NO ke GND)
#define PIN_LED_STATUS   2    // LED built-in ESP32 (indikator)

// ===== VARIABEL =====
bool blowerStatus     = false;   // Status blower saat ini
bool lastToggleState  = HIGH;    // State toggle sebelumnya
bool toggleHandled    = false;   // Cegah aksi berulang

unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 80; // ms — sedikit lebih lama untuk toggle

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(500);

  // Konfigurasi pin
  pinMode(PIN_SSR,        OUTPUT);
  pinMode(PIN_LED_STATUS, OUTPUT);
  pinMode(PIN_TOGGLE,     INPUT_PULLUP); // Pull-up internal

  // === BACA POSISI AWAL TOGGLE ===
  // Saat ESP32 menyala, langsung baca posisi toggle
  bool posisiAwal = digitalRead(PIN_TOGGLE);
  if (posisiAwal == LOW) {
    // Toggle sudah di posisi ON saat nyala
    nyalakanBlower();
    Serial.println("[INIT] Toggle sudah di posisi ON saat startup");
  } else {
    // Toggle di posisi OFF saat nyala
    matikanBlower();
    Serial.println("[INIT] Toggle di posisi OFF saat startup");
  }
  lastToggleState = posisiAwal;

  // Pesan awal
  Serial.println("========================================");
  Serial.println("  KONTROL BLOWER + TOGGLE SWITCH");
  Serial.println("========================================");
  Serial.println("Toggle Switch GPIO18:");
  Serial.println("  ON  = PIN1(COM) ke PIN3(NO) tersambung");
  Serial.println("  OFF = PIN1(COM) terputus");
  Serial.println("----------------------------------------");
  Serial.println("Perintah Serial:");
  Serial.println("  ON / OFF / STATUS");
  Serial.println("========================================");
}

// ===== LOOP UTAMA =====
void loop() {
  bacaSerial();
  bacaToggle();
}

// ===== FUNGSI: BACA TOGGLE SWITCH =====
// Perbedaan dengan tombol momentary:
// Toggle = baca STATE langsung, bukan deteksi tekan/lepas
void bacaToggle() {
  bool currentState = digitalRead(PIN_TOGGLE);

  // Debounce: tunggu state stabil
  if (currentState != lastToggleState) {
    lastDebounceTime = millis();
    toggleHandled = false; // reset, siap proses state baru
  }

  // Jika sudah stabil lebih dari debounceDelay ms
  if ((millis() - lastDebounceTime) > debounceDelay && !toggleHandled) {
    toggleHandled = true; // tandai sudah diproses

    if (currentState == LOW) {
      // ===== TOGGLE DIGESER KE ON =====
      if (!blowerStatus) { // hanya aksi jika status berubah
        nyalakanBlower();
        Serial.println("[TOGGLE] Switch digeser ke ON");
      }
    } else {
      // ===== TOGGLE DIGESER KE OFF =====
      if (blowerStatus) { // hanya aksi jika status berubah
        matikanBlower();
        Serial.println("[TOGGLE] Switch digeser ke OFF");
      }
    }
  }

  lastToggleState = currentState;
}

// ===== FUNGSI: BACA SERIAL =====
void bacaSerial() {
  if (Serial.available() > 0) {
    String perintah = Serial.readStringUntil('\n');
    perintah.trim();
    perintah.toUpperCase();

    if (perintah == "ON") {
      nyalakanBlower();
    } else if (perintah == "OFF") {
      matikanBlower();
    } else if (perintah == "STATUS") {
      tampilkanStatus();
    } else {
      Serial.println("[!] Perintah tidak dikenal. Gunakan: ON / OFF / STATUS");
    }
  }
}

// ===== FUNGSI: NYALAKAN BLOWER =====
void nyalakanBlower() {
  blowerStatus = true;
  digitalWrite(PIN_SSR,        HIGH);
  digitalWrite(PIN_LED_STATUS, HIGH);
  Serial.println("[ON]  Blower MENYALA ✓");
}

// ===== FUNGSI: MATIKAN BLOWER =====
void matikanBlower() {
  blowerStatus = false;
  digitalWrite(PIN_SSR,        LOW);
  digitalWrite(PIN_LED_STATUS, LOW);
  Serial.println("[OFF] Blower MATI ✓");
}

// ===== FUNGSI: STATUS =====
void tampilkanStatus() {
  Serial.println("========================================");
  Serial.print  ("  Blower : ");
  Serial.println(blowerStatus ? "MENYALA (ON)" : "MATI (OFF)");
  Serial.print  ("  Toggle : GPIO");
  Serial.print  (PIN_TOGGLE);
  Serial.print  (" = ");
  Serial.println(digitalRead(PIN_TOGGLE) == LOW ? "ON (LOW)" : "OFF (HIGH)");
  Serial.println("========================================");
}

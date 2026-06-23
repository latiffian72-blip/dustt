// ================================================================
//  SENSOR CO — MQ-7 + ESP32
//  Validasi vs Air Quality Detector (ppm)
//  Versi : v1.3
// ================================================================
//
//  KONFIGURASI PIN ESP32:
//   MQ-7 AOUT → VoltDiv → GPIO 34 (ADC1_CH6)
//   MQ-7 VCC  → 5V  (WAJIB 5V)
//   MQ-7 GND  → GND
//
//   Voltage Divider:
//    AOUT ──┬── R1 (20kΩ) ──► GPIO 34
//            └── R2 (10kΩ) ──► GND
//    VDIV = 1.5
//
//  THRESHOLD STATUS:
//   < 10 ppm  → AMAN
//   10–35 ppm → SEDANG
//   > 35 ppm  → BAHAYA
//
//  PERINTAH SERIAL (115200 baud):
//   S → Simpan sampel + input nilai referensi
//   L → Lihat tabel log
//   R → Reset log
// ================================================================

// ================================================================
//  SEMUA STRUCT DI PALING ATAS — WAJIB SEBELUM FUNGSI APAPUN
// ================================================================

struct HasilBaca {
  float adc_raw;
  float adc_ma;
  float ppm_instant;
  float ppm_smooth;
  float delta_ppm;
  bool  vout_over;
};

struct DataSampel {
  int   no;
  float ppm_instant;
  float ppm_smooth;
  float co_ref;
};

struct MovAvg {
  float buf[10];
  int   idx;
  int   cnt;
  float sum;
};

struct AdpEMA {
  float val;
  float prev;
  bool  init;
};

// ================================================================
//  PIN & PARAMETER
// ================================================================

#define MQ7_AOUT 34

const float RL      = 10.0f;
const float Ro      = 2.117f;
const float VDIV    = 1.5f;
const float VCC_MQ  = 5.0f;
const float VADC    = 3.3f;
const float ADCMAX  = 4095.0f;
const float PPM_A   = 99.042f;
const float PPM_B   = -1.518f;
const float PPM_MIN = 1.0f;
const float PPM_MAX = 2000.0f;

#define ADC_N      50
#define ADC_DLY    20
#define MA_SIZE    10
#define PRINT_MS   2000
#define WARMUP_MS  30000UL

unsigned long startMs     = 0;
unsigned long lastPrintMs = 0;

#define MAX_LOG 30
DataSampel logData[MAX_LOG];
int totalLog  = 0;
int noSampel  = 1;
float ppm_prev = 0;

MovAvg  ma_adc;
AdpEMA  aema;

// ================================================================
//  FUNGSI MOVING AVERAGE
// ================================================================

void ma_init(MovAvg &m) {
  for (int i = 0; i < MA_SIZE; i++) m.buf[i] = 0.0f;
  m.idx = 0; m.cnt = 0; m.sum = 0.0f;
}

float ma_run(MovAvg &m, float v) {
  if (m.cnt == MA_SIZE) m.sum -= m.buf[m.idx];
  m.buf[m.idx] = v;
  m.sum += v;
  m.idx = (m.idx + 1) % MA_SIZE;
  if (m.cnt < MA_SIZE) m.cnt++;
  return m.sum / m.cnt;
}

// ================================================================
//  FUNGSI ADAPTIVE EMA
// ================================================================

void aema_init(AdpEMA &e) {
  e.val  = 0.0f;
  e.prev = 0.0f;
  e.init = false;
}

float aema_run(AdpEMA &e, float v) {
  if (!e.init) {
    e.val  = v;
    e.prev = v;
    e.init = true;
    return v;
  }
  float d = v - e.prev;
  if (d < 0.0f) d = -d;
  float a;
  if      (d > 20.0f) a = 0.85f;
  else if (d > 10.0f) a = 0.65f;
  else if (d >  3.0f) a = 0.40f;
  else                a = 0.15f;
  e.prev = v;
  e.val  = a * v + (1.0f - a) * e.val;
  return e.val;
}

// ================================================================
//  FUNGSI HELPER KALKULASI
// ================================================================

float adcToV(float adc_val) {
  return adc_val * (VADC / ADCMAX) * VDIV;
}

float vToRs(float v_act) {
  if (v_act >= VCC_MQ) return 0.0f;
  if (v_act < 0.01f) v_act = 0.01f;
  float rs = RL * ((VCC_MQ - v_act) / v_act);
  return (rs < 0.0f) ? 0.0f : rs;
}

float rsToPPM(float rs) {
  if (rs <= 0.0f) return PPM_MAX;
  float ratio = rs / Ro;
  if (ratio <= 0.01f) return PPM_MAX;
  float ppm = PPM_A * pow(ratio, PPM_B);
  if (ppm < PPM_MIN) return PPM_MIN;
  if (ppm > PPM_MAX) return PPM_MAX;
  return ppm;
}

const char* statusCO(float ppm) {
  if (ppm <  10.0f) return "AMAN  ";
  if (ppm <  35.0f) return "SEDANG";
  return                    "BAHAYA";
}

// ================================================================
//  BACA SENSOR
// ================================================================

HasilBaca bacaSensor() {
  HasilBaca b;
  b.vout_over = false;

  uint32_t total = 0;
  for (int i = 0; i < ADC_N; i++) {
    total += (uint32_t)analogRead(MQ7_AOUT);
    delay(ADC_DLY);
  }
  b.adc_raw = (float)total / (float)ADC_N;
  b.adc_ma  = ma_run(ma_adc, b.adc_raw);

  float v_raw  = adcToV(b.adc_raw);
  b.vout_over  = (v_raw >= VCC_MQ);

  float rs_raw    = vToRs(v_raw);
  b.ppm_instant   = b.vout_over ? 0.0f : rsToPPM(rs_raw);

  float v_ma      = adcToV(b.adc_ma);
  float rs_ma     = vToRs(v_ma);
  float ppm_ma    = b.vout_over ? 0.0f : rsToPPM(rs_ma);
  b.ppm_smooth    = aema_run(aema, ppm_ma);

  b.delta_ppm = b.ppm_instant - ppm_prev;
  ppm_prev    = b.ppm_instant;

  return b;
}

// ================================================================
//  TAMPILKAN DATA SERIAL
// ================================================================

void tampilkan(HasilBaca b, bool warmup_done) {
  float elapsed = millis() / 1000.0f;

  Serial.println();
  Serial.println(F("========================================="));
  Serial.println(F("       SENSOR CO — MQ-7  v1.3"));
  Serial.println(F("========================================="));

  if (!warmup_done) {
    float sisa = (float)(WARMUP_MS - (millis() - startMs)) / 1000.0f;
    Serial.print(F("  WARMING UP... sisa "));
    Serial.print(sisa, 0);
    Serial.println(F(" detik"));
  } else {
    Serial.print(F("  PPM Realtime : "));
    Serial.print(b.ppm_instant, 2);
    Serial.println(F(" ppm"));

    Serial.print(F("  PPM Smooth   : "));
    Serial.print(b.ppm_smooth, 2);
    Serial.println(F(" ppm"));

    Serial.print(F("  Delta PPM    : "));
    Serial.print(b.delta_ppm, 2);
    Serial.println(F(" ppm/2s"));

    Serial.print(F("  STATUS CO    : "));
    Serial.println(statusCO(b.ppm_instant));
  }

  Serial.println(F("-----------------------------------------"));
  Serial.println(F("  [S]=Simpan  [L]=Log  [R]=Reset"));
  Serial.println(F("========================================="));

  Serial.print(F("CSV,"));
  Serial.print(elapsed, 1);        Serial.print(F(","));
  Serial.print(b.ppm_instant, 2);  Serial.print(F(","));
  Serial.print(b.ppm_smooth,  2);  Serial.print(F(","));
  Serial.print(b.delta_ppm,   2);  Serial.print(F(","));
  Serial.print(statusCO(b.ppm_instant));
  Serial.println(warmup_done ? F(",VALID") : F(",WARMUP"));
}

// ================================================================
//  SIMPAN SAMPEL PEMBANDING
// ================================================================

void ambilSampel(HasilBaca b) {
  if (totalLog >= MAX_LOG) {
    Serial.println(F(">> Penuh! Ketik R untuk reset."));
    return;
  }

  Serial.println(F("\n========================================="));
  Serial.print  (F(  " SAMPEL #")); Serial.println(noSampel);
  Serial.println(F(  "========================================="));
  Serial.print  (F(  " PPM Realtime : ")); Serial.print(b.ppm_instant, 2); Serial.println(F(" ppm"));
  Serial.print  (F(  " PPM Smooth   : ")); Serial.print(b.ppm_smooth,  2); Serial.println(F(" ppm"));
  Serial.println(F(  "-----------------------------------------"));
  Serial.println(F(  " Masukkan CO dari Air Quality Detector"));
  Serial.println(F(  " (ketik angka ppm lalu Enter):"));
  Serial.print  (F(  " >> "));

  String buf = "";
  unsigned long t0 = millis();
  while (millis() - t0 < 30000UL) {
    if (Serial.available()) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') { if (buf.length() > 0) break; }
      else buf += c;
    }
    delay(5);
  }
  float ref = buf.toFloat();

  logData[totalLog].no          = noSampel;
  logData[totalLog].ppm_instant = b.ppm_instant;
  logData[totalLog].ppm_smooth  = b.ppm_smooth;
  logData[totalLog].co_ref      = ref;

  float sel = b.ppm_instant - ref;
  if (sel < 0.0f) sel = -sel;
  float ep = (ref > 0.0f) ? (sel / ref * 100.0f) : 0.0f;

  Serial.println(F("\n Tersimpan!"));
  Serial.print  (F(" PPM Sensor  : ")); Serial.print(b.ppm_instant, 2); Serial.println(F(" ppm"));
  Serial.print  (F(" CO Ref      : ")); Serial.print(ref,           2); Serial.println(F(" ppm"));
  Serial.print  (F(" Selisih     : ")); Serial.print(sel,           2); Serial.println(F(" ppm"));
  Serial.print  (F(" Error       : ")); Serial.print(ep,            1); Serial.println(F(" %\n"));

  totalLog++;
  noSampel++;
}

// ================================================================
//  TAMPILKAN TABEL LOG
// ================================================================

void tampilLog() {
  Serial.println(F("\n================================================"));
  Serial.println(F(  "  No | PPM_SENSOR | PPM_SMOOTH |  CO_REF | ERR%"));
  Serial.println(F(  "------------------------------------------------"));

  if (totalLog == 0) {
    Serial.println(F("  Belum ada sampel tersimpan."));
  }

  float sumSel = 0.0f;
  float sumEp  = 0.0f;

  for (int i = 0; i < totalLog; i++) {
    float sel = logData[i].ppm_instant - logData[i].co_ref;
    if (sel < 0.0f) sel = -sel;
    float ep = (logData[i].co_ref > 0.0f) ? (sel / logData[i].co_ref * 100.0f) : 0.0f;
    sumSel += sel;
    sumEp  += ep;

    Serial.print(F("  "));
    if (logData[i].no < 10) Serial.print(F(" "));
    Serial.print(logData[i].no);
    Serial.print(F("  | "));
    Serial.print(logData[i].ppm_instant, 2);
    Serial.print(F("      | "));
    Serial.print(logData[i].ppm_smooth,  2);
    Serial.print(F("      | "));
    Serial.print(logData[i].co_ref,      2);
    Serial.print(F("  | "));
    Serial.println(ep, 1);
  }

  if (totalLog > 0) {
    Serial.println(F("------------------------------------------------"));
    Serial.print  (F("  Rata selisih : ")); Serial.print(sumSel / totalLog, 2); Serial.println(F(" ppm"));
    Serial.print  (F("  Rata error   : ")); Serial.print(sumEp  / totalLog, 1); Serial.println(F(" %"));
    Serial.print  (F("  Total        : ")); Serial.println(totalLog);
  }
  Serial.println(F("================================================\n"));
}

// ================================================================
//  SETUP
// ================================================================

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  startMs = millis();

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  ma_init(ma_adc);
  aema_init(aema);

  Serial.println(F("================================================"));
  Serial.println(F("  MQ-7 CO Monitor — Validasi vs AQD  v1.3"));
  Serial.println(F("================================================"));
  Serial.print  (F("  Ro     : ")); Serial.print(Ro,   3); Serial.println(F(" kOhm"));
  Serial.print  (F("  RL     : ")); Serial.print(RL,   1); Serial.println(F(" kOhm"));
  Serial.print  (F("  VDIV   : ")); Serial.println(VDIV, 1);
  Serial.println(F("  Filter : MA(N=10) + Adaptive EMA"));
  Serial.println(F("  STATUS : <10=AMAN | 10-35=SEDANG | >35=BAHAYA"));
  Serial.println(F("================================================"));
  Serial.println(F("  PIN: AOUT->VDiv(20k/10k)->GPIO34  VCC->5V"));
  Serial.println(F("================================================\n"));

  Serial.println(F("CSV,TIME_S,PPM_INSTANT,PPM_SMOOTH,DELTA_PPM,STATUS,WARMUP"));
}

// ================================================================
//  LOOP
// ================================================================

void loop() {
  if (millis() - lastPrintMs < PRINT_MS) return;
  lastPrintMs = millis();

  bool warmup_done = (millis() - startMs) >= WARMUP_MS;
  HasilBaca b = bacaSensor();
  tampilkan(b, warmup_done);

  if (Serial.available()) {
    char cmd = (char)Serial.read();
    while (Serial.available()) Serial.read();
    if      (cmd == 's' || cmd == 'S') ambilSampel(b);
    else if (cmd == 'l' || cmd == 'L') tampilLog();
    else if (cmd == 'r' || cmd == 'R') {
      totalLog = 0; noSampel = 1;
      Serial.println(F(">> Log direset."));
    }
  }
}

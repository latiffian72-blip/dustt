# Sistem Filtrasi Kualitas Udara Dari Hasil Pembakaran Sampah

## 📖 Deskripsi

sistem filtrasi kualitas udara dari hasil pembakaran sampah (Smart Dust Suppression and Air Quality Monitoring System) adalah sistem pemantauan kualitas udara dan pengendalian debu berbasis ESP32 yang dirancang untuk mengurangi konsentrasi debu di lingkungan menggunakan sistem penyemprotan air otomatis.

Sistem memonitor tingkat debu, kualitas udara, konsentrasi gas hasil pembakaran, serta kondisi air pada bak penampungan. Data ditampilkan secara lokal melalui LCD I2C dan dapat dipantau maupun dikendalikan melalui antarmuka web yang terhubung ke database PostgreSQL.

---

## 🎯 Tujuan Sistem

* Mengurangi konsentrasi debu di lingkungan kerja.
* Melakukan pemantauan kualitas udara secara real-time.
* Mengontrol kecepatan pompa penyemprot berdasarkan tingkat debu.
* Memantau kekeruhan air pada bak penampung.
* Mengendalikan blower melalui web maupun tombol fisik.
* Menyimpan data monitoring ke database PostgreSQL.
* Menyediakan dashboard monitoring berbasis web.

---

# ⚡ Arsitektur Kelistrikan

## Sumber Daya

Sistem menggunakan sumber listrik PLN 220 VAC.

Distribusi daya dilindungi menggunakan 3 buah MCB:

| MCB     | Fungsi                       |
| ------- | ---------------------------- |
| MCB 10A | MCB utama (main protection)  |
| MCB 6A  | Jalur blower                 |
| MCB 2A  | Jalur PSU dan sistem kontrol |

### Alur Kelistrikan

```text
PLN 220 VAC
      │
      ▼
   MCB 10A
      │
 ┌────┴────┐
 ▼         ▼
MCB 6A   MCB 2A
 │         │
 ▼         ▼
Blower    PSU 220V → 12V/5V
 │         │
SSR       ESP32
           │
           ├── Sensor
           ├── LCD I2C
           ├── Web Interface
           └── Database PostgreSQL
```

---

# 🌪 Sistem Blower

Blower digunakan untuk membantu sirkulasi udara dan proses penanganan debu.

## Pengendalian Blower

Blower dapat dikendalikan melalui:

### Tombol Fisik

Operator dapat menyalakan atau mematikan blower menggunakan push button.

### Web Interface

Operator dapat mengendalikan blower secara remote melalui dashboard web.

### Solid State Relay (SSR)

ESP32 mengontrol blower menggunakan SSR sebagai aktuator switching.

```text
ESP32
   │
   ▼
 SSR
   │
   ▼
Blower 220 VAC
```

---

# 💧 Sistem Pengendalian Pompa Sprayer

Pompa digunakan untuk menyemprotkan air guna menekan konsentrasi debu di udara.

## Sensor Kendali Pompa

Sensor:

* Sharp GP2Y1010AU0F (Sensor Debu 1)

Digunakan sebagai sensor utama untuk menentukan kecepatan pompa.

## Logika Pengendalian

### Kondisi Bersih

* Pompa tetap aktif.
* Kecepatan minimum 50%.
* Tidak boleh kurang dari 50%.

### Kondisi Agak Kotor

* Kecepatan pompa meningkat.

### Kondisi Kotor

* Pompa berputar lebih cepat.

### Kondisi Sangat Kotor

* Pompa berputar pada kecepatan maksimum.
* Sprayer bekerja secara maksimal.

Contoh logika:

| Konsentrasi Debu | Kecepatan Pompa |
| ---------------- | --------------- |
| Bersih           | 50%             |
| Rendah           | 60%             |
| Sedang           | 75%             |
| Tinggi           | 100%            |

---

# 🌫 Sistem Monitoring Debu

Sensor:

* Sharp GP2Y1010AU0F (Sensor Debu 2)

Digunakan khusus untuk monitoring kualitas udara.

Parameter yang diamati:

* Konsentrasi debu udara
* Indikasi tingkat kebersihan lingkungan
* Riwayat kualitas udara

Data ditampilkan pada:

* LCD I2C
* Dashboard Web
* Database PostgreSQL

---

# 🚰 Monitoring Kekeruhan Air

Sensor:

* Turbidity Sensor

Fungsi:

* Memantau kualitas air dalam bak penampungan.
* Menentukan kebutuhan pengurasan atau penggantian air.

Kondisi:

| Nilai Turbidity | Status           |
| --------------- | ---------------- |
| Rendah          | Air Bersih       |
| Sedang          | Perlu Pemantauan |
| Tinggi          | Air Keruh        |
| Sangat Tinggi   | Perlu Pengurasan |

---

# 🌬 Monitoring Kualitas Udara

## Sensor BME

Digunakan untuk:

* Monitoring VOC (Volatile Organic Compounds)
* Deteksi polutan dari asap pembakaran sampah

Parameter:

* VOC Index
* Temperatur
* Kelembapan

---

## Sensor MQ-7

Digunakan untuk mendeteksi:

* Gas Carbon Monoxide (CO)

Fungsi:

* Monitoring potensi pencemaran udara akibat pembakaran.

---

# 🧠 Sistem Kontrol

Mikrokontroler utama:

* ESP32

Tugas ESP32:

* Membaca seluruh sensor
* Mengontrol pompa
* Mengontrol blower
* Menampilkan data ke LCD
* Mengirim data ke web server
* Menyimpan data ke PostgreSQL

---

# 🖥 LCD Display

Jenis:

* LCD 16x2 I2C

Menampilkan:

* Nilai debu
* Status pompa
* Status blower
* Kekeruhan air
* Nilai VOC
* Nilai CO

---

# 🌐 Dashboard Web

Fitur:

* Monitoring real-time
* Grafik histori data sensor
* Kontrol blower ON/OFF
* Status pompa
* Status kualitas udara
* Status kekeruhan air
* Notifikasi kondisi abnormal

---

# 🗄 Database

Database yang digunakan:

* PostgreSQL

Data yang disimpan:

* Data debu
* Data VOC
* Data CO
* Data turbidity
* Status pompa
* Status blower
* Log aktivitas sistem

---

# 📦 Komponen Utama

## Elektrikal

* MCB 10A
* MCB 6A
* MCB 2A
* Power Supply
* SSR
* Blower
* Pompa Air

## Kontrol

* ESP32
* LCD I2C

## Sensor

* 2x Sharp GP2Y1010AU0F
* Turbidity Sensor
* BME Sensor (VOC)
* MQ-7

---

# 🚀 Pengembangan Selanjutnya

* [ ] Sistem alarm otomatis
* [ ] Notifikasi Telegram
* [ ] Notifikasi WhatsApp
* [ ] Logging cloud server
* [ ] Prediksi kualitas udara berbasis AI
* [ ] Auto drain bak penampungan

---

# 📄 License

MIT License

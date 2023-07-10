
#include <Adafruit_Fingerprint.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>

const char* ssid = "iPhone";      // Nama SSID WiFi Anda
const char* password = "1234567890";  // Kata sandi WiFi Anda
const char* servergetar = "http://smartkost.id.mtsshifa.com/api/getars/api";
const char* servergas = "http://smartkost.id.mtsshifa.com/api/gases/api";
const char* serverfinger = "http://smartkost.id.mtsshifa.com/api/fingers";

#if (defined(__AVR__) || defined(ESP8266)) && !defined(__AVR_ATmega2560__)
// For UNO and others without hardware serial, we must use software serial...
// pin #2 is IN from sensor (GREEN wire)
// pin #3 is OUT from arduino  (WHITE wire)
// Set up the serial port to use softwareserial..
SoftwareSerial mySerial(D3, D4);
LiquidCrystal_I2C lcd(0x27, 16, 2); // 0x27 adalah alamat I2C default untuk modul LCD I2C

#else
// On Leonardo/M0/etc, others with hardware serial, use hardware serial!
// #0 is green wire, #1 is white
#define mySerial Serial1

#endif


Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
const int relayPin = D5;

void setup()
{
  Serial.begin(9600);
   pinMode(relayPin, OUTPUT);
  while (!Serial);  // For Yun/Leo/Micro/Zero/...
  delay(100);
  Serial.println("\n\nAdafruit finger detect test");
  // Inisialisasi koneksi I2C
  Wire.begin();
  
  // Inisialisasi LCD
  lcd.begin(16, 2);  // Mengatur ukuran LCD (16 kolom dan 2 baris)
   lcd.backlight();
   Serial.println();
  Serial.print("Menghubungkan ke ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Koneksi WiFi berhasil");
  Serial.print("Alamat IP: ");
  Serial.println(WiFi.localIP());

  // set the data rate for the sensor serial port
  finger.begin(57600);
  delay(5);
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
//    while (1) { delay(1); }
  }

  Serial.println(F("Reading sensor parameters"));
  finger.getParameters();
  Serial.print(F("Status: 0x")); Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x")); Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: ")); Serial.println(finger.capacity);
  Serial.print(F("Security level: ")); Serial.println(finger.security_level);
  Serial.print(F("Device address: ")); Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: ")); Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: ")); Serial.println(finger.baud_rate);

  finger.getTemplateCount();

  if (finger.templateCount == 0) {
    Serial.print("Sensor doesn't contain any fingerprint data. Please run the 'enroll' example.");
  }
  else {
    Serial.println("Waiting for valid finger...");
      Serial.print("Sensor contains "); Serial.print(finger.templateCount); Serial.println(" templates");
  }
}
int getValue(String data, String label) {
  int valueIndex = data.indexOf(label) + label.length(); // Menentukan indeks awal nilai
  int newlineIndex = data.indexOf('\n', valueIndex); // Menentukan indeks akhir nilai
  String valueString = data.substring(valueIndex, newlineIndex); // Mendapatkan string nilai
  int value = valueString.toInt(); // Mengonversi string menjadi integer
  return value;
}
void gas(){
  if (Serial.available() > 0) {
    String data = Serial.readStringUntil('\n'); // Membaca data dari Arduino hingga karakter newline
    Serial.println("Received data: " + data);
    
    // Parsing data untuk mendapatkan nilai gas dan getar
    Serial.println(data);
    int gasValue = getValue(data, "Gas: ");
    int getarValue = getValue(data, "Getar: ");
    
    // Menampilkan nilai gas dan getar
    Serial.print("Gas: ");
    Serial.println(gasValue);
    sendSensorgas(gasValue);
    Serial.print("Getar: ");
    Serial.println(getarValue);
    sendSensorgetar(getarValue);
    
    // Lakukan proses data sesuai kebutuhan Anda
  }
}

void loop()                     // run over and over again
{
  getFingerprintID();
  // Mematikan relay
  digitalWrite(relayPin, HIGH);
  Serial.println("Relay OFF");
  delay(50);            //don't ned to run this at full speed.
}

uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
      gas();
      lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("smart ");
  lcd.setCursor(0,1);
  lcd.print("kost");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    digitalWrite(relayPin, LOW);
  Serial.println("Relay ON");
  // Contoh menampilkan waktu di baris kedua LCD
      lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Selamat Datang");
  sendSensorfinger(finger.fingerID);
  delay(5000);

    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
      lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Tidak Ada Jari");
  delay(1000);
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);

  return finger.fingerID;
}

// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  return finger.fingerID;
}
void sendSensorgas(int value) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;

    // Membuat permintaan POST ke API
    http.begin(client, servergas);

    // Mengatur header permintaan
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    // Membuat payload data
    String postData = "gas=" + String(value) + "&value=" + String(value);

    // Mengirim permintaan POST dengan payload data
    int httpResponseCode = http.POST(postData);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Respon dari server: " + response);
    } else {
      Serial.println("Gagal mengirim permintaan POST");
    }

    http.end();
  } else {
    Serial.println("Tidak terhubung ke jaringan WiFi");
  }
}
void sendSensorgetar(int value) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;

    // Membuat permintaan POST ke API
    http.begin(client, servergetar);

    // Mengatur header permintaan
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    // Membuat payload data
    String postData = "getaran=" + String(value) + "&value=" + String(value);

    // Mengirim permintaan POST dengan payload data
    int httpResponseCode = http.POST(postData);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Respon dari server: " + response);
    } else {
      Serial.println("Gagal mengirim permintaan POST");
    }

    http.end();
  } else {
    Serial.println("Tidak terhubung ke jaringan WiFi");
  }
}
void sendSensorfinger(int value) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;

    // Membuat permintaan POST ke API
    http.begin(client, serverfinger);

    // Mengatur header permintaan
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    // Membuat payload data
    String postData = "nama=" + String("dandy") + "&sidikjari=" + String(value);

    // Mengirim permintaan POST dengan payload data
    int httpResponseCode = http.POST(postData);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Respon dari server: " + response);
    } else {
      Serial.println("Gagal mengirim permintaan POST");
    }

    http.end();
  } else {
    Serial.println("Tidak terhubung ke jaringan WiFi");
  }
}

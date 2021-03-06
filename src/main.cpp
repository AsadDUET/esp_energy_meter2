#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include "time.h"
#include "EEPROM.h"
#include "secrets.h"

int addr = 0;
#define EEPROM_DATA_SIZE 9
#define WIFI_SSID "OPPO A12"
#define WIFI_PASSWORD "1520332020"


const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 6 * 60 * 60;
const int daylightOffset_sec = 0;
char timeStringBuff[9];
String date = "";
struct tm timeinfo;

uint16_t max_c = 0, max_v = 0;
float p = 0, e = 0, a = 100, c, v, dt, price;
int l1 = 18, l2 = 2, l3 = 5, l4 = 4, l5 = 15, i, s_max_c, s_max_v;
int r1 = 14, r2 = 26, r3 = 27, r4 = 13, r5 = 25, ct = 34, pt = 33;
int s_l1 = 1, s_l2 = 1, s_l3 = 1, s_l4 = 1, s_l5 = 1;
unsigned long t = 0, tt = 0;
char EEPROMtimeStringBuff[9];
LiquidCrystal_I2C lcd(0x27, 20, 4);
FirebaseData firebaseData;
String UID = "/esp_energy/cli";

float p_map(float x, float y)
{
  float w = x * y;
  return (0.0000009) * pow(w, 3) - 0.0011 * pow(w, 2) + 1.1934 * w - 1.1923;
}
String ss;
void send_sms()
{
  Serial.printf("\nDate %s usage: %f KWh. Bill: %f.", EEPROMtimeStringBuff, e, e * price);
  Serial2.println("AT+CMGF=1"); // Set the Mode as Text Mode

  delay(150);

  Serial2.println("AT+CMGS=\"+8801936133540\""); // Specify the recipient's number in international format by replacing the 91

  delay(150);
  ss=EEPROMtimeStringBuff;
  Serial2.printf("Date %s-%s-%s \nUsage: %f KWh. \nBill: %f TK.", ss.substring(0,4),ss.substring(4,6),ss.substring(6,8), e, e * price); // Enter the custom message

  delay(150);

  Serial2.write((byte)0x1A); // End of message character 0x1A : Equivalent to Ctrl+z

  delay(50);

  Serial2.println();
}

void save_eeprom()
{
  if (!EEPROM.begin(EEPROM_DATA_SIZE))
  {
    Serial.println("failed to init EEPROM");
    delay(1000000);
  }
  // writing byte-by-byte to EEPROM
  for (int i = 0; i < EEPROM_DATA_SIZE - 1; i++)
  {
    EEPROM.write(addr, timeStringBuff[i]);
    addr += 1;
  }
  EEPROM.commit();
}
void read_eeprom()
{
  for (int i = 0; i < EEPROM_DATA_SIZE - 1; i++)
  {
    byte readValue = EEPROM.read(i);

    if (readValue == 0)
    {
      break;
    }

    EEPROMtimeStringBuff[i] = char(readValue);
  }
  Serial.print(EEPROMtimeStringBuff);
  Serial.print("   eeprom data");
}
void chk_btn()
{
  if (digitalRead(l1))
  {
    s_l1 = !s_l1;
    digitalWrite(r1, s_l1);
    if (Firebase.setString(firebaseData, UID + "/loads/1", String(s_l1)))
    {
    }
    delay(100);
  }
  if (digitalRead(l2))
  {
    s_l2 = !s_l2;
    digitalWrite(r2, s_l2);
    if (Firebase.setString(firebaseData, UID + "/loads/2", String(s_l2)))
    {
    }
    delay(100);
  }
  if (digitalRead(l3))
  {
    s_l3 = !s_l3;
    digitalWrite(r3, s_l3);
    if (Firebase.setString(firebaseData, UID + "/loads/3", String(s_l3)))
    {
    }
    delay(100);
  }
  if (digitalRead(l4))
  {
    s_l4 = !s_l4;
    digitalWrite(r4, s_l4);
    if (Firebase.setString(firebaseData, UID + "/loads/4", String(s_l4)))
    {
    }
    delay(100);
  }
  if (digitalRead(l5))
  {
    s_l5 = !s_l5;
    digitalWrite(r5, s_l5);
    if (Firebase.setString(firebaseData, UID + "/loads/5", String(s_l5)))
    {
    }
    delay(100);
  }
}

void setup()
{

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(r1, OUTPUT);
  pinMode(r2, OUTPUT);
  pinMode(r3, OUTPUT);
  pinMode(r4, OUTPUT);
  pinMode(r5, OUTPUT);
  pinMode(l1, INPUT);
  pinMode(l2, INPUT);
  pinMode(l3, INPUT);
  pinMode(l4, INPUT);
  pinMode(l5, INPUT);
  pinMode(ct, INPUT);
  pinMode(pt, INPUT);
  digitalWrite(r1, 1);
  digitalWrite(r2, 1);
  digitalWrite(r3, 1);
  digitalWrite(r4, 1);
  digitalWrite(r5, 1);
  lcd.init();
  // turn on LCD backlight
  lcd.backlight();
  lcd.clear();
  lcd.print("Connecting...");
  Serial.begin(115200);
  Serial2.begin(115200);
  EEPROM.begin(EEPROM_DATA_SIZE);
  read_eeprom();
  delay(5000);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  lcd.clear();
  lcd.print("Connected");
  Serial.println(WiFi.localIP());
  Serial.println();
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  delay(2000);
  ////////////////////////////////////////////////////////// NTP Time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  ////////////////////////////////////////////Get Time from server

  if (Firebase.getString(firebaseData, UID + "/loads/1"))
  {
    if (firebaseData.stringData() == "0")
      s_l1 = 0;
    else
      s_l1 = 1;
  }

  if (Firebase.getString(firebaseData, UID + "/loads/2"))
  {
    if (firebaseData.stringData() == "0")
      s_l2 = 0;
    else
      s_l2 = 1;
  }
  if (Firebase.getString(firebaseData, UID + "/loads/3"))
  {
    if (firebaseData.stringData() == "0")
      s_l3 = 0;
    else
      s_l3 = 1;
  }

  if (Firebase.getString(firebaseData, UID + "/loads/4"))
  {
    if (firebaseData.stringData() == "0")
      s_l4 = 0;
    else
      s_l4 = 1;
  }

  if (Firebase.getString(firebaseData, UID + "/loads/5"))
  {
    if (firebaseData.stringData() == "0")
      s_l5 = 0;
    else
      s_l5 = 1;
  }
  tt = millis();
}

// the loop function runs over and over again forever
void loop()
{
  digitalWrite(r1, s_l1);
  digitalWrite(r2, s_l2);
  digitalWrite(r3, s_l3);
  digitalWrite(r4, s_l4);
  digitalWrite(r5, s_l5);
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    while (1)
    {
    }
  }
  else
  {
    if (Firebase.getFloat(firebaseData, "/esp_energy/price"))
    {
      price = firebaseData.floatData();
    }
    strftime(timeStringBuff, sizeof(timeStringBuff), "%Y%m%d", &timeinfo);
    read_eeprom();
    if ((EEPROMtimeStringBuff[0] == timeStringBuff[0]) && (EEPROMtimeStringBuff[1] == timeStringBuff[1]) && (EEPROMtimeStringBuff[2] == timeStringBuff[2]) && (EEPROMtimeStringBuff[3] == timeStringBuff[3]) && (EEPROMtimeStringBuff[4] == timeStringBuff[4]) && (EEPROMtimeStringBuff[5] == timeStringBuff[5]) && (EEPROMtimeStringBuff[6] == timeStringBuff[6]) && (EEPROMtimeStringBuff[7] == timeStringBuff[7]))
    {
      if (Firebase.getFloat(firebaseData, UID + "/" + timeStringBuff + "/kwh"))
      {
        e = firebaseData.floatData();
      }
    }
    else
    {
      if (Firebase.getFloat(firebaseData, UID + "/" + EEPROMtimeStringBuff + "/kwh"))
      {
        e = firebaseData.floatData();
      }
      send_sms();
      Serial.printf("\nDate %s usage: %f KWh. Bill: %f.", EEPROMtimeStringBuff, e, e * price);
      save_eeprom();
      e = 0;
      if (Firebase.setFloat(firebaseData, UID + "/" + timeStringBuff + "/kwh", e))
      {
      }
    }
  }
  for (int i = 0; i < 100; i++)
  {
    digitalWrite(r1, s_l1);
    digitalWrite(r2, s_l2);
    digitalWrite(r3, s_l3);
    digitalWrite(r4, s_l4);
    digitalWrite(r5, s_l5);
    e += p * ((millis() - tt) / 3600000.0);
    tt = millis();
    while ((millis() - tt) < 1000)
    {
      max_c = 0;
      s_max_c = 0;
      max_v = 0;
      s_max_v = 0;
      p = 0;
      t = millis();
      a = 0;
      while ((millis() - t) < 500)
      {
        a++;
        max_c = max(analogRead(ct), max_c);
        s_max_c += max_c;
        max_v = max(analogRead(pt), max_v);
        s_max_v += max_v;
        // Serial.print(max_c);
        // Serial.print(",");
        // Serial.println(analogRead(pt));
        chk_btn();
        //   Serial.print("Current: ");
        // Serial.println(max_c);
      }
      // c = (2800 - (s_max_c / a));
      // c = c * 0.003033;
      c = s_max_c / a;
      v = (s_max_v / a);
      v = v * 0.19;
      // p = abs(c*v);
      p = abs(2.0 * (c - 1062)) / 1000.0;
      dt = ((millis() - t) / 3600000.0);
      Serial.print("P: ");
      Serial.println(p);
      // Serial.printf("  e %f", e);
      // Serial.printf("  + %f", (p * dt));
      e += (p * dt);
      // Serial.printf("  E %f", e);

      // digitalWrite(r5,s_l5);
    }
    
    ss=EEPROMtimeStringBuff;
    Serial.printf("Date %s-%s-%s \nUsage: %f KWh. \nBill: %f TK.", ss.substring(0,4),ss.substring(4,6),ss.substring(6,8), e, e * price); // Enter the custom message



    chk_btn();
    tt = millis();
    lcd.clear();
    lcd.print("Load: ");
    lcd.print(p);
    lcd.print(" KW");
    lcd.setCursor(0, 1);
    lcd.print("Total: ");
    lcd.print(e);
    lcd.print(" KWh");
    lcd.setCursor(0, 2);
    lcd.print("Bill: ");
    lcd.print(e * price);
    lcd.print(" TK");
    lcd.setCursor(0, 3);
    chk_btn();
    if (Firebase.setFloat(firebaseData, UID + "/" + timeStringBuff + "/kwh", e))
    {
    }
    chk_btn();
    if (Firebase.setFloat(firebaseData, UID + "/" + timeStringBuff + "/bill", e * price))
    {
    }
    chk_btn();
    if (Firebase.setFloat(firebaseData, UID + "/kw", p))
    {
    }
    chk_btn();
    if (Firebase.getString(firebaseData, UID + "/loads/1"))
    {
      if (firebaseData.stringData() == "0")
        s_l1 = 0;
      else
        s_l1 = 1;
    }
    chk_btn();
    if (Firebase.getString(firebaseData, UID + "/loads/2"))
    {
      if (firebaseData.stringData() == "0")
        s_l2 = 0;
      else
        s_l2 = 1;
    }
    chk_btn();
    if (Firebase.getString(firebaseData, UID + "/loads/3"))
    {
      if (firebaseData.stringData() == "0")
        s_l3 = 0;
      else
        s_l3 = 1;
    }
    chk_btn();
    if (Firebase.getString(firebaseData, UID + "/loads/4"))
    {
      if (firebaseData.stringData() == "0")
        s_l4 = 0;
      else
        s_l4 = 1;
    }
    chk_btn();
    if (Firebase.getString(firebaseData, UID + "/loads/5"))
    {
      if (firebaseData.stringData() == "0")
        s_l5 = 0;
      else
        s_l5 = 1;
    }
    chk_btn();
  }
  chk_btn();
}

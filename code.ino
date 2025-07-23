#define BLYNK_TEMPLATE_ID "TMPL3G4GZXYjL"
#define BLYNK_TEMPLATE_NAME "Baby care"
#define BLYNK_AUTH_TOKEN "uFGMPCwCATUvnGUZtOpCANP4Elw2_vGv"

#include <WiFi.h>
#include <Wire.h>
#include <DHT.h>
#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>
#include "MPU6050.h"
 
char ssid[] = "Wokwi-GUEST";
char pass[] = "";
 
#define DHTPIN 19      
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
 
Servo cradleServo;
#define SERVOPIN 15
 
MPU6050 mpu;
int16_t ax, ay, az;
int16_t gx, gy, gz;
 
bool swing = false;
String cryStatus = "Sleeping";
String movementStatus = "Baby Sleeping";
 
bool cryEventSent = false;
bool tempEventSent = false;
 
BLYNK_WRITE(V6) {
  swing = param.asInt();
}
 
void swingCradle() {
  cradleServo.write(70);
  delay(200);
  cradleServo.write(110);
  delay(200);
}
 
void setup() {
  Serial.begin(9600);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  dht.begin();
  Wire.begin();
  mpu.initialize();
  cradleServo.setPeriodHertz(50);
  cradleServo.attach(SERVOPIN, 500, 2400);
}
 
void loop() {
  Blynk.run();
 
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input == "cry") {
      cryStatus = "Crying";
      Serial.println("Baby is crying");
    } else if (input == "sleep") {
      cryStatus = "Sleeping";
      Serial.println("Baby is sleeping");
    } else {
      Serial.println("Unknown input. Type 'cry' or 'sleep'");
    }
  }
 
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  Blynk.virtualWrite(V1, temp);
  Blynk.virtualWrite(V0, hum);
  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.print(" °C, Humidity: ");
  Serial.print(hum);
  Serial.println(" %");
 
  if (temp > 25 && !tempEventSent) {
    Blynk.logEvent("room_temperature", "🌡️ Temperature above 25°C");
    tempEventSent = true;
  }
  if (temp <= 25) {
    tempEventSent = false; // Reset flag to allow future alerts
  }
 
  mpu.getAcceleration(&ax, &ay, &az);
  mpu.getRotation(&gx, &gy, &gz);
 
  float ax_g = ax / 16384.0;
  float ay_g = ay / 16384.0;
  float az_g = az / 16384.0;
 
  float gx_d = gx / 131.0;
  float gy_d = gy / 131.0;
  float gz_d = gz / 131.0;
 
  Serial.print("Accel (g): X=");
  Serial.print(ax_g);
  Serial.print(" Y=");
  Serial.print(ay_g);
  Serial.print(" Z=");
  Serial.println(az_g);
 
  Serial.print("Gyro (°/s): X=");
  Serial.print(gx_d);
  Serial.print(" Y=");
  Serial.print(gy_d);
  Serial.print(" Z=");
  Serial.println(gz_d);
 
  bool accel_movement = abs(ax_g) > 0.6 || abs(ay_g) > 0.6 || abs(az_g) > 1.2;
  bool gyro_movement = abs(gx_d) > 30 || abs(gy_d) > 30 || abs(gz_d) > 30;
 
  if (accel_movement || gyro_movement) {
    movementStatus = "Baby Moving";
    Serial.println("Movement detected!");
  } else {
    movementStatus = "Baby Sleeping";
    Serial.println("No significant movement.");
  }
 
  String combinedStatus;
  if (cryStatus == "Crying") {
    combinedStatus = "Crying";
    if (!cryEventSent) {
      Blynk.logEvent("baby_status", "🍼 Baby is crying!");
      cryEventSent = true;
    }
  } else {
    cryEventSent = false;
    if (movementStatus == "Baby Moving") {
      combinedStatus = "Baby Moving";
    } else {
      combinedStatus = "Sleeping";
    }
  }
 
  Blynk.virtualWrite(V2, combinedStatus);
 
  if (swing) swingCradle();
  else cradleServo.write(90);
 
  delay(1000);
}
 

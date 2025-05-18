#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#define VIBRATION_PIN 16  // GPIO16 for vibration motor

Adafruit_MPU6050 mpu1; // Will use default address 0x68
Adafruit_MPU6050 mpu2; // Will set to 0x69

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  pinMode(VIBRATION_PIN, OUTPUT);
  digitalWrite(VIBRATION_PIN, LOW); // Motor off initially

  // Initialize first MPU6050 (address 0x68, AD0 to GND)
  if (!mpu1.begin(0x68)) {
    Serial.println("Failed to find MPU6050 #1 at 0x68. Check wiring!");
    while (1) delay(10);
  }
  // Initialize second MPU6050 (address 0x69, AD0 to 3.3V)
  if (!mpu2.begin(0x69)) {
    Serial.println("Failed to find MPU6050 #2 at 0x69. Check wiring!");
    while (1) delay(10);
  }
  Serial.println("Both MPU6050 sensors initialized.");

  mpu1.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu1.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu1.setFilterBandwidth(MPU6050_BAND_21_HZ);

  mpu2.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu2.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu2.setFilterBandwidth(MPU6050_BAND_21_HZ);

  delay(100);
}

void loop() {
  sensors_event_t a1, g1, t1;
  sensors_event_t a2, g2, t2;

  mpu1.getEvent(&a1, &g1, &t1);
  mpu2.getEvent(&a2, &g2, &t2);

  // Output 12 features: Acc1_X, Acc1_Y, Acc1_Z, Acc2_X, Acc2_Y, Acc2_Z, Gyro1_X, Gyro1_Y, Gyro1_Z, Gyro2_X, Gyro2_Y, Gyro2_Z
  Serial.print(a1.acceleration.x); Serial.print(",");
  Serial.print(a1.acceleration.y); Serial.print(",");
  Serial.print(a1.acceleration.z); Serial.print(",");
  Serial.print(a2.acceleration.x); Serial.print(",");
  Serial.print(a2.acceleration.y); Serial.print(",");
  Serial.print(a2.acceleration.z); Serial.print(",");
  Serial.print(g1.gyro.x); Serial.print(",");
  Serial.print(g1.gyro.y); Serial.print(",");
  Serial.print(g1.gyro.z); Serial.print(",");
  Serial.print(g2.gyro.x); Serial.print(",");
  Serial.print(g2.gyro.y); Serial.print(",");
  Serial.println(g2.gyro.z);

  // Listen for vibration commands from PC
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    if (cmd == "VIBRATE") {
      digitalWrite(VIBRATION_PIN, HIGH); // Turn motor ON
    } else if (cmd == "STOP") {
      digitalWrite(VIBRATION_PIN, LOW);  // Turn motor OFF
    }
  }
  delay(400); 
}

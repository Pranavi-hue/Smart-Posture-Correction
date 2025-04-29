#include "TensorFlowLite_ESP32.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/micro/micro_allocator.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"

#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "Posture_Model.h" // Your converted model .h file

#define VIBRATION_PIN 16
#define TENSOR_ARENA_SIZE (60 * 1024)
uint8_t tensor_arena[TENSOR_ARENA_SIZE];

// TFLite variables
const tflite::Model* model;
tflite::MicroInterpreter* interpreter;
TfLiteTensor* input;
TfLiteTensor* output;

// MPU6050s
Adafruit_MPU6050 mpu1;
Adafruit_MPU6050 mpu2;

// Scaler values
float mean[12] = {7.82449, -0.29669, 5.56886, -4.82285, -0.80857, 6.29029, -0.04221, -0.04479, 0.00821, -0.03732, 0.01160, -0.01308};
float scale[12] = {1.68467, 1.40333, 2.02314, 2.94223, 2.18785, 2.00042, 0.09370, 0.10891, 0.06862, 0.09858, 0.10851, 0.09063};

// Timer for incorrect posture
unsigned long incorrectStart = 0;
bool incorrectPostureOngoing = false;

// Error reporter
tflite::MicroErrorReporter micro_error_reporter;
tflite::ErrorReporter* error_reporter = &micro_error_reporter;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Initialize MPU6050s
  if (!mpu1.begin(0x68)) {
    Serial.println("Failed to find MPU6050 #1 at 0x68");
    while (1) delay(10);
  }
  Serial.println("MPU6050 #1 found");

  if (!mpu2.begin(0x69)) {
    Serial.println("Failed to find MPU6050 #2 at 0x69");
    while (1) delay(10);
  }
  Serial.println("MPU6050 #2 found");

  mpu1.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu1.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu1.setFilterBandwidth(MPU6050_BAND_10_HZ);

  mpu2.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu2.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu2.setFilterBandwidth(MPU6050_BAND_10_HZ);

  pinMode(VIBRATION_PIN, OUTPUT);
  digitalWrite(VIBRATION_PIN, LOW);

  // Initialize TFLite model
  tflite::InitializeTarget();
  model = tflite::GetModel(Posture_Model);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("Model schema mismatch!");
    while (1);
  }

  static tflite::MicroMutableOpResolver<5> resolver;
  resolver.AddFullyConnected();
  resolver.AddReshape();
  resolver.AddLogistic();
  resolver.AddRelu();

  static tflite::MicroInterpreter static_interpreter(
    model, resolver, tensor_arena, TENSOR_ARENA_SIZE, error_reporter
  );
  interpreter = &static_interpreter;

  interpreter->AllocateTensors();
  input = interpreter->input(0);
  output = interpreter->output(0);

  Serial.println("Model initialized. Ready to predict posture.");
  Serial.print("Output tensor size: ");
  Serial.println(output->bytes / sizeof(float));
}

void loop() {
  // Read sensor data from MPU6050s
  sensors_event_t a1, g1, t1;
  sensors_event_t a2, g2, t2;

  mpu1.getEvent(&a1, &g1, &t1);
  mpu2.getEvent(&a2, &g2, &t2);

  float input_data[12] = {
    a1.acceleration.x, a1.acceleration.y, a1.acceleration.z,
    a2.acceleration.x, a2.acceleration.y, a2.acceleration.z,
    g1.gyro.x, g1.gyro.y, g1.gyro.z,
    g2.gyro.x, g2.gyro.y, g2.gyro.z
  };

  Serial.print("Normalized input: ");
  for (int i = 0; i < 12; i++) {
    input->data.f[i] = (input_data[i] - mean[i]) / scale[i];
    Serial.print(input->data.f[i], 6);
    Serial.print(", ");
  }
  Serial.println();

  // Inference
  if (interpreter->Invoke() != kTfLiteOk) {
    Serial.println("Invoke failed");
    return;
  }

  // Interpret output
  int output_size = output->bytes / sizeof(float);
  if (output_size == 1) {
    float prediction = output->data.f[0];
    Serial.print("Prediction raw score: ");
    Serial.println(prediction, 6);
    int posture = prediction > 0.5 ? 1 : 0;
    Serial.print("Predicted posture: ");
    Serial.println(posture);
    handlePosture(posture);
  } else if (output_size == 2) {
    float score0 = output->data.f[0];
    float score1 = output->data.f[1];
    Serial.print("Class 0 Score: ");
    Serial.print(score0, 6);
    Serial.print(" | Class 1 Score: ");
    Serial.println(score1, 6);
    int posture = score1 > score0 ? 1 : 0;
    Serial.print("Predicted posture: ");
    Serial.println(posture);
    handlePosture(posture);
  } else {
    Serial.println("Unexpected output tensor size!");
  }

  delay(1000); // Delay before next prediction
}

void handlePosture(int posture) {
  if (posture == 0) {
    if (!incorrectPostureOngoing) {
      incorrectStart = millis();
      incorrectPostureOngoing = true;
    } else if (millis() - incorrectStart >= 60000) {
      Serial.println("Incorrect posture for 1 minute! Vibrating...");
      digitalWrite(VIBRATION_PIN, HIGH);
      delay(5000);
      digitalWrite(VIBRATION_PIN, LOW);
      incorrectStart = millis();
    }
  } else {
    incorrectPostureOngoing = false;
  }
  // Listen for vibration commands from PC
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    if (cmd == "VIBRATE") {
      digitalWrite(VIBRATION_PIN, HIGH); // Turn motor ON
    } else if (cmd == "STOP") {
      digitalWrite(VIBRATION_PIN, LOW);  // Turn motor OFF
    }
  }
  delay(400); // 10 Hz update rate
}
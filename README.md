SmartPosture is a real-time, low-power wearable device designed to monitor and correct upper-body sitting posture using dual MPU6050 IMU sensors and a lightweight Feedforward Neural Network (FNN) deployed on an ESP32 microcontroller. The system detects poor posture and immediately alerts the user through a built-in vibration motor, encouraging corrective action without external apps or cloud dependence.

Key Features:
Real-Time Monitoring: Classifies posture into 1 ideal and 4 improper categories.

Edge AI: Lightweight FNN model with 97.5% accuracy deployed directly on ESP32 (offline).

Instant Feedback: Integrated vibration motor for immediate tactile alerts.

Low Power & Portable: Designed for daily use without cloud or Bluetooth reliance.

Data-Driven: Built from a balanced dataset with over 14,000 labeled IMU samples collected from 20 participants.

Technologies Used: ESP32, MPU6050, TensorFlow Lite, TinyML, Python, Embedded C++.

Components:
ESP32-WROOM-32D – Microcontroller for inference and control.

MPU6050 IMUs – Two sensors placed on the upper spine and shoulder for 6-axis motion data.

Vibration Motor – Provides haptic feedback when bad posture is detected.

Orthopedic Belt – Custom wearable frame for sensor mounting and comfort.

Model Summary:
Accuracy - 97.5%
F1-Score - 0.980
Precision - 0.970
Recall - 0.980
Model Size - 36 KB (.tflite)
Inference Time	<20 ms on ESP32

The model was trained using TensorFlow and converted into a .tflite file for on-device inference. It uses 12 IMU features (accelerometer + gyroscope data) as input.

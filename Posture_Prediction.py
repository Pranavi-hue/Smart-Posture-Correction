import serial
import numpy as np
import tensorflow as tf
import time
import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.metrics import accuracy_score
from tensorflow.keras import Sequential
from tensorflow.keras.layers import Dense
from sklearn.utils import class_weight

# === 1. Load and preprocess dataset ===
dataset_path = r"C:\Users\narni\OneDrive\Documents\Analog Systems Design\Posture Data\Final_Posture_Dataset.csv"
df = pd.read_csv(dataset_path)
print(f"Dataset shape: {df.shape}")

df = df.dropna()
df = df.apply(pd.to_numeric, errors='coerce')
df = df.dropna()

features = df.iloc[:, :-1].values
labels = df.iloc[:, -1].values

# === 2. Normalize (Handle std=0 issue) ===
mean = np.mean(features, axis=0)
std = np.std(features, axis=0)
std[std == 0] = 1e-6  # Prevent divide-by-zero

np.save("mean_values.npy", mean)
np.save("std_values.npy", std)

features_normalized = (features - mean) / std

# === 3. Train/Test split ===
X_train, X_test, y_train, y_test = train_test_split(features_normalized, labels, test_size=0.2, random_state=42)

# Compute class weights
class_weights = class_weight.compute_class_weight('balanced', classes=np.unique(y_train), y=y_train)
class_weights = dict(enumerate(class_weights))

# === 4. Build and train the model ===
model = Sequential()
model.add(Dense(64, input_dim=X_train.shape[1], activation='relu'))
model.add(Dense(32, activation='relu'))
model.add(Dense(1, activation='sigmoid'))
model.compile(loss='binary_crossentropy', optimizer='adam', metrics=['accuracy'])

# Train the model with class weights
model.fit(X_train, y_train, epochs=10, batch_size=32, validation_split=0.2, class_weight=class_weights)

# === 5. Evaluate model ===
y_pred = model.predict(X_test)
y_pred = (y_pred > 0.5).astype(int)
accuracy = accuracy_score(y_test, y_pred)
print(f"Model Accuracy: {accuracy * 100:.2f}%")

# === 6. Convert to TFLite ===
model.save("Posture_Model.h5")
converter = tf.lite.TFLiteConverter.from_keras_model(model)

# Add optimization and supported types
converter.optimizations = [tf.lite.Optimize.DEFAULT]
converter.target_spec.supported_types = [tf.float32]

tflite_model = converter.convert()

with open("Posture_Model.tflite", "wb") as f:
    f.write(tflite_model)

# === 7. Load TFLite model ===
interpreter = tf.lite.Interpreter(model_path="Posture_Model.tflite")
interpreter.allocate_tensors()

input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

# === 8. Setup Serial Communication ===
ser = serial.Serial('COM5', 115200, timeout=1)  # Adjust COM port as needed
time.sleep(2)

bad_posture_start = None
vibration_duration = 2000  # 2 seconds

# Load normalization stats again for inference
mean = np.load("mean_values.npy")
std = np.load("std_values.npy")
std[std == 0] = 1e-6  # Redundant safety check

print("Posture Monitoring Started...")

# === Helper ===
def parse_data(line):
    if line == "":
        return None
    try:
        values = list(map(float, line.strip().split(',')))
        if len(values) == 12:
            return np.array(values)
        else:
            print(f"Invalid feature count: {len(values)}")
    except Exception as e:
        print(f"Parsing error: {e}")
    return None

# === 9. Live Inference Loop ===
while True:
    try:
        line = ser.readline().decode().strip()
        if line == "":
            print("Empty line received.")
            continue

        print(f"Received: {line}")
        features = parse_data(line)

        if features is not None:
            print(f"Raw: {features}")

            # Normalize the features
            normalized = (features - mean) / std

            # Optional clipping — if training used it
            normalized = np.clip(normalized, -3, 3)
            print(f"Normalized and Clipped: {normalized}")

            input_data = np.expand_dims(normalized.astype(np.float32), axis=0)
            interpreter.set_tensor(input_details[0]['index'], input_data)
            interpreter.invoke()

            prediction = interpreter.get_tensor(output_details[0]['index'])[0][0]
            
            # === Enhanced Logging ===
            posture = "GOOD" if prediction > 0.5 else "BAD"
            print(f"Prediction Score: {prediction:.4f} → {posture} POSTURE")

            current_time = time.time()
            if posture == "BAD":
                if bad_posture_start is None:
                    bad_posture_start = current_time
                elif current_time - bad_posture_start >= 60:
                    print("⚠️  Vibrating due to bad posture for 1 min.")
                    ser.write(b'VIBRATE\n')
                    bad_posture_start = current_time
            else:
                bad_posture_start = None
                ser.write(b'STOP\n')

        else:
            print("Parsing failed or invalid data.")

    except KeyboardInterrupt:
        print("Exiting...")
        ser.write(b'STOP\n')
        break
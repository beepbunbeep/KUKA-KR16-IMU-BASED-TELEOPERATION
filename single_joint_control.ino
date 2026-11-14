#include <Wire.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#define MPU_ADDR 0x68

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

float initialPitch = 0.0;
bool isCalibrated = false;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) override {
        deviceConnected = true;
        Serial.println("BLE Client Connected!");
    }

    void onDisconnect(BLEServer* pServer) override {
        deviceConnected = false;
        Serial.println("BLE Client Disconnected. Restarting advertising...");
        pServer->startAdvertising();
    }
};

void setup() {
  Serial.begin(115200);
  while(!Serial) { delay(10); }

  // REPLACE 4 AND 5 WITH YOUR BOARD'S EXACT SDA AND SCL PINS IF DIFFERENT
  Wire.begin(4, 5); 

  // Wake up MPU-6500/9250 core (write 0 to PWR_MGMT_1 register 0x6B)
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0x00);
  byte error = Wire.endTransmission();

  if (error != 0) {
    Serial.println("I2C communication error! Check wiring.");
    while (1) { delay(1000); }
  }
  Serial.println("MPU sensor initialized successfully via direct registers!");

  // Initialize BLE Device
  BLEDevice::init("KUKA_ESP32_C3");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  BLEDevice::startAdvertising();
  Serial.println("BLE Server active. Waiting for connection...");

  // Calibration delay (2 seconds)
  Serial.println("Hold sensor steady for calibration...");
  delay(2000);

  float sumPitch = 0;
  int samples = 30;
  for (int i = 0; i < samples; i++) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B); // Start of accelerometer registers
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, 6, true);

    if (Wire.available() == 6) {
      int16_t ax = (Wire.read() << 8) | Wire.read();
      int16_t ay = (Wire.read() << 8) | Wire.read();
      int16_t az = (Wire.read() << 8) | Wire.read();
      float pitch = atan2((float)ax, sqrt((float)ay * ay + (float)az * az)) * 180.0 / M_PI;
      sumPitch += pitch;
    }
    delay(20);
  }
  initialPitch = sumPitch / samples;
  isCalibrated = true;
  Serial.print("Calibration complete! Initial Pitch Offset: ");
  Serial.println(initialPitch);
}

void loop() {
  if (!isCalibrated) return;

  // Read raw accelerometer data (6 bytes starting at 0x3B)
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6, true);

  if (Wire.available() == 6) {
    int16_t ax = (Wire.read() << 8) | Wire.read();
    int16_t ay = (Wire.read() << 8) | Wire.read();
    int16_t az = (Wire.read() << 8) | Wire.read();

    // Compute pitch angle and subtract calibration offset
    float currentPitch = atan2((float)ax, sqrt((float)ay * ay + (float)az * az)) * 180.0 / M_PI;
    float deltaPitch = currentPitch - initialPitch;
    float jointAngle = constrain(deltaPitch, -60.0, 60.0);

    // Print locally for debugging
    Serial.print("Joint Angle: ");
    Serial.println(jointAngle);

    // Transmit via BLE if connected
    if (deviceConnected) {
      char angleStr[10];
      dtostrf(jointAngle, 6, 2, angleStr);
      pCharacteristic->setValue(angleStr);
      pCharacteristic->notify();
    }
  }

  delay(20); // ~50 Hz update rate
}
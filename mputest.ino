#include <Wire.h>

#define SDA_PIN 8
#define SCL_PIN 9

#define MPU9250_ADDR 0x68
#define AK8963_ADDR  0x0C

// MPU-9250 registers
#define WHO_AM_I       0x75
#define PWR_MGMT_1     0x6B
#define ACCEL_CONFIG   0x1C
#define GYRO_CONFIG    0x1B
#define ACCEL_XOUT_H   0x3B

// AK8963 magnetometer registers
#define AK8963_WHO_AM_I 0x00
#define AK8963_CNTL1    0x0A
#define AK8963_ST1      0x02
#define AK8963_XOUT_L   0x03

void writeRegister(uint8_t address, uint8_t reg, uint8_t value) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

uint8_t readRegister(uint8_t address, uint8_t reg) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.endTransmission(false);

  Wire.requestFrom(address, (uint8_t)1);

  if (Wire.available()) {
    return Wire.read();
  }

  return 0;
}

void readRegisters(uint8_t address, uint8_t reg, uint8_t count, uint8_t *data) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.endTransmission(false);

  Wire.requestFrom(address, count);

  for (uint8_t i = 0; i < count && Wire.available(); i++) {
    data[i] = Wire.read();
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("MPU-9250 ESP32 Test");
  Serial.println("--------------------");

  // Start I2C
  Wire.begin(SDA_PIN, SCL_PIN);

  // Wake up MPU-9250
  writeRegister(MPU9250_ADDR, PWR_MGMT_1, 0x00);
  delay(100);

  // Check MPU-9250
  uint8_t whoAmI = readRegister(MPU9250_ADDR, WHO_AM_I);

  Serial.print("MPU-9250 WHO_AM_I: 0x");
  Serial.println(whoAmI, HEX);

  if (whoAmI == 0x71 || whoAmI == 0x73) {
    Serial.println("MPU-9250 detected!");
  } else {
    Serial.println("WARNING: MPU-9250 not detected.");
    Serial.println("Check wiring and I2C address.");
  }

  // Configure accelerometer ±2g
  writeRegister(MPU9250_ADDR, ACCEL_CONFIG, 0x00);

  // Configure gyro ±250 degrees/sec
  writeRegister(MPU9250_ADDR, GYRO_CONFIG, 0x00);

  // Initialize magnetometer
  writeRegister(AK8963_ADDR, AK8963_CNTL1, 0x00);
  delay(10);

  writeRegister(AK8963_ADDR, AK8963_CNTL1, 0x16);
  delay(10);

  uint8_t magWhoAmI = readRegister(AK8963_ADDR, AK8963_WHO_AM_I);

  Serial.print("AK8963 WHO_AM_I: 0x");
  Serial.println(magWhoAmI, HEX);

  if (magWhoAmI == 0x48) {
    Serial.println("Magnetometer detected!");
  } else {
    Serial.println("WARNING: Magnetometer not detected.");
  }

  Serial.println();
  Serial.println("Starting sensor readings...");
  Serial.println();
}

void loop() {

  // -------------------------
  // Accelerometer + Gyroscope
  // -------------------------

  uint8_t data[14];

  readRegisters(MPU9250_ADDR, ACCEL_XOUT_H, 14, data);

  int16_t accelX = (data[0] << 8) | data[1];
  int16_t accelY = (data[2] << 8) | data[3];
  int16_t accelZ = (data[4] << 8) | data[5];

  int16_t gyroX = (data[8] << 8) | data[9];
  int16_t gyroY = (data[10] << 8) | data[11];
  int16_t gyroZ = (data[12] << 8) | data[13];

  // ±2g -> 16384 LSB/g
  float ax = accelX / 16384.0;
  float ay = accelY / 16384.0;
  float az = accelZ / 16384.0;

  // ±250°/s -> 131 LSB/(°/s)
  float gx = gyroX / 131.0;
  float gy = gyroY / 131.0;
  float gz = gyroZ / 131.0;

  // -------------------------
  // Magnetometer
  // -------------------------

  float mx = 0;
  float my = 0;
  float mz = 0;

  uint8_t magStatus = readRegister(AK8963_ADDR, AK8963_ST1);

  if (magStatus & 0x01) {

    uint8_t magData[7];

    readRegisters(AK8963_ADDR, AK8963_XOUT_L, 7, magData);

    int16_t rawMx = (int16_t)((magData[1] << 8) | magData[0]);
    int16_t rawMy = (int16_t)((magData[3] << 8) | magData[2]);
    int16_t rawMz = (int16_t)((magData[5] << 8) | magData[4]);

    // AK8963 16-bit mode ≈ 0.15 µT/LSB
    mx = rawMx * 0.15;
    my = rawMy * 0.15;
    mz = rawMz * 0.15;
  }

  // -------------------------
  // Print results
  // -------------------------

  Serial.print("ACC [g]   ");
  Serial.print(ax, 3);
  Serial.print("  ");
  Serial.print(ay, 3);
  Serial.print("  ");
  Serial.print(az, 3);

  Serial.print("    GYRO [deg/s]   ");
  Serial.print(gx, 2);
  Serial.print("  ");
  Serial.print(gy, 2);
  Serial.print("  ");
  Serial.print(gz, 2);

  Serial.print("    MAG [uT]   ");
  Serial.print(mx, 2);
  Serial.print("  ");
  Serial.print(my, 2);
  Serial.print("  ");
  Serial.println(mz, 2);

  delay(100);
}
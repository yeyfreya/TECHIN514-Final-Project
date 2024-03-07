#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLECharacteristic.h>
#include <BLE2902.h>


// UUIDs for the BLE service and characteristics
#define SERVICE_UUID           "6ae875c2-acd9-4c4c-83f9-d5f4f319e828" // Custom Service UUID
#define MOTION_CHARACTERISTIC_UUID "77c1eb1a-ad50-41c6-bd7a-6eab19a488e0" // Custom Characteristic UUIDs
#define TOUCH_CHARACTERISTIC_UUID  "d3decb63-f959-4a91-aadc-dcad399504f3"
#define DISTANCE_CHARACTERISTIC_UUID "aa1aebd5-ff03-46e3-91f0-bfc714f2283f"

Adafruit_MPU6050 mpu;
int touchPin = 1;
const int ledPin = LED_BUILTIN;

BLEServer* pServer = NULL;
BLEService* pService = NULL;
BLECharacteristic* pMotionCharacteristic = NULL;
BLECharacteristic* pTouchCharacteristic = NULL;
BLECharacteristic* pDistanceCharacteristic = NULL;

bool deviceConnected = false;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};


void setup() {
    Serial.begin(115200);
    BLEDevice::init("SensorDevice");
  
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    pService = pServer->createService(SERVICE_UUID);

    // Create a BLE Characteristic for each piece of data
    pMotionCharacteristic = pService->createCharacteristic(
                                        MOTION_CHARACTERISTIC_UUID,
                                        BLECharacteristic::PROPERTY_READ | 
                                        BLECharacteristic::PROPERTY_NOTIFY);
    pMotionCharacteristic->addDescriptor(new BLE2902());

    pTouchCharacteristic = pService->createCharacteristic(
                                        TOUCH_CHARACTERISTIC_UUID,
                                        BLECharacteristic::PROPERTY_READ |
                                        BLECharacteristic::PROPERTY_NOTIFY);
    pTouchCharacteristic->addDescriptor(new BLE2902());

    pDistanceCharacteristic = pService->createCharacteristic(
                                        DISTANCE_CHARACTERISTIC_UUID,
                                        BLECharacteristic::PROPERTY_READ |
                                        BLECharacteristic::PROPERTY_NOTIFY);
    pDistanceCharacteristic->addDescriptor(new BLE2902());

    pService->start();

    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    BLEDevice::startAdvertising();
    Serial.println("Waiting for a client connection to notify...");
    
    pinMode(ledPin, OUTPUT);
    if (!mpu.begin()) {
        Serial.println("Failed to find MPU6050 chip");
        while (1) {
            delay(10);
        }
    }
    
    Serial.println("MPU6050 Found!");
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
}


void loop() {
    /* Read the accelerometer and gyroscope */
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    Serial.print("Accel X: "); Serial.print(a.acceleration.x); Serial.print(" m/s^2, ");
    Serial.print("Y: "); Serial.print(a.acceleration.y); Serial.print(" m/s^2, ");
    Serial.print("Z: "); Serial.print(a.acceleration.z); Serial.println(" m/s^2");
    
    Serial.print("Gyro X: "); Serial.print(g.gyro.x); Serial.print(" rad/s, ");
    Serial.print("Y: "); Serial.print(g.gyro.y); Serial.print(" rad/s, ");
    Serial.print("Z: "); Serial.print(g.gyro.z); Serial.println(" rad/s");


    // Reading from the touch sensor
    int touchValue = touchRead(touchPin); // Read the touch sensor on touchPin
    Serial.print("Touch Sensor Value: ");
    Serial.println(touchValue);
    
    // Prepare MPU6050 data string
    char mpuData[128];
    snprintf(mpuData, sizeof(mpuData), "AccelX:%f,AccelY:%f,AccelZ:%f,GyroX:%f,GyroY:%f,GyroZ:%f", 
         a.acceleration.x, a.acceleration.y, a.acceleration.z,
         g.gyro.x, g.gyro.y, g.gyro.z);
    
    
    if (deviceConnected) {
        pMotionCharacteristic->setValue(mpuData);
        pMotionCharacteristic->notify();

        pTouchCharacteristic->setValue(String(touchValue).c_str());
        pTouchCharacteristic->notify();
        
        digitalWrite(ledPin, LOW);
        delay(500);
        digitalWrite(ledPin, HIGH);
        delay(500);

        // Since RSSI is a property of the connection, not the server's state,
        // it cannot be directly sent from here. It will be read on the client side.

        delay(100); 
    }
    
    
     

}

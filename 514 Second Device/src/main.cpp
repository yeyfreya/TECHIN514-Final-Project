#include <Arduino.h>
#include <FastLED.h>
#include <Stepper.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// Stepper motor pins
const int motorPin1 = D0; // Coil 1
const int motorPin2 = D1; // Coil 1
const int motorPin3 = D3; // Coil 2
const int motorPin4 = D2; // Coil 2

// Stepper motor configuration
#define STEPS 315 // Steps per revolution for your stepper motor
Stepper myStepper(STEPS, motorPin1, motorPin2, motorPin3, motorPin4);

// Define target positions for "far", "middle", and "near" stages
const int positionFar = STEPS / 3; // Far
const int positionMiddle = STEPS / 3 * 2; // Middle
const int positionNear = STEPS; // Near

int currentPosition = 0; // Current position of the stepper motor


// Define the sequence of activation for a 4-phase stepper motor
const int motorSteps[8][4] = {
  {HIGH, LOW, LOW, LOW},
  {HIGH, HIGH, LOW, LOW},
  {LOW, HIGH, LOW, LOW},
  {LOW, HIGH, HIGH, LOW},
  {LOW, LOW, HIGH, LOW},
  {LOW, LOW, HIGH, HIGH},
  {LOW, LOW, LOW, HIGH},
  {HIGH, LOW, LOW, HIGH}
};

// LED strip configuration
#define LED_PIN     D8
#define NUM_LEDS    3

CRGB leds[NUM_LEDS];

// Speaker pin
const int speakerPin = D9;

// UUIDs for service and characteristics
static BLEUUID serviceUUID("6ae875c2-acd9-4c4c-83f9-d5f4f319e828");
static BLEUUID motionCharUUID("77c1eb1a-ad50-41c6-bd7a-6eab19a488e0");
static BLEUUID touchCharUUID("d3decb63-f959-4a91-aadc-dcad399504f3");
static BLEUUID distanceCharUUID("aa1aebd5-ff03-46e3-91f0-bfc714f2283f");

// Flag for the connected state
static boolean connected = false;
// Flag to indicate a scan should be started
static boolean doScan = true;
// The remote characteristic to subscribe to
static BLERemoteCharacteristic* pRemoteCharacteristic;
// The device that we want to connect to
static BLEAdvertisedDevice* myDevice;

static BLEClient* pClient  = nullptr;

const int ledPin = LED_BUILTIN;

// Movement and touch threshold logic
bool isNear = false; // Assuming this variable will be set to true when the IR sensor reads "near"
bool touchTriggered = false; // Set to true when touch threshold is exceeded
bool movementDetected = false;
bool touchThresholdReached = false;
const float movementThreshold = 2.0; // Adjust based on sensor sensitivity

void blinkRedLED() {
    for(int i = 0; i < NUM_LEDS; i++) leds[i] = CRGB::Red;
    FastLED.show();
    delay(200);
    for(int i = 0; i < NUM_LEDS; i++) leds[i] = CRGB::Black;
    FastLED.show();
    delay(200);
}

void playTone() {
    tone(speakerPin, 1000); // Play a 1kHz tone
    delay(200); // Wait for 200ms
    noTone(speakerPin); // Stop the tone
    Serial.println("Playing tone");
}

// Function to initialize motor control pins
void setupStepper() {
    pinMode(motorPin1, OUTPUT);
    pinMode(motorPin2, OUTPUT);
    pinMode(motorPin3, OUTPUT);
    pinMode(motorPin4, OUTPUT);
}

void stepMotor(int thisStep) {
    digitalWrite(motorPin1, motorSteps[thisStep][0]);
    digitalWrite(motorPin2, motorSteps[thisStep][1]);
    digitalWrite(motorPin3, motorSteps[thisStep][2]);
    digitalWrite(motorPin4, motorSteps[thisStep][3]);
}

// Simplify moveStepper to ensure consistent direction movement
void moveStepper(int steps) {
    for (int i = 0; i < steps; i++) {
        for (int step = 0; step < 8; step++) {
            stepMotor(step); // Corrected to pass the current step index
            delay(10); // Adjust delay to control speed
        }
    }
}


// // Function to move stepper motor based on proximity value
// void moveStepper(int steps) {
//     int direction = steps > 0 ? 1 : -1; // Determine direction based on sign of steps
//     steps = abs(steps); // Use absolute value for steps count

//     for(int i = 0; i < steps; i++) {
//         for (int step = 0; step < 8; step++) { // Loop through step sequence
//             // Set motor pins according to motorSteps sequence
//             digitalWrite(motorPin1, motorSteps[step][0]);
//             digitalWrite(motorPin2, motorSteps[step][1]);
//             digitalWrite(motorPin3, motorSteps[step][2]);
//             digitalWrite(motorPin4, motorSteps[step][3]);
//             delay(10); // Short delay to control step speed
//         }
//     }

//     // Optionally add here a method to reset pins to LOW to reduce power consumption if needed
// }

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    connected = true;
    Serial.println("Connected to the server.");
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("Disconnected from the server.");
  }
};

// Callback for receiving notifications from the server
// static void notifyCallback(
//   BLERemoteCharacteristic* pBLERemoteCharacteristic,
//   uint8_t* pData,
//   size_t length,
//   bool isNotify) {
//     Serial.print("Received Data from ");
//     Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
//     Serial.print(": ");
//     for (size_t i = 0; i < length; i++) {
//         Serial.print((char)pData[i]);
//     }
//     Serial.println();
// }

void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    Serial.print("Received Data: ");
    String data((char*)pData, length);
    Serial.println(data);

    // Parsing touch sensor data
    if (pBLERemoteCharacteristic->getUUID().equals(touchCharUUID)) {
        int touchValue = data.toInt();
        if (touchValue > 70000) { // Assuming 40000 is the threshold
            Serial.println("Touch threshold exceeded. Triggering actuators.");
            blinkRedLED();
            playTone();

            // For the stepper motor, assume a simple forward movement for demonstration
            moveStepper(50); // Move 50 steps as an example
        }
    }

//     // Parsing touch sensor data
//     if (pBLERemoteCharacteristic->getUUID().equals(touchCharUUID)) {
//         int touchValue = data.toInt();
//         touchThresholdReached = (touchValue > 40000); 
//     }

//     // Parsing motion (MPU6050) data
//     else if (pBLERemoteCharacteristic->getUUID().equals(motionCharUUID)) {
//         float accelX, accelY, accelZ, gyroX, gyroY, gyroZ;
//         sscanf(data.c_str(), "AccelX:%f,AccelY:%f,AccelZ:%f,GyroX:%f,GyroY:%f,GyroZ:%f", 
//                &accelX, &accelY, &accelZ, &gyroX, &gyroY, &gyroZ);

//         // Simple movement detection logic
//         movementDetected = (abs(accelX) > movementThreshold || abs(accelY) > movementThreshold || 
//                             abs(accelZ) > movementThreshold || abs(gyroX) > movementThreshold || 
//                             abs(gyroY) > movementThreshold || abs(gyroZ) > movementThreshold);
//     }

//     // Trigger actions if both movement is detected and touch threshold is reached
//     if (movementDetected && touchThresholdReached) {
//         Serial.println("Action triggered due to movement and touch threshold.");
//         // Example action: blink an LED, vibrate a motor, etc.
//         // Reset flags if you want the action to be one-time until conditions are met again
//         movementDetected = false;
//         touchThresholdReached = false;
//         blinkRedLED();
//         playTone();
//     }
}


// Function to connect and setup notifications
bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());

    pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");
    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the server
    if (!pClient->connect(myDevice)) {
      Serial.println("Failed to connect");
      return false;
    }
    
    // Obtain a reference to the service
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.println("Failed to find our service UUID.");
      return false;
    }
    Serial.println(" - Found our service");

    // Subscribe to the motion characteristic
    pRemoteCharacteristic = pRemoteService->getCharacteristic(motionCharUUID);
    if (pRemoteCharacteristic != nullptr && pRemoteCharacteristic->canNotify()) {
      pRemoteCharacteristic->registerForNotify(notifyCallback);
    }

    // Subscribe to the touch characteristic
    pRemoteCharacteristic = pRemoteService->getCharacteristic(touchCharUUID);
    if (pRemoteCharacteristic != nullptr && pRemoteCharacteristic->canNotify()) {
      pRemoteCharacteristic->registerForNotify(notifyCallback);
    }

    // Subscribe to the distance characteristic
    pRemoteCharacteristic = pRemoteService->getCharacteristic(distanceCharUUID);
    if (pRemoteCharacteristic != nullptr && pRemoteCharacteristic->canNotify()) {
      pRemoteCharacteristic->registerForNotify(notifyCallback);
    }

    return true;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.getServiceUUID().equals(serviceUUID)) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doScan = false;
      Serial.println("Device found. Connecting!");
    }
  }
};


void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");

  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  pinMode(speakerPin, OUTPUT);

  setupStepper(); // Initialize stepper motor pins

  myStepper.setSpeed(100); // Set the speed of the stepper motor

  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}

void loop() {
  // If we are not yet connected but have found the device we want to connect to
  if (!connected && !doScan) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothing more we will do.");
    }
  }
  

  if (connected && pClient) {
    int rssi = pClient->getRssi();
    Serial.print("RSSI: ");
    Serial.println(rssi);

    // Trigger LED and speaker based on proximity
    if (rssi > -15) { // Near
      Serial.println("Near");
      blinkRedLED();
      playTone();
    }
    rssi = 0;

    // Example RSSI to position mapping logic
    int targetPosition;
    if (rssi > -15) { // Near
      targetPosition = positionNear;
    } else if (rssi > -50) { // Middle
      targetPosition = positionMiddle;
    } else { // Far
      targetPosition = positionFar;
    }

    // Move stepper motor to target position
    int stepsToTarget = targetPosition - currentPosition;
    myStepper.step(stepsToTarget);
    currentPosition = targetPosition; // Update current position


    // Your existing delay or logic
    delay(2000);
  }

  // If disconnected, start scanning again
  if (!connected && !doScan) {
    BLEDevice::getScan()->start(0, false); // 0 means continuous scanning
    doScan = true;
  }
}



// Movement detection using MPU6050 data involves interpreting acceleration or gyro readings. For simplicity, we'll assume movement is detected if any acceleration reading exceeds a certain threshold.
// The stepper motor's rotation will indicate proximity, with the number of steps representing the RSSI strength (signal strength inversely related to distance).
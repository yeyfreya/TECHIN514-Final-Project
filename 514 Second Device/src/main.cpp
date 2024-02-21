#include <Arduino.h>
#include <FastLED.h>

// Stepper motor pins
const int motorPin1 = 0; // Coil 1
const int motorPin2 = 1; // Coil 1
const int motorPin3 = 2; // Coil 2
const int motorPin4 = 3; // Coil 2

// LED strip configuration
#define LED_PIN     8
#define NUM_LEDS    10 
CRGB leds[NUM_LEDS];

// Speaker pin
const int speakerPin = 7; /
// Function to step the motor
void stepMotor(int step) {
    static const int steps[4][4] = {
        {HIGH, LOW, LOW, LOW},
        {LOW, HIGH, LOW, LOW},
        {LOW, LOW, HIGH, LOW},
        {LOW, LOW, LOW, HIGH}
    };

    digitalWrite(motorPin1, steps[step][0]);
    digitalWrite(motorPin2, steps[step][1]);
    digitalWrite(motorPin3, steps[step][2]);
    digitalWrite(motorPin4, steps[step][3]);
}

// Setup function
void setup() {
  // Initialize stepper motor pins
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(motorPin3, OUTPUT);
  pinMode(motorPin4, OUTPUT);

  // Initialize LED strip
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);

  // Initialize speaker pin
  pinMode(speakerPin, OUTPUT);
}

// Loop function
void loop() {
  // Step the motor
  for (int step = 0; step < 4; step++) {
    stepMotor(step);
    delay(10); 
  }

  // Blink the LED strip
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::White; // Turn on the LED to white
  }
  FastLED.show();
  delay(500);

  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black; // Turn off the LED
  }
  FastLED.show();
  delay(500);

  // Play a tone on the speaker
  tone(speakerPin, 1000); // Play a 1kHz tone
  delay(500);
  noTone(speakerPin); // Stop the tone
  delay(500);
}

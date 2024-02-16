#include <CapacitiveSensor.h>

CapacitiveSensor sensor = CapacitiveSensor(D0,D2);

void setup()
{
  Serial.begin(9600);

  // Disable the automßatic re-calibration feature of the
  // capacitive sensor library
  sensor.set_CS_AutocaL_Millis(0xFFFFFFFF);
}

void loop()
{
  long current_millis = millis();
  long capacitance = sensor.capacitiveSensor(30);

  // Print the result of the sensor reading
  // Note that the capacitance value is an arbitrary number
  // See: https://playground.arduino.cc/Main/CapacitiveSensor/ for details
  Serial.println(capacitance);

  // Wait for 50 milliseconds
  while(millis() - current_millis < 50);
}
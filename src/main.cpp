#include <Arduino.h>

// Define LED_PIN as the pin number where the onboard LED is connected.
// On most Arduino boards, including the Feather, it's usually pin 13.
const int LED_PIN = 13;

void setup() {
  // Initialize the LED pin as an output.
  pinMode(LED_PIN, OUTPUT);

  // Initialize the serial communication at 9600 baud rate.
  Serial.begin(9600);
}

void loop() {
  // Turn the LED on by making the voltage HIGH.
  digitalWrite(LED_PIN, HIGH);

  // Print to the Serial Monitor.
  Serial.println("LED ON");

  // Wait for a second.
  delay(1000);

  // Turn the LED off by making the voltage LOW.
  digitalWrite(LED_PIN, LOW);

  // Print to the Serial Monitor.
  Serial.println("LED OFF");

  // Wait for a second.
  delay(1000);
}

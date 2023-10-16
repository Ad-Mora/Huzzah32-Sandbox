#include "HX711.h"

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 14;
const int LOADCELL_SCK_PIN = 13;

HX711 scale;

void setup() {
  Serial.begin(115200);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(388.0);
  scale.tare();
  Serial.println("Starting scale...");
}

// void setup() {
//   Serial.begin(115200);
//   delay(2000);
//   Serial.println("Starting setup...");

//   scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
//   scale.tare();
  
//   Serial.println("Place known weight on the scale");
//   delay(10000);
//   Serial.println("Delay done. Reading...");
  
//   long rawReading = scale.get_units(50);

//   float knownWeight = 500.0;
//   Serial.println("Known weight:");
//   Serial.println(knownWeight);
  
//   Serial.println("Raw reading:");
//   Serial.println(rawReading);
  
//   float calibrationFactor = rawReading / knownWeight;

//   Serial.println("Calibration factor (raw / known):");
//   Serial.println(calibrationFactor, 3);
// }


void loop() {
  if (scale.is_ready()) {
    // float weight = scale.get_units(10);
    double weight = scale.get_units();
    Serial.print("Grams: ");
    Serial.println(weight);
  } else {
    Serial.println("HX711 not found.");
  }

  delay(1000);
}

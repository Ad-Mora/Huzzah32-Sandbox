#include <Arduino.h>

const int speakerPin = 21; // Signal pin connected to the Stemma speaker

void setup() {
    Serial.begin(115200);
}

// In this example, the tone is continuous. You can modify the loop to change tones, add delays, etc.
void loop() {
    tone(speakerPin, 523, 3000); // C5
    tone(speakerPin, 0, 2000);
    tone(speakerPin, 1046, 3000); // C6
}


// tone(speakerPin, 523, 150); // C5
// delay(400);
// tone(speakerPin, 523, 150);
// delay(1500);

// tone(speakerPin, 523, 250); // C5
// tone(speakerPin, 740, 250); // Fs5
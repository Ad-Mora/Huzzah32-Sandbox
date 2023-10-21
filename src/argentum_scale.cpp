#include <HX711.h>
#include <vector>

const int ONBOARD_LED_PIN = 13;
const int LOADCELL_DOUT_PIN = 14;
const int LOADCELL_SCK_PIN = 15;
const int ALERT_PIN = 21;
const int LOCK_INDICATOR_PIN = 33;

const int REFRESH_PERIOD_MILLIS = 30 * 1000; // refresh allowed weights every 30 seconds
const float CALIBRATION_FACTOR = 400.7;
const float CALIBRATION_WEIGHT = 500.0; // in grams
const float WEIGHT_ERROR_MARGIN = 10; // in grams

HX711 scale;
std::vector<int> allowed_weights;
unsigned long last_refreshed = 0;


void calibrate_scale() {
  Serial.println("Starting calibration...");

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.tare();
  
  Serial.println("Place known weight on the scale");
  delay(10000);
  Serial.println("Delay done. Reading...");
  
  long raw_reading = scale.get_units(50);

  Serial.println("Known weight:");
  Serial.println(CALIBRATION_WEIGHT);
  
  Serial.println("Raw reading:");
  Serial.println(raw_reading);
  
  float calibration_factor = raw_reading / CALIBRATION_WEIGHT;

  Serial.println("Calibration factor (raw / known):");
  Serial.println(calibration_factor, 3);
}

bool refresh_weights() {
  // TODO: stub for now. replace with actual API call
  allowed_weights.clear();
  allowed_weights = {500};
  return true;
}

bool weight_is_valid(float weight) {
  for (const int &value : allowed_weights) {
    int low_bound = value - WEIGHT_ERROR_MARGIN;
    int high_bound = value + WEIGHT_ERROR_MARGIN;
    if (low_bound <= weight && weight <= high_bound) {
      return true;
    }
  }
  return false;
}

bool check_and_validate_weight(bool recheck = false) {
  float weight = scale.get_units(25);
  Serial.print("Grams: ");
  Serial.println(weight);

  if (weight_is_valid(weight)) {
    Serial.println("Weight is valid!");
    return true;
  } else if (recheck == false) {
    delay(2500);
    Serial.println("First reading invalid. Rechecking...");
    return check_and_validate_weight(true);
  } else {
    Serial.println("Invalid weight detected.");
    return false;
  }
}

void init_scale() {
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(CALIBRATION_FACTOR);
  scale.tare();
  Serial.println("Starting scale...");
}

/**
 * Init or update allowed_weights by querying the API. Only update weights if
 * enough time has passed since the last update.
*/
void handle_weight_refresh() {
  unsigned long time_since_refresh = millis() - last_refreshed;
  Serial.print("Time since refresh: ");
  Serial.println(time_since_refresh);
  if (last_refreshed == 0 || time_since_refresh > REFRESH_PERIOD_MILLIS) {
    Serial.println("Refresh needed!");
    // keep looping until weight refresh succeeds
    while (true) {
      bool success = refresh_weights();
      if (success) {
          Serial.print("Successfully refreshed weights! (first weight | array size): ");
          Serial.print(allowed_weights[0]);
          Serial.print(" | ");
          Serial.println(allowed_weights.size());
          last_refreshed = millis();
          Serial.print("Last refresh time set to: ");
          Serial.println(last_refreshed);
          break;
      } else {
        Serial.println("Error refreshing weights. Retrying...");
        digitalWrite(ONBOARD_LED_PIN, HIGH);
      }
      delay(1000);
    }
  }
  digitalWrite(ONBOARD_LED_PIN, LOW);
}


void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("Starting setup...");

  allowed_weights.reserve(100);

  // setup pins
  pinMode(ALERT_PIN, OUTPUT);
  pinMode(LOCK_INDICATOR_PIN, OUTPUT);
  pinMode(ONBOARD_LED_PIN, OUTPUT);

  // prepare scale
  init_scale();
}


void loop() {
  Serial.print("Begin loop. Timestamp: ");
  Serial.println(millis());

  // check pins
  Serial.print("ONBOARD LED PIN: ");
  Serial.println(digitalRead(ONBOARD_LED_PIN));
  Serial.print("LOCK INDICATOR PIN: ");
  Serial.println(digitalRead(LOCK_INDICATOR_PIN));
  Serial.print("ALERT PIN: ");
  Serial.println(digitalRead(ALERT_PIN));

  // refresh allowed weights if appropriate. update globals
  handle_weight_refresh();

  // set lock indicator PIN
  bool is_locked = allowed_weights.size() == 1;
  digitalWrite(LOCK_INDICATOR_PIN, is_locked);

  // check if weight is valid; if not, alert
  bool weight_is_valid = check_and_validate_weight();
  digitalWrite(ALERT_PIN, !weight_is_valid);

  delay(1000);
}

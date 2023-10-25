#include <HX711.h>
#include <vector>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// MCU pin config
const int ONBOARD_LED_PIN = 13;
const int LOADCELL_DOUT_PIN = 14;
const int LOADCELL_SCK_PIN = 15;
const int ALERT_PIN = 21;
const int LOCK_INDICATOR_PIN = 33;

// scale config
const int REFRESH_PERIOD_MILLIS = 45 * 1000; // refresh allowed weights every 30 seconds
const float CALIBRATION_FACTOR = 400.7;
const float CALIBRATION_WEIGHT = 500.0; // in grams
const float WEIGHT_ERROR_MARGIN = 20; // in grams

// wifi config
const char* ssid = "Adrian's WiFi";
const char* password = "mahogany648";
const char* host = "us-east4-argentum-397607.cloudfunctions.net";
const int https_port = 443;
const char* root_cert = R"EOF(-----BEGIN CERTIFICATE-----
MIIFVzCCAz+gAwIBAgINAgPlk28xsBNJiGuiFzANBgkqhkiG9w0BAQwFADBHMQsw
CQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEU
MBIGA1UEAxMLR1RTIFJvb3QgUjEwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAw
MDAwWjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZp
Y2VzIExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjEwggIiMA0GCSqGSIb3DQEBAQUA
A4ICDwAwggIKAoICAQC2EQKLHuOhd5s73L+UPreVp0A8of2C+X0yBoJx9vaMf/vo
27xqLpeXo4xL+Sv2sfnOhB2x+cWX3u+58qPpvBKJXqeqUqv4IyfLpLGcY9vXmX7w
Cl7raKb0xlpHDU0QM+NOsROjyBhsS+z8CZDfnWQpJSMHobTSPS5g4M/SCYe7zUjw
TcLCeoiKu7rPWRnWr4+wB7CeMfGCwcDfLqZtbBkOtdh+JhpFAz2weaSUKK0Pfybl
qAj+lug8aJRT7oM6iCsVlgmy4HqMLnXWnOunVmSPlk9orj2XwoSPwLxAwAtcvfaH
szVsrBhQf4TgTM2S0yDpM7xSma8ytSmzJSq0SPly4cpk9+aCEI3oncKKiPo4Zor8
Y/kB+Xj9e1x3+naH+uzfsQ55lVe0vSbv1gHR6xYKu44LtcXFilWr06zqkUspzBmk
MiVOKvFlRNACzqrOSbTqn3yDsEB750Orp2yjj32JgfpMpf/VjsPOS+C12LOORc92
wO1AK/1TD7Cn1TsNsYqiA94xrcx36m97PtbfkSIS5r762DL8EGMUUXLeXdYWk70p
aDPvOmbsB4om3xPXV2V4J95eSRQAogB/mqghtqmxlbCluQ0WEdrHbEg8QOB+DVrN
VjzRlwW5y0vtOUucxD/SVRNuJLDWcfr0wbrM7Rv1/oFB2ACYPTrIrnqYNxgFlQID
AQABo0IwQDAOBgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4E
FgQU5K8rJnEaK0gnhS9SZizv8IkTcT4wDQYJKoZIhvcNAQEMBQADggIBAJ+qQibb
C5u+/x6Wki4+omVKapi6Ist9wTrYggoGxval3sBOh2Z5ofmmWJyq+bXmYOfg6LEe
QkEzCzc9zolwFcq1JKjPa7XSQCGYzyI0zzvFIoTgxQ6KfF2I5DUkzps+GlQebtuy
h6f88/qBVRRiClmpIgUxPoLW7ttXNLwzldMXG+gnoot7TiYaelpkttGsN/H9oPM4
7HLwEXWdyzRSjeZ2axfG34arJ45JK3VmgRAhpuo+9K4l/3wV3s6MJT/KYnAK9y8J
ZgfIPxz88NtFMN9iiMG1D53Dn0reWVlHxYciNuaCp+0KueIHoI17eko8cdLiA6Ef
MgfdG+RCzgwARWGAtQsgWSl4vflVy2PFPEz0tv/bal8xa5meLMFrUKTX5hgUvYU/
Z6tGn6D/Qqc6f1zLXbBwHSs09dR2CQzreExZBfMzQsNhFRAbd03OIozUhfJFfbdT
6u9AWpQKXCBfTkBdYiJ23//OYb2MI3jSNwLgjt7RETeJ9r/tSQdirpLsQBqvFAnZ
0E6yove+7u7Y/9waLd64NnHi/Hm3lCXRSHNboTXns5lndcEZOitHTtNCjv0xyBZm
2tIMPNuzjsmhDYAPexZ3FL//2wmUspO8IFgV6dtxQ/PeEMMA3KgqlbbC1j+Qa3bb
bP6MvPJwNQzcmRk13NfIRmPVNnGuV/u3gm3c
-----END CERTIFICATE-----
)EOF";

// alert config
const int WARNING_ALERT_TIME = 20 * 1000;

HX711 scale;
std::vector<int> allowed_weights;
unsigned long last_refreshed = 0;
bool last_weight_is_valid = true;
unsigned long alert_start = 0;


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

bool refresh_weights() {
    WiFiClientSecure client;
    client.setCACert(root_cert);

    Serial.print("Connecting to ");
    Serial.println(host);

    if (!client.connect(host, https_port)) {
        Serial.println("Connection failed!");
        return false;
    }
    Serial.println("Connection succeeded!");

    String url = "/get_allowed_weights";
    client.println("GET " + url + " HTTP/1.1");
    client.println(String("Host: ") + host);
    client.println("User-Agent: ESP32");
    client.println("Connection: close");
    client.println();

    while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
            break;
        }
    }

    // get json response
    String json_response = client.readString();
    Serial.println("JSON response: " + json_response);

    StaticJsonDocument<1000> doc;
    DeserializationError error = deserializeJson(doc, json_response);

    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return false;
    }
    Serial.println("Successfully deserialized json!");

    JsonObject data = doc["data"];
    JsonArray weights = data["weights"].as<JsonArray>();

    allowed_weights.clear();
    for (int weight: weights) {
        allowed_weights.push_back(weight);
    }

    client.stop();
    Serial.println("Connection closed.");
    return true;
}

bool check_and_validate_weight(bool recheck = false) {
    float weight = scale.get_units(10);
    Serial.print("Grams: ");
    Serial.println(weight);

    if (weight_is_valid(weight)) {
        Serial.println("Weight is valid!");
        return true;
    } else if (recheck == false) {
        delay(500);
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

void init_wifi() {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Successfully connected to WiFi!");
}

void play_warning() {
    for (int i = 0; i < 2; ++i) {
        tone(ALERT_PIN, 587, 200); // D5
        tone(ALERT_PIN, 0, 300);
        tone(ALERT_PIN, 587, 200); // D5
        delay(2000);
    }
}

void play_full_alert() {
    for (int i = 0; i < 10; ++i) {
        tone(ALERT_PIN, 740, 250); // Fs5
        tone(ALERT_PIN, 523, 250); // C5
        delay(500);
    }
}

void play_valid_weight() {
    tone(ALERT_PIN, 523, 100); // C5
    tone(ALERT_PIN, 1046, 100); // C6
}

void handle_alert(bool weight_is_valid, bool is_transition) {
    if (!weight_is_valid) {
        unsigned long time_since_alert_start = millis() - alert_start;
        Serial.println("Time since alert start: " + String(time_since_alert_start));
        if (time_since_alert_start > WARNING_ALERT_TIME) {
            Serial.println("Playing full alert...");
            play_full_alert();
        } else {
            Serial.println("Playing warning alert...");
            play_warning();
        }
    } else if (is_transition && weight_is_valid) {
        Serial.println("Playing valid weight alert...");
        play_valid_weight();
    }
}

void handle_weight_check_and_alert() {
    // detect transitioning from alert to non-alerting, and vv
    bool new_weight_is_valid = check_and_validate_weight();
    bool is_alert_transition = false;

    if (new_weight_is_valid != last_weight_is_valid) {
        is_alert_transition = true;
        if (!new_weight_is_valid) {
            alert_start = millis();
        }
    }
    last_weight_is_valid = new_weight_is_valid;

    Serial.println("new_weight_is_valid: " + String(new_weight_is_valid));
    Serial.println("is_alert_transition: " + String(is_alert_transition));

    // play alert
    handle_alert(new_weight_is_valid, is_alert_transition);
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

    // setup wifi
    init_wifi();

    // prepare scale
    init_scale();
}


void loop() {
    Serial.println();
    Serial.print("Begin loop. Timestamp: ");
    Serial.println(millis());
    Serial.println();

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

    // check the weight and decide whether to alert
    handle_weight_check_and_alert();

    delay(100);
}

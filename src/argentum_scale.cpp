#include <HX711.h>
#include <vector>
#include <WiFi.h>
#include <WiFiClientSecure.h>

// MCU pin config
const int ONBOARD_LED_PIN = 13;
const int LOADCELL_DOUT_PIN = 14;
const int LOADCELL_SCK_PIN = 15;
const int ALERT_PIN = 21;
const int LOCK_INDICATOR_PIN = 33;

// scale config
const int REFRESH_PERIOD_MILLIS = 30 * 1000; // refresh allowed weights every 30 seconds
const float CALIBRATION_FACTOR = 400.7;
const float CALIBRATION_WEIGHT = 500.0; // in grams
const float WEIGHT_ERROR_MARGIN = 10; // in grams

// wifi config
const char* ssid = "Adrian's WiFi";
const char* password = "mahogany648";
const char* host = "example.com";
const int https_port = 443;
const char* root_cert = R"EOF(-----BEGIN CERTIFICATE-----
MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD
QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB
CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97
nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt
43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P
T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4
gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO
BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR
TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw
DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr
hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg
06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF
PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls
YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk
CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=
-----END CERTIFICATE-----
)EOF";

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
    allowed_weights = {313};
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

void get_allowed_weights() {
    WiFiClientSecure client;
    client.setCACert(root_cert);

    Serial.print("Connecting to ");
    Serial.println(host);

    if (!client.connect(host, https_port)) {
        Serial.println("Connection failed!");
        return;
    }
    Serial.println("Connection succeeded!");

    String url = "/";
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

    // Print response
    String line = client.readString();
    Serial.println("Client response: " + line);

    client.stop();
    Serial.println("Connection closed.");
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

    // tmp
    get_allowed_weights();
}

void loop() {
    Serial.println("stub...");
    delay(1000);
}


void looptmp() {
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

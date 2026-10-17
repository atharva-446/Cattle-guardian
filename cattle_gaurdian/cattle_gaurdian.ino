
/*
 * CATTLEGUARDIAN - Smart Cattle Monitoring Prototype
 * Board: Arduino Uno
 *
 * Reference implementation based on the supplied diagrams.
 * Verify pin assignments and module interfaces before wiring.
 *
 * Libraries:
 *   - DHT sensor library by Adafruit
 *   - Adafruit Unified Sensor (dependency)
 *   - LiquidCrystal (included with Arduino IDE)
 */

#include <DHT.h>
#include <LiquidCrystal.h>

// ---------- Proposed pin assignments ----------

// Digital sensors
const byte FLAME_PIN = 2;
const byte PIR_PIN   = 3;
const byte DHT_PIN   = 4;

// Outputs
const byte RED_LED_PIN = 5;
const byte BUZZER_PIN  = 6;
const byte RELAY_PIN   = 7;

// Analog sensors
const byte LDR_PIN   = A0;
const byte GAS_PIN   = A1;
const byte WATER_PIN = A2;

// Parallel 16x2 LCD: RS, E, D4, D5, D6, D7
LiquidCrystal lcd(8, 9, 10, 11, 12, 13);

#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

// ---------- Configuration ----------

// Verify these thresholds experimentally for your modules.
const int GAS_THRESHOLD = 400;
const int DARK_THRESHOLD = 350;
const int WATER_THRESHOLD = 400;

// Many flame modules output LOW when flame is detected.
// Change this if your module has opposite polarity.
const byte FLAME_ACTIVE_STATE = LOW;

// Many relay modules are active LOW.
// Change this to HIGH if your module is active HIGH.
const byte RELAY_ON_STATE = LOW;

const unsigned long SENSOR_INTERVAL_MS = 2000;

// Set true only after verifying that relay-controlled
// operation is appropriate for your actual circuit.
const bool ENABLE_RELAY_CONTROL = false;

unsigned long lastReadTime = 0;

void setRelay(bool enabled) {
  if (!ENABLE_RELAY_CONTROL) {
    // Keep relay output in its OFF state during initial testing.
    digitalWrite(
      RELAY_PIN,
      RELAY_ON_STATE == LOW ? HIGH : LOW
    );
    return;
  }

  digitalWrite(
    RELAY_PIN,
    enabled ? RELAY_ON_STATE :
              (RELAY_ON_STATE == LOW ? HIGH : LOW)
  );
}

void showStatus(
  float temperature,
  float humidity,
  bool dhtValid,
  bool flameDetected,
  bool motionDetected,
  bool gasAlert,
  bool waterDetected,
  bool dark
) {
  lcd.clear();

  if (flameDetected || gasAlert) {
    lcd.setCursor(0, 0);
    lcd.print("FIRE/GAS ALERT");
  } else if (waterDetected) {
    lcd.setCursor(0, 0);
    lcd.print("WATER DETECTED");
  } else if (motionDetected) {
    lcd.setCursor(0, 0);
    lcd.print("MOTION DETECTED");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("CATTLEGUARDIAN");
  }

  lcd.setCursor(0, 1);

  if (dhtValid) {
    lcd.print("T:");
    lcd.print(temperature, 0);
    lcd.print("C H:");
    lcd.print(humidity, 0);
    lcd.print("%");
  } else {
    lcd.print(dark ? "DARK " : "LIGHT ");
    lcd.print("System Active");
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(FLAME_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);

  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);

  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // Keep the relay inactive at startup.
  digitalWrite(
    RELAY_PIN,
    RELAY_ON_STATE == LOW ? HIGH : LOW
  );

  lcd.begin(16, 2);
  dht.begin();

  lcd.setCursor(0, 0);
  lcd.print("CATTLEGUARDIAN");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  delay(1500);

  Serial.println("CATTLEGUARDIAN started.");
}

void loop() {
  if (millis() - lastReadTime < SENSOR_INTERVAL_MS) {
    return;
  }

  lastReadTime = millis();

  // Read sensors
  bool flameDetected =
    digitalRead(FLAME_PIN) == FLAME_ACTIVE_STATE;

  bool motionDetected =
    digitalRead(PIR_PIN) == HIGH;

  int lightValue = analogRead(LDR_PIN);
  int gasValue = analogRead(GAS_PIN);
  int waterValue = analogRead(WATER_PIN);

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  bool dhtValid =
    !isnan(temperature) && !isnan(humidity);

  bool gasAlert = gasValue >= GAS_THRESHOLD;
  bool waterDetected = waterValue >= WATER_THRESHOLD;
  bool dark = lightValue < DARK_THRESHOLD;

  // General warning output.
  bool warning =
    flameDetected || gasAlert || motionDetected;

  digitalWrite(RED_LED_PIN, warning ? HIGH : LOW);
  digitalWrite(BUZZER_PIN, warning ? HIGH : LOW);

  // Relay control remains disabled by default.
  // Do not activate a solenoid until the load, relay,
  // power supply, and intended behavior are verified.
  setRelay(false);

  showStatus(
    temperature,
    humidity,
    dhtValid,
    flameDetected,
    motionDetected,
    gasAlert,
    waterDetected,
    dark
  );

  // Serial diagnostics
  Serial.println("--- Sensor Status ---");

  Serial.print("Flame detected: ");
  Serial.println(flameDetected ? "YES" : "NO");

  Serial.print("Motion detected: ");
  Serial.println(motionDetected ? "YES" : "NO");

  Serial.print("LDR analog value: ");
  Serial.println(lightValue);

  Serial.print("Gas sensor analog value: ");
  Serial.println(gasValue);

  Serial.print("Water sensor analog value: ");
  Serial.println(waterValue);

  if (dhtValid) {
    Serial.print("Temperature (C): ");
    Serial.println(temperature);

    Serial.print("Humidity (%): ");
    Serial.println(humidity);
  } else {
    Serial.println("DHT11 reading unavailable.");
  }

  Serial.println();
}

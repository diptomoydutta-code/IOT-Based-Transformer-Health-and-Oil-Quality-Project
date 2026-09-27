/*
  ESP32 transformer-oil monitor:
  reads temperature, turbidity, voltage, and current sensors, then
  reports values to Blynk and activates the LED and buzzer on faults.
*/

#define BLYNK_TEMPLATE_ID "TMPL3OHbWVse0"
#define BLYNK_TEMPLATE_NAME "Transformer oil health"
#define BLYNK_AUTH_TOKEN "nr6lacd5-0FCuog60lKY0xmVvL4KI2Pc"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 4
#define TURBIDITY_PIN 6
#define VOLTAGE_PIN 1
#define CURRENT_PIN 5
#define LED_PIN 2
#define BUZZER_PIN 15

const char ssid[] = "CirkitWifi";
const char pass[] = "";

const float TEMP_LIMIT = 75.0;
const int TURBIDITY_LIMIT = 1700;
const float VOLTAGE_LIMIT = 235.0;
const float CURRENT_LIMIT = 8.0;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature temperatureSensor(&oneWire);
BlynkTimer timer;

bool faultActive = false;
bool previousFault = false;

void readAndReport()
{
  float temperature;
  int turbidity;
  float voltage;
  float current;

  temperatureSensor.requestTemperatures();
  temperature = temperatureSensor.getTempCByIndex(0);

  if (temperature == DEVICE_DISCONNECTED_C) {
    temperature = 25.0;
  }

  turbidity = analogRead(TURBIDITY_PIN);

  int voltageRaw = analogRead(VOLTAGE_PIN);
  voltage = (voltageRaw / 4095.0) * 300.0;

  int currentRaw = analogRead(CURRENT_PIN);
  current = ((currentRaw - 2048) / 4095.0) * 30.0;

  if (current < 0.0) {
    current = 0.0;
  }

  faultActive = temperature > TEMP_LIMIT;
  faultActive = faultActive || turbidity > TURBIDITY_LIMIT;
  faultActive = faultActive || voltage > VOLTAGE_LIMIT;
  faultActive = faultActive || current > CURRENT_LIMIT;

  digitalWrite(LED_PIN, faultActive ? HIGH : LOW);

  if (faultActive) {
    tone(BUZZER_PIN, 1000);

    if (!previousFault) {
      Blynk.logEvent(
        "transformer_fault",
        "CRITICAL: transformer parameter exceeded its limit"
      );
    }
  } else {
    noTone(BUZZER_PIN);
  }

  previousFault = faultActive;

  Blynk.virtualWrite(V1, temperature);
  Blynk.virtualWrite(V2, turbidity);
  Blynk.virtualWrite(V3, voltage);
  Blynk.virtualWrite(V4, current);

  Serial.print("Temp: ");
  Serial.print(temperature, 1);
  Serial.print(" C | Turbidity: ");
  Serial.print(turbidity);
  Serial.print(" | Voltage: ");
  Serial.print(voltage, 1);
  Serial.print(" V | Current: ");
  Serial.print(current, 1);
  Serial.println(" A");
}

void setup()
{
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  analogReadResolution(12);
  temperatureSensor.begin();

  Serial.println("Connecting to Blynk...");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  Serial.println("Connected.");

  timer.setInterval(2000L, readAndReport);
}

void loop()
{
  Blynk.run();
  timer.run();
}

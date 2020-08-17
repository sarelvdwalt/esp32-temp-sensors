#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <InfluxDbClient.h>
#include <WiFi.h>

#include <secrets.h>

#define BLINK_LED 2
#define DEVICE "ESP32"

// GPIO where the DS18B20 is connected to
const int oneWireBus = 4;

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

// InfluxDB  server url. Don't use localhost, always server name or ip address.
// For InfluxDB 2 e.g. http://192.168.1.48:9999 (Use: InfluxDB UI -> Load Data -> Client Libraries), 
// For InfluxDB 1 e.g. http://192.168.1.48:8086
#define INFLUXDB_URL "http://192.168.1.33:8086"

#define INFLUXDB_DB_NAME "esp32"

// Single InfluxDB instance
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_DB_NAME);

// Data point
Point pointDevice("device_status");

void setup() {
  // put your setup code here, to run once:
  // Serial.begin(115200);
  // Serial.write("Start");
  // pinMode(BLINK_LED, OUTPUT);

  Serial.begin(115200);         // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println('\n');

  delay(1000);
  WiFi.begin(ssid, password);             // Connect to the network
  Serial.print("Connecting to ");
  Serial.print(ssid);

  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(500);
    Serial.print('.');
  }

  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());         // Send the IP address of the ESP8266 to the computer

  // Add tags
  pointDevice.addTag("device", DEVICE);
  pointDevice.addTag("SSID", WiFi.SSID());
  // Add data
  pointDevice.addField("rssi", WiFi.RSSI());
  pointDevice.addField("uptime", millis());

  // Check server connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }

  // Write data
  client.writePoint(pointDevice);
}

Point pointTemp("temperature");

void loop() {

  // // locate devices on the bus
  // Serial.print("Locating devices...");
  // Serial.print("Found ");
  // Serial.print(sensors.getDeviceCount(), DEC);
  // Serial.println(" devices.");

  // // report parasite power requirements
  // Serial.print("Parasite power is: ");
  // if (sensors.isParasitePowerMode()) Serial.println("ON");
  // else Serial.println("OFF");

  sensors.requestTemperatures();
  float temperatureC = sensors.getTempCByIndex(0);
  float temperatureF = sensors.getTempFByIndex(0);

  // On cold boot, he sensors are sometimes not calibrated properly and will return bogus data to inform us that it's having trouble.
  if (temperatureC == 25.00) {
    Serial.println("Sensur uncalibrated... skipping...");
    sleep(1); // sleep 1 second
    return; // break out of the loop
  }
  
  Serial.print(temperatureC);
  Serial.println("ºC");
  Serial.print(temperatureF);
  Serial.println("ºF");
  Serial.println();

  // Serial.println(sensors.getAddress())
  client.resetBuffer(); // make sure there's nothing remaining from previous loop
  pointTemp.clearTags();
  pointTemp.clearFields();

  pointTemp.addTag("location", "studeerkamer");
  pointTemp.addField("value", temperatureC);
  pointTemp.addField("fahrenheit", temperatureF);
  
  Serial.println(pointTemp.toLineProtocol());
  client.writePoint(pointTemp);

  

  delay(5000);

  static unsigned long nextSwitchTime = millis() + 300000; // every 5 minutes (300k milliseconds)
  if (nextSwitchTime < millis()) {
    
    client.resetBuffer();
    pointDevice.clearTags();
    pointDevice.clearFields();
    
    // Add tags
    pointDevice.addTag("device", DEVICE);
    pointDevice.addTag("SSID", WiFi.SSID());
    
    // Add data
    pointDevice.addField("rssi", WiFi.RSSI());
    pointDevice.addField("uptime", millis());

    // Write data
    client.writePoint(pointDevice);

    nextSwitchTime = millis() + 300000;
  }
  

  // digitalWrite(BLINK_LED, LOW);
  // Serial.println("LED is low");
  // delay(900);
  // digitalWrite(BLINK_LED, HIGH);
  // Serial.println("LED is high");
  // delay(100);
}
//  SMART GARDEN PROJECT
//  5 Relays for Solenoide Valves
//  5 Moisture Sensors
//
// Working:
//   Solenoid Valves will open every morning (7:00 AM) and evening (5:00 PM) for 5 minutes or until moisture sensor detects water. When water is detected, the valve will close and the next valve will open.
//   Moisture sensors will be read every 5 minutes and the data will be sent to the cloud. Watering will be skipped if the soil is wet. Config file from cloud will be fetched every time the device is restarted.
//   Watering Times will be set from the cloud. Valves can be manually opened or closed from the cloud. Entire machine can be turned off from the cloud. MQTT will be used for communication.

// Default Values

#define WIFI_SSID "Flamingo"
#define WIFI_PASSWORD "123456789"
#define MQTT_SERVER "broker.hivemq.com"
#define MQTT_USERNAME ""
#define MQTT_PASSWORD ""
#define MQTT_CLIENT_NAME "SmartGarden-82FA"
#define MQTT_PORT 1883
#define DEFAULT_WATERING_DURATION 5 * 60
#define DEFAULT_WATERING_INTERVAL 30

// Pin Definitions
#define SOLENOID_VALVE_1 0
#define SOLENOID_VALVE_2 1
#define SOLENOID_VALVE_3 2
#define SOLENOID_VALVE_4 3
#define SOLENOID_VALVE_5 4
#define SOLENOID_VALVE_COUNT 5

#define MOISTURE_SENSOR_1 5
#define MOISTURE_SENSOR_2 6
#define MOISTURE_SENSOR_3 7
#define MOISTURE_SENSOR_4 8
#define MOISTURE_SENSOR_5 9
#define MOISTURE_SENSOR_COUNT 5

// Commands from Cloud
#define CMD_OPEN_VALVE "open_valve"
#define CMD_CLOSE_VALVE "close_valve"
#define CMD_GET_MOISTURE_SENSOR "get_moisture_sensor"
#define CMD_GET_VALVE_STATUS "get_valve_status"

#define CMD_GET_CONFIG "get_config"
#define CMD_SET_CONFIG "set_config"

#define CMD_RESTART "restart"
#define CMD_SHUTDOWN "shutdown"
#define CMD_GET_UPTIME "get_uptime"
#define CMD_GET_STATUS "get_status"

// Commands to Cloud
#define CMD_STATUS "status"
#define CMD_MOISTURE_SENSOR "moisture_sensor"
#define CMD_VALVE_STATUS "valve_status"
#define CMD_UPTIME "uptime"
#define CMD_CONFIG "config"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PCF8575.h>
#include <EspMQTTClient.h>
#include <EEPROM.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Wire.h>

// Moisture sensor pins as array
uint8_t moistureSensorPins[MOISTURE_SENSOR_COUNT] = {MOISTURE_SENSOR_1, MOISTURE_SENSOR_2, MOISTURE_SENSOR_3, MOISTURE_SENSOR_4, MOISTURE_SENSOR_5};
// Solenoid Valve pins as array
uint8_t solenoidValvePins[SOLENOID_VALVE_COUNT] = {SOLENOID_VALVE_1, SOLENOID_VALVE_2, SOLENOID_VALVE_3, SOLENOID_VALVE_4, SOLENOID_VALVE_5};

EspMQTTClient client(
    WIFI_SSID,
    WIFI_PASSWORD,
    MQTT_SERVER,      // MQTT Broker server ip
    MQTT_USERNAME,    // Can be omitted if not needed
    MQTT_PASSWORD,    // Can be omitted if not needed
    MQTT_CLIENT_NAME, // Client name that uniquely identify your device
    MQTT_PORT         // The MQTT port, default to 1883. this line can be omitted
);

WiFiUDP ntpUDP;               // UDP client
NTPClient timeClient(ntpUDP); // NTP client

PCF8575 PCF;

#define SERIAL_DEBUG 1

#if SERIAL_DEBUG
#define P(x) Serial.print(x)
#define PN(x) Serial.println(x)
#define PT(x)             \
  Serial.print(millis()); \
  Serial.print(F(": "));  \
  Serial.println(x)
#define PS(x) serializeJsonPretty(x, Serial)
#else
#define P(x)
#define PN(x)
#define PT(x)
#define PS(x)
#endif

StaticJsonDocument<800> config;

String getTopicName(const String &topic)
{
  return "/" + String(MQTT_CLIENT_NAME) + topic;
}

void openValve(int valve)
{
  PCF.write(solenoidValvePins[valve], LOW);
  PT("Valve " + String(solenoidValvePins[valve]) + " Opened");
}

void closeValve(int valve)
{
  PCF.write(solenoidValvePins[valve], HIGH);
  PT("Valve " + String(solenoidValvePins[valve]) + " Closed");
}

bool valveStatus(int valve)
{
  if (PCF.read(solenoidValvePins[valve]) == 0)
  {
    return true;
  }
  else
  {
    return false;
  }
}

void openAllValves()
{

  for (int i = 0; i < SOLENOID_VALVE_COUNT; i++)
  {
    openValve(i);
  }
}

void closeAllValves()
{
  for (int i = 0; i < SOLENOID_VALVE_COUNT; i++)
  {
    closeValve(i);
  }
}

bool readMoistureSensor(int sensor)
{
  uint8_t moisture = PCF.read(moistureSensorPins[sensor]);
  PT("Moisture Sensor " + String(sensor) + " Value: " + String(moisture));
  if (moisture == 0)
  {
    return true;
  }
  else
  {
    return false;
  }
}

String convertToJsonString(JsonDocument &doc)
{
  String output;
  serializeJson(doc, output);
  return output;
}

void sendFeedbackToCloud(JsonDocument &payload, const String &topic)
{
  client.publish(getTopicName(topic), convertToJsonString(payload));
  payload.clear();
}

void sendFeedbackToCloud(JsonDocument &payload)
{
  sendFeedbackToCloud(payload, "/feedback");
}

void loadDefaultConfig()
{
  config["watering_duration"] = 5 * 60;
  config["watering_interval"] = 30;
  config["watering_enabled"] = true;
  JsonArray arr = config["watering_times"].to<JsonArray>();
  arr.add(7 * 60);
  arr.add(17 * 60);
  config["ssid"] = WIFI_SSID;
  config["password"] = WIFI_PASSWORD;
  config["mqtt_server"] = MQTT_SERVER;
  config["mqtt_port"] = MQTT_PORT;
  config["mqtt_username"] = MQTT_USERNAME;
  config["mqtt_password"] = MQTT_PASSWORD;
  config["mqtt_client_name"] = MQTT_CLIENT_NAME;
}
// Function to serialize and store config to EEPROM as json.
void saveConfig(JsonDocument &config)
{
  // Serialize config to json string
  String configJson;
  serializeJson(config, configJson);
  PT("Saving config to EEPROM: " + configJson);

  // Get configJson length
  int configJsonLength = configJson.length();

  // Write configJson length to EEPROM
  EEPROM.write(0, configJsonLength >> 8);
  EEPROM.write(1, configJsonLength & 0xFF);

  // Write configJson to EEPROM
  for (int i = 0; i < configJsonLength; ++i)
  {
    EEPROM.write(2 + i, configJson[i]);
  }

  // Commit EEPROM
  EEPROM.commit();
}

// Function to read and deserialize config from EEPROM.
void loadConfig(JsonDocument &config)
{
  // Read configJson length from EEPROM
  int configJsonLength = (EEPROM.read(0) << 8) + EEPROM.read(1);
  PT("Reading config from EEPROM, length: " + String(configJsonLength));

  // Check if configJson length is valid

  if (configJsonLength <= 0 || configJsonLength > 4096)
  {
    PT("Config json length is invalid, using default config.");
    return;
  }

  config.clear();

  // Read configJson from EEPROM
  String configJson;
  for (int i = 0; i < configJsonLength; ++i)
  {
    configJson += char(EEPROM.read(2 + i));
  }

  // Deserialize configJson
  DeserializationError error = deserializeJson(config, configJson);
  if (error)
  {
    PT("EEPROM : deserializeJson() failed: ");
    PT(error.c_str());
    return;
  }
}

void onConnectionEstablished()
{
  // (Re)Subscribe to MQTT topic to receive commands
  client.subscribe(getTopicName("/commands"), [](const String &payload)
                   {
                     PT("Received payload: ");

                     // Parse JSON object
                     StaticJsonDocument<200> i_pld;
                     DeserializationError error = deserializeJson(i_pld, payload);
                     if (error)
                     {
                       PT("deserializeJson() failed: ");
                       PT(error.c_str());
                       return;
                     }
                     PS(i_pld);
                     // Check if JSON object contains expected fields
                     if (!i_pld.containsKey("command"))
                     {
                       PT("Missing command field");
                       return;
                     }

                     // Fetch values
                     String command = i_pld["command"];
                     if (command == CMD_OPEN_VALVE)
                     {
                       int valve = i_pld["valve"].as<int>();
                       openValve(valve);
                     }
                     else if (command == CMD_CLOSE_VALVE)
                     {
                       int valve = i_pld["valve"].as<int>();
                       closeValve(valve);
                     }
                     else if (command == CMD_GET_MOISTURE_SENSOR)
                     {
                       int sensor = i_pld["sensor"];
                       bool moisture = readMoistureSensor(sensor);
                       StaticJsonDocument<200> o_pld;
                       o_pld["moisture"] = moisture;
                       o_pld["sensor"] = sensor;
                       o_pld["command"] = CMD_MOISTURE_SENSOR;
                       sendFeedbackToCloud(o_pld);
                     }
                     else if (command == CMD_GET_VALVE_STATUS)
                     {

                       int valve = i_pld["valve"];
                       bool status = valveStatus(valve);
                       StaticJsonDocument<200> o_pld;
                       o_pld["command"] = CMD_VALVE_STATUS;
                       o_pld["valve"] = valve;
                       o_pld["status"] = status;
                       sendFeedbackToCloud(o_pld);
                     }
                     else if (command == CMD_GET_CONFIG)
                     {
                       return;
                     }
                     else if (command == CMD_GET_UPTIME)
                     {
                       StaticJsonDocument<200> o_pld;
                       o_pld["command"] = CMD_UPTIME;
                       o_pld["uptime"] = ESP.getCycleCount() / (ESP.getCpuFreqMHz() * 1E6);
                       sendFeedbackToCloud(o_pld);
                       return;
                     }
                     else if (command == CMD_GET_CONFIG){
                                      config[command] = CMD_CONFIG;
                       client.publish(getTopicName("/feedback"),
                                      convertToJsonString(config));
                       return;
                     }
                     else if (command == CMD_SET_CONFIG)
                     {
                      saveConfig(i_pld);
                      loadConfig(config);

                      return;
                     }
                     
                     else if (command == CMD_GET_STATUS)
                     {
                        StaticJsonDocument<800> o_pld;
                        o_pld["command"] = CMD_STATUS;
                        o_pld["uptime"] = ESP.getCycleCount() / (ESP.getCpuFreqMHz() * 1E6);
                        for (uint8_t i = 0; i < MOISTURE_SENSOR_COUNT; i++)
                        {
                          o_pld["moisture" + String(i)] = readMoistureSensor(i);
                        }
                        for (uint8_t i = 0; i < SOLENOID_VALVE_COUNT; i++)
                        {
                          o_pld["valve" + String(i)] = valveStatus(i);
                        }
                        sendFeedbackToCloud(o_pld);
                        return;
                     }
                     else if (command == CMD_SHUTDOWN)
                     {
                        int time =  i_pld["time"].as<int>();
                        PT("Shutting down");
                        ESP.deepSleep( time * 1E6);
                       return;
                     }
                     else if (command == CMD_RESTART)
                     {
                       PT("Restarting...");
                       ESP.restart();
                       return;
                     }
                     else
                     {
                       PT("Unknown command");
                       return;
                     } }

  );

  client.executeDelayed(2 * 1000, []()
                        { client.publish(getTopicName("/status"), "Online"); });
}

byte scanForI2CAddress()
{
  // Scan I2C bus for devices
  PT("Scanning I2C bus for devices...");
  Wire.begin();

  byte error, address;
  for (address = 0x20; address < 0x28; address++)
  {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0)
    {
      P("I2C device found at address 0x");
      Serial.print(address, HEX);
      P(" !");
      break;
    }
  }
  return address;
}

void setup()
{
#if SERIAL_DEBUG
  Serial.begin(115200);
#endif
  loadDefaultConfig();
  saveConfig(config);
  EEPROM.begin(1024);
  loadConfig(config);

  client.setMqttClientName(config["mqtt_client_name"]);
  client.setWifiCredentials(config["ssid"], config["password"]);
  client.setMqttServer(config["mqtt_server"], config["mqtt_username"], config["mqtt_password"], config["mqtt_port"]);
  client.setOnConnectionEstablishedCallback(onConnectionEstablished);

#if SERIAL_DEBUG
  client.enableDebuggingMessages(true);
#endif

  PCF.setAddress(scanForI2CAddress());
  if (!PCF.begin())
  {
    PT("PCF8575 not found");
  }
  if (!PCF.isConnected())
  {
    PT("PCF8575 not connected");
    while (1)
      delay(1000);
  }

  // Set Time client to GMT +5:30
  timeClient.setTimeOffset(19800);
  timeClient.begin();
}

unsigned long lastRun = 0;
boolean wateringStarted = false;
uint8_t currentWateringValve = 0;
uint8_t lastWateredDay = 0;
uint8_t lastWateredTime = 0;
bool isWateringTime = false;
int currentWateringTime = 0;

unsigned long lastWateringStart = 0;

unsigned long lastupdate = 0;

void loop()
{
  client.loop();

  // run basic function once in 10 seconds
  if (millis() - lastRun > 10E3)
  {

    lastRun = millis();
    // Check if any valve is open and close it if moisture is detected
    for (uint8_t i = 0; i < MOISTURE_SENSOR_COUNT; i++)
    {
      if (readMoistureSensor(i) && valveStatus(i))
      {
        closeValve(i);
      }
    }

    int currentTime = timeClient.getHours() * timeClient.getMinutes();
    int today = timeClient.getDay();

    // Check if it is time to water
    JsonArray wateringTimes = config["watering_times"];

    if (!isWateringTime)
      for (uint8_t i = 0; i < wateringTimes.size(); i++)
      {
        if (currentTime >= wateringTimes[i].as<int>() && currentTime <= (wateringTimes[i].as<int>() + config["watering_duration"].as<int>() * 5 + config["watering_interval"].as<int>() * MOISTURE_SENSOR_COUNT))
        {
          if (lastWateredDay != today || (lastWateredDay == today && lastWateredTime != wateringTimes[i]))
          {
            isWateringTime = true;
            currentWateringTime = wateringTimes[i];
          }
        }
      }

    if (isWateringTime && !wateringStarted)
    {
      wateringStarted = true;
      currentWateringValve = 0;
      openValve(currentWateringValve);
      lastWateringStart = millis();
    }
    else if (isWateringTime && wateringStarted)
    {
      if (millis() - lastWateringStart > config["watering_duration"].as<int>() * 1E3)
      {
        closeValve(currentWateringValve);
        currentWateringValve++;
        if (currentWateringValve > SOLENOID_VALVE_COUNT - 1)
        {
          wateringStarted = false;
          isWateringTime = false;
          if (lastWateredDay != today)
          {
            lastWateredDay = today;
          }
          lastWateredTime = currentWateringTime;
        }
        else
        {
          openValve(currentWateringValve);
          lastWateringStart = millis();
        }
      }
    }
    else if (!isWateringTime && wateringStarted)
    {
      closeValve(currentWateringValve);
      wateringStarted = false;
    }

    if (wateringStarted && millis() - lastWateringStart > config["watering_duration"].as<int>() * 2 * 1E3)
    {
      PT("Ending watering due to unexpected error");
      closeAllValves();
      lastWateredDay = timeClient.getDay();
      lastWateredTime = currentWateringTime;
      isWateringTime = false;
      wateringStarted = false;
    }
  }

  // sent all sensor reading to server once in 5 minutes
  if (millis() - lastupdate > 5 * 60E3)
  {
    lastupdate = millis();
    StaticJsonDocument<200> o_pld;
    JsonArray arr = o_pld["moisture"].to<JsonArray>();
    for (uint8_t i = 0; i < MOISTURE_SENSOR_COUNT; i++)
      arr.add(readMoistureSensor(i));
    JsonArray arr2 = o_pld["valve"].to<JsonArray>();
    for (uint8_t i = 0; i < SOLENOID_VALVE_COUNT; i++)
      arr2.add(valveStatus(i));
    o_pld["client_name"] = config["mqtt_client_name"];
    sendFeedbackToCloud(o_pld, "/sensor-data");
  }
}
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <InfluxDb.h>
#include <Wire.h>

#include <config.h>

#include "ClosedCube_HDC1080.h"
#include "ccs811.h"

ClosedCube_HDC1080 hdc1080;
CCS811 ccs811(CCS811_WAK);

Influxdb influx(INFLUXDB_HOST);
WiFiClient espClient;

void blink(int times, int t_delay)
{
  for (int i = 0; i < times; i++)
  {
    digitalWrite(INDICATOR_LIGHT, LOW);
    delay(t_delay);
    digitalWrite(INDICATOR_LIGHT, HIGH);
  }
}

void setupInfluxDB()
{
  influx.setDbAuth(INFLUXDB_DATABASE, INFLUXDB_USER, INFLUXDB_PASSWORD);
  influx.setVersion(1);
  influx.setPort(INFLUXDB_PORT);
}

void setupWifi()
{
  itoa(ESP.getChipId(), hostname, 16);
  WiFi.hostname(hostname);
  Serial.print("Hostname:");
  Serial.println(hostname);

  Serial.print("Connecting to ");
  Serial.println(CONNECT_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(CONNECT_SSID, CONNECT_PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(INDICATOR_LIGHT, LOW);
    delay(200);
    Serial.print(".");
    digitalWrite(INDICATOR_LIGHT, HIGH);
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println(ESP.getChipId());
}

void setupSerial()
{
  Serial.begin(MONITOR_SPEED);
}

void printSerialNumber()
{
  Serial.print("Device Serial Number=");
  HDC1080_SerialNumber sernum = hdc1080.readSerialNumber();
  char format[12];
  sprintf(format, "%02X-%04X-%04X", sernum.serialFirst, sernum.serialMid, sernum.serialLast);
  Serial.println(format);
}

void setupPins()
{
  pinMode(INDICATOR_LIGHT, OUTPUT);
  hdc1080.begin(0x40);
  printSerialNumber();
  delay(10);
  Wire.begin();
  ccs811.set_i2cdelay(50);
  bool ok = ccs811.begin();
  if (!ok)
    Serial.println("setup: CCS811 begin FAILED");
  // Print CCS811 versions
  Serial.print("setup: hardware    version: ");
  Serial.println(ccs811.hardware_version(), HEX);
  Serial.print("setup: bootloader  version: ");
  Serial.println(ccs811.bootloader_version(), HEX);
  Serial.print("setup: application version: ");
  Serial.println(ccs811.application_version(), HEX);

  // Start measuring
  ok = ccs811.start(CCS811_MODE_1SEC);
  if (!ok)
    Serial.println("setup: CCS811 start FAILED");
}

float computeHeatIndex(float temperature, float percentHumidity)
{

  float hi;

  temperature = temperature + 0;        //include TEMP_OFFSET in HeatIndex computation too
  temperature = 1.8 * temperature + 32; //convertion to *F

  hi = 0.5 * (temperature + 61.0 + ((temperature - 68.0) * 1.2) + (percentHumidity * 0.094));

  if (hi > 79)
  {
    hi = -42.379 +
         2.04901523 * temperature +
         10.14333127 * percentHumidity +
         -0.22475541 * temperature * percentHumidity +
         -0.00683783 * pow(temperature, 2) +
         -0.05481717 * pow(percentHumidity, 2) +
         0.00122874 * pow(temperature, 2) * percentHumidity +
         0.00085282 * temperature * pow(percentHumidity, 2) +
         -0.00000199 * pow(temperature, 2) * pow(percentHumidity, 2);

    if ((percentHumidity < 13) && (temperature >= 80.0) && (temperature <= 112.0))
      hi -= ((13.0 - percentHumidity) * 0.25) * sqrt((17.0 - abs(temperature - 95.0)) * 0.05882);

    else if ((percentHumidity > 85.0) && (temperature >= 80.0) && (temperature <= 87.0))
      hi += ((percentHumidity - 85.0) * 0.1) * ((87.0 - temperature) * 0.2);
  }

  hi = (hi - 32) / 1.8;
  return hi; //return Heat Index, in *C
}

void sendSensorData()
{
  InfluxData row("device_status");
  row.addTag("alias", alias);
  row.addTag("hostname", hostname);
  row.addTag("SSID", WiFi.SSID());
  row.addTag("sensor", SENSOR_TYPE);
  row.addField("rssi", WiFi.RSSI());
  row.addField("uptime", millis());

  float temperature = hdc1080.readTemperature();
  float humidity = hdc1080.readHumidity();

  row.addField("temperature", temperature);
  row.addField("humidity", humidity);
  row.addField("heatIndex", computeHeatIndex(temperature, humidity));

  ccs811.set_envdata210(float(hdc1080.readTemperature()), float(hdc1080.readHumidity()));
  // ccs811.set_envdata(float(hdc1080.readTemperature()), float(hdc1080.readHumidity()));

  uint16_t eco2, etvoc, errstat, raw;
  ccs811.read(&eco2, &etvoc, &errstat, &raw);
  if (errstat == CCS811_ERRSTAT_OK)
  {
    row.addField("eCO2", eco2);
    row.addField("TVOC", etvoc);
  }

  if (influx.write(row))
  {
    blink(3, 250);
  }
  else
  {
    blink(2, 500);
  };
}

void setup()
{
  setupPins();
  setupSerial();
  setupWifi();
  setupInfluxDB();
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");
}

void loop()
{
  sendSensorData();
  delay(READ_DELAY * 1000);
}
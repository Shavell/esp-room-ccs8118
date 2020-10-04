// HARDWARE DEFINITIONS
#define INDICATOR_LIGHT 2
#define SENSOR_TYPE "CJMCU-8118"
#define CCS811_ADDR 0x5A // 0x5B: Default I2C Address 0x5A: Alternate I2C Address

// SYSTEM DEFINITIONS
#define TZ_INFO "TRT-3"
#define READ_DELAY 5
const int MONITOR_SPEED = 19200;

// WIFI DEFINITIONS
const char *CONNECT_SSID = "FiberHGW_ZT4J35_2.4GHz";
const char *CONNECT_PASS = "YfHfrtHsX7";

// InfluxDB DEFINITIONS
#define INFLUXDB_HOST "51.91.252.225"
#define INFLUXDB_PORT 8086
#define INFLUXDB_DATABASE "demo"
#define INFLUXDB_USER "root"
#define INFLUXDB_PASSWORD "supersecret"

// VARIABLES
char *hostname = "esp_sensor";
const char FIRMWARE_VERSION[] = "0.0.1";
const char *alias = "ODAM";
#define DASHBOARD_URL "http://51.91.252.225:3000"

#define CCS811_WAK D3
#include "sht21.h"
#include "credentials.h"
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#define PRODUCTION

// DEFINES

// CREATE A credentials.h HEADER FILE WITH THESE DEFINES:
// #define WLAN_SSID       "YOURID"
// #define WLAN_PASS       "YOURPW"
// #define DEVICE_NAME     "bathroom"
// #define AIO_SERVER      "servername"
// #define AIO_SERVERPORT  1883

#define SLEEP_TIME 300
#define MAX_WIFI_CONNECT_RETRIES 20
#define MAX_MEASURE_RETRIES 10

// MACROS
#define TOPIC(location, type) "sensor/" location "/" type
#define TEMP_TOPIC TOPIC(DEVICE_NAME, "temperature")
#define HUMID_TOPIC TOPIC(DEVICE_NAME, "humidity")
#ifndef PRODUCTION
#undef SLEEP_TIME
#define SLEEP_TIME 30
#define DEVICE_NAME "DEBUG_SENSOR"
#define DEBUG_TOPIC TOPIC(DEVICE_NAME, "debug")
#endif
#define HOST_NAME "esp_sensor_" DEVICE_NAME

// FORWARD DECLARATIONS
void MQTT_connect();

// GLOBALS
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT);
Adafruit_MQTT_Publish temperatureTopic = Adafruit_MQTT_Publish(&mqtt, TEMP_TOPIC);
Adafruit_MQTT_Publish humidityTopic = Adafruit_MQTT_Publish(&mqtt, HUMID_TOPIC);
#ifndef PRODUCTION
Adafruit_MQTT_Publish debugTopic = Adafruit_MQTT_Publish(&mqtt, DEBUG_TOPIC);
#endif

// USER FUNCTIONS
void sleep(long sleeptime)
{
  WiFi.disconnect();  
  Serial.print("Sleeping for ");
  Serial.print(sleeptime);
  Serial.println(" seconds");
  //1,000,000 = 1 second
  ESP.deepSleep(1000000 * sleeptime);
}

float calculateTemperature(uint16_t temp)
{
  return -46.85 + 175.72 * ((float)temp) / 65536.0;
}

float calculateHumidity(uint16_t temp)
{
  return -6.0 + 125.0 * ((float)temp) / 65536.0;
}

void measurementDoneCb(const uint16_t *temp, const uint16_t *humid)
{
  static int deathCount = 0;
  Serial.println("Meas cb called");
  if (*temp && *humid)
  {
    mqttPublish(calculateTemperature(*temp), calculateHumidity(*humid));
    sleep(SLEEP_TIME); // sleep 5 minutes
  }
  if(deathCount++ > MAX_MEASURE_RETRIES)
  {
    deathCount = 0;
    Serial.println("Sensor not found");
    #ifndef PRODUCTION
    MQTT_connect();
    debugTopic.publish(42);
    mqtt.disconnect();
    #endif
    sleep(SLEEP_TIME); // sleep 5 minutes
  }
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect()
{
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected())
  {
    Serial.print("Already connected to MQTT...");
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0)
  { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000); // wait 5 seconds
    retries--;
    if (retries == 0)
    {
      // basically die and wait for WDT to reset me
      while (1)
        ;
    }
  }

  Serial.println("MQTT Connected!");
}

void mqttPublish(float temperature, float humidity)
{
  Serial.print(TEMP_TOPIC);
  Serial.println(temperature);
  Serial.print(HUMID_TOPIC);
  Serial.println(humidity);
  MQTT_connect();

  temperatureTopic.publish(temperature);
  humidityTopic.publish(humidity);
  delay(1000);

  // ping the server to keep the mqtt connection alive
  if (!mqtt.ping())
  {
    mqtt.disconnect();
  }
}

// SETUP AND LOOP
void setup()
{
  Serial.begin(115200);
  // put your setup code here, to run once:
#ifdef PRODUCTION
  Serial.println("Production mode");
#else
  Serial.setDebugOutput(true);
  Serial.println("Development mode");
  WiFi.printDiag(Serial);
#endif

  Serial.println("Set to STA mode");
  WiFi.mode(WIFI_STA);

  Serial.print("Scan start ... ");
  int n = WiFi.scanNetworks();
  Serial.print(n);
  Serial.println(" network(s) found");
  for (int i = 0; i < n; i++)
  {
    Serial.println(WiFi.SSID(i));
  }

  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  WiFi.hostname(HOST_NAME);
  int retry = MAX_WIFI_CONNECT_RETRIES;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
    if(retry-- == 0)
    {
      sleep(30);
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  sht_init();
}

void loop()
{
  // put your main code here, to run repeatedly:
  sht_trigger_measurement(TRG_TEMP_MEAS_NO_HOLD, measurementDoneCb);
  delay(1000);
  sht_trigger_measurement(TRG_HUMI_MEAS_NO_HOLD, measurementDoneCb);
  delay(1000);
}

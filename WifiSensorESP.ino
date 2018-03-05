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

#define SLEEP_TIME      300

// MACROS
#define TOPIC(location, type) "sensor/" location "/" type
#define TEMP_TOPIC TOPIC(DEVICE_NAME, "temperature")
#define HUMID_TOPIC TOPIC(DEVICE_NAME, "humidity")
#define HOST_NAME "esp_sensor_" DEVICE_NAME

// GLOBALS
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT);
Adafruit_MQTT_Publish temperatureTopic = Adafruit_MQTT_Publish(&mqtt, TEMP_TOPIC);
Adafruit_MQTT_Publish humidityTopic = Adafruit_MQTT_Publish(&mqtt, HUMID_TOPIC);

// USER FUNCTIONS

void sleep(long sleeptime) {
  Serial.print("Sleeping for ");
  Serial.print(sleeptime);
  Serial.println(" seconds");
  //1,000,000 = 1 second
  ESP.deepSleep(1000000 * sleeptime);
}

float calculateTemperature(uint16_t temp) {
  return -46.85 + 175.72 * ((float)temp) / 65536.0;
}

float calculateHumidity(uint16_t temp) {
  return -6.0 + 125.0 * ((float)temp) / 65536.0;
}

void measurementDoneCb(const uint16_t *temp, const  uint16_t *humid)
{
  Serial.println("Meas cb called");
  if (*temp && *humid) {
    mqttPublish(calculateTemperature(*temp), calculateHumidity(*humid));
    sleep(SLEEP_TIME); // sleep 5 minutes
  }
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
    retries--;
    if (retries == 0) {
      // basically die and wait for WDT to reset me
      while (1);
    }
  }

  Serial.println("MQTT Connected!");
}

void mqttPublish(float temperature, float humidity) {
  Serial.print(TEMP_TOPIC);
  Serial.println(temperature);
  Serial.print(HUMID_TOPIC);
  Serial.println(humidity);
  MQTT_connect();

  temperatureTopic.publish(temperature);
  humidityTopic.publish(humidity);
  delay(1000);

  // ping the server to keep the mqtt connection alive
  if (! mqtt.ping()) {
    mqtt.disconnect();
  }
}

// SETUP AND LOOP
void setup() {
  Serial.begin(115200);
  // put your setup code here, to run once:
#ifdef PRODUCTION
  Serial.println("Production mode");
#else
  Serial.println("Development mode");
#endif
  Serial.println("Set to STA mode");
  ///WiFi.mode(WIFI_STA);
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  WiFi.hostname(HOST_NAME);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  sht_init();

}

void loop() {
  // put your main code here, to run repeatedly:
  sht_trigger_measurement(TRG_TEMP_MEAS_NO_HOLD, measurementDoneCb);
  delay(1000);
  sht_trigger_measurement(TRG_HUMI_MEAS_NO_HOLD, measurementDoneCb);
  delay(1000);
}

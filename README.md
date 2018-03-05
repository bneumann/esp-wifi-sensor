# Temperature/Humidity Sensor SHT21 with ESP8266
There are a lot of IOT sensor projects out there. Why would I do my own one? First: Because I can and the fun it brings. Or frustration. Or a bit of both. Second, I was looking for a decent driver that actually works with the SHT21. None of the stock Adafruit libraries worked for me. And when I checked my logic analyzer all the drives had the same bug.
SO: I also wrote my own driver. Later on I ported it to the ESP-01 which only has 2 Pins available so I bit banged my own i2c driver too.
Maybe some people can learn from the code (sorry for the sparse comments), that's why I put it online. Feel free to ask questions!

I also provide my layout file which I use for the sensors in my house.

## Wifi credentials
Because I don't want you guys to spoof around my wifi, I didn't check in the credentials. Clever ey? So you will need to create your own `credentials.h` file in the base directory:
```c
#define WLAN_SSID       "YOURID"
#define WLAN_PASS       "YOURPW"
#define DEVICE_NAME     "bathroom"
#define AIO_SERVER      "servername"
#define AIO_SERVERPORT  1883
```

## I2C driver
As I said, that one is simple bit banging the basic start, stop, read and write methods. Check the header file `i2c.h` for available methods. Might be a cool starting point if you want your own sensor or peripheral and only have 2 pins.

## SHT21 driver
On top of the i2c driver sits the SHT21 driver using the no hold measurement. Which basically means, that I am polling the lil fella until he's done measuring.
All my SHT21s I have didn't work with the Adafruit driver which uses the hold measurement. They pull down the clock line until the SHT21 is done measuring and continue from there. 
The driver contains of a few basic steps:
```c
// Create a callback method
void measurementDoneCb(const uint16_t *temp, const  uint16_t *humid)
{
  if (*temp && *humid) {
    // do what you want with the data
  }
}
/* .... other code .... */
// initialise SHT
sht_init(); // takes about 15ms
// Trigger a measurement
sht_trigger_measurement(TRG_TEMP_MEAS_NO_HOLD, measurementDoneCb);
// Trigger another one. You could do that in the callback if you want too
sht_trigger_measurement(TRG_HUMI_MEAS_NO_HOLD, measurementDoneCb);
```

### Sensor data conversion
The data from the sensor is some digital data. You need to calculate it using these formulas. I conveniently put them into C code already:
```c
float calculateTemperature(uint16_t temp) {
  return -46.85 + 175.72 * ((float)temp) / 65536.0;
}

float calculateHumidity(uint16_t temp) {
  return -6.0 + 125.0 * ((float)temp) / 65536.0;
}
```

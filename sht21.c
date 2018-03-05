/*
   i2c.c

    Created on: 15.01.2017
        Author: benni
*/

#include "sht21.h"

#include <Arduino.h>
#include "i2c.h"

#define READ_FLAG 1
#define MEAS_TIME 150 ///< Measurement time in ms
#define MEAS_AND_CHECK 2 ///< Read 2 databytes and the checksum
// #define ICACHE_FLASH_ATTR

typedef struct {
  uint16_t temperature;
  uint16_t humidity;
  uint16_t checksum;
  uint8_t retries;
  bool running;
  tSht21MeasureTypes type;
} tSht21Context;

static MeasurementDoneHandler userCallback;
static tSht21Context tContext = {0, 0, 0, 0, false, TRG_TEMP_MEAS_NO_HOLD};

static bool sht_measurement_done();

typedef enum {
  ADDRESS 				= 0x80,
  TRG_TEMP_MEAS_HOLD 		= 0xE3,
  TRG_HUMI_MEAS_HOLD 		= 0xE5,
  //	TRG_TEMP_MEAS_NO_HOLD 	= 0xF3,
  //	TRG_HUMI_MEAS_NO_HOLD 	= 0xF5,
  WRITE_USER_REG			= 0xE6,
  READ_USER_REG			= 0xE7,
  TRG_SOFT_RESET			= 0xFE
} tSht21Defines;



static void  measureTimerCb()
{
  bool ack = sht_measurement_done();
  // If the measurement is already done we call the users callback otherwise we wait
  if (!ack)
  {
    if (tContext.retries++ > 3)
    {
      tContext.running = false;
      sht_soft_reset();
      tContext.retries = 0;
      if (userCallback != NULL)
      {
        userCallback(&tContext.temperature, &tContext.humidity);
      }
      return;
    }
    delay(150);
    measureTimerCb();
  }
  else
  {
    tContext.running = false;
    sht_soft_reset();
    tContext.retries = 0;
    //		printf("meas  done, calling user\n");
    if (userCallback != NULL)
    {
      userCallback(&tContext.temperature, &tContext.humidity);
    }
  }
}

// Interface methods
void sht_init()
{
  i2c_init();
  delay(15);
}

bool sht_trigger_measurement(tSht21MeasureTypes type, MeasurementDoneHandler cb)
{
  bool ret = false;
  if (!tContext.running)
  {
    tContext.running = true;
    tContext.type = type;
    userCallback = cb;
    bool ret;
    i2c_start();
    ret = i2c_write(ADDRESS);
    if (i2c_write(type)) {
      delayMicroseconds(20);
    }
    i2c_stop();
    delay(1);
    measureTimerCb();
    ret = true;
  }
  return ret;
}

static bool sht_measurement_done()
{
  bool ack = false;
  if (tContext.running)
  {
    uint16 *data = tContext.type == TRG_TEMP_MEAS_NO_HOLD ? &tContext.temperature : &tContext.humidity;
    i2c_start();
    ack = i2c_write(ADDRESS | READ_FLAG);
    if (ack)
    {
      uint8_t meas;
      *data = 0;
      for (meas = 0; meas <= MEAS_AND_CHECK; meas++)
      {
        uint8_t tmp = i2c_read(meas < MEAS_AND_CHECK);
        if (meas < MEAS_AND_CHECK) {
          *data |= tmp << (MEAS_AND_CHECK - meas - 1) * 8;
          //				printf("we received: %d\n", tmp);
        }
      }
    }
    i2c_stop();
    // Clear last two bits
    // TODO: Use last bits to retrieve which measurement has been started
    *data &= ~0x02;
  }
  return ack;
}

bool sht_soft_reset()
{
  // FIXME: Add reset time of 15 ms where no
  // communication can be established after reset.
  // Sending data in the reset time can render the
  // sensor inoperable until the next restart.
  return true;
  if (!tContext.running)
  {
    i2c_start();
    i2c_write(ADDRESS);
    i2c_write(TRG_SOFT_RESET);
    i2c_stop();
    return true;
  }
  return false;
}

tShtSettings  sht_get_user()
{
  i2c_start();
  i2c_write(ADDRESS);
  i2c_write(READ_USER_REG);
  i2c_start();
  i2c_write(ADDRESS | READ_FLAG);
  byte readVal = i2c_read(true);
  i2c_stop();
  tShtSettings ret = {0, 0, 0, 0};
  ret.BatteryState = (readVal >> 6) & 0x01;
  ret.DisableOtpReaload = (readVal >> 1) & 0x01;
  ret.EnableHeater = (readVal >> 2) & 0x01;
  ret.MeasResolution = ((readVal >> 6) & 0x02) | (readVal & 0x01);
  return ret;
}


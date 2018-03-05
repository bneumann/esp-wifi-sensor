/*
  i2c.c

  Created on: 15.01.2017
   Author: benni
*/

#include "i2c.h"
#include "Arduino.h"

#define PRODUCTION

#ifdef PRODUCTION
#define PIN_SDA 2
#define PIN_SCL 0
#else
#define PIN_SDA 0
#define PIN_SCL 2
#endif

#define BLOCKING 1

static bool _initialized = false;

static void i2c_setscl();
static bool i2c_getscl();
static void i2c_clearscl();
static void i2c_setsda();
static bool i2c_getsda();
static void i2c_clearsda();
static void i2c_throw_error();


// Interface functions
void i2c_init()
{
  if (!_initialized)
  {
    pinMode(PIN_SDA, OUTPUT_OPEN_DRAIN);
    pinMode(PIN_SCL, OUTPUT);
    _initialized = true;
  }
  i2c_setscl();
}


bool i2c_write(byte data)
{
  if (!_initialized)
  {
    i2c_throw_error();
    return 0;
  }
  byte outBits;
  bool ack = false;

  for (outBits = 0; outBits < 8; outBits++)
  {
    if (data & 0x80)
    {
      i2c_setsda();
    }
    else
    {
      i2c_clearsda();
    }
    data <<= 1;
    // Clock cycle
    i2c_setscl();
    i2c_clearscl();
  }
  i2c_setscl();
  //delayMicroseconds(1);
  ack = i2c_getsda();
  i2c_clearscl();
  return !ack;
}

byte i2c_read(bool ack)
{
  if (!_initialized)
  {
    i2c_throw_error();
    return 0;
  }
  byte inData, inBits;

  inData = 0x00;

  for (inBits = 0; inBits < 8; inBits++)
  {
    inData <<= 1;
    i2c_setscl();
    inData |= i2c_getsda();
    i2c_clearscl();
  }
  if (ack)
  {
    i2c_ack();
  }
  else
  {
    i2c_setscl();
    i2c_clearscl();
  }

  return inData;
}

void i2c_start()
{
  if (!_initialized)
  {
    i2c_throw_error();
    return;
  }
  // I2C Start condition, data line goes low when clock is high
  //       ___
  // SDA:     \______ ...
  //       _____
  // SCL:       \____ ...
  i2c_setsda();
  i2c_setscl();
  i2c_clearsda();
  i2c_clearscl();
}

void i2c_stop()
{
  if (!_initialized)
  {
    i2c_throw_error();
    return;
  }
  // I2C Stop condition, data line goes high after clock is high
  //                ____
  // SDA: ... _____/
  //              ______
  // SCL: ... ___/
  i2c_clearscl();
  i2c_clearsda();
  i2c_setscl();
  i2c_setsda();
}

void i2c_ack()
{
  i2c_clearsda();
  i2c_setscl();
  i2c_clearscl();
  i2c_setsda();
}


// Low level GPIO access
static void i2c_setscl()
{
  digitalWrite(PIN_SCL, HIGH);
  delayMicroseconds(1);
}

static bool i2c_getscl()
{
  return digitalRead(PIN_SCL) == 1;
}

static void i2c_clearscl()
{
  digitalWrite(PIN_SCL, LOW);
  delayMicroseconds(1);
}

static void i2c_setsda()
{
  digitalWrite(PIN_SDA, 1);
}

static bool i2c_getsda()
{
  return digitalRead(PIN_SDA) == 1;
}

static void i2c_clearsda()
{
  digitalWrite(PIN_SDA, 0);
  delayMicroseconds(1);
}

static void i2c_throw_error()
{
  printf("I2C not initialized!");
}

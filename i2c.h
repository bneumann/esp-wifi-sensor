/*
 * i2c.h
 *
 *  Created on: 15.01.2017
 *      Author: benni
 */

#ifndef DRIVER_I2C_I2C_H_
#define DRIVER_I2C_I2C_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "c_types.h"

typedef uint8_t byte;

void i2c_init();

bool i2c_write(byte data);

byte i2c_read(bool ack);

void i2c_start();

void i2c_stop();

void i2c_ack();

#ifdef __cplusplus
}
#endif

#endif /* DRIVER_I2C_I2C_H_ */

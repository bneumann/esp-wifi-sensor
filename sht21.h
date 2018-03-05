/*
 * sht21.h
 *
 *  Created on: 15.01.2017
 *      Author: benni
 */

#ifndef DRIVER_SHT21_SHT21_H_
#define DRIVER_SHT21_SHT21_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "c_types.h"

typedef void (*MeasurementDoneHandler)(const uint16_t *temperature, const uint16_t *humidity);

typedef enum {
	TRG_TEMP_MEAS_NO_HOLD 	= 0xF3,
	TRG_HUMI_MEAS_NO_HOLD 	= 0xF5,
}tSht21MeasureTypes;

typedef enum {
	Good, 	///< Voltage > 2.25 V
	Bad 	///< Voltage < 2.25 V
} tSht21BatteryState;

typedef enum {		// Humidity     Temperature
	HighRes = 0x00, // 12 bit 		14 bit
	GoodRes = 0x02, // 10 bit   	13 bit
	MidRes 	= 0x01,	// 	8 bit		12 bit
	LowRes 	= 0x03,	// 11 bit		11 bit
}tSht21Resolution;

typedef struct {
	tSht21Resolution MeasResolution : 2;
	tSht21BatteryState BatteryState : 1;
	bool EnableHeater : 1;
	bool DisableOtpReaload : 1;
}tShtSettings;

void sht_init();
bool sht_soft_reset();
bool sht_trigger_measurement(tSht21MeasureTypes type, MeasurementDoneHandler cb);
tShtSettings sht_get_user();

#ifdef __cplusplus
}
#endif

#endif /* DRIVER_SHT21_SHT21_H_ */

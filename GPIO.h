/*
 * GPIO.h
 *
 *  Created on: 18-Jan-2014
 *      Author: OWNER
 */

#ifndef _GPIO_H_
#define _GPIO_H_

#define IN 0
#define OUT 1

#define RFID_IN 		(23)
#define VISIT_SWITCH  		(24)
#define DBG_LED 		(25)
#define LEARNING_MODE_SW  	(22)
#define PWR_OFF_SWITCH	(18)
//#define RETURN_SWITCH	(27)
#define SERVO_REQ_PIN		(2)
#define SERVO_CMP_PIN		(27)

#define LOW 0
#define HIGH 1

int GPIOExport(int pin);
int GPIOUnexport(int pin);
int GPIODirection(int pin, int dir);
int GPIORead(int pin);
int GPIOWrite(int pin, int value);

#endif /* GPIO_H_ */

/*! \file ds1307.c \brief Dallas DS1307 RTC Driver Library. */
//*****************************************************************************
//
// File Name	: 'ds1307.c'
// Title		: Dallas DS1307 RTC Driver Library
// Author		:
// Created		:
// Revised		:
// Version		: 0.1
// Target MCU	: Atmel AVR Series
//
//*****************************************************************************

#include <avr/io.h>
#include <avr/interrupt.h>

#include "global.h"
#include "timer128.h"
#include "i2c.h"
#include "ds1307.h"
//#include "rprintf.h"

char ds1307_settime(u08 i2cAddr, time_t time) {
	u08 retval= I2C_OK;
	uint8_t msgbuf[8];

	msgbuf[0] = 0x00; // start address
	msgbuf[1] = DEC2BCD(time.sec);
	msgbuf[2] = DEC2BCD(time.min);
	msgbuf[3] = DEC2BCD(time.hr);
	msgbuf[4] = DEC2BCD(time.day);
	msgbuf[5] = DEC2BCD(time.dat);
	msgbuf[6] = DEC2BCD(time.mon);
	msgbuf[7] = DEC2BCD(time.yr);
	/*
	printf("SET sec:%d secBCD:%d\n",time.sec,DEC2BCD(time.sec));
	printf("SET min:%d minBCD:%d\n",time.min,DEC2BCD(time.min));
	printf("SET hr :%d hrBCD :%d\n",time.hr,DEC2BCD(time.hr));
	printf("SET day:%d dayBCD:%d\n",time.day,DEC2BCD(time.day));
	printf("SET dat:%d datBCD:%d\n",time.dat,DEC2BCD(time.dat));
	printf("SET mon:%d monBCD:%d\n",time.mon,DEC2BCD(time.mon));
	printf("SET yr :%d yrBCD :%d\n",time.yr,DEC2BCD(time.yr));
	*/
	retval=i2cMasterSendNI(i2cAddr, 8, msgbuf);
	if (retval == I2C_OK) {
		return true;
	} else {//I2C_ERROR_NODEV
		return false;
	}
}

char ds1307_gettime(u08 i2cAddr, time_t* time) {

	uint8_t msgbuf[7];
	u08 retval= I2C_OK;

	// send the "set position"
	msgbuf[0]=0x00;
	retval=i2cMasterSendNI(i2cAddr, 1, msgbuf);
	// read the results
	retval=i2cMasterReceiveNI(i2cAddr, 7, msgbuf);
	/*
	printf("\n\n\rDS1307 GET BCDsec :%d  sec  :%d\n",msgbuf[0],BCD2DEC(msgbuf[0]));
	printf("DS1307 GET BCDmin :%d  min  :%d\n",msgbuf[1],BCD2DEC(msgbuf[1]));
	printf("DS1307 GET BCDhr  :%d  hr   :%d\n",msgbuf[2],BCD2DEC(msgbuf[2]));
	printf("DS1307 GET BCDday :%d  day  :%d\n",msgbuf[3],BCD2DEC(msgbuf[3]));
	printf("DS1307 GET BCDdate:%d  date :%d\n",msgbuf[4],BCD2DEC(msgbuf[4]));
	printf("DS1307 GET BCDmon :%d  mon  :%d\n",msgbuf[5],BCD2DEC(msgbuf[5]));
	printf("DS1307 GET BCDyr  :%d  yr   :%d\n",msgbuf[6],BCD2DEC(msgbuf[6]));
	*/
	time->sec 	= BCD2DEC (msgbuf[0]);
	time->min 	= BCD2DEC (msgbuf[1]);
	time->hr 	= BCD2DEC (msgbuf[2]);
	time->day 	= BCD2DEC (msgbuf[3]);
	time->dat 	= BCD2DEC (msgbuf[4]);
	time->mon 	= BCD2DEC (msgbuf[5]);
	time->yr 	= BCD2DEC (msgbuf[6]);

	if (retval == I2C_OK) {
		return true;
	} else {//I2C_ERROR_NODEV
		return false;
	}
}

char ds1307_enable(u08 i2cAddr) {

	u08 retval= I2C_OK;
	uint8_t msgbuf[2];
	msgbuf[0]=0x00;
	// set the position register
	retval=i2cMasterSendNI(i2cAddr, 1, msgbuf);
	// read the secs field, contains the CH bit
	retval=i2cMasterReceiveNI(i2cAddr, 1, msgbuf);

	// Write new value to 0x00 only if needed
	if ( !(msgbuf[0] & _BV(7)) ){
		msgbuf[1]=msgbuf[0];
		//printf("DS1307 already enabled:%d\n",msgbuf[1]);
	}
	else{
		msgbuf[1] = msgbuf[0] &~_BV(7);
		//printf("DS1307 needs to be enabled:%d\n",msgbuf[1]);
	}
	msgbuf[0]=0x00;
	retval=i2cMasterSendNI(i2cAddr, 2, msgbuf);
	if (retval == I2C_OK) {
		return true;
	} else {//I2C_ERROR_NODEV
		return false;
	}
}

char ds1307_disable(u08 i2cAddr) {

	u08 retval= I2C_OK;
	uint8_t msgbuf[2];

	// Get the existing conf
	msgbuf[0]=0x00;
	retval=i2cMasterSendNI(i2cAddr, 1, msgbuf);
	// read the secs field, contains the CH bit
	retval=i2cMasterReceiveNI(i2cAddr, 1, msgbuf);

	// Write new value to 0x00
	msgbuf[1] = msgbuf[0] | _BV(7);
	msgbuf[0]=0x00;
	retval=i2cMasterSendNI(i2cAddr, 2, msgbuf);

	if (retval == I2C_OK) {
		return true;
	} else {//I2C_ERROR_NODEV
		return false;
	}}

char ds1307_24hmode(u08 i2cAddr){
	u08 retval= I2C_OK;
	uint8_t msgbuf[2];

	// set the position register
	msgbuf[0]=0x02;
	retval=i2cMasterSendNI(i2cAddr, 1, msgbuf);
	// read the hours field, contains the 12/24 bit (6th bit in register)
	retval=i2cMasterReceiveNI(i2cAddr, 1, msgbuf);
	// Write new value to 0x02 : clear bit 6
	msgbuf[1] = msgbuf[0] &~( ( 1 ) << 6);
	msgbuf[0]=0x02;
	//rprintf("Sending 24h mode byte %d\n",msgbuf[1]);
	retval=i2cMasterSendNI(i2cAddr, 2, msgbuf);
	if (retval == I2C_OK) {
		return true;
	}
	else {//I2C_ERROR_NODEV
		return false;
	}
}

char ds1307_12hmode(u08 i2cAddr){
	u08 retval= I2C_OK;
	uint8_t msgbuf[2];

	// set the position register
	msgbuf[0]=0x02;
	retval=i2cMasterSendNI(i2cAddr, 1, msgbuf);
	// read the hours field, contains the 12/24 bit (6th bit in register)
	retval=i2cMasterReceiveNI(i2cAddr, 1, msgbuf);
	// Write new value to 0x02: set bit 6
	msgbuf[1] = msgbuf[0] | ( 1 ) << 6;
	msgbuf[0]=0x02;
	//rprintf("Sending 12h mode byte %d\n",msgbuf[1]);
	retval=i2cMasterSendNI(i2cAddr, 2, msgbuf);
	if (retval == I2C_OK) {
		return true;
	}
	else {//I2C_ERROR_NODEV
		return false;
	}
}


// 1 means 12 hours mode
// 0 means 24 hours mode
char ds1307_get1224mode(u08 i2cAddr){

	u08 retval= I2C_OK;
	uint8_t msgbuf[2];

	// set the position register
	msgbuf[0]=0x02;
	retval=i2cMasterSendNI(i2cAddr, 1, msgbuf);
	// read the hours field, contains the 12/24 bit (6th bit in register)
	retval=i2cMasterReceiveNI(i2cAddr, 1, msgbuf);
	return msgbuf[0]&_BV(6);
}

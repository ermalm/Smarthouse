/**
 * \file
 * \author  Jon Ove Storhaug <jostorha@gmail.com>
 * \author  Asbjørn Tveito <atveito@gmail.com>
 * \version 1.0
 * \brief Slave Module 
 *
 * \section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * \section DESCRIPTION
 *
 * This file is the main program for the Slave Module. It gets data from 
 * the Main Module, turn a relay on or off, read a DS1621 (DS1631) digital 
 * temperature and sends data back to the Main Module.
 */
#include <stdlib.h>
#include <avr/io.h>
#include "global.h"
#include "uart.h"
#include <avr/interrupt.h>	// include interrupt support
#include "i2c.h"			// include i2c support
#include "ds1631.h"			// include DS1631 support
#include <util/delay.h>
#include <string.h>
#include <stdio.h>
#include <avr/wdt.h>

#define BAUD 9600

//! Disables the watchdog timer
///
void get_mcusr(void) \
      __attribute__((naked)) \
      __attribute__((section(".init3")));

//! Compare name in received data with the name of this slave module,
/// read status in the received data, read temperature from sencor. 
/// Send data to Main Module
///
void compare(void);

//! Send data with name, status and temperature to Main Module
///
void sendPacket(void);

//! Read and save data from the Main Module to a buffer
///
void readtobuffer(void);

//! Get name from XBee modem
///
void XBeeGetName(void);

//! Init the temperature sencor, DS1621 or DS1631
///
void ds1631init(void);

//! Read the temperature sencor
///
void ds1631getTemp(void);

uint8_t mcusr_mirror;
int i=0;
signed char temp;
int counter=0;
char buffer1[11];
char name[10];//="~ROUTER0";
FILE uart_str = FDEV_SETUP_STREAM(uartSendByte, uartGetByte, _FDEV_SETUP_RW);

int main(void)
{
	get_mcusr();
	CLKPR=0x80;
	CLKPR=0x00;
	DDRB=0xff; //PORTB is output
    uartInit();
	uartSetBaudRate(BAUD);
	stdout = stdin = &uart_str;
	_delay_ms(2000);
	ds1631init();
	XBeeGetName();
		
	wdt_enable(WDTO_8S);
	wdt_reset();
	while(1)
	{		
		wdt_reset();
		readtobuffer();
	}
	return 0;	
}

void readtobuffer(void)
{
	while(uartReceiveBufferIsEmpty())
		;;
	if (uartReceiveByte(&buffer1[counter]))
	{
		if(buffer1[0]=='~')
		{
			counter++;			
			if (counter>=11)
			{
				counter=0;
				compare();
			}
		}
	}
}
	
void compare(void)
{
	if (strncmp(buffer1,name,8)==0)
	{
		if((buffer1[9]=='o')&&(buffer1[10]=='n'))
		{
			PORTB |=1<<1;
			ds1631getTemp();
			sendPacket();
		}
		else if((buffer1[9]=='o')&&(buffer1[10]=='f'))
		{
			PORTB &=~(1<<1);
			ds1631getTemp();
			sendPacket();
		}
		else if((buffer1[9]=='n')&&(buffer1[10]=='c'))
		{
			ds1631getTemp();
			sendPacket();
		}            
	}
}

void sendPacket(void)
{
    char status[3];
	printf(name);
    if(PINB&(1<<PINB1)) 
    sprintf(status,"on");
    else
		sprintf(status,"of");
 
	printf("_");
	printf(status);
	printf("_");
	uartSendByte(temp);
}

void XBeeGetName(void)
{
	unsigned char status[4];
	uartFlushReceiveBuffer();
	printf("+++"); // Enter command mode
	int i=0;
		_delay_ms(1100);
		uartFlushReceiveBuffer();
		printf("atni\r"); // Ask for XBee's name
	i=1;
	// Receive the name
	while(i!=20)
	{
		name[0]='~';
		if (uartReceiveByte(&name[i]))
		{
			if (name[i]=='\r')
			{
				name[i]='\0';
				i=20;
			}
			else
				i++;
		}
	}
	printf("atcn\r"); // Exit command mode
	uartFlushReceiveBuffer();
}

void ds1631init(void)
{
	// initialize i2c function library
	i2cInit();
	i2cSetBitrate(100);
	// initialize
	ds1631Init(DS1631_I2C_ADDR);
	// set config
	ds1631SetConfig(DS1631_I2C_ADDR, 0x0F);
	// set the temperature limit registers
	ds1631SetTH(DS1631_I2C_ADDR, 35<<8);
	ds1631SetTL(DS1631_I2C_ADDR, 30<<8);	
}

void ds1631getTemp(void)
{
	s16 T=0;	
	// start convert
	ds1631StartConvert(DS1631_I2C_ADDR);
	// wait until done
	while(!(ds1631GetConfig(DS1631_I2C_ADDR) & DS1631_CONFIG_DONE));
	// Read temp
	T = ds1631ReadTemp(DS1631_I2C_ADDR);
	temp=T>>8; // The temperature is saved to temp, no decimals, just the integer
}

void get_mcusr(void)
    {
      mcusr_mirror = MCUSR;
      MCUSR = 0;
      wdt_disable();
    }
/**
 * \file
 * \author  Jon Ove Storhaug <jostorha@gmail.com>
 * \version 1.0
 * \brief Main Module 
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
 * This file is used for sending and receiving packets to and from the Webserver
 * and the main module
 */
#include <stdlib.h>
#include <avr/io.h>
#include "global.h"
#include "uart2.h"
#include <avr/interrupt.h>	// include interrupt support
#include "i2c.h"			// include i2c support
#include "ds1307.h"			// include DS1307 support
#include <util/delay.h>
#include <string.h>
#include <stdio.h>
#include <avr/wdt.h> 
#include "webcom.h"

#define BAUD 9600
#define XBeeUART 0
#define EthUART 1

FILE uart0_str = FDEV_SETUP_STREAM(uart0SendByte, NULL, _FDEV_SETUP_WRITE);
FILE uart1_str = FDEV_SETUP_STREAM(uart1SendByte, uart1GetByte, _FDEV_SETUP_RW);
//! Sends data to Slave Module
/// \param number Slave module to send data to.
/// \param status The status you want slave module to have: 0=off, 1=on, 2=no change
///
void sendSlaveModule(unsigned char number,unsigned char status);

//! Disables the watchdog timer
///
void get_mcusr(void) \
	__attribute__((naked)) \
		__attribute__((section(".init3")));

uint8_t mcusr_mirror;
time_t time={3,37,1,3,2,4,9};
slaveModule sm[noOfModules]={{0,1,-5},{0,0,15},{0,1,20}};
slaveModule sm_web[noOfModules];
signed char ethPacket[noOfBytes];
unsigned char XBeePacketCounter[noOfModules];

int main(void)
{	
	unsigned char tempSec;
	get_mcusr(); // Disable the watchdog timer
	uartInit(); // Init both UARTs
	uartSetBaudRate(0,BAUD);
	uartSetBaudRate(1,BAUD);
	stdout = stdin = &uart1_str; // Init the stdout and stdin (printf, scanf...) functions
	stderr = &uart0_str; // Init the stderror functions
	i2cInit();
	i2cSetBitrate(100);	
	ds1307_enable(DS1307_I2C_ADDR); // Enable the DS1307 RTC
	// Make a flash on LEDs connected to PORTB when starting / restarting
	DDRB=0xff; // PORTB output
	PORTB=0xff; // All bits on port B = 1 
	_delay_ms(1000); // Wait until XBee modem has started
	PORTB=0x00; // All bits on port B = 0
	// Empty the receive buffer of both UARTs
	uartFlushReceiveBuffer(EthUART);
	uartFlushReceiveBuffer(XBeeUART);
	wdt_enable(WDTO_2S); // Enable watchdog, reset after 2 seconds
	wdt_reset();
	while(1)
	{	
		wdt_reset(); // Reset the watchdog timer		
		tempSec=time.sec;
		if(ds1307_gettime(DS1307_I2C_ADDR, &time))
		{	//if(tempSec!=time.sec)	
			if((time.sec%2==0)&&(tempSec!=time.sec)) // True every 2nd second
			{
				while(!uartReceiveBufferIsEmpty(XBeeUART))
				{
					int i;
					i=0;
					signed char XBeePacket[12];
					// Read data from slave module 
					while (i<13)
					{
						while (uartReceiveBufferIsEmpty(XBeeUART))
							;;
						// Syncronization
						uartReceiveByte(XBeeUART, &XBeePacket[i]);
						if (XBeePacket[i]=='~') // Counter is set to 0 if the current character is '~'
							i=0;
						if (XBeePacket[0]=='~') //Increment if the first character in received data is '~'
							i++;
					}					
					sm[XBeePacket[7]-'0'].temp=XBeePacket[12]-7; // Compensate for heat from components in slave module
					// Check status of slave module and make status ready to send to webserver
					if (XBeePacket[10]=='f')
						sm[XBeePacket[7]-'0'].status=0;
					else if (XBeePacket[10]=='n')
						sm[XBeePacket[7]-'0'].status=1;
					XBeePacketCounter[XBeePacket[7]-'0']=1; // Set packet counter for this slave module to 1
				}
				// Check packet counter of all modules, if false: status=2 is sent to the webserver
				// Webserver will then now which modules that are unconnected
				for (int i=0; i<noOfModules; i++)
				{		
					if (!XBeePacketCounter[i])
						sm[i].status=2;
					XBeePacketCounter[i]=0;
				}
				// Receive data from webserver
				if (checkForEthPacket(ethPacket))
				{
					if (ethPacket[pHour]<24)// edit time 
					{
						time.yr=ethPacket[pYear];
						time.mon=ethPacket[pMonth];
						time.dat=ethPacket[pDate];
						time.hr=ethPacket[pHour];
						time.min=ethPacket[pMin];
						ds1307_settime(DS1307_I2C_ADDR,time);
					}
					int number=0;
					while (number<noOfModules)
					{
						sm_web[number].type=ethPacket[pType+number*pFieldsModules];
						sm_web[number].status=ethPacket[pStatus+number*pFieldsModules];
						sm_web[number].temp=ethPacket[pTemp+number*pFieldsModules];
						switch (sm_web[number].type)
						{
							case 0:
								if (sm_web[number].status==0) // Check if the current slave module is disabled on the website
									sendSlaveModule(number,0); // Send data to slave module: turn off relay
								else
								{
									if ((sm[number].temp)<(sm_web[number].temp-1)) // Measured temperature < wanted temperature - 1 ?
										sendSlaveModule(number,1); // Send data to slave module: turn on relay
									else if ((sm[number].temp)>(sm_web[number].temp+1)) // Measured temperature > wanted temperature + 1 ?
										sendSlaveModule(number,0); // Send data to slave module: turn off relay
									else 
										sendSlaveModule(number,2); // Send data to slave module
								}
								break;
						}
						number++; // next slave module
					}
				}				
				sendEthPacket(time, sm); // send packet with data to webserver
			}
		}
	}				
	return 0;	
}

// Sends data to the slave modules
void sendSlaveModule(unsigned char number,unsigned char status)
{
	switch (status)
	{
		case 0:
			fprintf(stderr,"~ROUTER%d_of",number);
			break;
		case 1:
			fprintf(stderr,"~ROUTER%d_on",number);
			break;
		case 2:
			fprintf(stderr,"~ROUTER%d_nc",number);
			break;
	}
}

// Disables the watchdog timer
void get_mcusr(void)
    {
      mcusr_mirror = MCUSR;
      MCUSR = 0;
      wdt_disable();
    }
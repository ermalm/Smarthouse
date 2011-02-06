/**
 * \file webcom.h
 * \author  Jon Ove Storhaug <jostorha@gmail.com>
 * \version 1.0
 * \brief Serial communication between Webserver and Main Module for the Main Module 
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
 * and the main module. This file is for the Main Module
 */

#ifndef WEBCOM_H_
#define WEBCOM_H_

// Include UART functions
#include "uart2.h" 
// Include DS1307 RTC functions
#include "ds1307.h" 
// Include Delay functions
#include <util/delay.h> 

#define XBeeUART 0 ///< UART used for XBee/Slave Module communication
#define EthUART 1 ///< UART used for webserver communication

#define pSlave_modules 0 ///< Number of slave modules
#define pYear 1
#define pMonth 2
#define pDate 3
#define pHour 4 
#define pMin 5
#define pSec 6
#define pNumber 7 ///< The current slave modules number
#define pType 8 ///< Type of slave module, 0=Heat
#define pStatus 9 ///< Status of slave module, 0=off, 1=on, 2=no change if sent to slave module or not connected when sent to webserver
#define pTemp 10 ///< The temperature

#define pModuleStart 7 ///< First byte that is different for each module
#define pLastField 10 ///< The last field/byte in the packet
#define pFieldsModules 4 ///< Number of fields/bytes in one module = pLastField-pModuleStart+1
#define noOfModules 2
#define noOfBytes (pModuleStart+((1+pLastField-pModuleStart)*noOfModules))

/// Slave_module structure
///
typedef struct  { 
	unsigned char type; ///< Type of slave module. 0=heat control
	unsigned char status; ///< Status of slave module. 0=off, 1=on, 2=no change
	signed char temp; ///< The temperature in degrees Celcius
} slaveModule;

//! Checks if a packet is ready to be received from the webserver
//! \param ethPacket The received packet
//! \return True if a packet is received, false if not.
///
int checkForEthPacket(signed char* ethPacket);

//! Send a packet to the webserver
//! \param time The actual time the packet is sent (sec, min, hr, dat, mon, yr)
//! \param sm The slave modules (type, status, temp)
///
void sendEthPacket(time_t time, slaveModule* sm);




#endif /*webcom_H_*/

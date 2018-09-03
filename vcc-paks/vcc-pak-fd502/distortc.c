/*
 Copyright 2015 by Joseph Forgione
 This file is part of VCC (Virtual Color Computer).
 
 VCC (Virtual Color Computer) is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with VCC (Virtual Color Computer).  If not, see <http://www.gnu.org/licenses/>.
*/

/*****************************************************************************/
/*
	Disto Real Time Clock
 
	currently mapped to $FF50-$FF51
 */
/*****************************************************************************/

#include "distortc.h"

#include <time.h>
#include <assert.h>

/*****************************************************************************/

/* Table description:							   Bit3  Bit2  Bit1  Bit0
Write to $FF51 read from $FF50
	0x00		1-second digit register				 S8    S4    S2    S1
	0x01		10-second digit register			  x   S40   S20   S10
	0x02		1-minute digit register				Mi8   Mi4   Mi2   Mi1
	0x03		10-minute digit register			  x  Mi40  Mi20  Mi10
	0x04		1-hour digit register				 H8    H4    H2    H1
	0x05		PM/AM, 10-hour digit register		  x   P/A   H20   H10
	0x06		1-day digit register				 D8    D4    D2    D1
	0x07		10-day digit register				  x     x   D20   D10
	0x08		1-month digit register				Mo8   Mo4   Mo2   Mo1
	0x09		10-month digit register			   Mo80  Mo40  Mo20  Mo10
	0x0A		1-year digit register				 Y8    Y4    Y2    Y1
	0x0B		10-yead digit register				Y80   Y40   Y20   Y10
	0x0C		Week register						  x    W4    W2    W1

													 30
	0x0D		Control register D					Sec   IRQ  Busy  Hold
													adj  Flag
													           ITRP
	0x0E		Control register E					 T1    T0 /STND  Mask
													  
													           
	0x0F		Control register F				   Test 24/12  Stop  Rest
													  
	
	Note: Digits are BDC. Registers only four bits wide.
	X denotes 'not used'
*/

/*****************************************************************************/
/**
 */
unsigned char read_time(emudevice_t * pEmuDevice, unsigned short port)
{
	distortc_t *	pDistoRTC = (distortc_t *)pEmuDevice;
	unsigned char	ret_val	= 0;
	time_t			clock;
	struct tm *		actualtime;
	
	ASSERT_DISTORTC(pDistoRTC);
	
	if ( port == (pDistoRTC->baseAddress & 0xFF)+0 )
	{
		time(&clock);
		actualtime = localtime(&clock);
	
		switch ( pDistoRTC->time_register )
		{
			case 0:
				ret_val = actualtime->tm_sec % 10;
			break;
				
			case 1:
				ret_val = actualtime->tm_sec / 10;
			break;
				
			case 2:
				ret_val = actualtime->tm_min % 10;
			break;
				
			case 3:
				ret_val = actualtime->tm_min / 10;
			break;
				
			case 4:
				ret_val = actualtime->tm_hour % 10;
			break;
				
			case 5:
				ret_val = actualtime->tm_hour / 10;
			break;
				
			case 6:
				ret_val = actualtime->tm_mday % 10;				// TODO: check - should this be day of week?
			break;
				
			case 7:
				ret_val = actualtime->tm_mday /10;				// TODO: check - should this be day of week?
			break;
				
			case 8:
				ret_val = actualtime->tm_mon % 10;
			break;
				
			case 9:
				ret_val = actualtime->tm_mon / 10 ;
			break;
				
			case 0xA:
				ret_val = actualtime->tm_year % 10;
			break;
				
			case 0xB:
				ret_val = (actualtime->tm_year % 100) / 10;
			break;
				
			case 0xC:
				ret_val = (unsigned char)actualtime->tm_wday;	// TODO: May not be right
			break;
				
			case 0xD:

			break;
				
			case 0xE:

			break;
				
			case 0xF:
			//	Hour12
			break;
				
			default:
				ret_val = 0;
			break;
		}
	}
	
	return ret_val;
}

/*****************************************************************************/
/**
 */
void write_time(emudevice_t * pEmuDevice, unsigned char data, unsigned char port)
{
	distortc_t *	pDistoRTC = (distortc_t *)pEmuDevice;
	
	ASSERT_DISTORTC(pDistoRTC);
	
    if ( port == (pDistoRTC->baseAddress & 0xFF)+0 )
    {
        switch ( pDistoRTC->time_register )
        {
            case 0x0F:
            //	Hour12 = !((data & 4)>>2);
            //	if (Hour12==0)
            //		MessageBox(0,"Setting format 0","Ok",0);
            //	else
            //		MessageBox(0,"Setting format 1","Ok",0);
            break;

            default:
            break;
        }
    }
    else if ( port == (pDistoRTC->baseAddress & 0xFF)+1 )
    {
			pDistoRTC->time_register = (data & 0xF);
	}
}

/*********************************************************************/
/**
 */
/*
 void distoRTCConfig(cocopak_t * pPak, unsigned char MenuID)
{
	assert(0);
}
*/

/*********************************************************************/
/**
 */
void distoRTCPortWrite(emudevice_t * pEmuDevice, unsigned char Port, unsigned char Data)
{
    distortc_t *    pDistoRTC = (distortc_t *)pEmuDevice;
    
    ASSERT_DISTORTC(pDistoRTC);
    
	if (  (Port == (pDistoRTC->baseAddress & 0xFF)+0)
		| (Port == (pDistoRTC->baseAddress & 0xFF)+1)
		)
	{
		/*
		 write to clock
		 */
		write_time(pEmuDevice,Data,Port);
	}
}

/*********************************************************************/
/**
 */
unsigned char distoRTCPortRead(emudevice_t * pEmuDevice, unsigned char Port)
{
    distortc_t *    pDistoRTC = (distortc_t *)pEmuDevice;
    
    ASSERT_DISTORTC(pDistoRTC);
    
	if (   (Port == (pDistoRTC->baseAddress & 0xFF)+0)
		 | (Port == (pDistoRTC->baseAddress & 0xFF)+1)
	   )
	{
		return read_time(pEmuDevice,Port);
	}
	
	return 0;
}

/*********************************************************************/
/**
 @param pPak Pointer to self
 */
void distoRTCReset(cocopak_t * pPak)
{
}

/**********************************************************/

result_t distoRTCEmuDevDestroy(emudevice_t * pEmuDevice)
{
	return XERROR_NONE;
}

/*****************************************************************************/

#if (defined _LIB)
XAPI_EXPORT cocopak_t * vccpakModuleCreate()
#else
cocopak_t * distoRTCCreate()
#endif			
{
	distortc_t *	pDistoRTC;
	
	pDistoRTC = malloc(sizeof(distortc_t));
	if ( pDistoRTC != NULL )
	{
		memset(pDistoRTC,0,sizeof(distortc_t));
		
		pDistoRTC->pak.device.id		= EMU_DEVICE_ID;
		pDistoRTC->pak.device.idModule	= VCC_COCOPAK_ID;
		pDistoRTC->pak.device.idDevice	= VCC_COCOPAK_DISTORTC_ID;
		
		pDistoRTC->pak.device.pfnDestroy	= distoRTCEmuDevDestroy;
		//pDistoRTC->pak.device.pfnSave		= distoRTCEmuDevConfSave;
		//pDistoRTC->pak.device.pfnLoad		= distoRTCEmuDevConfLoad;
		
		/*
			Initialize PAK API function pointers
		 */
		//pDistoRTC->pak.pakapi.pfnPakDestroy			= distoRTCDestroy;
		
		pDistoRTC->pak.pakapi.pfnPakReset               = distoRTCReset;
		
		//pDistoRTC->pak.pakAPIFunctions.pfnPakConfig	= distoRTCConfig;
		
		pDistoRTC->pak.pakapi.pfnPakPortRead			= distoRTCPortRead;
		pDistoRTC->pak.pakapi.pfnPakPortWrite			= distoRTCPortWrite;
		
		strcpy(pDistoRTC->pak.device.Name,"Disto RTC");

		return &pDistoRTC->pak;
	}
	
	return NULL;
}

/*****************************************************************************/

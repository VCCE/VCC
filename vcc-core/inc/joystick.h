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

#ifndef _joystick_h_
#define _joystick_h_

/*****************************************************************************/

#include "emuDevice.h"

/*****************************************************************************/

#define VCC_JOYSTICK_ID    XFOURCC('k','y','b','d')

#if (defined DEBUG)
#define ASSERT_JOYSTICK(pJoystick)      assert(pJoystick != NULL);    \
                                        assert(pJoystick->device.id == EMU_DEVICE_ID); \
                                        assert(pJoystick->device.idModule == VCC_JOYSTICK_ID)
#else
#define ASSERT_JOYSTICK(pJoystick)
#endif

/**************************************************/
/**
 joystick status
 */
typedef struct JoyStick
{
    unsigned char UseMouse;
    unsigned char Up;
    unsigned char Down;
    unsigned char Left;
    unsigned char Right;
    unsigned char Fire1;
    unsigned char Fire2;
    unsigned char DiDevice;
} JoyStick;

/*****************************************************************************/

typedef struct joystick_t
{
    emudevice_t             device;
    
    unsigned short          LeftStickX;
    unsigned short          LeftStickY;
    unsigned short          RightStickX;
    unsigned short          RightStickY;
    
    unsigned char           LeftButton1Status;
    unsigned char           RightButton1Status;
    unsigned char           LeftButton2Status;
    unsigned char           RightButton2Status;
    
    unsigned char           LeftStickNumber;
    unsigned char           RightStickNumber;
    
    JoyStick                Left;
    JoyStick                Right;
} joystick_t;

/*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
	
    joystick_t *    joystickCreate(void);
    
    // TODO: handle differently - called from CoCo3 keyboard scan to set registers
	unsigned short	get_pot_value(joystick_t * pJoystick, unsigned char pot);

#ifdef __cplusplus
}
#endif
				
/*****************************************************************************/

#endif // _joystick_h_

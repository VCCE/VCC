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

#ifndef _keyboard_h_
#define _keyboard_h_

/*****************************************************************************/

#include "emuDevice.h"

/*****************************************************************************/

#define KEYBOARDTABLE_NUM_ENTRIES    100

/*****************************************************************************/
/*
	Coco virtual key codes
 */

#define CCVK_SHIFT			0x01
#define CCVK_ALT			0x02
#define CCVK_CTRL			0x03
#define CCVK_CAPS			0x04
#define CCVK_F1				0x05
#define CCVK_F2				0x06

#define CCVK_UP				0x07
#define CCVK_DOWN			0x08
#define CCVK_LEFT			0x09
#define CCVK_RIGHT			0x0A

#define CCVK_BREAK			0x0B
#define CCVK_ESC			0x0C
#define CCVK_ENTER			0x0D
#define CCVK_CLEAR			0x0E
#define CCVK_DELETE			0x0F
#define CCVK_UNDERSCORE		0x1B
#define CCVK_SPACE			0x20
#define CCVK_EXCLAMATION	0x21
#define CCVK_QUOTATION		0x22
#define CCVK_NUMBER			0x23
#define CCVK_DOLLAR			0x24
#define CCVK_PERCENT		0x25
#define CCVK_AMPERSAND		0x26
#define CCVK_APOSTROPHE		0x27
#define CCVK_LEFTPAREN		0x28
#define CCVK_RIGHTPAREN		0x29
#define CCVK_ASTERISK		0x2A
#define CCVK_PLUS			0x2B
#define CCVK_COMMA			0x2C
#define CCVK_MINUS			0x2D
#define CCVK_PERIOD			0x2E
#define CCVK_SLASH			0x2F

#define CCVK_0				0x30
#define CCVK_1				0x31
#define CCVK_2				0x32
#define CCVK_3				0x33
#define CCVK_4				0x34
#define CCVK_5				0x35
#define CCVK_6				0x36
#define CCVK_7				0x37
#define CCVK_8				0x38
#define CCVK_9				0x39

#define CCVK_COLON			0x3A
#define CCVK_SEMICOLON		0x3B
#define CCVK_LESSTHAN		0x3C
#define CCVK_EQUALS			0x3D
#define CCVK_GREATERTHAN	0x3E
#define CCVK_QUESTIONMARK	0x3F
#define CCVK_AT				0x40

#define CCVK_A				0x41
#define CCVK_B				0x42
#define CCVK_C				0x43
#define CCVK_D				0x44
#define CCVK_E				0x45
#define CCVK_F				0x46
#define CCVK_G				0x47
#define CCVK_H				0x48
#define CCVK_I				0x49
#define CCVK_J				0x4A
#define CCVK_K				0x4B
#define CCVK_L				0x4C
#define CCVK_M				0x4D
#define CCVK_N				0x4E
#define CCVK_O				0x4F
#define CCVK_P				0x50
#define CCVK_Q				0x51
#define CCVK_R				0x52
#define CCVK_S				0x53
#define CCVK_T				0x54
#define CCVK_U				0x55
#define CCVK_V				0x56
#define CCVK_W				0x57
#define CCVK_X				0x58
#define CCVK_Y				0x59
#define CCVK_Z				0x5A

#define CCVK_LEFTBRACKET	0x5B
#define CCVK_BACKSLASH		0x5C
#define CCVK_RIGHTBRACKET	0x5D
#define CCVK_CARET			0x5E
#define CCVK_LEFTBRACE		0x7B
#define CCVK_PIPE			0x7C
#define CCVK_RIGHTBRACE		0x7D
#define CCVK_TILDE			0x7E

/*****************************************************************************/
/**
 */
typedef struct VCCKeyInfo
{
    unsigned int        ScanCode1;
    unsigned int        ScanCode2;
    unsigned char       Row1;
    unsigned char       Col1;
    unsigned char       Row2;
    unsigned char       Col2;
} VCCKeyInfo;

/*****************************************************************************/

#define VCC_KEYBOARD_ID    XFOURCC('k','y','b','d')

#if (defined DEBUG)
#define ASSERT_KEYBOARD(pKeyboard)      assert(pKeyboard != NULL);    \
                                        assert(pKeyboard->device.id == EMU_DEVICE_ID); \
                                        assert(pKeyboard->device.idModule == VCC_KEYBOARD_ID)
#else
#define ASSERT_KEYBOARD(pKeyboard)
#endif

/*****************************************************************************/

typedef struct keyboard_t
{
    emudevice_t         device;
    
    VCCKeyInfo          RawTable[4][KEYBOARDTABLE_NUM_ENTRIES];
    unsigned int        RolloverTable[8];
    unsigned int        ScanTable[256];
    VCCKeyInfo          Table[KEYBOARDTABLE_NUM_ENTRIES];

    // TODO: conf
    //int               KeyMap;
    //char              KeymapFile[XPATH_MAX_LENGTH];
} keyboard_t;

/*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
	
    keyboard_t *            keyboardCreate(void);

    unsigned int            keyboardTranslateSystemToCCVK(unsigned int uiSysKey);
    const VCCKeyInfo *      keyboardGetKeyInfoForCCVK(int ccvkCode);
    
	void			        keyboardBuildTable(keyboard_t * pKeyboard);
	void			        keyboardSortTable(keyboard_t * pKeyboard, unsigned char KeyBoardMode);
	
	unsigned char	        keyboardScan(keyboard_t * pKeyboard, unsigned char);
	void			        keyboardEvent(keyboard_t * pKeyboard, /* unsigned char, */ unsigned int, keyevent_e);
    
#ifdef __cplusplus
}
#endif
		
/*****************************************************************************/

#endif // _keyboard_h_

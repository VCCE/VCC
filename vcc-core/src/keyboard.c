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

/*****************************************************************/
/*
    TODO: save/load of keyboard table
    TODO: Config/edit of keyboard table
    TODO: restore joystick functionality
 
    TODO: SHIFT-0 does not work
    TODO: some keys are not translated correctly
    TODO: CAPS lock in incorrect state initially
 
 // grabbed from MAME source drv/coco3.c
 //-------------------------------------------------
 //  INPUT_PORTS( coco3_keyboard )
 //-------------------------------------------------
 //
 //  CoCo 3 keyboard
 //
 //         PB0 PB1 PB2 PB3 PB4 PB5 PB6 PB7
 //    PA6: Ent Clr Brk Alt Ctr F1  F2 Shift
 //    PA5: 8   9   :   ;   ,   -   .   /
 //    PA4: 0   1   2   3   4   5   6   7
 //    PA3: X   Y   Z   Up  Dwn Lft Rgt Space
 //    PA2: P   Q   R   S   T   U   V   W
 //    PA1: H   I   J   K   L   M   N   O
 //    PA0: @   A   B   C   D   E   F   G
 //-------------------------------------------------
 */
/*****************************************************************/

#include "keyboard.h"

#include "Vcc.h"
#include "tcc1014registers.h"
#include "joystick.h"

#include "xDebug.h"

/********************************************************************/
/*
    Direct table

    Scancode1 = Coco virtual key
    Scancode2 = Coco virtual key modifier

    Row1 = Coco keyboard grid row 1
    Col1 = Coco keyboard grid col 1
    Row2 = Coco keyboard grid row 2
    Col2 = Coco keyboard grid col 2
 */
const VCCKeyInfo gk_CocoKeyboard_Direct[KEYBOARDTABLE_NUM_ENTRIES] =
{
    // ScanCode1,        ScanCode2,          Row1, Col1, Row2, Col2,
    // 0-9
    CCVK_A,                0,                 1,  1,        0,  0,        // A
    CCVK_B,                0,                 1,  2,        0,  0,        // B
    CCVK_C,                0,                 1,  3,        0,  0,        // C
    CCVK_D,                0,                 1,  4,        0,  0,        // D
    CCVK_E,                0,                 1,  5,        0,  0,        // E
    CCVK_F,                0,                 1,  6,        0,  0,        // F
    CCVK_G,                0,                 1,  7,        0,  0,        // G
    CCVK_H,                0,                 2,  0,        0,  0,        // H
    CCVK_I,                0,                 2,  1,        0,  0,        // I
    CCVK_J,                0,                 2,  2,        0,  0,        // J
    
    // 10-19
    CCVK_K,                0,                 2,  3,        0,  0,        // K
    CCVK_L,                0,                 2,  4,        0,  0,        // L
    CCVK_M,                0,                 2,  5,        0,  0,        // M
    CCVK_N,                0,                 2,  6,        0,  0,        // N
    CCVK_O,                0,                 2,  7,        0,  0,        // O
    CCVK_P,                0,                 4,  0,        0,  0,        // P
    CCVK_Q,                0,                 4,  1,        0,  0,        // Q
    CCVK_R,                0,                 4,  2,        0,  0,        // R
    CCVK_S,                0,                 4,  3,        0,  0,        // S
    CCVK_T,                0,                 4,  4,        0,  0,        // T
    
    // 20-29
    CCVK_U,                0,                 4,  5,        0,  0,        // U
    CCVK_V,                0,                 4,  6,        0,  0,        // V
    CCVK_W,                0,                 4,  7,        0,  0,        // W
    CCVK_X,                0,                 8,  0,        0,  0,        // X
    CCVK_Y,                0,                 8,  1,        0,  0,        // Y
    CCVK_Z,                0,                 8,  2,        0,  0,        // Z
    CCVK_UP,               0,                 8,  3,        0,  0,        // Up arrow
    CCVK_DOWN,             0,                 8,  4,        0,  0,        // Down arrow
    CCVK_LEFT,             0,                 8,  5,        0,  0,        // Left arrow
    CCVK_RIGHT,            0,                 8,  6,        0,  0,        // Right arrow
    
    // 30-39
    CCVK_SPACE,            0,                 8,  7,        0,  0,        // Space
    CCVK_0,                0,                16,  0,        0,  0,        // 0
    CCVK_1,                0,                16,  1,        0,  0,        // 1
    CCVK_2,                0,                16,  2,        0,  0,        // 2
    CCVK_3,                0,                16,  3,        0,  0,        // 3
    CCVK_4,                0,                16,  4,        0,  0,        // 4
    CCVK_5,                0,                16,  5,        0,  0,        // 5
    CCVK_6,                0,                16,  6,        0,  0,        // 6
    CCVK_7,                0,                16,  7,        0,  0,        // 7
    CCVK_8,                0,                32,  0,        0,  0,        // 8
    
    // 40-49
    CCVK_9,                0,                32,  1,        0,  0,        // 9
    CCVK_COLON,            CCVK_SHIFT,       32,  2,        0,  0,        // :
    CCVK_SEMICOLON,        0,                32,  3,        0,  0,        // ;
    CCVK_COMMA,            0,                32,  4,        0,  0,        // ,
    CCVK_MINUS,            0,                32,  5,        0,  0,        // -
    CCVK_PERIOD,           0,                32,  6,        0,  0,        // .
    CCVK_SLASH,            0,                32,  7,        0,  0,        // /
    CCVK_ENTER,            0,                64,  0,        0,  0,        // ENTER
    CCVK_CLEAR,            0,                64,  1,        0,  0,        // HOME (CLEAR)
    CCVK_BREAK,            0,                64,  2,        0,  0,        // BREAK
    
    // 50-59
    CCVK_ALT,              0,                64,  3,        0,  0,        // ALT
    CCVK_CTRL,             0,                64,  4,        0,  0,        // CTRL
    CCVK_F1,               0,                64,  5,        0,  0,        // F1
    CCVK_F2,               0,                64,  6,        0,  0,        // F2
    CCVK_EXCLAMATION,      CCVK_SHIFT,       16,  1,        64, 7,        // !
    CCVK_QUOTATION,        CCVK_SHIFT,       16,  2,        64, 7,        // "
    CCVK_AMPERSAND,        CCVK_SHIFT,       16,  6,        64, 7,        // &
    CCVK_CARET,            CCVK_SHIFT,        8,  3,        0,  0,        // ^
    CCVK_APOSTROPHE,       0,                16,  7,        64, 7,        // '
    CCVK_LEFTPAREN,        CCVK_SHIFT,       32,  0,        64, 7,        // (
    
    // 60-69
    CCVK_RIGHTPAREN,       CCVK_SHIFT,       32,  1,        64, 7,        // )
    CCVK_ASTERISK,         CCVK_SHIFT,       32,  2,        64, 7,        // *
    CCVK_PLUS,             CCVK_SHIFT,       32,  3,        64, 7,        // +
    CCVK_LESSTHAN,         CCVK_SHIFT,       32,  4,        64, 7,        // <
    CCVK_EQUALS,           0,                32,  5,        64, 7,        // =
    CCVK_QUESTIONMARK,     CCVK_SHIFT,       32,  7,        64, 7,        // ?
    CCVK_LEFTBRACKET,      0,                64,  7,        8,  4,        // [
    CCVK_GREATERTHAN,      CCVK_SHIFT,       32,  6,        64, 7,        // >
    
    // 70-79
    CCVK_RIGHTBRACKET,     0,                64,  7,        8,  6,        // ]
    CCVK_DOLLAR,           0,                16,  4,       64,  7,        // $    NEEDS ROW/COL
    CCVK_PERCENT,          0,                16,  5,       64,  7,        // %    NEEDS ROW/COL
    CCVK_AT,               CCVK_SHIFT,        1,  0,       0,   0,        // @    NEEDS ROW/COL
    CCVK_NUMBER,           0,                16,  3,       64,  7,        // #
    CCVK_ESC,              0,                64,  2,       64,  7,        // ESC
    0,                     0,                 0,  0,        0,  0,        // ???
    0,                     0,                 0,  0,        0,  0,        // ???
    0,                     0,                 0,  0,        0,  0,        // ???
    0,                     0,                 0,  0,        0,  0,        // ???
    
    //80-89
    // NOTE: these had blank entries previously
    CCVK_UNDERSCORE,       CCVK_SHIFT,       32,  5,       64,  4,        // _ (Underscore)
    CCVK_TILDE,            CCVK_SHIFT,       16,  3,       64,  4,        // ~
    CCVK_LEFTBRACE,        CCVK_SHIFT,       64,  4,       32,  4,        // {
    CCVK_RIGHTBRACE,       CCVK_SHIFT,       64,  4,       32,  6,        // }
    CCVK_BACKSLASH,        CCVK_SHIFT,       32,  7,       64,  4,        // "\"
    CCVK_PIPE,             CCVK_SHIFT,       16,  1,       64,  4,        // | (Pipe)
    0,                     0,                 0,  0,        0,  0,        // ???
    0,                     0,                 0,  0,        0,  0,        // ???
    0,                     0,                 0,  0,        0,  0,        // ???
    0,                     0,                 0,  0,        0,  0,        // ???
    
    //90-99
    0,                     0,                 0,  0,        0,  0,        // ???
    0,                     0,                 0,  0,        0,  0,        // ???
    0,                     0,                 0,  0,        0,  0,        // ???
    0,                     0,                 0,  0,        0,  0,        // ???
    0,                     0,                 0,  0,        0,  0,        // ???
    0,                     0,                 0,  0,        0,  0,        // ???
    0,                     0,                 0,  0,        0,  0,        // ???
    0,                     0,                 0,  0,        0,  0,        // ???
    CCVK_CAPS,             0,                16,  0,        64, 7,        // CAPS
    CCVK_SHIFT,            0,                64,  7,        0,  0,        // SHIFT
};

/********************************************************************/

const VCCKeyInfo * keyboardGetKeyInfoForCCVK(int ccvkCode)
{
    const VCCKeyInfo * keyInfo;
    
    for (int i=0; i<KEYBOARDTABLE_NUM_ENTRIES; i++)
    {
        keyInfo = &gk_CocoKeyboard_Direct[i];
        
        if (keyInfo->ScanCode1 == ccvkCode)
        {
            return keyInfo;
        }
    }
    
    return NULL;
}

/****************************************************************************/

result_t kbEmuDevDestroy(emudevice_t * pEmuDevice)
{
    keyboard_t *    pKeyboard;
    
    pKeyboard = (keyboard_t *)pEmuDevice;
    
    ASSERT_KEYBOARD(pKeyboard);
    
    if ( pKeyboard != NULL )
    {
        free(pKeyboard);
    }
    
    return XERROR_NONE;
}

/****************************************************************************/

result_t kbEmuDevConfSave(emudevice_t * pEmuDevice, config_t * config)
{
    result_t    errResult    = XERROR_INVALID_PARAMETER;
    keyboard_t *    pKeyboard;
    
    pKeyboard = (keyboard_t *)pEmuDevice;
    
    ASSERT_KEYBOARD(pKeyboard);
    if ( pKeyboard != NULL )
    {
        // do nothing for now
        
        errResult = XERROR_NONE;
    }
    
    return errResult;
}

/****************************************************************************/

result_t kbEmuDevConfLoad(emudevice_t * pEmuDevice, config_t * config)
{
    result_t    errResult    = XERROR_INVALID_PARAMETER;
    keyboard_t *    pKeyboard;
    
    pKeyboard = (keyboard_t *)pEmuDevice;
    
    ASSERT_KEYBOARD(pKeyboard);
    if ( pKeyboard != NULL )
    {
        // do nothing for now
        
        errResult = XERROR_NONE;
    }

    return errResult;
}

/****************************************************************************/

#pragma mark -
#pragma mark --- implementation ---

/****************************************************************************/
/**
 */
keyboard_t * keyboardCreate()
{
    keyboard_t *    pKeyboard;

    pKeyboard = calloc(1,sizeof(keyboard_t));
    if ( pKeyboard != NULL )
    {
        pKeyboard->device.id          = EMU_DEVICE_ID;
        pKeyboard->device.idModule    = VCC_KEYBOARD_ID;
        strcpy(pKeyboard->device.Name,"Keyboard");
        
        pKeyboard->device.pfnDestroy    = kbEmuDevDestroy;
        pKeyboard->device.pfnSave       = kbEmuDevConfSave;
        pKeyboard->device.pfnLoad       = kbEmuDevConfLoad;
        
        emuDevRegisterDevice(&pKeyboard->device);
        
        // initialize keyboard
        keyboardBuildTable(pKeyboard);
        keyboardSortTable(pKeyboard, 0); // RSDOS
    }
    
    return pKeyboard;
}

/*****************************************************************/
/*
 */
void _keyboardUpdateScanTable(keyboard_t * pKeyboard)
{
	int	Index;
	unsigned int	LockOut = 0;
	
	assert(pKeyboard != NULL);
	
	// clear rollover table
	for (Index=0; Index<=7; Index++)
	{
		pKeyboard->RolloverTable[Index] = 0;
	}
	
	for (Index=0; Index<100; Index++)
	{
		if ( LockOut != pKeyboard->Table[Index].ScanCode1 )
		{
			if (   (pKeyboard->Table[Index].ScanCode1 != 0)
				 & (pKeyboard->Table[Index].ScanCode2 == 0)
			   )
			{
				// Single input key
				assert(pKeyboard->Table[Index].ScanCode1 < 256);
				if ( pKeyboard->ScanTable[pKeyboard->Table[Index].ScanCode1] == 1 )
				{
					assert(pKeyboard->Table[Index].Col1 < 8);
					pKeyboard->RolloverTable[pKeyboard->Table[Index].Col1] |= pKeyboard->Table[Index].Row1;
					assert(pKeyboard->Table[Index].Col2 < 8);
					pKeyboard->RolloverTable[pKeyboard->Table[Index].Col2] |= pKeyboard->Table[Index].Row2;
				}
			}
			
			if (   (pKeyboard->Table[Index].ScanCode1 != 0)
				 & (pKeyboard->Table[Index].ScanCode2 != 0)
			   )		
			{
				// Double Input Key
				assert(pKeyboard->Table[Index].ScanCode1 < 256);
				assert(pKeyboard->Table[Index].ScanCode2 < 256);
				if (   (pKeyboard->ScanTable[pKeyboard->Table[Index].ScanCode1] == 1)
					 & (pKeyboard->ScanTable[pKeyboard->Table[Index].ScanCode2] == 1)
				   )
				{
                    // TODO: review, removed because of a static analysis warning
					// LockOut = pKeyboard->Table[Index].ScanCode1;
                    
					assert(pKeyboard->Table[Index].Col1 < 8);
					pKeyboard->RolloverTable[pKeyboard->Table[Index].Col1] |= pKeyboard->Table[Index].Row1;
					assert(pKeyboard->Table[Index].Col2 < 8);
					pKeyboard->RolloverTable[pKeyboard->Table[Index].Col2] |= pKeyboard->Table[Index].Row2;
					break;
				}
			}
		}
	}
}

/*****************************************************************/
/**
    keyboard event handler

    Sent from platform UI

    @param pCoco3
    @param key
    @param ScanCode
    @param Status Key event type =
*/
void keyboardEvent(keyboard_t * pKeyboard, unsigned int ScanCode, keyevent_e eKeyEvent)
{
	int Index = 0;
	vccinstance_t *	pInstance;
	
	assert(pKeyboard != NULL);
	pInstance = (vccinstance_t *)emuDevGetParentModuleByID(&pKeyboard->device,VCC_CORE_ID);
	assert(pInstance != NULL);
	
	switch ( eKeyEvent )
	{
		case eEventKeyDown:
		{
			// key is pressed
			pKeyboard->ScanTable[ScanCode] = 1;
			
			_keyboardUpdateScanTable(pKeyboard);
				
            gimeAssertKeyboardInterrupt(pInstance->pCoco3->pGIME,pInstance->pCoco3->machine.pCPU);
		}
		break;

		case eEventKeyUp:
		{
			// key is no longer pressed
			pKeyboard->ScanTable[ScanCode] = 0;
			
			// Clean out rollover table on shift release
			if ( ScanCode == CCVK_SHIFT ) 
			{
				for (Index=0; Index<100; Index++)
				{
					pKeyboard->ScanTable[Index]=0;
				}
			}

			_keyboardUpdateScanTable(pKeyboard);
		}
		break;
			
		default:
			assert(0 && "unrecognized key event status");
		break;
	}
}

/*****************************************************************/
/*
    called from pia0_read
 
 // TODO: separate joystick handling
*/
unsigned char keyboardScan(keyboard_t * pKeyboard, unsigned char Col)
{
	unsigned char temp,x,mask,ret_val=0;
	unsigned short StickValue=0;
    vccinstance_t *    pInstance;
    
    assert(pKeyboard != NULL);
    pInstance = (vccinstance_t *)emuDevGetParentModuleByID(&pKeyboard->device,VCC_CORE_ID);
    assert(pInstance != NULL);

	temp = ~Col; //Get colums
	mask = 1;
	ret_val=0;
	
	for (x=0; x<=7; x++)
	{
		if ( temp & mask ) // Found an active column scan
		{
			ret_val |= pKeyboard->RolloverTable[x];
		}
		mask = (mask<<1);
	}
	ret_val = 127 - ret_val;
	
    //
    // joystick
    //
    
//	MuxSelect = GetMuxState();								// Collect CA2 and CB2 from the PIA (1of4 Multiplexer)
	StickValue = get_pot_value(pInstance->pCoco3->pJoystick,mc6821_GetMuxState(pInstance->pCoco3->pMC6821));
	
	if ( StickValue != 0 )									// OS9 joyin routine needs this (koronis rift works now)
	{
		if ( StickValue >= mc6821_DACState(pInstance->pCoco3->pMC6821) )		// Set bit of stick >= DAC output $FF20 Bits 7-2
		{
			ret_val |= 0x80;
		}
	}

	if ( pInstance->pCoco3->pJoystick->LeftButton1Status == 1 )
	{
		ret_val = ret_val & 0xFD;	//Left Joystick Button 1 Down?
	}
	
	if ( pInstance->pCoco3->pJoystick->RightButton1Status == 1 )
	{
		ret_val = ret_val & 0xFE;	//Right Joystick Button 1 Down?
	}
	
	if ( pInstance->pCoco3->pJoystick->LeftButton2Status == 1 )
	{
		ret_val = ret_val & 0xF7;	//Left Joystick Button 2 Down?
	}
	
	if ( pInstance->pCoco3->pJoystick->RightButton2Status == 1 )
	{
		ret_val = ret_val & 0xFB;	//Right Joystick Button 2 Down?
	}
	
    //
    // end joystick
    //
    
	if ( (ret_val & 0x7F) != 0x7F ) 
	{
        // TODO: is this when the interrupt should be fired?
        gimeAssertKeyboardInterrupt(pInstance->pCoco3->pGIME,pInstance->pCoco3->machine.pCPU);
	}

	return ret_val;
}

/********************************************************************/
/*
	Sort the keyboard table

	Copy raw keyboard table to main keyboard table used during emulation
	entries are sorted so that double key combinations come first
	and the shift key by itself comes last.
*/
void keyboardSortTable(keyboard_t * pKeyboard, unsigned char KeyBoardMode) //Any SHIFT+ [char] entries need to be placed first
{
	int Index1;
	int Index2;
	
	// 'repair' raw key table
	// Shift must be first
	for (Index1=0; Index1<=KEYBOARDTABLE_NUM_ENTRIES; Index1++)		
	{
		// check for SHIFT on scan code 2
		if ( pKeyboard->RawTable[KeyBoardMode][Index1].ScanCode2 == CCVK_SHIFT )
		{
			// swap scancode 1 & 2
			pKeyboard->RawTable[KeyBoardMode][Index1].ScanCode2 = pKeyboard->RawTable[KeyBoardMode][Index1].ScanCode1;
			pKeyboard->RawTable[KeyBoardMode][Index1].ScanCode1 = CCVK_SHIFT;
		}
		
		// look for scan code 2 used, but scan code 1 not set
		if (   (pKeyboard->RawTable[KeyBoardMode][Index1].ScanCode1 == 0)
			 & (pKeyboard->RawTable[KeyBoardMode][Index1].ScanCode2 != 0)
		   )
		{
			// swap scancode 1 & 2
			pKeyboard->RawTable[KeyBoardMode][Index1].ScanCode1 = pKeyboard->RawTable[KeyBoardMode][Index1].ScanCode2;
			pKeyboard->RawTable[KeyBoardMode][Index1].ScanCode2 = 0;
		}
	}
	
	// clear invalid/incomplete entries in raw table
	for (Index1=0; Index1<=KEYBOARDTABLE_NUM_ENTRIES; Index1++)
	{
		if ( pKeyboard->RawTable[KeyBoardMode][Index1].ScanCode1 == 0 )
		{
			pKeyboard->RawTable[KeyBoardMode][Index1].Col1=0;
			pKeyboard->RawTable[KeyBoardMode][Index1].Col2=0;
			pKeyboard->RawTable[KeyBoardMode][Index1].Row1=0;
			pKeyboard->RawTable[KeyBoardMode][Index1].Row2=0;
		}
	}
	
	// clear destination table
	for (Index2=0; Index2<=KEYBOARDTABLE_NUM_ENTRIES; Index2++)
	{
		pKeyboard->Table[Index2].ScanCode1=0;
		pKeyboard->Table[Index2].ScanCode2=0;
		pKeyboard->Table[Index2].Col1=0;
		pKeyboard->Table[Index2].Col2=0;
		pKeyboard->Table[Index2].Row1=0;
		pKeyboard->Table[Index2].Row2=0;
	}
	
	//
	// copy raw keyboard table to main keyboard table
	//
	// only valid entries will be copied
	//
	Index2 = 0;
	
	// Double key combos first
	for (Index1=0; Index1<=KEYBOARDTABLE_NUM_ENTRIES; Index1++)		
	{
		if (   (pKeyboard->RawTable[KeyBoardMode][Index1].ScanCode1 != 0)
			 & (pKeyboard->RawTable[KeyBoardMode][Index1].ScanCode2 != 0)
		   )
		{
			pKeyboard->Table[Index2++] = pKeyboard->RawTable[KeyBoardMode][Index1];
		}
	}
	// copy remaining
	for (Index1=0; Index1<=KEYBOARDTABLE_NUM_ENTRIES; Index1++)
	{
		if (   (pKeyboard->RawTable[KeyBoardMode][Index1].ScanCode1 != 0)
			 & (pKeyboard->RawTable[KeyBoardMode][Index1].ScanCode2 == 0)
			 & (pKeyboard->RawTable[KeyBoardMode][Index1].ScanCode1 != CCVK_SHIFT)
		  )
		{
			pKeyboard->Table[Index2++] = pKeyboard->RawTable[KeyBoardMode][Index1];
		}
	}
	// Shift must be last
	pKeyboard->Table[Index2].ScanCode1 = CCVK_SHIFT;
	pKeyboard->Table[Index2].Col1		= 7;
	pKeyboard->Table[Index2].Row1		= 64;
}

/********************************************************************/
/**
 Build raw keyboard table (Mac OS X version)
 */
void keyboardBuildTable(keyboard_t * pKeyboard)
{
    assert(pKeyboard != NULL);
    
	/*
		clear all raw tables
	*/
	memset(pKeyboard->RawTable,0,sizeof(pKeyboard->RawTable));
	
	/*
		copy raw tables
	*/
	
	// direct - keyboard mode 0
	assert(sizeof(pKeyboard->RawTable[0]) == sizeof(gk_CocoKeyboard_Direct));
	memcpy(&pKeyboard->RawTable[0],gk_CocoKeyboard_Direct,sizeof(gk_CocoKeyboard_Direct));
	
#if 0
	// DECB - keyboard mode 0
	assert(sizeof(pKeyboard->RawTable[0]) == sizeof(gk_CocoKeyboard_DECB));
	memcpy(&pKeyboard->RawTable[1],gk_CocoKeyboard_DECB,sizeof(gk_CocoKeyboard_DECB));

	// OS-9 - keyboard mode 1
	assert(sizeof(pKeyboard->RawTable[0]) == sizeof(gk_CocoKeyboard_OS9));
	memcpy(&pKeyboard->RawTable[1],gk_CocoKeyboard_OS9,sizeof(gk_CocoKeyboard_OS9));
#endif
}

/********************************************************************/

// TODO: prep for reading and writing the keymap file
// it will need to use keywords and not virtual key codes of any kind
// so it can be used on any platform without issue

#if 0
void ReadKeymapFile(pvccinstance_t pInstance, char *KeyFile)
{
	/// TODO: Read keymap file
	XTRACE("TODO: Read keymap file\n");
	
#if 0
	unsigned char MapBuffer[MAXPOS*6]="";
	unsigned int Index=0,KeyIndex=0;
	HANDLE MapFile=NULL;
	unsigned long BytesMoved=0;
	MapFile = CreateFile(KeyFile,GENERIC_READ ,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	ReadFile(MapFile,MapBuffer,MAXPOS*6,&BytesMoved,NULL);
	CloseHandle(MapFile);
	for (KeyIndex=0;KeyIndex<MAXPOS;KeyIndex++)
	{
		pInstance->RawTable[3][KeyIndex].ScanCode1=MapBuffer[Index++];
		pInstance->RawTable[3][KeyIndex].ScanCode2=MapBuffer[Index++];
		pInstance->RawTable[3][KeyIndex].Col1=MapBuffer[Index++];
		pInstance->RawTable[3][KeyIndex].Col2=MapBuffer[Index++];
		pInstance->RawTable[3][KeyIndex].Row1=MapBuffer[Index++];
		pInstance->RawTable[3][KeyIndex].Row2=MapBuffer[Index++];
	}
#endif
	
	return;
}
#endif

/********************************************************************/

#if 0
void WriteKeymapFile(pvccinstance_t pInstance, char *KeyFile)
{
	/// TODO: write keymap file
	XTRACE("TODO: Write keymap file\n");
	
#if 0
	unsigned char MapBuffer[MAXPOS*6]="";
	unsigned int Index=0,KeyIndex=0;
	HANDLE MapFile=NULL;
	unsigned long BytesMoved=0;
	for (KeyIndex=0;KeyIndex<MAXPOS;KeyIndex++)
	{
		MapBuffer[Index++]=pInstance->RawTable[3][KeyIndex].ScanCode1;
		MapBuffer[Index++]=pInstance->RawTable[3][KeyIndex].ScanCode2;
		MapBuffer[Index++]=pInstance->RawTable[3][KeyIndex].Col1;
		MapBuffer[Index++]=pInstance->RawTable[3][KeyIndex].Col2;
		MapBuffer[Index++]=pInstance->RawTable[3][KeyIndex].Row1;
		MapBuffer[Index++]=pInstance->RawTable[3][KeyIndex].Row2;
	}
	MapFile = CreateFile(KeyFile,GENERIC_READ | GENERIC_WRITE,0,0,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
	if (MapFile == INVALID_HANDLE_VALUE)	
		return;
	WriteFile(MapFile,MapBuffer,MAXPOS*6,&BytesMoved,NULL);
	CloseHandle(MapFile);
#endif
	
	return;
}
#endif

/*****************************************************************/


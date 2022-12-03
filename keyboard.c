/*****************************************************************************/
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
	Keyboard handling / translation - system -> emulator

	TODO: any key(s) that results in a multi-key (eg. SHIFT/ALT/CTRL)
	        combination on the CoCo side is only sending one interrupt
			signal for both keys.  This seems to work in virtually all cases,
			but is it an accurate emulation?
*/
/*****************************************************************************/

#define DIRECTINPUT_VERSION 0x0800
// this must be before dinput.h as it contains Windows types and not Windows.h include
#include <windows.h>
#include <dinput.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "defines.h"
#include "keyboard.h"
#include "mc6821.h"
#include "tcc1014registers.h"
#include "Vcc.h"
#include "joystickinput.h"
#include "keyboardLayout.h"
#include "config.h"

#include <queue>

#include "xDebug.h"

/*****************************************************************************/
/*
	Forward declarations
*/

unsigned char SetMouseStatus(unsigned char, unsigned char);
bool pasting = false;  //Are the keyboard functions in the middle of a paste operation?
bool GetNextScanInPasteQueue(unsigned char col);
bool IsShiftKeyDown();

/*****************************************************************************/
//	Global variables
/*****************************************************************************/

// key translation table maximum size, (arbitrary) most of the layouts are < 80 entries
#define KBTABLE_ENTRY_COUNT 100
#define KEY_DOWN	1
#define KEY_UP		0

// Simulated Keypresses for Clipboard pasting.
std::queue<unsigned char> PasteInputQueue;
LARGE_INTEGER Frequency;
LARGE_INTEGER LastAdvance;
double PasteDelay = 0.008;		// This is the interkey delay in seconds - it sets the time between KEYDOWN and KEYUP

int CurrentThrottle = 0;
bool IsShift = false;

enum PasteState
{
	Idle,
	KeyDown,
	KeyUp,
	CheckDone,
};
PasteState CurrentPasteState;


/* track all keyboard scan codes state (up/down) */
static int ScanTable[256];

/* run-time 'rollover' table to pass to the MC6821 when a key is pressed */
static unsigned char RolloverTable[8];	// CoCo 'keys' for emulator

// run-time keyboard layout table (key(s) to keys(s) translation)
static keytranslationentry_t KeyTransTable[KBTABLE_ENTRY_COUNT];

/*****************************************************************************/
//	Get CoCo 'scan' code called from MC6821.c to read the keyboard/joystick state
/*****************************************************************************/

unsigned char
vccKeyboardGetScan(unsigned char Col)
{
	unsigned char temp;
	unsigned char x;
	unsigned char mask;
	unsigned char ret_val;

	ret_val = 0;

	GetNextScanInPasteQueue(Col);

	temp = ~Col; //Get colums
	mask = 1;

	for (x=0; x<8; x++)
	{
		if ( (temp & mask) ) // Found an active column scan
		{
			ret_val |= RolloverTable[x];
		}

		mask = (mask<<1);
	}
	ret_val = 127 - ret_val;

    // Add joystick button and analog compare bits.
    ret_val = vccJoystickGetScan(ret_val);

#if 0 // no noticible change when this is disabled
	// TODO: move to MC6821/GIME
	{
		/** another keyboard IRQ flag - this should really be in the GIME code*/
		static unsigned char IrqFlag = 0;
		if ((ret_val & 0x7F) != 0x7F)
		{
			if ((IrqFlag == 0) & GimeGetKeyboardInteruptState())
			{
				GimeAssertKeyboardInterupt();
				IrqFlag = 1;
			}
		}
		else
		{
			IrqFlag = 0;
		}
	}
#endif

	return (ret_val);
}

/*****************************************************************************/
void _vccKeyboardUpdateRolloverTable()
{
	int				Index;
	unsigned char	LockOut = 0;

	// clear the rollover table
	for (Index = 0; Index < 8; Index++)
	{
		RolloverTable[Index] = 0;
	}

	// set rollover table based on ScanTable key status
	for (Index = 0; Index<KBTABLE_ENTRY_COUNT; Index++)
	{
		// stop at last entry
		if (  (KeyTransTable[Index].ScanCode1 == 0)
			& (KeyTransTable[Index].ScanCode2 == 0)
			)
		{
			break;
		}
	
		if ( LockOut != KeyTransTable[Index].ScanCode1 )
		{
			// Single input key
			if (   (KeyTransTable[Index].ScanCode1 != 0)
				&& (KeyTransTable[Index].ScanCode2 == 0)
				)
			{
				// check if key pressed
				if (ScanTable[KeyTransTable[Index].ScanCode1] == KEY_DOWN)
				{
					int col;
				
					col = KeyTransTable[Index].Col1;
					assert(col >=0 && col <8);
					RolloverTable[col] |= KeyTransTable[Index].Row1;
				
					col = KeyTransTable[Index].Col2;
					assert(col >= 0 && col <8);
					RolloverTable[col] |= KeyTransTable[Index].Row2;
				}
			}

			// Double Input Key
			if (   (KeyTransTable[Index].ScanCode1 != 0)
				&& (KeyTransTable[Index].ScanCode2 != 0)
				)
			{
				// check if both keys pressed
				if (   (ScanTable[KeyTransTable[Index].ScanCode1] == KEY_DOWN)
					&& (ScanTable[KeyTransTable[Index].ScanCode2] == KEY_DOWN)
					)
				{
					int col;

					col = KeyTransTable[Index].Col1;
					assert(col >=0 && col <8);
					RolloverTable[col] |= KeyTransTable[Index].Row1;
				
					col = KeyTransTable[Index].Col2;
					assert(col >= 0 && col <8);
					RolloverTable[col] |= KeyTransTable[Index].Row2;
				
					// always SHIFT
					LockOut = KeyTransTable[Index].ScanCode1;

					break;
				}
			}
		}
	}
}

/*****************************************************************************/
//	Dispatch keyboard event to the emulator.
//
//	Called from system. eg. WndProc : WM_KEYDOWN/WM_SYSKEYDOWN/WM_SYSKEYUP/WM_KEYUP
//
//	@param key Windows virtual key code (VK_XXXX - not used)
//	@param ScanCode keyboard scan code (DIK_XXXX - DirectInput)
//	@param Status Key status - kEventKeyDown/kEventKeyUp
/*****************************************************************************/

void vccKeyboardHandleKey(unsigned char key, unsigned char ScanCode, keyevent_e keyState)
{
	//XTRACE("Key  : %c (%3d / 0x%02X)  Scan : %d / 0x%02X\n",key,key,key, ScanCode, ScanCode);
	//If requested, abort pasting operation.
	if (ScanCode == 0x01 || ScanCode == 0x43 || ScanCode == 0x3F) { pasting = false; }

	// check for shift key
	// Left and right shift generate different scan codes
	if (ScanCode == DIK_RSHIFT)
	{
		ScanCode = DIK_LSHIFT;
	}
#if 0 // TODO: CTRL and/or ALT?
	// CTRL key - right -> left
	if (ScanCode == DIK_RCONTROL)
	{
		ScanCode = DIK_LCONTROL;
	}
	// ALT key - right -> left
	if (ScanCode == DIK_RMENU)
	{
		ScanCode = DIK_LMENU;
	}
#endif

	switch (keyState)
	{
		default:
			// internal error
		break;

		// Key Down
		case kEventKeyDown:
		    ScanCode = SetMouseStatus(ScanCode, 1); // In case keyboard joystick

			// track key is down
			ScanTable[ScanCode] = KEY_DOWN;
			if (ScanCode == DIK_LSHIFT) {
				IsShift = true;
			}
			_vccKeyboardUpdateRolloverTable();

			if ( GimeGetKeyboardInteruptState() ) {
				GimeAssertKeyboardInterupt();
			}
		break;

		// Key Up
		case kEventKeyUp:
			ScanCode = SetMouseStatus(ScanCode, 0);

			// reset key (released)
			ScanTable[ScanCode] = KEY_UP;

			// TODO: verify this is accurate emulation
			// Clean out rollover table on shift release
			if ( ScanCode == DIK_LSHIFT ) {
				IsShift = false; 
				for (int Index = 0; Index < KBTABLE_ENTRY_COUNT; Index++) {
					ScanTable[Index] = KEY_UP;
				}
			}
			_vccKeyboardUpdateRolloverTable();
		break;
	}
}

/*****************************************************************************/
/*
 *	Key translation table compare function for sorting (with qsort)
*/
int keyTransCompare(const void * e1, const void * e2)
{
	keytranslationentry_t *	entry1 = (keytranslationentry_t *)e1;
	keytranslationentry_t *	entry2 = (keytranslationentry_t *)e2;
	int						result = 0;

	// ascending
	// elem1 - elem2

	// empty listing push to end
	if (   entry1->ScanCode1 == 0
		&& entry1->ScanCode2 == 0
		&& entry2->ScanCode1 != 0
		)
	{
		return 1;
	}
	else
	if (   entry2->ScanCode1 == 0
		&& entry2->ScanCode2 == 0
		&& entry1->ScanCode1 != 0
		)
	{
		return -1;
	}
	else
	// push shift/alt/control by themselves to the end
	if (   entry1->ScanCode2 == 0
		&& (   entry1->ScanCode1 == DIK_LSHIFT
		    || entry1->ScanCode1 == DIK_LMENU
		    || entry1->ScanCode1 == DIK_LCONTROL
		   )
		)
	{
		result = 1;
	}
	else
	// push shift/alt/control by themselves to the end
	if (   entry2->ScanCode2 == 0
		&& (   entry2->ScanCode1 == DIK_LSHIFT
		    || entry2->ScanCode1 == DIK_LMENU
		    || entry2->ScanCode1 == DIK_LCONTROL
			)
		)
	{
		result = -1;
	}
	else
	// move double key combos in front of single ones
	if (   entry1->ScanCode2 == 0
		&& entry2->ScanCode2 != 0
		)
	{
		result = 1;
	}
	else
	// move double key combos in front of single ones
	if (   entry2->ScanCode2 == 0
		&& entry1->ScanCode2 != 0
		)
	{
		result = -1;
	}
	else
	{
		result = entry1->ScanCode1 - entry2->ScanCode1;

		if (result == 0)
		{
			result = entry1->Row1 - entry2->Row1;
		}

		if (result == 0)
		{
			result = entry1->Col1 - entry2->Col1;
		}

		if (result == 0)
		{
			result = entry1->Row2 - entry2->Row2;
		}

		if (result == 0)
		{
			result = entry1->Col2 - entry2->Col2;
		}
	}

	return result;
}

/*****************************************************************************/
/*
 *	Rebuilds the run-time keyboard translation lookup table based on the
 *	current keyboard layout.
 *
 * 	The entries are sorted.  Any SHIFT + [char] entries need to be placed first
*/
void vccKeyboardBuildRuntimeTable(keyboardlayout_e keyBoardLayout)
{
	int Index1 = 0;
	int Index2 = 0;
	keytranslationentry_t *		keyLayoutTable = NULL;
	keytranslationentry_t		keyTransEntry;

	assert(keyBoardLayout >= 0 && keyBoardLayout < kKBLayoutCount);

	switch (keyBoardLayout)
	{
		case kKBLayoutCoCo:
			keyLayoutTable = keyTranslationsCoCo;
		break;

		case kKBLayoutNatural:
			keyLayoutTable = keyTranslationsNatural;
			break;

		case kKBLayoutCompact:
			keyLayoutTable = keyTranslationsCompact;
			break;

		case kKBLayoutCustom:
			keyLayoutTable = keyTranslationsCustom;
			break;

		default:
			assert(0 && "unknown keyboard layout");
		break;
	}

	XTRACE("Building run-time key table for layout # : %d - %s\n", keyBoardLayout, k_keyboardLayoutNames[keyBoardLayout]);

	// copy the selected keyboard layout to the run-time table
	memset(KeyTransTable, 0, sizeof(KeyTransTable));
	Index2 = 0;
	for (Index1 = 0; ; Index1++)
	{
		memcpy(&keyTransEntry, &keyLayoutTable[Index1], sizeof(keytranslationentry_t));

		//
		// Change entries to what the code expects
		//
		// Make sure ScanCode1 is never 0
		// If the key combo uses SHIFT, put it in ScanCode1
		// Completely clear unused entries (ScanCode1+2 are 0)
		//

		//
		// swaps ScanCode1 with ScanCode2 if ScanCode2 == DIK_LSHIFT
		//
		if ( keyTransEntry.ScanCode2 == DIK_LSHIFT )
		{
			keyTransEntry.ScanCode2 = keyTransEntry.ScanCode1;
			keyTransEntry.ScanCode1 = DIK_LSHIFT;
		}

		//
		// swaps ScanCode1 with ScanCode2 if ScanCode1 is zero
		//
		if (  (keyTransEntry.ScanCode1 == 0)
			& (keyTransEntry.ScanCode2 != 0)
			)
		{
			keyTransEntry.ScanCode1 = keyTransEntry.ScanCode2;
			keyTransEntry.ScanCode2 = 0;
		}

		// check for terminating entry
		if (   keyTransEntry.ScanCode1 == 0
			&& keyTransEntry.ScanCode2 == 0
			)
		{
			break;
		}

		memcpy(&KeyTransTable[Index2++], &keyTransEntry,sizeof(keytranslationentry_t));

		assert(Index2 <= KBTABLE_ENTRY_COUNT && "keyboard layout table is longer than we can handle");
	}

	//
	// Sort the key translation table
	//
	// Since the table is searched from beginning to end each
	// time a key is pressed, we want them to be in the correct
	// order.
	//
	qsort(KeyTransTable, KBTABLE_ENTRY_COUNT, sizeof(keytranslationentry_t), keyTransCompare);

#ifdef _DEBUG
	//
	// Debug dump the table
	//
	for (Index1 = 0; Index1 < KBTABLE_ENTRY_COUNT; Index1++)
	{
		// check for null entry
		if (   KeyTransTable[Index1].ScanCode1 == 0
			&& KeyTransTable[Index1].ScanCode2 == 0
			)
		{
			// done
			break;
		}

		XTRACE("Key: %3d - 0x%02X (%3d) 0x%02X (%3d) - %2d %2d  %2d %2d\n",
			Index1,
			KeyTransTable[Index1].ScanCode1,
			KeyTransTable[Index1].ScanCode1,
			KeyTransTable[Index1].ScanCode2,
			KeyTransTable[Index1].ScanCode2,
			KeyTransTable[Index1].Row1,
			KeyTransTable[Index1].Col1,
			KeyTransTable[Index1].Row2,
			KeyTransTable[Index1].Col2
		);
	}
#endif
}

bool GetPaste() {
	return pasting;
}
void SetPaste(bool tmp) {
	pasting = tmp;
}

void PasteIntoQueue(std::string txt)
{
	vccKeyboardBuildRuntimeTable((keyboardlayout_e)1);

	for (auto& c : txt)
	{
		PasteInputQueue.push(c);
	}
	QueryPerformanceFrequency(&Frequency);
	QueryPerformanceCounter(&LastAdvance);
	CurrentPasteState = PasteState::KeyDown;
	CurrentThrottle = SetSpeedThrottle(QUERY);
	SetSpeedThrottle(0);
	SetPaste(true);
}

#include <sstream>

bool GetNextScanInPasteQueue(unsigned char col)
{
	if (CurrentPasteState == PasteState::Idle)
	{
		return false;
	}

	// Get the Current Tick
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);

	// Time to advance state?
	float elapsed = (now.QuadPart - LastAdvance.QuadPart) / (float)Frequency.QuadPart;
	if (elapsed < PasteDelay)
	{
		return false;
	}

	LastAdvance = now;

	switch (CurrentPasteState)
	{
	case PasteState::KeyDown:
	{
		// Peek at next character.
		unsigned char next = PasteInputQueue.front();
		// Shift Key?
		if (next == 0x36)
		{
			// Pop it and get the next key.
			PasteInputQueue.pop();
			vccKeyboardHandleKey(0x36, 0x36, kEventKeyDown);
			next = PasteInputQueue.front();
		}
		vccKeyboardHandleKey(next, next, kEventKeyDown);
		CurrentPasteState = KeyUp;
		break;
	}

	case PasteState::KeyUp:
	{
		// Which key was down?
		unsigned char last = PasteInputQueue.front();
		vccKeyboardHandleKey(0x36, 0x36, kEventKeyUp);
		vccKeyboardHandleKey(0x42, last, kEventKeyUp);
		// Ok, done with that character.
		PasteInputQueue.pop();
		CurrentPasteState = CheckDone;
		break;
	}

	case PasteState::CheckDone:
	{
		if (PasteInputQueue.size() == 0)
		{
			SetPaste(false);
			vccKeyboardBuildRuntimeTable((keyboardlayout_e)GetKeyboardLayout());
			SetSpeedThrottle(CurrentThrottle);
			CurrentPasteState = PasteState::Idle;
		}
		else
		{
			CurrentPasteState = PasteState::KeyDown;
		}
		break;
	}
	}
	return true;
}
bool IsShiftKeyDown() {
	return IsShift;
}
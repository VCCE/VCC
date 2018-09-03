/********************************************************************/
/*
 *  _keyboard.c
 */
/********************************************************************/

#include "keyboard.h"

#include "xDebug.h"

#if !(defined _WIN32)
#	error "WINDOWS implementation"
#endif

#include <Windows.h>
#include <assert.h>

/********************************************************************/
/*
	Translation table to convert from Windows virtual key to Coco virtual key
 */
const unsigned int gk_TranslateToCCVK[] = 
{
	CCVK_A,				0x41,		// A
	CCVK_B,				0x42,		// B
	CCVK_C,				0x43,		// C
	CCVK_D,				0x44,		// D
	CCVK_E,				0x45,		// E
	CCVK_F,				0x46,		// F
	CCVK_G,				0x47,		// G
	CCVK_H,				0x48,		// H
	CCVK_I,				0x49,		// I
	CCVK_J,				0x4A,		// J
	CCVK_K,				0x4B,		// K
	CCVK_L,				0x4C,		// L
	CCVK_M,				0x4D,		// M
	CCVK_N,				0x4E,		// N
	CCVK_O,				0x4F,		// O
	CCVK_P,				0x50,		// P
	CCVK_Q,				0x51,		// Q
	CCVK_R,				0x52,		// R
	CCVK_S,				0x53,		// S
	CCVK_T,				0x54,		// T
	CCVK_U,				0x55,		// U
	CCVK_V,				0x56,		// V
	CCVK_W,				0x57,		// W
	CCVK_X,				0x58,		// X
	CCVK_Y,				0x59,		// Y
	CCVK_Z,				0x5A,		// Z
	
	CCVK_A,				0x61,		// a
	CCVK_B,				0x62,		// b
	CCVK_C,				0x63,		// c
	CCVK_D,				0x64,		// d
	CCVK_E,				0x65,		// e
	CCVK_F,				0x66,		// f
	CCVK_G,				0x67,		// g
	CCVK_H,				0x68,		// h
	CCVK_I,				0x69,		// i
	CCVK_J,				0x6A,		// j
	CCVK_K,				0x6B,		// k
	CCVK_L,				0x6C,		// l
	CCVK_M,				0x6D,		// m
	CCVK_N,				0x6E,		// n
	CCVK_O,				0x6F,		// o
	CCVK_P,				0x70,		// p
	CCVK_Q,				0x71,		// q
	CCVK_R,				0x72,		// r
	CCVK_S,				0x73,		// s
	CCVK_T,				0x74,		// t
	CCVK_U,				0x75,		// u
	CCVK_V,				0x76,		// v
	CCVK_W,				0x77,		// w
	CCVK_X,				0x78,		// x
	CCVK_Y,				0x79,		// y
	CCVK_Z,				0x7A,		// z
	
	CCVK_UP,			VK_UP,		// Up arrow
	CCVK_DOWN,			VK_DOWN,	// Down arrow
	CCVK_LEFT,			VK_LEFT,	// Left arrow
	CCVK_RIGHT,			VK_RIGHT,	// Right arrow

	CCVK_SPACE,			0x20,		// Space

	CCVK_0,				0x30,		// 0
	CCVK_1,				0x31,		// 1
	CCVK_2,				0x32,		// 2
	CCVK_3,				0x33,		// 3
	CCVK_4,				0x34,		// 4
	CCVK_5,				0x35,		// 5
	CCVK_6,				0x36,		// 6
	CCVK_7,				0x37,		// 7
	CCVK_8,				0x38,		// 8
	CCVK_9,				0x39,		// 9
	
	CCVK_COLON,			0x3A,		// :
	CCVK_SEMICOLON,		0x3B,		// ;
	CCVK_COMMA,			0x2C,		// ,
	CCVK_MINUS,			0x2D,		// -
	CCVK_PERIOD,		0x2E,		// .
	CCVK_SLASH,			0x2F,		// /
	CCVK_ENTER,			VK_RETURN,		// ENTER
	CCVK_CLEAR,			VK_HOME,		// HOME (CLEAR)
	CCVK_BREAK,			VK_ESCAPE,		// ESC
	
	CCVK_F1,			VK_F1,		// F1
	CCVK_F2,			VK_F2,		// F2
	
	CCVK_EXCLAMATION,	0x21,		// !
	CCVK_AT,			0x40,		// @
	CCVK_AMPERSAND,		0x26,		// &
	CCVK_CARET,			0x5e,		// ^
	CCVK_APOSTROPHE,	0x27,		// '
	CCVK_LEFTPAREN,		0x28,		// (
	CCVK_RIGHTPAREN,	0x29,		// )
	CCVK_ASTERISK,		0x42,		// *
	CCVK_PLUS,			0x2B,		// +
	CCVK_LESSTHAN,		0x3C,		// <
	CCVK_GREATERTHAN,	0x3E,		// >
	CCVK_EQUALS,		0x3D,		// =
	CCVK_QUESTIONMARK,	0x3F,		// ?
	CCVK_LEFT,			VK_BACK,// BACKSPACE (same as left arrow)
	CCVK_LEFTBRACKET,	0x5B,		// [
	CCVK_RIGHTBRACKET,	0x5D,		// ]
	CCVK_LEFTBRACE,		0x7B,		// {
	CCVK_RIGHTBRACE,	0x7D,		// }
	CCVK_BACKSLASH,		0x5C,		// "\"
	CCVK_PIPE,			0x7C,		// | (Pipe)
	CCVK_UNDERSCORE,	0x5F,		// _ (Underscore)
	CCVK_TILDE,			0x7E,		// ~
	CCVK_NUMBER,		0x23,		// #
	CCVK_ASTERISK,		0x2A,		// *
	CCVK_DOLLAR,		0x24,		// $
	CCVK_PERCENT,		0x25,		// %
	CCVK_QUOTATION,		0x22,		// "

#if 0 // sent by Mac OS X UI code as key codes
	CCVK_ALT,			0,			// ALT
	CCVK_CTRL,			0,			// CTRL
	CCVK_CAPS,			0.			// CAPS lock
#endif
	
	0,					0			// Terminate list
} ;

/********************************************************************/

unsigned int keyTranslateSystemToCCVK(unsigned int uiSysKey)
{
	int	iIndex;
	
	for (iIndex=0; gk_TranslateToCCVK[iIndex] != 0; iIndex += 2)
	{
		if ( gk_TranslateToCCVK[iIndex+1] == uiSysKey )
		{
			return gk_TranslateToCCVK[iIndex];
		}
	}
	
	//XTRACE("Key not translated : %d 0x(%04X)\n",uiSysKey,uiSysKey);
	
	// untranslated key
	return 0;
}

/********************************************************************/

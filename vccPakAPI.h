#ifndef __VCCPAKAPI_H__
#define __VCCPAKAPI_H__

/****************************************************************************/
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
/****************************************************************************/
/*
	VCC Pak API definition used by all plug-in Pak modules
*/
/****************************************************************************/

/*
	Module/Pak status flags
*/
#define HASCONFIG		(1 << 0)
#define HASIOWRITE		(1 << 1)
#define HASIOREAD		(1 << 2)
#define NEEDSCPUIRQ		(1 << 3)
#define DOESDMA			(1 << 4)
#define NEEDHEARTBEAT	(1 << 5)
#define ANALOGAUDIO		(1 << 6)
#define CSWRITE			(1 << 7)
#define CSREAD			(1 << 8)
#define RETURNSSTATUS	(1 << 9)
#define CARTRESET		(1 << 10)
#define SAVESINI		(1 << 11)
#define ASSERTCART		(1 << 12)

/****************************************************************************/

/*
 Forward definitions
*/

typedef struct vccpakapi_t vccpakapi_t;
typedef struct vccpak_t vccpak_t;

/*
 Function definitions needed by the Pak API
*/
typedef void(*vccapi_setcart_t)(unsigned char);
typedef void(*vccapi_dynamicmenucallback_t)(char *, int, int);

/*
function definitions for reading/writing memory
and i/o ports
*/
typedef unsigned char(*vccpakapi_portread_t)(unsigned char);
typedef void(*vccpakapi_portwrite_t)(unsigned char, unsigned char);
typedef unsigned char(*vcccpu_read8_t)(unsigned short);
typedef void(*vcccpu_write8_t)(unsigned char, unsigned short);

/*
	API version
*/
#define VCC_PAKAPI_VERSION	2
#define VCC_PAKAPI_REVISION	0

/*
	Function definitions for plug-in API
*/
//typedef vccpak_t * (*INITIALIZE)(void);
typedef void(*vccpakapi_getname_t)(char * modName, char * catNumber, vccapi_dynamicmenucallback_t , void * wndHandle);
typedef void (*vccpakapi_config_t)(unsigned char);
typedef void (*vccpakapi_heartbeat_t) (void);
typedef void (*vccpakapi_assertinterrupt_t) (unsigned char, unsigned char);
typedef void (*vccpakapi_status_t)(char *);
typedef void (*vccpakapi_setmemptrs_t) (vcccpu_read8_t, vcccpu_write8_t);
typedef void (*vccpakapi_setcartptr_t)(vccapi_setcart_t);
typedef void (*vccpakapi_setintptr_t) (vccpakapi_assertinterrupt_t);
typedef unsigned short (*vccpakapi_getaudiosample_t)(void);
typedef void (*vccpakapi_reset_t)(void);
typedef void (*vccpakapi_setinipath_t)(char *);

/*
	VCCPak API function names

	VCC will query the loaded DLL for these function names

	Names must match definitions below
*/
//#define VCC_PAKAPI_INITIALIZE		"vccPakAPIInit"		// INITIALIZE

#define VCC_PAKAPI_MEMREAD			"vccPakAPIMemRead8"		// vcccpu_read8_t
#define VCC_PAKAPI_DEF_MEMREAD		vccPakAPIMemRead8

#define VCC_PAKAPI_MEMWRITE			"vccPakAPIMemWrite8"		// vcccpu_write8_t
#define VCC_PAKAPI_DEF_MEMWRITE		vccPakAPIMemWrite8

#define VCC_PAKAPI_PORTREAD			"vccPakAPIPortRead"		// vccpakapi_portread_t
#define VCC_PAKAPI_DEF_PORTREAD		vccPakAPIPortRead

#define VCC_PAKAPI_PORTWRITE		"vccPakAPIPortWrite"		// vccpakapi_portwrite_t
#define VCC_PAKAPI_DEF_PORTWRITE	vccPakAPIPortWrite

#define VCC_PAKAPI_MEMPOINTERS		"vccPakAPISetMemPointers"		// vccpakapi_setmemptrs_t
#define VCC_PAKAPI_DEF_MEMPOINTERS	vccPakAPISetMemPointers

#define VCC_PAKAPI_SETCART			"vccPakAPISetCart"			// vccpakapi_setintptr_t
#define VCC_PAKAPI_DEF_SETCART		vccPakAPISetCart

#define VCC_PAKAPI_GETNAME			"vccPakAPIGetModuleName"	// vccpakapi_getname_t
#define VCC_PAKAPI_DEF_GETNAME		vccPakAPIGetModuleName

#define VCC_PAKAPI_CONFIG			"vccPakAPIConfig"		// vccpakapi_config_t
#define VCC_PAKAPI_DEF_CONFIG		vccPakAPIConfig

#define VCC_PAKAPI_SETINIPATH		"vccPakAPISetIniPath"		// vccpakapi_setinipath_t
#define VCC_PAKAPI_DEF_SETINIPATH	vccPakAPISetIniPath

#define VCC_PAKAPI_ASSERTINTERRUPT		"vccPakAPIAssertInterupt"	// vccpakapi_assertinterrupt_t
#define VCC_PAKAPI_DEF_ASSERTINTERRUPT	vccPakAPIAssertInterupt

#define VCC_PAKAPI_HEARTBEAT		"vccPakAPIHeartBeat"			// vccpakapi_heartbeat_t
#define VCC_PAKAPI_DEF_HEARTBEAT	vccPakAPIHeartBeat

#define VCC_PAKAPI_STATUS			"vccPakAPIStatus"		// vccpakapi_status_t
#define VCC_PAKAPI_DEF_STATUS		vccPakAPIStatus

#define VCC_PAKAPI_AUDIOSAMPLE		"vccPakAPIAudioSample"	// vccpakapi_getaudiosample_t
#define VCC_PAKAPI_DEF_AUDIOSAMPLE	vccPakAPIAudioSample

#define VCC_PAKAPI_RESET			"vccPakAPIReset"		// vccpakapi_reset_t
#define VCC_PAKAPI_DEF_RESET		vccPakAPIReset

/****************************************************************************/

/**
	VCC Pak API

	Functions implemented by the Pak called by the emulator
*/
struct vccpakapi_t
{
	//
	// VCC callbacks
	//
	//vccpakapi_assertinterrupt_t			vccAssertInt;
	//vccapi_dynamicmenucallback_t		vccDynamicMenuCallback;
	//vcccpu_read8_t				vccMemRead8;
	//vcccpu_write8_t				vccMemWrite8;

	//
	// Functions defined by Pak 
	//

	// interface for Pak reading/writing memory or i/o port
	vccpakapi_portread_t			portRead;
	vccpakapi_portwrite_t			portWrite;
	vcccpu_read8_t				memRead;
	vcccpu_write8_t				memWrite;

	//INITIALIZE				init;
	vccapi_dynamicmenucallback_t		dynMenu;
	vccpakapi_getname_t		getName;
	vccpakapi_config_t				config;
	vccpakapi_heartbeat_t				heartbeat;
	vccpakapi_assertinterrupt_t			assertInterrupt;
	vccpakapi_status_t			status;
	vccpakapi_setmemptrs_t			memPointers;
	vccpakapi_setcartptr_t			setCartPtr;
	vccpakapi_setintptr_t	setInterruptCallPtr;
	vccpakapi_getaudiosample_t		getSample;
	vccpakapi_reset_t				reset;
	vccpakapi_setinipath_t				setINIPath;
};

/**
	VCC Pak object
*/
struct vccpak_t
{
	vccpakapi_t		api;
};

/****************************************************************************/
//
// Pak load/insertion error codes
//
#define NOMODULE	1
#define NOTVCC	2

//
// dynamic menu definitions
//
#define	HEAD 0
#define SLAVE 1
#define STANDALONE 2

/****************************************************************************/

#endif // __VCCPAKAPI_H__



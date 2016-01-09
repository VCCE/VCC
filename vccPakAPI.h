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
typedef void(*SETCART)(unsigned char);

/*
function definitions for reading/writing memory
and i/o ports
*/
typedef unsigned char(*PACKPORTREAD)(unsigned char);
typedef void(*PACKPORTWRITE)(unsigned char, unsigned char);
typedef unsigned char(*MEMREAD8)(unsigned short);
typedef void(*MEMWRITE8)(unsigned char, unsigned short);

/*
	API version
*/
#define VCC_PAKAPI_VERSION	2
#define VCC_PAKAPI_REVISION	0

/*
	Function definitions for plug-in API
*/
typedef vccpak_t * (*INITIALIZE)(void);
typedef void (*DYNAMICMENUCALLBACK)(char *, int, int);
typedef void (*GETNAME)(char *, char *, DYNAMICMENUCALLBACK);
typedef void (*CONFIGIT)(unsigned char);
typedef void (*HEARTBEAT) (void);
typedef void (*ASSERTINTERUPT) (unsigned char, unsigned char);
typedef void (*MODULESTATUS)(char *);
typedef void (*DMAMEMPOINTERS) (MEMREAD8, MEMWRITE8);
typedef void (*SETCARTPOINTER)(SETCART);
typedef void (*SETINTERUPTCALLPOINTER) (ASSERTINTERUPT);
typedef unsigned short (*MODULEAUDIOSAMPLE)(void);
typedef void (*MODULERESET)(void);
typedef void (*SETINIPATH)(char *);

/*
	VCCPak API function names

	VCC will query the loaded DLL for these function names
*/
#define VCC_PAKAPI_INITIALIZE		"vccPakInit"		// INITIALIZE

#define VCC_PAKAPI_MEMREAD			"PakMemRead8"		// MEMREAD8
#define VCC_PAKAPI_MEMWRITE			"PakMemWrite8"		// MEMWRITE8

#define VCC_PAKAPI_PORTREAD			"PackPortRead"		// PACKPORTREAD
#define VCC_PAKAPI_PORTWRITE		"PackPortWrite"		// PACKPORTWRITE

#define VCC_PAKAPI_MEMPOINTERS		"MemPointers"		// DMAMEMPOINTERS
#define VCC_PAKAPI_SETCART			"SetCart"			// SETINTERUPTCALLPOINTER

#define VCC_PAKAPI_GETNAME			"ModuleName"		// GETNAME
#define VCC_PAKAPI_CONFIG			"ModuleConfig"		// CONFIGIT
#define VCC_PAKAPI_SETINIPATH		"SetIniPath"		// SETINIPATH
#define VCC_PAKAPI_ASSERTINTERRUPT	"AssertInterupt"	// ASSERTINTERUPT
#define VCC_PAKAPI_HEARTBEAT		"HeartBeat"			// HEARTBEAT
#define VCC_PAKAPI_STATUS			"ModuleStatus"		// MODULESTATUS
#define VCC_PAKAPI_AUDIOSAMPLE		"ModuleAudioSample"	// MODULEAUDIOSAMPLE
#define VCC_PAKAPI_RESET			"ModuleReset"		// MODULERESET

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
	//ASSERTINTERUPT			vccAssertInt;
	//DYNAMICMENUCALLBACK		vccDynamicMenuCallback;
	//MEMREAD8				vccMemRead8;
	//MEMWRITE8				vccMemWrite8;

	//
	// Functions defined by Pak 
	//

	// interface for Pak reading/writing memory or i/o port
	PACKPORTREAD			portRead;
	PACKPORTWRITE			portWrite;
	MEMREAD8				memRead;
	MEMWRITE8				memWrite;

	INITIALIZE				init;
	DYNAMICMENUCALLBACK		dynMenu;
	GETNAME					getName;
	CONFIGIT				config;
	HEARTBEAT				heartbeat;
	ASSERTINTERUPT			assertInterrupt;
	MODULESTATUS			status;
	DMAMEMPOINTERS			memPointers;
	SETCARTPOINTER			setCartPtr;
	SETINTERUPTCALLPOINTER	setInterruptCallPtr;
	MODULEAUDIOSAMPLE		getSample;
	MODULERESET				reset;
	SETINIPATH				setINIPath;
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



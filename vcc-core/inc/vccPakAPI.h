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
	Defines the VCC plug-in Pak API
*/
/****************************************************************************/
/*
	Revision history:

	2016-01-12 - @Wersley - moved the Pak API into common code and 
					removed redundant definitions.
*/
/****************************************************************************/

#include "vccPakDynMenu.h"

#if (defined VCCPAKAPI_EXPORTS)
#	if (defined WIN32)
#		define VCCPAK_API __declspec(dllexport)
#	elif (defined OSX)
#		define VCCPAK_API __attribute__((visibility("public")))
#	else
#		error "platformed undefined for library build"
#	endif
#else
#	define VCCPAK_API
#endif

/****************************************************************************/

#define VCC_RESET_PENDING_NONE			0
#define VCC_RESET_PENDING_SOFT			1
#define VCC_RESET_PENDING_HARD			2
#define VCC_RESET_PENDING_CLEAR			3
#define VCC_RESET_PENDING_UPDATECONFIG	4

/****************************************************************************/

/*
	API version

	Needs to be returned 
*/
#define VCC_PAKAPI_VERSION_MAJOR		2	///< Current VCC Pak API Version (major)
#define VCC_PAKAPI_VERSION_MINOR		0	///< Current VCC Pak API Version (minor)
#define VCC_PAKAPI_VERSION_REVISION		0	///< Current VCC Pak API Revision

/**
	VCC Pak API versioning information

	Returned within the created Pak to verify it is compatible
	with the running version of VCC.

	Versioning rules:
	TODO: review

	Major change	- If VCC version major is higher, the Pak cannot be used (eg. a new required function was added or the parameters have changed)
	Minor change	- If VCC version minor is higher, the Pak can still be used (eg. an optional function was added)
	Revision change - Minor change with no effect on compatibility
*/
typedef struct vccpakapiversion_t
{
	int major;
	int minor;
	int revision;
} vccpakapiversion_t;

/*
	Forward definitions
*/

/** VCC plug-in program Pak object */
typedef struct vccpak_t vccpak_t;
/** VCC plug-in Pak API call container */
typedef struct vccpakapi_t vccpakapi_t;

/*
	Function definitions needed by the Pak API

	prototype definitions for functions defined in VCC passed
	into each Pak
*/
/** VCC SetCart function. The Pak calls this to enable/disable the CART interrupt */
typedef void (*vccapi_setcart_t)(unsigned char);
/** VCC DynamicCallback function. The Pak calls this to add items to the Cartridge menu */
typedef void (*vccapi_dynamicmenucallback_t)(int, const char *, int, dynmenutype_t);
/** VCC AssertInterrupt function.  The Pak uses this to initiate an interrupt */
typedef void (*vccpakapi_assertinterrupt_t)(unsigned char, unsigned char);

/*
	function definitions for reading/writing memory
	and i/o ports.  
*/
/** I/O port read function */
typedef unsigned char (*vccpakapi_portread_t)(unsigned char);
/** I/O port write function */
typedef void (*vccpakapi_portwrite_t)(unsigned char, unsigned char);
/** Memory read (a byte) function */
typedef unsigned char (*vcccpu_read8_t)(unsigned short);
/** Memory write (a byte) function */
typedef void (*vcccpu_write8_t)(unsigned char, unsigned short);

/*
	Function definitions for plug-in API
*/
/** Initialize an instance of the Pak. (required) */
typedef void (*vccpakapi_init_t)(int id, void * wndHandle, vccapi_dynamicmenucallback_t dynMenuCallback);
/** Destroy the Pak. (required) */
typedef void (*vccpakapi_destroy_t)(vccpak_t * pPak);

/** Pass in VCC memory read and write functions (optional) */
typedef void (*vccpakapi_setmemptrs_t) (vcccpu_read8_t, vcccpu_write8_t);
/** Pass in VCC SetCart function (optional) */
typedef void(*vccpakapi_setcartptr_t)(vccapi_setcart_t);
/** Pass in VCC AssertInterrupt function (optional) */
typedef void (*vccpakapi_setintptr_t) (vccpakapi_assertinterrupt_t);

/** Get module name. (required) */
typedef void (*vccpakapi_getname_t)(char * modName, char * catNumber);
/** Request Pak to rebuild its dynamic menu (optional) */
typedef void (*vccpakapi_dynmenubuild_t)(void);
/** Provide the INI file path for this module to use (optional) */
typedef void (*vccpakapi_setinipath_t)(char *);
/** Call Pak specific configuration UI (optional) */
typedef void (*vccpakapi_config_t)(int);
/** Periodic call to Pak from emulator.  TODO: document frequency here (optional) */
typedef void (*vccpakapi_heartbeat_t)(void);
/** Get Pak status (optional) */
typedef void (*vccpakapi_status_t)(char * buffer, size_t bufferSize);
/** Get audio sample from Pak (optional) */
typedef unsigned short (*vccpakapi_getaudiosample_t)(void);
/** Reset Pak (optional) */
typedef void (*vccpakapi_reset_t)(void);

/*
	VCCPak API function names

	VCC will query the loaded DLL for these function names

	TODO: do this a better way so each does not have to be specified twice
*/
#define VCC_PAKAPI_INIT				"vccPakAPIInit"			// vccpakapi_init_t
#define VCC_PAKAPI_DEF_INIT			vccPakAPIInit

#define VCC_PAKAPI_MEMREAD			"vccPakAPIMemRead8"		// vcccpu_read8_t
#define VCC_PAKAPI_DEF_MEMREAD		vccPakAPIMemRead8

#define VCC_PAKAPI_MEMWRITE			"vccPakAPIMemWrite8"	// vcccpu_write8_t
#define VCC_PAKAPI_DEF_MEMWRITE		vccPakAPIMemWrite8

#define VCC_PAKAPI_PORTREAD			"vccPakAPIPortRead"		// vccpakapi_portread_t
#define VCC_PAKAPI_DEF_PORTREAD		vccPakAPIPortRead

#define VCC_PAKAPI_PORTWRITE		"vccPakAPIPortWrite"	// vccpakapi_portwrite_t
#define VCC_PAKAPI_DEF_PORTWRITE	vccPakAPIPortWrite

#define VCC_PAKAPI_MEMPOINTERS		"vccPakAPISetMemPointers"	// vccpakapi_setmemptrs_t
#define VCC_PAKAPI_DEF_MEMPOINTERS	vccPakAPISetMemPointers

#define VCC_PAKAPI_SETCART			"vccPakAPISetCart"		// vccpakapi_setcartptr_t
#define VCC_PAKAPI_DEF_SETCART		vccPakAPISetCart

#define VCC_PAKAPI_GETNAME			"vccPakAPIGetModuleName"// vccpakapi_getname_t
#define VCC_PAKAPI_DEF_GETNAME		vccPakAPIGetModuleName

#define VCC_PAKAPI_CONFIG			"vccPakAPIConfig"		// vccpakapi_config_t
#define VCC_PAKAPI_DEF_CONFIG		vccPakAPIConfig

#define VCC_PAKAPI_SETINIPATH		"vccPakAPISetIniPath"	// vccpakapi_setinipath_t
#define VCC_PAKAPI_DEF_SETINIPATH	vccPakAPISetIniPath

#define VCC_PAKAPI_ASSERTINTERRUPT		"vccPakAPIAssertInterupt"	// vccpakapi_assertinterrupt_t
#define VCC_PAKAPI_DEF_ASSERTINTERRUPT	vccPakAPIAssertInterupt

#define VCC_PAKAPI_HEARTBEAT		"vccPakAPIHeartBeat"	// vccpakapi_heartbeat_t
#define VCC_PAKAPI_DEF_HEARTBEAT	vccPakAPIHeartBeat

#define VCC_PAKAPI_STATUS			"vccPakAPIStatus"		// vccpakapi_status_t
#define VCC_PAKAPI_DEF_STATUS		vccPakAPIStatus

#define VCC_PAKAPI_AUDIOSAMPLE		"vccPakAPIAudioSample"	// vccpakapi_getaudiosample_t
#define VCC_PAKAPI_DEF_AUDIOSAMPLE	vccPakAPIAudioSample

#define VCC_PAKAPI_RESET			"vccPakAPIReset"		// vccpakapi_reset_t
#define VCC_PAKAPI_DEF_RESET		vccPakAPIReset

#define VCC_PAKAPI_DYNMENUBUILD		"vccPakAPIDynMenuBuild"		// vccpakapi_dynmenubuild_t
#define VCC_PAKAPI_DEF_DYNMENUBUILD	vccPakAPIDynMenuBuild

/****************************************************************************/

/**
	VCC Pak API

	Functions implemented by the Pak called by the emulator

	TODO: VCC Pak API call to get module/pak status shoud be changed to
	use safe versions of sprintf, strcat and so should each Pak
	(including us).  The buffer size should be passed into the
	call to us as well.
	*/
struct vccpakapi_t
{
	//
	// Functions defined by Pak VCC will call/invoke
	//
	vccpakapi_init_t					init;
	vccpakapi_getname_t					getName;
	vccpakapi_config_t					config;
	vccpakapi_heartbeat_t				heartbeat;
	vccpakapi_status_t					status;
	vccpakapi_getaudiosample_t			getAudioSample;
	vccpakapi_reset_t					reset;
	vccpakapi_dynmenubuild_t			dynMenuBuild;

	// interface for Pak reading/writing memory or i/o port
	// will be removed once the vcc-core in in a library
	vccpakapi_portread_t				portRead;
	vccpakapi_portwrite_t				portWrite;
	vcccpu_read8_t						memRead;
	vcccpu_write8_t						memWrite;

	// will be removed once the vcc-core in in a library
	vccpakapi_setmemptrs_t				memPointers;
	vccpakapi_setcartptr_t				setCartPtr;
	vccpakapi_setintptr_t				setInterruptCallPtr;
	vccpakapi_assertinterrupt_t			assertInterrupt;
	vccpakapi_setinipath_t				setINIPath;
	vccapi_dynamicmenucallback_t		dynMenuCallback;
};

/****************************************************************************/

#endif // __VCCPAKAPI_H__



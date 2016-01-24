#ifndef __VCCPAK_H__
#define __VCCPAK_H__

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

#include "vccPakAPI.h"

#include <stdio.h>	// for FILENAME_MAX

/****************************************************************************/

/*
	Pak status flags

	These are set from scanning the exported functions available
	in the plugin's dynamic library
*/
#define VCCPAK_HASCONFIG		(1 << 0)
#define VCCPAK_HASIOWRITE		(1 << 1)
#define VCCPAK_HASIOREAD		(1 << 2)
#define VCCPAK_NEEDSCPUIRQ		(1 << 3)
#define VCCPAK_DOESDMA			(1 << 4)
#define VCCPAK_NEEDHEARTBEAT	(1 << 5)
#define VCCPAK_ANALOGAUDIO		(1 << 6)
#define VCCPAK_CSWRITE			(1 << 7)
#define VCCPAK_CSREAD			(1 << 8)
#define VCCPAK_RETURNSSTATUS	(1 << 9)
#define VCCPAK_CARTRESET		(1 << 10)
#define VCCPAK_SAVESINI			(1 << 11)
#define VCCPAK_ASSERTCART		(1 << 12)

/****************************************************************************/
/*
	Pak load/insertion error codes
*/

#define VCCPAK_LOADED		0
#define VCCPAK_ERROR		-1
#define VCCPAK_NOMODULE		1
#define VCCPAK_NOTVCC		2

/****************************************************************************/
/*
	Pak type codes
*/

typedef enum vccpaktype_t
{
	VCCPAK_TYPE_UNKNOWN = 0,
	VCCPAK_TYPE_NOFILE,
	VCCPAK_TYPE_PLUGIN,
	VCCPAK_TYPE_ROM
} vccpaktype_t;

/****************************************************************************/
/**
	VCC Plug-in Pak object

	Encapsulates an external Program Pak ROM or Plug-in DLL
*/
typedef struct vccpak_t vccpak_t;
/**
	VCC Plug-in Pak object
*/
struct vccpak_t
{
	// note: for now, if you change the order of any of these make sure
	// to update where the global Pak is initialized in pakinterface.c

	void *				hDLib;						///< System dynamic library handle
	char				path[FILENAME_MAX];			///< Path of loaded Pak
	char				name[FILENAME_MAX];			///< Name of Pak for display
	char				catnumber[MAX_LOADSTRING];	///< Catalog number
	char				label[FILENAME_MAX];		///< ???

	// info for plug-in DLLs
	int					params;						///< Pak Status Flags indicating what this Pak supports via the Pak API
	vccpakapi_t			api;						///< functions defined by the plug-in DLL (empty if ROM is loaded)

	// Storage for Pak ROMs
	unsigned char *		ExternalRomBuffer;			///< 
	int					RomPackLoaded;				///< 
	unsigned int		BankedCartOffset;			///< 

	vccpakapiversion_t	version;					///< VCC Pak API supported by this Pak
} ;

/****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
		
	//
	// VCC Pak API calls to interface with the Pak
	//
	VCCCORE_API void			vccPakTimer(void);
	VCCCORE_API unsigned char	vccPakPortRead(unsigned char);
	VCCCORE_API void			vccPakPortWrite(unsigned char, unsigned char);
	VCCCORE_API unsigned char	vccPakMem8Read(unsigned short);
	VCCCORE_API void			vccPakMem8Write(unsigned char, unsigned char);
	VCCCORE_API void			vccPakGetStatus(char *, size_t);
	VCCCORE_API void			vccPakReset(void);
	VCCCORE_API unsigned short	vccPackGetAudioSample(void);
	VCCCORE_API void			vccPakSetInterruptCallPtr(vccpakapi_assertinterrupt_t CPUAssertInterupt);
	
	VCCCORE_API int				vccPakWantsCart(vccpak_t * pPak);

	//
	// General Pak support functions
	//
	VCCCORE_API vccpaktype_t	vccPakGetType(const char * Filename);
	VCCCORE_API char *			vccPakGetLastPath();
	VCCCORE_API void			vccPakSetLastPath(char *);

	VCCCORE_API int				vccPakLoadCart(void);
	VCCCORE_API int				vccPakLoadExtROM(char *);
	VCCCORE_API void			vccPakGetCurrentModule(char *);
	VCCCORE_API int				vccPakInsertModule(char *);
	VCCCORE_API void			vccPakUnloadDll(void);
	VCCCORE_API void			vccPakUnload(void);

	VCCCORE_API void			vccPakRebuildMenu();
	VCCCORE_API void			vccPakDynMenuActivated(int);
	VCCCORE_API void			vccPakDynMenuRefresh(void);
	VCCCORE_API void			vccPakDynMenuCallback(int, const char *, int, dynmenutype_t);

#ifdef __cplusplus
}
#endif

/****************************************************************************/

#endif // __VCCPAK_H__


#ifndef __PAKINTERFACE_H__
#define __PAKINTERFACE_H__

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

#include <Windows.h>

/****************************************************************************/

/*
	Pak status flags
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
/**
	VCC Plug-in Pak object

	ROM or Plug-in DLL
*/
typedef struct vccpak_t vccpak_t;
struct vccpak_t
{
	void *			hDLib;			///< System dynamic library handle
	char			path[MAX_PATH];	///< Path of loaded Pak
	char			name[MAX_PATH];	///< Name of Pak for display

	int				params;			///< Pak Status Flags indicating what this Pak supports via the Pak API
	vccpakapi_t		api;			///< functions defined by the plug-in DLL (empty if ROM is loaded)
};

/****************************************************************************/

#ifndef __cplusplus
extern "C"
{
#endif

	void			vccPakTimer(void);
	unsigned char	vccPakPortRead(unsigned char);
	void			vccPakPortWrite(unsigned char, unsigned char);
	unsigned char	vccPakMem8Read(unsigned short);
	void			vccPakMem8Write(unsigned char, unsigned char);
	void			vccPakGetStatus(char *);
	void			vccPakReset(void);
	unsigned short	vccPackGetAudioSample(void);
	void			vccPakSetInterruptCallPtr(void);
	
	char *			vccPakGetLastPath();
	void			vccPakSetLastPath(char *);

	int				LoadCart(void);
	int				load_ext_rom(char *);
	void			GetCurrentModule(char *);
	int				InsertModule(char *);
	void			UnloadDll(void);
	void			UnloadPack(void);
	void			DynamicMenuActivated(unsigned char);
	void			RefreshDynamicMenu(void);

#ifndef __cplusplus
}
#endif

/****************************************************************************/

//
// Defines the start and end IDs for the dynamic menus
//
#define ID_SDYNAMENU 5000	
#define ID_EDYNAMENU 5100

/****************************************************************************/

#endif // __PAKINTERFACE_H__


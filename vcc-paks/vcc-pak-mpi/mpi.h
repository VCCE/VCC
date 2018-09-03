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

#ifndef _mpi_h_
#define _mpi_h_

/*****************************************************************************/

#include "pakinterface.h"

/*****************************************************************************/

#define MPI_DEFAULT_NUM_SLOTS       4

#define MPI_COMMAND_NONE            0
#define MPI_COMMAND_CONFIG          1

#define MPI_COMMAND_PAK_LOAD        10

#define MPI_COMMAND_PAK_EJECT       20

/*****************************************************************************/

#define MPI_CONF_SECTION            "mpi"

#define MPI_CONF_NUMSLOTS           "numSlots"
#define MPI_CONF_SWITCHSLOT         "switchSlot"
#define MPI_CONF_PAKPATH            "pakpath_slot"  // actual format 'pakpath_slot_#'

/*****************************************************************************/

// TODO: move to private header
typedef struct mpi_t mpi_t;
struct mpi_t
{
	cocopak_t		pak;				// base
	
    int				confNumSlots;       // config: number of MPI slots
    int             confSwitchSlot;     // config: switch slot
    
    int             numSlots;           // number of MPI slots (current)
	cocopak_t **	slots;				// the MPI slot cartridges
    int             switchSlot;         // Switch slot selected

	int	            chipSelectSlot;     // CTS
	int	            spareSelectSlot;    // SCS
	unsigned char	slotRegister;       // MPI slot register
} ;

/*****************************************************************************/

#define VCC_PAK_MPI_ID	XFOURCC('c','m','p','i')

#if DEBUG
    #define ASSERT_MPI(mpi)     assert(mpi != NULL);    \
                                assert(mpi->pak.device.id == EMU_DEVICE_ID); \
                                assert(mpi->pak.device.idModule == VCC_COCOPAK_ID); \
                                assert(mpi->pak.device.idDevice == VCC_PAK_MPI_ID)
#else
    #define ASSERT_MPI(mpi)
#endif

#define CAST_MPI(mpi)          (mpi_t *)mpi;

/*****************************************************************************/

#if (defined __cplusplus)
extern "C"
{
#endif

	XAPI_EXPORT cocopak_t * vccpakModuleCreate(void);

#if (defined __cplusplus)
}
#endif

/*****************************************************************************/

#endif

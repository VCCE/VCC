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

#ifndef _mc6821_h_
#define _mc6821_h_

/*****************************************************************************/

#include "cpu.h"
#include "emuDevice.h"

/*****************************************************************************/

#define VCC_MC6821_ID	XFOURCC('6','8','2','1')

#define ASSERT_MC6821(pInstance)	assert(pInstance != NULL);	\
                                    assert(pInstance->device.id == EMU_DEVICE_ID); \
                                    assert(pInstance->device.idModule == VCC_MC6821_ID)

/*****************************************************************************/

typedef enum hsphase_e
{
    FALLING = 0,
    RISING = 1,
    ANY = 2
} hsphase_e;


typedef enum fsphase_e
{
    HighToLow = 0,
    LowToHigh
} fsphase_e;


/*****************************************************************************/
/*****************************************************************************/

typedef struct mc6821_t mc6821_t;
struct mc6821_t
{
	emudevice_t			device;

    unsigned char		rega[4];    // side A control register
	unsigned char		rega_dd[4]; // side A data
	
	unsigned char		regb[4];    // side B control register
	unsigned char		regb_dd[4]; // side B data

    signal_e            CART;       // CART signal state
    
	unsigned char		VDG_gmode;
	
	unsigned char		Asample;
	unsigned char		Ssample;
	unsigned char		Csample;
	
    unsigned short      LastLeft;
    unsigned short      LastRight;
} ;

/*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
	
    mc6821_t *		mc6821Create(void);

    void            mc6821_Reset(mc6821_t *);

    void			mc6821_IRQ_HS(mc6821_t *,cpu_t * pCPU, hsphase_e phase);
	void			mc6821_IRQ_FS(mc6821_t *,cpu_t * pCPU, fsphase_e phase);
    void            mc6821_IRQ_CART(mc6821_t * pMC6821, cpu_t * pCPU);

    void            mc6821_SetCART(mc6821_t *, signal_e);
    
	unsigned char	mc6821_GetMuxState(mc6821_t *);
	unsigned char	mc6821_DACState(mc6821_t *);
	unsigned int	mc6821_GetDACSample(mc6821_t *);
	unsigned char	mc6821_GetCasSample(mc6821_t *);
	void			mc6821_SetCasSample(mc6821_t *, unsigned char);

	unsigned char	mc6821_pia0Read(emudevice_t * pEmuDevice, unsigned char port);
	void			mc6821_pia0Write(emudevice_t * pEmuDevice, unsigned char port, unsigned char data);
	unsigned char	mc6821_pia1Read(emudevice_t * pEmuDevice, unsigned char port);
	void			mc6821_pia1Write(emudevice_t * pEmuDevice, unsigned char port, unsigned char data);
	
#ifdef __cplusplus
}
#endif
		
/*****************************************************************************/

#endif

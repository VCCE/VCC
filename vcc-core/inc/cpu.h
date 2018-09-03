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

#ifndef _cpu_h_
#define _cpu_h_

/*****************************************************************************/

#include "xTypes.h"
#include "emuDevice.h"

/*****************************************************************************/
/*
	interrupt types
 */
typedef enum irqtype_e
{
    IRQ     = (1 << 0),
    FIRQ    = (1 << 1),
    NMI     = (1 << 2)
} irqtype_e;

/*****************************************************************************/

/*
    forward declaration of CPU
 */
typedef struct _cpu_t_ cpu_t;

/*****************************************************************************/

/*
    CPU plug-in API
*/
typedef void (*cpuinit_t)(cpu_t * pCPU);
typedef int  (*cpuexec_t)(cpu_t * pCPU, int runCycles);
typedef void (*cpureset_t)(cpu_t * pCPU);
typedef void (*cpuassertinterrupt_t)(cpu_t * pCPU, irqtype_e Interrupt, int waiter);
typedef void (*cpudeassertinterrupt_t)(cpu_t * pCPU, irqtype_e Interrupt);
typedef void (*cpuforcepc_t)(cpu_t * pCPU, int pc);
typedef void (*cpudestructor_t)(cpu_t * pCPU);

/*
	MMU plug-in read/write API
 */
typedef void	(*memwrite8fn_t)(emudevice_t * pEmuDevice, uint8_t data, int address);
typedef uint8_t	(*memread8fn_t)(emudevice_t * pEmuDevice, int address);

/*****************************************************************************/
/**
    Basic MMU definition
*/
typedef struct _mmu_t_ mmu_t;
struct _mmu_t_
{
    emudevice_t         device;

    memread8fn_t        pMemRead8;
    memwrite8fn_t       pMemWrite8;
};

/*****************************************************************************/

#define EMU_CPU_ID	XFOURCC('e','c','p','u')

#define ASSERT_CPU(pCPU)	assert(pCPU != NULL);	\
                            assert(pCPU->device.id == EMU_DEVICE_ID); \
                            assert(pCPU->device.idModule == EMU_CPU_ID)



#define EMU_MMU_ID  XFOURCC('e','m','m','u')

#define ASSERT_MMU(mmu)	    assert(mmu != NULL);	\
                            assert(mmu->device.id == EMU_DEVICE_ID); \
                            assert(mmu->device.idModule == EMU_MMU_ID)

/*****************************************************************************/

/*
	generic cpu definition
 
    TODO: rename function pointers
 */
struct _cpu_t_
{
	emudevice_t				device;

    mmu_t *                 mmu;
    
    int                     CycleCounter;
    
	cpuinit_t				cpuInit;
	cpuexec_t				cpuExec;
	cpureset_t				cpuReset;
	cpuassertinterrupt_t	cpuAssertInterrupt;
	cpudeassertinterrupt_t	cpuDeAssertInterrupt;
	cpuforcepc_t			cpuForcePC;
	cpudestructor_t			cpuDestructor;
} ;

/*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
	
    XAPI void       cpuInit(cpu_t * pCPU);
	XAPI int        cpuExec(cpu_t * pCPU, int cycles);
	XAPI void       cpuReset(cpu_t * pCPU);
	XAPI void       cpuAssertInterrupt(cpu_t * pCPU, irqtype_e Interrupt, int waiter);
	XAPI void       cpuDeassertInterrupt(cpu_t * pCPU, irqtype_e Interrupt);
    void            cpuForcePC(cpu_t * pCPU, unsigned short pc);
    
    XAPI void       cpuDestroy(cpu_t * pCPU);
	
	XAPI uint8_t	cpuMemRead8(cpu_t * pCPU, int addr);
	XAPI void		cpuMemWrite8(cpu_t * pCPU, uint8_t data, int addr);
	
	XAPI uint16_t	cpuMemRead16(cpu_t * pCPU, int addr);
	XAPI void		cpuMemWrite16(cpu_t * pCPU, uint16_t data, int addr);
	
	XAPI uint32_t	cpuMemRead32(cpu_t * pCPU, int addr);
	XAPI void		cpuMemWrite32(cpu_t * pCPU, uint32_t data, int addr);

#ifdef __cplusplus
}
#endif

/*****************************************************************************/

#endif // _cpu_h_



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

/********************************************************************/
/*
    cpu.c
 */
/********************************************************************/

#include "cpu.h"

#include "xDebug.h"

/********************************************************************/

uint8_t cpuMemRead8(cpu_t * pCPU, int addr)
{
	ASSERT_CPU(pCPU);
	
	return (*pCPU->mmu->pMemRead8)(&pCPU->mmu->device,addr);
}

/********************************************************************/

void cpuMemWrite8(cpu_t * pCPU, uint8_t data, int addr)
{
    ASSERT_CPU(pCPU);
	
	(*pCPU->mmu->pMemWrite8)(&pCPU->mmu->device,data,addr);
}

/********************************************************************/

uint16_t cpuMemRead16(cpu_t * pCPU, int addr)
{
	uint16_t d;
	
    ASSERT_CPU(pCPU);
	
	d =   (   (*pCPU->mmu->pMemRead8)(&pCPU->mmu->device,addr) << 8
            | (*pCPU->mmu->pMemRead8)(&pCPU->mmu->device,addr + 1)
           );
	
	return d;
}

/********************************************************************/

void cpuMemWrite16(cpu_t * pCPU, uint16_t data, int addr)
{
    ASSERT_CPU(pCPU);

    assert(addr >= 0 && addr <= 0xFFFE);
    
	(*pCPU->mmu->pMemWrite8)(&pCPU->mmu->device, ((data >> 8) & 0xFF), addr);
	(*pCPU->mmu->pMemWrite8)(&pCPU->mmu->device, ((data >> 0) & 0xFF), addr + 1);
}

/**************************************************************************/

uint32_t cpuMemRead32(cpu_t * pCPU, int addr)
{
    ASSERT_CPU(pCPU);
	
	return ( (cpuMemRead16(pCPU,addr) << 16) | cpuMemRead16(pCPU,addr+2) );
	
}

/**************************************************************************/

void cpuMemWrite32(cpu_t * pCPU, uint32_t data, int addr)
{
    ASSERT_CPU(pCPU);
	
	cpuMemWrite16(pCPU, data >> 16, addr);
	cpuMemWrite16(pCPU, data & 0xFFFF, addr + 2);
}

/********************************************************************/

void cpuInit(cpu_t * pCPU)
{
    ASSERT_CPU(pCPU);
    
    if ( pCPU != NULL )
    {
        (pCPU)->cpuInit( (pCPU) );
    }
}

/********************************************************************/

int cpuExec(cpu_t * pCPU, int cycles)
{
    ASSERT_CPU(pCPU);
    
    if ( pCPU != NULL )
    {
        return pCPU->cpuExec(pCPU, cycles);
    }
    
    return 0;
}

/********************************************************************/

void cpuReset(cpu_t * pCPU)
{
    ASSERT_CPU(pCPU);
    
    if ( pCPU != NULL )
    {
    pCPU->cpuReset(pCPU);
    }
}

/********************************************************************/

void cpuAssertInterrupt(cpu_t * pCPU, irqtype_e Interrupt, int waiter)
{
    ASSERT_CPU(pCPU);
    
    if ( pCPU != NULL )
    {
        pCPU->cpuAssertInterrupt(pCPU, Interrupt, waiter);
    }
}

/********************************************************************/

void cpuDeassertInterrupt(cpu_t * pCPU, irqtype_e Interrupt)
{
    ASSERT_CPU(pCPU);
    
    if ( pCPU != NULL )
    {
        pCPU->cpuDeAssertInterrupt(pCPU, Interrupt);
    }
}

/********************************************************************/

void cpuForcePC(cpu_t * pCPU, unsigned short pc)
{
    ASSERT_CPU(pCPU);
    
    if ( pCPU != NULL )
    {
        pCPU->cpuForcePC(pCPU,pc);
    }
}

/********************************************************************/

void cpuDestroy(cpu_t * pCPU)
{
    ASSERT_CPU(pCPU);
    
	if ( pCPU != NULL )
	{
		if ( pCPU->cpuDestructor != NULL )
		{
			(*pCPU->cpuDestructor)(pCPU);
		}
		else 
		{
			free(pCPU);
		}
	}
}

/********************************************************************/


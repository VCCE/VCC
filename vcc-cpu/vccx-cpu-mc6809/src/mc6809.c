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

/*****************************************************************/
/**
	MC6809 CPU module
 
	Module is stand alone.  Create CPU object, then set memory
	read and write functions and it is ready to use.
 
	pCPU = cpuMC6809Create();
	pCPU->pMemRead8		= readByteFunction;
	pCPU->pMemRead16	= readWordFunction;
	pCPU->pMemWrite8	= writeByteFunction;
	pCPU->pMemWrite16	= writeWordFunction;

	cpuInit(pCPU);
	cpuReset(pCPU);
	cpuExec(pCPU,iCycleCountToExecute);
 
    TODO: save/load of CPU state
    TODO: review IRQWaiter
*/
/*****************************************************************/

#include "mc6809.h"
#include "mc6809defs.h"

#include "xDebug.h"

/********************************************************************************/

#pragma mark -
#pragma mark --- local macros ---

/********************************************************************************/

#define ASSERT_MC6809(pInstance)	assert(pInstance != NULL);	\
									assert(pInstance->cpu.device.id == EMU_DEVICE_ID); \
									assert(pInstance->cpu.device.idModule == EMU_CPU_ID); \
									assert(pInstance->cpu.device.idDevice == VCC_MC6809_ID)

#define D_REG	d.Reg
#define PC_REG	pc.Reg
#define X_REG	x.Reg
#define Y_REG	y.Reg
#define U_REG	u.Reg
#define S_REG	s.Reg
#define A_REG	d.B.msb
#define B_REG	d.B.lsb

#define NTEST8(r)			(r > 0x7F)
#define NTEST16(r)			(r > 0x7FFF)
#define OTEST8(c,a,b,r)		(c ^ (((a ^ b ^ r) >> 7) & 1))
#define OTEST16(c,a,b,r)	(c ^ (((a ^ b ^ r) >> 15) & 1))
#define ZTEST(r)			(!r)

/********************************************************************************/

#pragma mark -
#pragma mark --- local data types ---

/********************************************************************************/

/*
	CPU register
 */
typedef union
{
	unsigned short Reg;
	struct
	{
		unsigned char lsb;
		unsigned char msb;
	} B;
} cpuregister_t;

/*
 CPU instance for emulation
 */
typedef struct cpu6809_t
{
	cpu_t				cpu;
	
	cpuregister_t		pc;
	cpuregister_t		x;
	cpuregister_t		y;
	cpuregister_t		u;
	cpuregister_t		s;
	cpuregister_t		dp;
	cpuregister_t		d;
	
	unsigned int		cc[8];
	unsigned char *		ureg8[8]; 
	unsigned char		ccbits;
	unsigned short *	xfreg16[8];
	
	int		            SyncWaiting;
    int		            PendingInterrupts;      // see: irqtype_e
    int                 ActiveInterrupts;     // see: irqtype_e
	int		            IRQWaiter;
} cpu6809_t;

/********************************************************************************/

#pragma mark -
#pragma mark --- emulator device callbacks ---

/********************************************************************/

result_t mc6809EmuDevDestroy(emudevice_t * pEmuDevice)
{
	cpu6809_t *	pMC6809;
	
	pMC6809 = (cpu6809_t *)pEmuDevice;
	
	ASSERT_MC6809(pMC6809);
	
	if ( pMC6809 != NULL )
	{
		emuDevRemoveChild(pMC6809->cpu.device.pParent,&pMC6809->cpu.device);
		cpuDestroy(&pMC6809->cpu);
	}
	
	return XERROR_NONE;
}

/********************************************************************************/

result_t mc6809EmuDevConfSave(emudevice_t * pEmuDevice, config_t * config)
{
	result_t	errResult	= XERROR_INVALID_PARAMETER;
	cpu6809_t *	pMC6809;
	
	pMC6809 = (cpu6809_t *)pEmuDevice;
	
	ASSERT_MC6809(pMC6809);
	
	if ( pMC6809 != NULL )
	{
		// do nothing for now
		
		errResult = XERROR_NONE;
	}
	
	return errResult;
}

/********************************************************************************/

result_t mc6809EmuDevConfLoad(emudevice_t * pEmuDevice, config_t * config)
{
	result_t	errResult	= XERROR_INVALID_PARAMETER;
	cpu6809_t *	pMC6809;
	
	pMC6809 = (cpu6809_t *)pEmuDevice;
	
	ASSERT_MC6809(pMC6809);
	
	if ( pMC6809 != NULL )
	{
		// do nothing for now
		
		errResult = XERROR_NONE;
	}
	
	return errResult;
}

/********************************************************************************/

#pragma mark -
#pragma mark --- local functions ---

/*****************************************************************/

static void mc6809SetCC(cpu_t * pCPU, unsigned char bincc)
{
	unsigned char	bit;
	cpu6809_t *		p6809	= (cpu6809_t *)pCPU;
	
	assert(p6809 != NULL);
	
	for (bit=0; bit<=7; bit++)
	{
		p6809->cc[bit] = !!(bincc & (1 << bit));
	}
}

/*****************************************************************/

static unsigned char mc6809GetCC(cpu_t * pCPU)
{
	unsigned char bincc=0,bit=0;
	cpu6809_t *		p6809	= (cpu6809_t *)pCPU;
	
	assert(p6809 != NULL);
	
	for (bit=0; bit<=7; bit++)
	{
		if ( p6809->cc[bit])
		{
			bincc = bincc | (1 << bit);
		}
	}
	
	return(bincc);
}

/*****************************************************************/

static void mc6809CpuFIRQ(cpu_t * pCPU)
{
	cpu6809_t *		p6809	= (cpu6809_t *)pCPU;
	
	assert(p6809 != NULL);
	
	if ( !p6809->cc[F] )
	{
		p6809->ActiveInterrupts |= FIRQ;	//Flag to indicate FIRQ has been asserted
        
		p6809->cc[E] = 0;		// Turn E flag off
		cpuMemWrite8(pCPU,p6809->pc.B.lsb,--p6809->s.Reg);
		cpuMemWrite8(pCPU,p6809->pc.B.msb,--p6809->s.Reg);
		cpuMemWrite8(pCPU,mc6809GetCC(pCPU),--p6809->s.Reg);
		p6809->cc[I] = 1;
		p6809->cc[F] = 1;
		p6809->pc.Reg = cpuMemRead16(&p6809->cpu,VFIRQ);
	}
	
	p6809->PendingInterrupts = p6809->PendingInterrupts & ~FIRQ;

    // TODO: correct?
    p6809->cpu.CycleCounter += (5 + 4);
}

/*****************************************************************/

static void mc6809CpuIRQ(cpu_t * pCPU)
{
	cpu6809_t *		p6809	= (cpu6809_t *)pCPU;
	
	assert(p6809 != NULL);
	
	if ( p6809->ActiveInterrupts & FIRQ ) // If FIRQ is running postpone the IRQ
	{
		return;	
	}
    
	if ( (!p6809->cc[I]) )
	{
        p6809->ActiveInterrupts |= IRQ;
        
		p6809->cc[E]=1;
        
		cpuMemWrite8(pCPU, p6809->pc.B.lsb, --p6809->s.Reg);
		cpuMemWrite8(pCPU, p6809->pc.B.msb, --p6809->s.Reg);
		cpuMemWrite8(pCPU, p6809->u.B.lsb, --p6809->s.Reg);
		cpuMemWrite8(pCPU, p6809->u.B.msb, --p6809->s.Reg);
		cpuMemWrite8(pCPU, p6809->y.B.lsb, --p6809->s.Reg);
		cpuMemWrite8(pCPU, p6809->y.B.msb, --p6809->s.Reg);
		cpuMemWrite8(pCPU, p6809->x.B.lsb, --p6809->s.Reg);
		cpuMemWrite8(pCPU, p6809->x.B.msb, --p6809->s.Reg);
		cpuMemWrite8(pCPU, p6809->dp.B.msb, --p6809->s.Reg);
		cpuMemWrite8(pCPU, p6809->B_REG, --p6809->s.Reg);
		cpuMemWrite8(pCPU, p6809->A_REG, --p6809->s.Reg);
		cpuMemWrite8(pCPU, mc6809GetCC(pCPU), --p6809->s.Reg);
		p6809->pc.Reg=cpuMemRead16(&p6809->cpu,VIRQ);
		p6809->cc[I]=1; 
	} //Fi I test
	
	p6809->PendingInterrupts = p6809->PendingInterrupts & ~IRQ;

    // TODO: correct?
    p6809->cpu.CycleCounter += (5 + 12);
}

/*****************************************************************/

static void mc6809CpuNMI(cpu_t * pCPU)
{
	cpu6809_t *		p6809	= (cpu6809_t *)pCPU;
	
	assert(p6809 != NULL);
	
	p6809->cc[E]=1;
	cpuMemWrite8(pCPU, p6809->pc.B.lsb, --p6809->s.Reg);
	cpuMemWrite8(pCPU, p6809->pc.B.msb, --p6809->s.Reg);
	cpuMemWrite8(pCPU, p6809->u.B.lsb, --p6809->s.Reg);
	cpuMemWrite8(pCPU, p6809->u.B.msb, --p6809->s.Reg);
	cpuMemWrite8(pCPU, p6809->y.B.lsb, --p6809->s.Reg);
	cpuMemWrite8(pCPU, p6809->y.B.msb, --p6809->s.Reg);
	cpuMemWrite8(pCPU, p6809->x.B.lsb, --p6809->s.Reg);
	cpuMemWrite8(pCPU, p6809->x.B.msb, --p6809->s.Reg);
	cpuMemWrite8(pCPU, p6809->dp.B.msb, --p6809->s.Reg);
	cpuMemWrite8(pCPU, p6809->B_REG, --p6809->s.Reg);
	cpuMemWrite8(pCPU, p6809->A_REG, --p6809->s.Reg);
	cpuMemWrite8(pCPU, mc6809GetCC(pCPU), --p6809->s.Reg);
	p6809->cc[I]=1;
	p6809->cc[F]=1;
	p6809->pc.Reg = cpuMemRead16(&p6809->cpu,VNMI);
	p6809->PendingInterrupts = p6809->PendingInterrupts & ~NMI;
    
    // TODO: correct?
    p6809->cpu.CycleCounter += (5 + 12);
}

/*****************************************************************/
/**
	TODO: try table driven?
 */
unsigned short mc6809CalculateEA(cpu_t * pCPU, unsigned char postbyte)
{
	unsigned short int	ea		= 0;
	signed char			byte	= 0;
	unsigned char		Register;
	cpu6809_t *			p6809	= (cpu6809_t *)pCPU;
	
	assert(p6809 != NULL);
	
	Register = ((postbyte>>5)&3)+1;
	
	if ( postbyte & 0x80 ) 
	{
		switch ( postbyte & 0x1F )
		{
			case 0:
				ea = (*p6809->xfreg16[Register]);
				(*p6809->xfreg16[Register])++;
				p6809->cpu.CycleCounter+=2;
				break;
				
			case 1:
				ea = (*p6809->xfreg16[Register]);
				(*p6809->xfreg16[Register])+=2;
				p6809->cpu.CycleCounter+=3;
				break;
				
			case 2:
				(*p6809->xfreg16[Register])-=1;
				ea = (*p6809->xfreg16[Register]);
				p6809->cpu.CycleCounter+=2;
				break;
				
			case 3:
				(*p6809->xfreg16[Register])-=2;
				ea = (*p6809->xfreg16[Register]);
				p6809->cpu.CycleCounter+=3;
				break;
				
			case 4:
				ea = (*p6809->xfreg16[Register]);
				break;
				
			case 5:
				ea = (*p6809->xfreg16[Register])+((signed char)p6809->B_REG);
				p6809->cpu.CycleCounter+=1;
				break;
				
			case 6:
				ea = (*p6809->xfreg16[Register])+((signed char)p6809->A_REG);
				p6809->cpu.CycleCounter+=1;
				break;
				
			case 7:
				//			ea = (*p6809->xfreg16[Register])+((signed char)p6809->E_REG);
				p6809->cpu.CycleCounter+=1;
				break;
				
			case 8:
				ea = (*p6809->xfreg16[Register])+(signed char)cpuMemRead8(pCPU,p6809->pc.Reg++);
				p6809->cpu.CycleCounter+=1;
				break;
				
			case 9:
				ea = (*p6809->xfreg16[Register])+cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
				p6809->cpu.CycleCounter+=4;
				p6809->pc.Reg+=2;
				break;
				
			case 10:
				// ea = (*p6809->xfreg16[Register])+((signed char)p6809->F_REG);
				p6809->cpu.CycleCounter+=1;
				break;
				
			case 11:
				ea = (*p6809->xfreg16[Register])+p6809->D_REG; //Changed to unsigned 03/14/2005 NG Was signed
				p6809->cpu.CycleCounter+=4;
				break;
				
			case 12:
				ea = (signed short)p6809->pc.Reg+(signed char)cpuMemRead8(pCPU,p6809->pc.Reg)+1; 
				p6809->cpu.CycleCounter+=1;
				p6809->pc.Reg++;
				break;
				
			case 13: //MM
				ea = p6809->pc.Reg+cpuMemRead16(&p6809->cpu,p6809->pc.Reg)+2; 
				p6809->cpu.CycleCounter+=5;
				p6809->pc.Reg+=2;
				break;
				
			case 14:
				// ea = (*p6809->xfreg16[Register])+p6809->W_REG; 
				p6809->cpu.CycleCounter+=4;
				break;
				
			case 15: //01111
				byte = (postbyte>>5)&3;
				switch (byte)
			{
				case 0:
//				ea = W_REG;
					break;
				case 1:
//				ea = W_REG+cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
					p6809->pc.Reg+=2;
					break;
				case 2:
//				ea = W_REG;
//				W_REG+=2;
					break;
				case 3:
//				W_REG-=2;
//				ea=W_REG;
					break;
			}
				break;
				
			case 16: //10000
				byte = (postbyte>>5) & 3;
				switch (byte)
			{
				case 0:
					//				ea = cpuMemRead16(&p6809->cpu,W_REG);
					break;
				case 1:
					//				ea = cpuMemRead16(&p6809->cpu,W_REG+cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
					p6809->pc.Reg+=2;
					break;
				case 2:
					//				ea = cpuMemRead16(&p6809->cpu,W_REG);
					//				W_REG+=2;
					break;
				case 3:
					//				W_REG-=2;
					//				ea = cpuMemRead16(&p6809->cpu,W_REG);
					break;
			}
				break;
				
				
			case 17: //10001
				ea = (*p6809->xfreg16[Register]);
				(*p6809->xfreg16[Register])+=2;
				ea = cpuMemRead16(&p6809->cpu,ea);
				p6809->cpu.CycleCounter+=6;
				break;
				
			case 18: //10010
				//			MessageBox(0,"Hitting undecoded 18","Ok",0);
				p6809->cpu.CycleCounter+=6;
				break;
				
			case 19: //10011
				(*p6809->xfreg16[Register])-=2;
				ea = (*p6809->xfreg16[Register]);
				ea = cpuMemRead16(&p6809->cpu,ea);
				p6809->cpu.CycleCounter+=6;
				break;
				
			case 20: //10100
				ea = (*p6809->xfreg16[Register]);
				ea = cpuMemRead16(&p6809->cpu,ea);
				p6809->cpu.CycleCounter+=3;
				break;
				
			case 21: //10101
				ea = (*p6809->xfreg16[Register])+((signed char)p6809->B_REG);
				ea = cpuMemRead16(&p6809->cpu,ea);
				p6809->cpu.CycleCounter+=4;
				break;
				
			case 22: //10110
				ea = (*p6809->xfreg16[Register])+((signed char)p6809->A_REG);
				ea = cpuMemRead16(&p6809->cpu,ea);
				p6809->cpu.CycleCounter+=4;
				break;
				
			case 23: //10111
				//			ea=(*xfreg16[Register])+((signed char)p6809->E_REG);
				ea = cpuMemRead16(&p6809->cpu,ea);
				p6809->cpu.CycleCounter+=4;
				break;
				
			case 24: //11000
				ea = (*p6809->xfreg16[Register])+(signed char)cpuMemRead8(pCPU,p6809->pc.Reg++);
				ea = cpuMemRead16(&p6809->cpu,ea);
				p6809->cpu.CycleCounter+=4;
				break;
				
			case 25: //11001
				ea = (*p6809->xfreg16[Register])+cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
				ea = cpuMemRead16(&p6809->cpu,ea);
				p6809->cpu.CycleCounter+=7;
				p6809->pc.Reg+=2;
				break;
			case 26: //11010
				//			ea=(*xfreg16[Register])+((signed char)p6809->F_REG);
				ea = cpuMemRead16(&p6809->cpu,ea);
				p6809->cpu.CycleCounter+=4;
				break;
				
			case 27: //11011
				ea = (*p6809->xfreg16[Register])+p6809->D_REG;
				ea = cpuMemRead16(&p6809->cpu,ea);
				p6809->cpu.CycleCounter+=7;
				break;
				
			case 28: //11100
				ea = (signed short)p6809->pc.Reg+(signed char)cpuMemRead8(pCPU,p6809->pc.Reg)+1; 
				ea = cpuMemRead16(&p6809->cpu,ea);
				p6809->cpu.CycleCounter+=4;
				p6809->pc.Reg++;
				break;
				
			case 29: //11101
				ea = p6809->pc.Reg+cpuMemRead16(&p6809->cpu,p6809->pc.Reg)+2; 
				ea = cpuMemRead16(&p6809->cpu,ea);
				p6809->cpu.CycleCounter+=8;
				p6809->pc.Reg+=2;
				break;
				
			case 30: //11110
				// ea = (*p6809->xfreg16[Register])+p6809->W_REG; 
				ea = cpuMemRead16(&p6809->cpu,ea);
				p6809->cpu.CycleCounter+=7;
				break;
				
			case 31: //11111
				ea = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
				ea = cpuMemRead16(&p6809->cpu,ea);
				p6809->cpu.CycleCounter+=8;
				p6809->pc.Reg+=2;
				break;
				
		} //END Switch
	}
	else 
	{
		byte= (postbyte & 31);
		byte= (byte << 3);
		byte= byte /8;
		ea= *p6809->xfreg16[Register]+byte; //Was signed
		p6809->cpu.CycleCounter+=1;
	}
	
	return (ea);
}

/********************************************************************************/

#pragma mark -
#pragma mark --- cpu_t plug-in API ---

/*****************************************************************/
/**
	Initialize CPU
 
	This needs to be calld before reset.  Then memory read/write 
	functions should be set.
 */
void cpuMC6809Init(cpu_t * pCPU)
{	
	cpu6809_t *				p6809	= (cpu6809_t *)pCPU;
	
	assert(p6809 != NULL);
	
	emuDevRegisterDevice(&p6809->cpu.device);

	// reg pointers for TFR and EXG and LEA ops
	p6809->xfreg16[0] = &p6809->D_REG;
	p6809->xfreg16[1] = &p6809->X_REG;
	p6809->xfreg16[2] = &p6809->Y_REG;
	p6809->xfreg16[3] = &p6809->U_REG;
	p6809->xfreg16[4] = &p6809->S_REG;
	p6809->xfreg16[5] = &p6809->PC_REG;
	
	p6809->ureg8[0] = (unsigned char *)&p6809->A_REG;		
	p6809->ureg8[1] = (unsigned char *)&p6809->B_REG;		
	p6809->ureg8[2] = (unsigned char *)&p6809->ccbits;
	p6809->ureg8[3] = (unsigned char *)&p6809->dp.B.msb;
	p6809->ureg8[4] = (unsigned char *)&p6809->dp.B.msb;
	p6809->ureg8[5] = (unsigned char *)&p6809->dp.B.msb;
	p6809->ureg8[6] = (unsigned char *)&p6809->dp.B.msb;
	p6809->ureg8[7] = (unsigned char *)&p6809->dp.B.msb;
	
	p6809->cc[I]=1;
	p6809->cc[F]=1;
}

/*****************************************************************/
/**
	Reset CPU
 */
void cpuMC6809Reset(cpu_t * pCPU)
{ 
	char index;
	cpu6809_t *				p6809	= (cpu6809_t *)pCPU;
	
	assert(p6809 != NULL);

	// set all registers to 0
	for (index=0; index<=5; index++)
	{
		*p6809->xfreg16[index] = 0;
	}
	for (index=0; index<=7; index++)
	{
		*p6809->ureg8[index]=0;
	}
	
	// init CC register
	for (index=0;index<=7;index++)
	{
		p6809->cc[index]=0;
	}
	p6809->cc[I]=1;
	p6809->cc[F]=1;

	p6809->dp.Reg=0;
	p6809->SyncWaiting=0;

	// get the reset vector for the program counter
	p6809->pc.Reg = cpuMemRead16(&p6809->cpu,VRESET);	
}

/*****************************************************************/
/**
 */
void cpuMC6809AssertInterrupt(cpu_t * pCPU, irqtype_e Interrupt, int waiter)
{
	cpu6809_t *		p6809 = (cpu6809_t *)pCPU;
	
	assert(p6809 != NULL);
	
	p6809->SyncWaiting=0;
	p6809->PendingInterrupts = p6809->PendingInterrupts | Interrupt;
	p6809->IRQWaiter=waiter;
    //p6809->IRQWaiter = 0;
}

/*****************************************************************/
/**
*/
void cpuMC6809DeAssertInterrupt(cpu_t * pCPU, irqtype_e Interrupt)
{
	cpu6809_t *		p6809 = (cpu6809_t *)pCPU;
	
	assert(p6809 != NULL);
	
	p6809->PendingInterrupts = p6809->PendingInterrupts & ~Interrupt;
	p6809->ActiveInterrupts &= ~Interrupt;
}

/*****************************************************************/
/**
 */
void cpuMC6809ForcePC(cpu_t * pCPU, int NewPC)
{
	cpu6809_t *		p6809 = (cpu6809_t *)pCPU;
	
	assert(p6809 != NULL);
	
	p6809->pc.Reg = NewPC;
	p6809->PendingInterrupts = 0;
	p6809->SyncWaiting = 0;
}

/*****************************************************************/
/**
 */
int cpuMC6809Exec(cpu_t * pCPU, int CycleFor)
{
	unsigned char		msn;
	unsigned char		lsn;
	unsigned char		ucSource;
	unsigned char		ucDest;
	cpu6809_t *			p6809	= (cpu6809_t *)pCPU;
	uint32_t			temp32;
	uint16_t			temp16;
	uint8_t				temp8;
	unsigned char		postbyte;
	short unsigned		postword;
	signed char *		spostbyte;
	signed short *		spostword;
	
	assert(p6809 != NULL);
	
	spostbyte	= (signed char *)&postbyte;
	spostword	= (signed short *)&postword;
	
    int endCycle = p6809->cpu.CycleCounter + CycleFor;
	while ( p6809->cpu.CycleCounter < endCycle )
	{
		if ( p6809->PendingInterrupts )
		{
			if ( (p6809->PendingInterrupts & NMI) )
			{
				mc6809CpuNMI(pCPU);
			}
			
			if ( (p6809->PendingInterrupts & FIRQ) )
			{
				mc6809CpuFIRQ(pCPU);
			}
			
			if ( (p6809->PendingInterrupts & IRQ) )
			{
				if ( p6809->IRQWaiter == 0 )	// This is needed to fix a subtle timming problem
				{
					mc6809CpuIRQ(pCPU);		    // It allows the CPU to see $FF03 bit 7 high before
				}
				else							// The IRQ is asserted.
				{
					p6809->IRQWaiter -= 1;
				}
			}
		}

		if ( p6809->SyncWaiting == 1 )
		{
			return (0);
		}
		
		/* get opcode and execute it */
		switch ( cpuMemRead8(pCPU,p6809->pc.Reg++) )
		{
			case NEG_D: //0
				temp16 = (p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++));
				postbyte = cpuMemRead8(pCPU,temp16);
				temp8 = 0 - postbyte;
				p6809->cc[C] = temp8 > 0;
				p6809->cc[V] = (postbyte==0x80);
				p6809->cc[N] = NTEST8(temp8);
				p6809->cc[Z] = ZTEST(temp8);
				cpuMemWrite8(pCPU,temp8,temp16);
				p6809->cpu.CycleCounter+=6;
			break;

			case COM_D:
				temp16 = (p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++));
				temp8 = cpuMemRead8(pCPU,temp16);
				temp8 = 0xFF - temp8;
				p6809->cc[Z] = ZTEST(temp8);
				p6809->cc[N] = NTEST8(temp8); 
				p6809->cc[C] = 1;
				p6809->cc[V] = 0;
				cpuMemWrite8(pCPU,temp8,temp16);
				p6809->cpu.CycleCounter+=6;
			break;

			case LSR_D: //S2
				temp16 = (p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++));
				temp8 = cpuMemRead8(pCPU,temp16);
				p6809->cc[C]= temp8 & 1;
				temp8 = temp8 >>1;
				p6809->cc[Z]= ZTEST(temp8);
				p6809->cc[N]=0;
				cpuMemWrite8(pCPU,temp8,temp16);
				p6809->cpu.CycleCounter+=6;
			break;

			case ROR_D: //S2
				temp16 = (p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++));
				temp8 = cpuMemRead8(pCPU,temp16);
				postbyte = p6809->cc[C]<<7;
				p6809->cc[C] = temp8 & 1;
				temp8 = (temp8 >> 1)| postbyte;
				p6809->cc[Z] = ZTEST(temp8);
				p6809->cc[N] = NTEST8(temp8);
				cpuMemWrite8(pCPU,temp8,temp16);
				p6809->cpu.CycleCounter+=6;
			break;

			case ASR_D: //7
				temp16 = (p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++));
				temp8 = cpuMemRead8(pCPU,temp16);
				p6809->cc[C]= temp8 & 1;
				temp8 = (temp8 & 0x80) | (temp8 >>1);
				p6809->cc[Z]= ZTEST(temp8);
				p6809->cc[N]= NTEST8(temp8);
				cpuMemWrite8(pCPU,temp8,temp16);
				p6809->cpu.CycleCounter+=6;
			break;

			case ASL_D: //8
				temp16 = (p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++));
				temp8 = cpuMemRead8(pCPU,temp16);
				p6809->cc[C]= (temp8 & 0x80) >>7;
				p6809->cc[V]= p6809->cc[C] ^ ((temp8 & 0x40) >> 6);
				temp8 = temp8 <<1;
				p6809->cc[N] = NTEST8(temp8);
				p6809->cc[Z] = ZTEST(temp8);
				cpuMemWrite8(pCPU,temp8,temp16);
				p6809->cpu.CycleCounter+=6;
			break;

			case ROL_D:	//9
				temp16 = (p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++));
				temp8 = cpuMemRead8(pCPU,temp16);
				postbyte = p6809->cc[C];
				p6809->cc[C] = (temp8 & 0x80)>>7;
				p6809->cc[V] = p6809->cc[C] ^ ((temp8 & 0x40) >>6);
				temp8 = (temp8<<1) | postbyte;
				p6809->cc[Z] = ZTEST(temp8);
				p6809->cc[N] = NTEST8(temp8);
				cpuMemWrite8(pCPU,temp8,temp16);
				p6809->cpu.CycleCounter += 6;
			break;

			case DEC_D: //A
				temp16 = (p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++));
				temp8 = cpuMemRead8(pCPU,temp16)-1;
				p6809->cc[Z] = ZTEST(temp8);
				p6809->cc[N] = NTEST8(temp8);
				p6809->cc[V] = (temp8 == 0x7F);
				cpuMemWrite8(pCPU,temp8,temp16);
				p6809->cpu.CycleCounter += 6;
			break;

			case INC_D: //C
				temp16 = (p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++));
				temp8 = cpuMemRead8(pCPU,temp16)+1;
				p6809->cc[Z] = ZTEST(temp8);
				p6809->cc[V] = (temp8 == 0x80);
				p6809->cc[N] = NTEST8(temp8);
				cpuMemWrite8(pCPU,temp8,temp16);
				p6809->cpu.CycleCounter += 6;
			break;

			case TST_D: //D
				temp8 = cpuMemRead8(pCPU,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[Z] = ZTEST(temp8);
				p6809->cc[N] = NTEST8(temp8);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 6;
			break;

			case JMP_D:	//E
				p6809->pc.Reg = ((p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg)));
				p6809->cpu.CycleCounter+=3;
			break;

			case CLR_D:	// 0x0F
				cpuMemWrite8(pCPU,0,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[Z] = 1;
				p6809->cc[N] = 0;
				p6809->cc[V] = 0;
				p6809->cc[C] = 0;
				p6809->cpu.CycleCounter += 6;
			break;

			case Page2:	// 0x10
				switch ( cpuMemRead8(pCPU,p6809->pc.Reg++) ) 
				{
					case LBEQ_R: //1027
						if ( p6809->cc[Z] )
						{
							*spostword = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
							p6809->pc.Reg += *spostword;
							p6809->cpu.CycleCounter+=1;
						}
						p6809->pc.Reg+=2;
						p6809->cpu.CycleCounter+=5;
					break;

					case LBRN_R: //1021
						p6809->pc.Reg+=2;
						p6809->cpu.CycleCounter+=5;
					break;

					case LBHI_R: //1022
						if  (!(p6809->cc[C] | p6809->cc[Z]))
						{
							*spostword = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
							p6809->pc.Reg += *spostword;
							p6809->cpu.CycleCounter += 1;
						}
						p6809->pc.Reg+=2;
						p6809->cpu.CycleCounter+=5;
					break;

					case LBLS_R: //1023
						if ( p6809->cc[C] | p6809->cc[Z] )
						{
							*spostword = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
							p6809->pc.Reg += *spostword;
							p6809->cpu.CycleCounter+=1;
						}
						p6809->pc.Reg += 2;
						p6809->cpu.CycleCounter += 5;
					break;

					case LBHS_R: //1024
						if ( !p6809->cc[C] )
						{
							*spostword = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
							p6809->pc.Reg += *spostword;
							p6809->cpu.CycleCounter+=1;
						}
						p6809->pc.Reg += 2;
						p6809->cpu.CycleCounter += 6;
					break;

					case LBCS_R: //1025
						if ( p6809->cc[C] )
						{
							*spostword = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
							p6809->pc.Reg += *spostword;
							p6809->cpu.CycleCounter += 1;
						}
						p6809->pc.Reg+=2;
						p6809->cpu.CycleCounter+=5;
					break;

					case LBNE_R: //1026
						if ( !p6809->cc[Z] )
						{
							*spostword = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
							p6809->pc.Reg += *spostword;
							p6809->cpu.CycleCounter+=1;
						}
						p6809->pc.Reg += 2;
						p6809->cpu.CycleCounter += 5;
					break;

					case LBVC_R: //1028
						if ( !p6809->cc[V] )
						{
							*spostword = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
							p6809->pc.Reg += *spostword;
							p6809->cpu.CycleCounter+=1;
						}
						p6809->pc.Reg += 2;
						p6809->cpu.CycleCounter += 5;
					break;

					case LBVS_R: //1029
						if ( p6809->cc[V] )
						{
							*spostword = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
							p6809->pc.Reg += *spostword;
							p6809->cpu.CycleCounter += 1;
						}
						p6809->pc.Reg += 2;
						p6809->cpu.CycleCounter += 5;
					break;

					case LBPL_R: //102A
						if ( !p6809->cc[N] )
						{
							*spostword = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
							p6809->pc.Reg += *spostword;
							p6809->cpu.CycleCounter+=1;
						}
						p6809->pc.Reg += 2;
						p6809->cpu.CycleCounter += 5;
					break;

					case LBMI_R: //102B
					if ( p6809->cc[N] )
						{
							*spostword=cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
							p6809->pc.Reg += *spostword;
							p6809->cpu.CycleCounter += 1;
						}
						p6809->pc.Reg += 2;
						p6809->cpu.CycleCounter += 5;
					break;

					case LBGE_R: //102C
						if ( !(p6809->cc[N] ^ p6809->cc[V]))
						{
							*spostword = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
							p6809->pc.Reg += *spostword;
							p6809->cpu.CycleCounter += 1;
						}
						p6809->pc.Reg += 2;
						p6809->cpu.CycleCounter += 5;
					break;

					case LBLT_R: //102D
						if ( p6809->cc[V] ^ p6809->cc[N])
						{
							*spostword = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
							p6809->pc.Reg += *spostword;
							p6809->cpu.CycleCounter += 1;
						}
						p6809->pc.Reg += 2;
						p6809->cpu.CycleCounter += 5;
					break;

					case LBGT_R: //102E
						if ( !( p6809->cc[Z] | (p6809->cc[N] ^ p6809->cc[V] ) ))
						{
							*spostword = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
							p6809->pc.Reg += *spostword;
							p6809->cpu.CycleCounter += 1;
						}
						p6809->pc.Reg += 2;
						p6809->cpu.CycleCounter += 5;
					break;

					case LBLE_R:	//102F
						if ( p6809->cc[Z] | (p6809->cc[N] ^ p6809->cc[V]) )
						{
							*spostword = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
							p6809->pc.Reg += *spostword;
							p6809->cpu.CycleCounter += 1;
						}
						p6809->pc.Reg += 2;
						p6809->cpu.CycleCounter += 5;
					break;

					case SWI2_I: //103F
						p6809->cc[E] = 1;
						cpuMemWrite8(pCPU,p6809->pc.B.lsb, --p6809->s.Reg);
						cpuMemWrite8(pCPU,p6809->pc.B.msb, --p6809->s.Reg);
						cpuMemWrite8(pCPU,p6809->u.B.lsb, --p6809->s.Reg);
						cpuMemWrite8(pCPU,p6809->u.B.msb, --p6809->s.Reg);
						cpuMemWrite8(pCPU,p6809->y.B.lsb, --p6809->s.Reg);
						cpuMemWrite8(pCPU,p6809->y.B.msb, --p6809->s.Reg);
						cpuMemWrite8(pCPU,p6809->x.B.lsb, --p6809->s.Reg);
						cpuMemWrite8(pCPU,p6809->x.B.msb, --p6809->s.Reg);
						cpuMemWrite8(pCPU,p6809->dp.B.msb, --p6809->s.Reg);
						cpuMemWrite8(pCPU,p6809->B_REG, --p6809->s.Reg);
						cpuMemWrite8(pCPU,p6809->A_REG, --p6809->s.Reg);
						cpuMemWrite8(pCPU,mc6809GetCC(pCPU), --p6809->s.Reg);
						p6809->pc.Reg = cpuMemRead16(&p6809->cpu,VSWI2);
						p6809->cpu.CycleCounter += 20;
					break;

					case CMPD_M: //1083
						postword = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
						temp16 = p6809->D_REG - postword;
						p6809->cc[C] = temp16 > p6809->D_REG;
						p6809->cc[V] = OTEST16(p6809->cc[C],postword,temp16,p6809->D_REG); //p6809->cc[C]^((postword^temp16^p6809->D_REG)>>15);
						p6809->cc[N] = NTEST16(temp16);
						p6809->cc[Z] = ZTEST(temp16);
						p6809->pc.Reg += 2;
						p6809->cpu.CycleCounter += 5;
					break;

					case CMPY_M: //108C
						postword = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
						temp16 = p6809->y.Reg - postword;
						p6809->cc[C] = temp16 > p6809->y.Reg;
						p6809->cc[V] = OTEST16(p6809->cc[C],postword,temp16,p6809->y.Reg); //p6809->cc[C]^((postword^temp16^p6809->y.Reg)>>15);
						p6809->cc[N] = NTEST16(temp16);
						p6809->cc[Z] = ZTEST(temp16);
						p6809->pc.Reg += 2;
						p6809->cpu.CycleCounter += 5;
					break;
				
					case LDY_M: //108E
						p6809->y.Reg = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
						p6809->cc[Z] = ZTEST(p6809->y.Reg);
						p6809->cc[N] = NTEST16(p6809->y.Reg);
						p6809->cc[V] = 0;
						p6809->pc.Reg += 2;
						p6809->cpu.CycleCounter += 5;
					break;

					case CMPD_D: //1093
						postword =cpuMemRead16(&p6809->cpu,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
						temp16 = p6809->D_REG - postword ;
						p6809->cc[C] = temp16 > p6809->D_REG;
						p6809->cc[V] = OTEST16(p6809->cc[C],postword,temp16,p6809->D_REG); //p6809->cc[C]^((postword^temp16^p6809->D_REG)>>15);
						p6809->cc[N] = NTEST16(temp16);
						p6809->cc[Z] = ZTEST(temp16);
						p6809->cpu.CycleCounter += 7;
					break;

					case CMPY_D:	//109C
						postword = cpuMemRead16(&p6809->cpu,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
						temp16 = p6809->y.Reg - postword ;
						p6809->cc[C] = temp16 > p6809->y.Reg;
						p6809->cc[V] = OTEST16(p6809->cc[C],postword,temp16,p6809->y.Reg);//cc[C]^((postword^temp16^p6809->y.Reg)>>15);
						p6809->cc[N] = NTEST16(temp16);
						p6809->cc[Z] = ZTEST(temp16);
						p6809->cpu.CycleCounter += 7;
					break;

					case LDY_D: //109E
						p6809->y.Reg = cpuMemRead16(&p6809->cpu,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
						p6809->cc[Z] = ZTEST(p6809->y.Reg);	
						p6809->cc[N] = NTEST16(p6809->y.Reg);
						p6809->cc[V] = 0;
						p6809->cpu.CycleCounter += 6;
					break;
				
					case STY_D: //109F
						cpuMemWrite16(&p6809->cpu,p6809->y.Reg,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
						p6809->cc[Z] = ZTEST(p6809->y.Reg);
						p6809->cc[N] = NTEST16(p6809->y.Reg);
						p6809->cc[V] = 0;
						p6809->cpu.CycleCounter += 6;
					break;

					case CMPD_X: //10A3
						postword = cpuMemRead16(&p6809->cpu,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
						temp16 = p6809->D_REG - postword ;
						p6809->cc[C]= temp16 > p6809->D_REG;
						p6809->cc[V]= OTEST16(p6809->cc[C],postword,temp16,p6809->D_REG); //p6809->cc[C]^((postword^temp16^p6809->D_REG)>>15);
						p6809->cc[N]= NTEST16(temp16);
						p6809->cc[Z]= ZTEST(temp16);
						p6809->cpu.CycleCounter+=7;
					break;

					case CMPY_X: //10AC
						postword = cpuMemRead16(&p6809->cpu,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
						temp16 = p6809->y.Reg - postword ;
						p6809->cc[C]= temp16 > p6809->y.Reg;
						p6809->cc[V]= OTEST16(p6809->cc[C],postword,temp16,p6809->Y_REG);
						p6809->cc[N]= NTEST16(temp16);
						p6809->cc[Z]= ZTEST(temp16);
						p6809->cpu.CycleCounter+=7;
					break;

					case LDY_X: //10AE
						p6809->y.Reg = cpuMemRead16(&p6809->cpu,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
						p6809->cc[Z] = ZTEST(p6809->y.Reg);
						p6809->cc[N] = NTEST16(p6809->y.Reg);
						p6809->cc[V] = 0;
						p6809->cpu.CycleCounter += 6;
					break;

					case STY_X: //10AF
						cpuMemWrite16(&p6809->cpu,p6809->y.Reg,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
						p6809->cc[Z]= ZTEST(p6809->y.Reg);
						p6809->cc[N]= NTEST16(p6809->y.Reg);
						p6809->cc[V]= 0;
						p6809->cpu.CycleCounter += 6;
					break;

					case CMPD_E: //10B3
						postword = cpuMemRead16(&p6809->cpu,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
						temp16 = p6809->D_REG - postword;
						p6809->cc[C]= temp16 > p6809->D_REG;
						p6809->cc[V]= OTEST16(p6809->cc[C],postword,temp16,p6809->D_REG);
						p6809->cc[N]= NTEST16(temp16);
						p6809->cc[Z]= ZTEST(temp16);
						p6809->pc.Reg += 2;
						p6809->cpu.CycleCounter += 8;
					break;


					case CMPY_E: //10BC
						postword = cpuMemRead16(&p6809->cpu,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
						temp16 = p6809->y.Reg - postword;
						p6809->cc[C] = temp16 > p6809->y.Reg;
						p6809->cc[V] = OTEST16(p6809->cc[C],postword,temp16,p6809->Y_REG);
						p6809->cc[N] = NTEST16(temp16);
						p6809->cc[Z] = ZTEST(temp16);
						p6809->pc.Reg += 2;
						p6809->cpu.CycleCounter += 8;
					break;

					case LDY_E: //10BE
						p6809->y.Reg = cpuMemRead16(&p6809->cpu,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
						p6809->cc[Z] = ZTEST(p6809->y.Reg);
						p6809->cc[N] = NTEST16(p6809->y.Reg);
						p6809->cc[V] = 0;
						p6809->pc.Reg += 2;
						p6809->cpu.CycleCounter += 7;
					break;

					case STY_E: //10BF
						cpuMemWrite16(&p6809->cpu,p6809->y.Reg,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
						p6809->cc[Z] = ZTEST(p6809->y.Reg);
						p6809->cc[N] = NTEST16(p6809->y.Reg);
						p6809->cc[V] = 0;
						p6809->pc.Reg += 2;
						p6809->cpu.CycleCounter += 7;
					break;

					case LDS_I:  //10CE
						p6809->s.Reg = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
						p6809->cc[Z] = ZTEST(p6809->s.Reg);
						p6809->cc[N] = NTEST16(p6809->s.Reg);
						p6809->cc[V] = 0;
						p6809->pc.Reg += 2;
						p6809->cpu.CycleCounter += 4;
					break;

					case LDS_D: //10DE
						p6809->s.Reg = cpuMemRead16(&p6809->cpu,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
						p6809->cc[Z] = ZTEST(p6809->s.Reg);
						p6809->cc[N] = NTEST16(p6809->s.Reg);
						p6809->cc[V] = 0;
						p6809->cpu.CycleCounter += 6;
					break;

					case STS_D: //10DF
						cpuMemWrite16(&p6809->cpu,p6809->s.Reg,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
						p6809->cc[Z] = ZTEST(p6809->s.Reg);
						p6809->cc[N] = NTEST16(p6809->s.Reg);
						p6809->cc[V] = 0;
						p6809->cpu.CycleCounter += 6;
					break;

					case LDS_X: //10EE
						p6809->s.Reg = cpuMemRead16(&p6809->cpu,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
						p6809->cc[Z] = ZTEST(p6809->s.Reg);
						p6809->cc[N] = NTEST16(p6809->s.Reg);
						p6809->cc[V] = 0;
						p6809->cpu.CycleCounter += 6;
					break;

					case STS_X: //10EF
						cpuMemWrite16(&p6809->cpu,p6809->s.Reg,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
						p6809->cc[Z] = ZTEST(p6809->s.Reg);
						p6809->cc[N] = NTEST16(p6809->s.Reg);
						p6809->cc[V] = 0;
						p6809->cpu.CycleCounter += 6;
					break;

					case LDS_E: //10FE
						p6809->s.Reg = cpuMemRead16(&p6809->cpu,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
						p6809->cc[Z] = ZTEST(p6809->s.Reg);
						p6809->cc[N] = NTEST16(p6809->s.Reg);
						p6809->cc[V] = 0;
						p6809->pc.Reg += 2;
						p6809->cpu.CycleCounter += 7;
					break;

					case STS_E: //10FF
						cpuMemWrite16(&p6809->cpu,p6809->s.Reg,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
						p6809->cc[Z] = ZTEST(p6809->s.Reg);
						p6809->cc[N] = NTEST16(p6809->s.Reg);
						p6809->cc[V] = 0;
						p6809->pc.Reg += 2;
						p6809->cpu.CycleCounter += 7;
					break;

					default:
						//MessageBox(0,"Unhandled Op","Ok",0);
					break;
				} //Page 2 switch END
			break;
				 
			case Page3: // 0x11
				switch ( cpuMemRead8(pCPU,p6809->pc.Reg++) )
				{
					case SWI3_I: //113F
						p6809->cc[E]=1;
						cpuMemWrite8(pCPU, p6809->pc.B.lsb, --p6809->s.Reg);
						cpuMemWrite8(pCPU, p6809->pc.B.msb, --p6809->s.Reg);
						cpuMemWrite8(pCPU, p6809->u.B.lsb, --p6809->s.Reg);
						cpuMemWrite8(pCPU, p6809->u.B.msb, --p6809->s.Reg);
						cpuMemWrite8(pCPU, p6809->y.B.lsb, --p6809->s.Reg);
						cpuMemWrite8(pCPU, p6809->y.B.msb, --p6809->s.Reg);
						cpuMemWrite8(pCPU, p6809->x.B.lsb, --p6809->s.Reg);
						cpuMemWrite8(pCPU, p6809->x.B.msb, --p6809->s.Reg);
						cpuMemWrite8(pCPU, p6809->dp.B.msb, --p6809->s.Reg);
						cpuMemWrite8(pCPU, p6809->B_REG, --p6809->s.Reg);
						cpuMemWrite8(pCPU, p6809->A_REG, --p6809->s.Reg);
						cpuMemWrite8(pCPU, mc6809GetCC(pCPU), --p6809->s.Reg);
						p6809->pc.Reg = cpuMemRead16(&p6809->cpu,VSWI3);
						p6809->cpu.CycleCounter += 20;
					break;

					case CMPU_M: //1183
						postword = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
						temp16 = p6809->u.Reg - postword;
						p6809->cc[C]= temp16 > p6809->u.Reg;
						p6809->cc[V]= OTEST16(p6809->cc[C],postword,temp16,p6809->U_REG);
						p6809->cc[N]= NTEST16(temp16);
						p6809->cc[Z]= ZTEST(temp16);
						p6809->pc.Reg += 2;	
						p6809->cpu.CycleCounter +=5 ;
					break;

					case CMPS_M: //118C
						postword = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
						temp16 = p6809->s.Reg - postword;
						p6809->cc[C] = temp16 > p6809->s.Reg;
						p6809->cc[V] = OTEST16(p6809->cc[C],postword,temp16,p6809->S_REG);
						p6809->cc[N] = NTEST16(temp16);
						p6809->cc[Z] = ZTEST(temp16);
						p6809->pc.Reg += 2;
						p6809->cpu.CycleCounter += 5;
					break;

					case CMPU_D: //1193
						postword = cpuMemRead16(&p6809->cpu,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
						temp16 = p6809->u.Reg - postword ;
						p6809->cc[C] = temp16 > p6809->u.Reg;
						p6809->cc[V] = OTEST16(p6809->cc[C],postword,temp16,p6809->U_REG);
						p6809->cc[N] = NTEST16(temp16);
						p6809->cc[Z] = ZTEST(temp16);
						p6809->cpu.CycleCounter += 7;
					break;

					case CMPS_D: //119C
						postword=cpuMemRead16(&p6809->cpu,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
						temp16 = p6809->s.Reg - postword ;
						p6809->cc[C] = temp16 > p6809->s.Reg;
						p6809->cc[V] = OTEST16(p6809->cc[C],postword,temp16,p6809->S_REG);
						p6809->cc[N] = NTEST16(temp16);
						p6809->cc[Z] = ZTEST(temp16);
						p6809->cpu.CycleCounter += 7;
					break;

					case CMPU_X: //11A3
						postword = cpuMemRead16(&p6809->cpu,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
						temp16 = p6809->u.Reg - postword ;
						p6809->cc[C] = temp16 > p6809->u.Reg;
						p6809->cc[V] = OTEST16(p6809->cc[C],postword,temp16,p6809->U_REG);
						p6809->cc[N] = NTEST16(temp16);
						p6809->cc[Z] = ZTEST(temp16);
						p6809->cpu.CycleCounter += 7;
					break;

					case CMPS_X:  //11AC
						postword = cpuMemRead16(&p6809->cpu,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
						temp16 = p6809->s.Reg - postword ;
						p6809->cc[C] = temp16 > p6809->s.Reg;
						p6809->cc[V] = OTEST16(p6809->cc[C],postword,temp16,p6809->S_REG);
						p6809->cc[N] = NTEST16(temp16);
						p6809->cc[Z] = ZTEST(temp16);
						p6809->cpu.CycleCounter+=7;
						break;

					case CMPU_E: //11B3
						postword=cpuMemRead16(&p6809->cpu,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
						temp16 = p6809->u.Reg - postword;
						p6809->cc[C] = temp16 > p6809->u.Reg;
						p6809->cc[V] = OTEST16(p6809->cc[C],postword,temp16,p6809->U_REG);
						p6809->cc[N] = NTEST16(temp16);
						p6809->cc[Z] = ZTEST(temp16);
						p6809->pc.Reg += 2;
						p6809->cpu.CycleCounter += 8;
					break;

					case CMPS_E: //11BC
						postword = cpuMemRead16(&p6809->cpu,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
						temp16 = p6809->s.Reg - postword;
						p6809->cc[C]= temp16 > p6809->s.Reg;
						p6809->cc[V]= OTEST16(p6809->cc[C],postword,temp16,p6809->S_REG);
						p6809->cc[N]= NTEST16(temp16);
						p6809->cc[Z]= ZTEST(temp16);
						p6809->pc.Reg += 2;
						p6809->cpu.CycleCounter += 8;
					break;

					default:
						//MessageBox(0,"Unhandled Op","Ok",0);
					break;
				} //Page 3 switch END
			break;

			case NOP_I:	// 0x12
				p6809->cpu.CycleCounter += 2;
			break;

			case SYNC_I: // 0x13
				p6809->cpu.CycleCounter += CycleFor;
				p6809->SyncWaiting = 1;
			break;

			case LBRA_R: // 0x16
				*spostword = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
				p6809->pc.Reg += 2;
				p6809->pc.Reg += *spostword;
				p6809->cpu.CycleCounter += 5;
			break;

			case LBSR_R: // 0x17
				*spostword = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
				p6809->pc.Reg += 2;
				p6809->s.Reg--;
				cpuMemWrite8(pCPU,p6809->pc.B.lsb, p6809->s.Reg--);
				cpuMemWrite8(pCPU,p6809->pc.B.msb, p6809->s.Reg);
				p6809->pc.Reg+=*spostword;
				p6809->cpu.CycleCounter+=9;
				break;

			case DAA_I: // 0x19
			//	MessageBox(0,"DAA","Ok",0);
				msn = (p6809->A_REG & 0xF0) ;
				lsn = (p6809->A_REG & 0xF);
				temp8 = 0;
				if ( p6809->cc[H] ||  (lsn >9) )
					temp8 |= 0x06;

				if ( (msn>0x80) && (lsn>9)) 
					temp8|=0x60;
				
				if ( (msn>0x90) || p6809->cc[C] )
					temp8|=0x60;

				temp16 = p6809->A_REG + temp8;
				p6809->cc[C]|=((temp16 & 0x100)>>8);
				p6809->A_REG = temp16 & 0xFF;
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cpu.CycleCounter += 2;
			break;

			case ORCC_M: // 0x1A
				postbyte = cpuMemRead8(pCPU,p6809->pc.Reg++);
				temp8 = mc6809GetCC(pCPU);
				temp8 = (temp8 | postbyte);
				mc6809SetCC(pCPU,temp8);
				p6809->cpu.CycleCounter+=3;
			break;

			case ANDCC_M: // 0x1C
				postbyte = cpuMemRead8(pCPU,p6809->pc.Reg++);
				temp8 = mc6809GetCC(pCPU);
				temp8 = (temp8 & postbyte);
				mc6809SetCC(pCPU,temp8);
				p6809->cpu.CycleCounter+=3;
			break;

			case SEX_I: // 0x1D
			//	p6809->A_REG = ((signed char)B_REG < 0) ? 0xff : 0x00;
				p6809->A_REG = 0 -(p6809->B_REG>>7);
				p6809->cc[Z] = ZTEST(p6809->D_REG);
				p6809->cc[N] = NTEST16(p6809->D_REG);
				p6809->cpu.CycleCounter+=2;
			break;

			case EXG_M: // 0x1E
				postbyte = cpuMemRead8(pCPU,p6809->pc.Reg++);
				p6809->ccbits = mc6809GetCC(pCPU);
				if ( ((postbyte & 0x80)>>4)==(postbyte & 0x08)) //Verify like size registers
				{
					if (postbyte & 0x08) //8 bit EXG
					{
						temp8 = (*p6809->ureg8[((postbyte & 0x70) >> 4)]); 
						(*p6809->ureg8[((postbyte & 0x70) >> 4)]) = (*p6809->ureg8[postbyte & 0x07]);
						(*p6809->ureg8[postbyte & 0x07]) = temp8;
					}
					else // 16 bit EXG
					{
						temp16 = (*p6809->xfreg16[((postbyte & 0x70) >> 4)]);
						(*p6809->xfreg16[((postbyte & 0x70) >> 4)])=(*p6809->xfreg16[postbyte & 0x07]);
						(*p6809->xfreg16[postbyte & 0x07]) = temp16;
					}
				}
				mc6809SetCC(pCPU,p6809->ccbits);
				p6809->cpu.CycleCounter+=8;
			break;

			case TFR_M: // 0x1F
				postbyte = cpuMemRead8(pCPU,p6809->pc.Reg++);
				ucSource = postbyte>>4;
				ucDest = postbyte & 15;
				switch ( ucDest )
				{
					case 0:
					case 1:
					case 2:
					case 3:
					case 4:
					case 5:
					case 6:
					case 7:
						*p6809->xfreg16[ucDest]=0xFFFF;
						if ( (ucSource == 12) | (ucSource == 13) )
						{
							*p6809->xfreg16[ucDest]=0;
						}
						else
						{
							if ( ucSource <= 7 )
							{
								*p6809->xfreg16[ucDest] = *p6809->xfreg16[ucSource];
							}
						}
					break;

					case 8:
					case 9:
					case 10:
					case 11:
					case 14:
					case 15:
						p6809->ccbits = mc6809GetCC(pCPU);
						*p6809->ureg8[ucDest&7]=0xFF;
						if ( (ucSource==12) | (ucSource==13) )
						{
							*p6809->ureg8[ucDest&7]=0;
						}
						else
						{
							if ( ucSource > 7 )
							{
								*p6809->ureg8[ucDest&7] = *p6809->ureg8[ucSource&7];
							}
						}
						mc6809SetCC(pCPU,p6809->ccbits);
					break;
				}
				p6809->cpu.CycleCounter += 6;
			break;

			case BRA_R: // 0x20
				*spostbyte = cpuMemRead8(pCPU,p6809->pc.Reg++);
				p6809->pc.Reg += *spostbyte;
				p6809->cpu.CycleCounter += 3;
			break;

			case BRN_R: // 0x21
				p6809->cpu.CycleCounter += 3;
				p6809->pc.Reg++;
			break;

			case BHI_R: // 0x22
				if  ( !(p6809->cc[C] | p6809->cc[Z]) )
				{
					p6809->pc.Reg += (signed char)cpuMemRead8(pCPU,p6809->pc.Reg);
				}
				p6809->pc.Reg++;
				p6809->cpu.CycleCounter+=3;
			break;

			case BLS_R: //23
				if ( p6809->cc[C] | p6809->cc[Z] )
				{
					p6809->pc.Reg += (signed char)cpuMemRead8(pCPU,p6809->pc.Reg);
				}
				p6809->pc.Reg++;
				p6809->cpu.CycleCounter+=3;
			break;

			case BHS_R: //24
				if ( !p6809->cc[C] )
				{
					p6809->pc.Reg += (signed char)cpuMemRead8(pCPU,p6809->pc.Reg);
				}
				p6809->pc.Reg++;
				p6809->cpu.CycleCounter += 3;
			break;

			case BLO_R: //25
				if ( p6809->cc[C] )
				{
					p6809->pc.Reg += (signed char)cpuMemRead8(pCPU,p6809->pc.Reg);
				}
				p6809->pc.Reg++;
				p6809->cpu.CycleCounter+=3;
			break;

			case BNE_R: //26
				if ( !p6809->cc[Z] )
				{
					p6809->pc.Reg += (signed char)cpuMemRead8(pCPU,p6809->pc.Reg);
				}
				p6809->pc.Reg++;
				p6809->cpu.CycleCounter+=3;
			break;

			case BEQ_R: //27
				if ( p6809->cc[Z] )
				{
					p6809->pc.Reg += (signed char)cpuMemRead8(pCPU,p6809->pc.Reg);
				}
				p6809->pc.Reg++;
				p6809->cpu.CycleCounter+=3;
			break;

			case BVC_R: //28
				if ( !p6809->cc[V] )
				{
					p6809->pc.Reg += (signed char)cpuMemRead8(pCPU,p6809->pc.Reg);
				}
				p6809->pc.Reg++;
				p6809->cpu.CycleCounter+=3;
			break;

			case BVS_R: //29
				if ( p6809->cc[V] )
				{
					p6809->pc.Reg += (signed char)cpuMemRead8(pCPU,p6809->pc.Reg);
				}
				p6809->pc.Reg++;
				p6809->cpu.CycleCounter+=3;
			break;

			case BPL_R: //2A
				if ( !p6809->cc[N] )
				{
					p6809->pc.Reg += (signed char)cpuMemRead8(pCPU,p6809->pc.Reg);
				}
				p6809->pc.Reg++;
				p6809->cpu.CycleCounter+=3;
			break;

			case BMI_R: //2B
				if ( p6809->cc[N] )
				{
					p6809->pc.Reg += (signed char)cpuMemRead8(pCPU,p6809->pc.Reg);
				}
				p6809->pc.Reg++;
				p6809->cpu.CycleCounter+=3;
			break;

			case BGE_R: //2C
				if ( !(p6809->cc[N] ^ p6809->cc[V]))
				{
					p6809->pc.Reg += (signed char)cpuMemRead8(pCPU,p6809->pc.Reg);
				}
				p6809->pc.Reg++;
				p6809->cpu.CycleCounter+=3;
			break;

			case BLT_R: //2D
				if ( p6809->cc[V] ^ p6809->cc[N] )
				{
					p6809->pc.Reg += (signed char)cpuMemRead8(pCPU,p6809->pc.Reg);
				}
				p6809->pc.Reg++;
				p6809->cpu.CycleCounter+=3;
			break;

			case BGT_R: //2E
				if ( !(p6809->cc[Z] | (p6809->cc[N] ^ p6809->cc[V])) )
				{
					p6809->pc.Reg += (signed char)cpuMemRead8(pCPU,p6809->pc.Reg);
				}
				p6809->pc.Reg++;
				p6809->cpu.CycleCounter+=3;
			break;

			case BLE_R: //2F
				if ( p6809->cc[Z] | (p6809->cc[N] ^ p6809->cc[V]) )
				{
					p6809->pc.Reg += (signed char)cpuMemRead8(pCPU,p6809->pc.Reg);
				}
				p6809->pc.Reg++;
				p6809->cpu.CycleCounter+=3;
			break;

			case LEAX_X: //30
				p6809->x.Reg = mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++));
				p6809->cc[Z]= ZTEST(p6809->x.Reg);
				p6809->cpu.CycleCounter += 4;
			break;

			case LEAY_X: //31
				p6809->y.Reg = mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++));
				p6809->cc[Z]= ZTEST(p6809->y.Reg);
				p6809->cpu.CycleCounter += 4;
			break;

			case LEAS_X: //32
				p6809->s.Reg = mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++));
				p6809->cpu.CycleCounter+=4;
				break;

			case LEAU_X: //33
				p6809->u.Reg = mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++));
				p6809->cpu.CycleCounter += 4;
			break;

			case PSHS_M: //34
				postbyte = cpuMemRead8(pCPU,p6809->pc.Reg++);
				if ( postbyte & 0x80 )
				{
					cpuMemWrite8(pCPU, p6809->pc.B.lsb, --p6809->s.Reg);
					cpuMemWrite8(pCPU, p6809->pc.B.msb, --p6809->s.Reg);
					p6809->cpu.CycleCounter += 2;
				}
				if ( postbyte & 0x40 )
				{
					cpuMemWrite8(pCPU, p6809->u.B.lsb, --p6809->s.Reg);
					cpuMemWrite8(pCPU, p6809->u.B.msb, --p6809->s.Reg);
					p6809->cpu.CycleCounter += 2;
				}
				if ( postbyte & 0x20 )
				{
					cpuMemWrite8(pCPU, p6809->y.B.lsb, --p6809->s.Reg);
					cpuMemWrite8(pCPU, p6809->y.B.msb, --p6809->s.Reg);
					p6809->cpu.CycleCounter += 2;
				}
				if ( postbyte & 0x10 )
				{
					cpuMemWrite8(pCPU, p6809->x.B.lsb, --p6809->s.Reg);
					cpuMemWrite8(pCPU, p6809->x.B.msb, --p6809->s.Reg);
					p6809->cpu.CycleCounter += 2;
				}
				if ( postbyte & 0x08 )
				{
					cpuMemWrite8(pCPU, p6809->dp.B.msb, --p6809->s.Reg);
					p6809->cpu.CycleCounter += 1;
				}
				if ( postbyte & 0x04 )
				{
					cpuMemWrite8(pCPU,p6809->B_REG, --p6809->s.Reg);
					p6809->cpu.CycleCounter+=1;
				}
				if ( postbyte & 0x02 )
				{
					cpuMemWrite8(pCPU,p6809->A_REG, --p6809->s.Reg);
					p6809->cpu.CycleCounter += 1;
				}
				if ( postbyte & 0x01 )
				{
					cpuMemWrite8(pCPU,mc6809GetCC(pCPU), --p6809->s.Reg);
					p6809->cpu.CycleCounter += 1;
				}

				p6809->cpu.CycleCounter += 5;
			break;

			case PULS_M: //35
				postbyte = cpuMemRead8(pCPU,p6809->pc.Reg++);
				if ( postbyte & 0x01 )
				{
					mc6809SetCC(pCPU,cpuMemRead8(pCPU,p6809->s.Reg++));
					p6809->cpu.CycleCounter += 1;
				}
				if ( postbyte & 0x02 )
				{
					p6809->A_REG = cpuMemRead8(pCPU,p6809->s.Reg++);
					p6809->cpu.CycleCounter += 1;
				}
				if ( postbyte & 0x04 )
				{
					p6809->B_REG = cpuMemRead8(pCPU,p6809->s.Reg++);
					p6809->cpu.CycleCounter += 1;
				}
				if ( postbyte & 0x08 )
				{
					p6809->dp.B.msb=cpuMemRead8(pCPU,p6809->s.Reg++);
					p6809->cpu.CycleCounter += 1;
				}
				if ( postbyte & 0x10 )
				{
					p6809->x.B.msb = cpuMemRead8(pCPU,p6809->s.Reg++);
					p6809->x.B.lsb = cpuMemRead8(pCPU,p6809->s.Reg++);
					p6809->cpu.CycleCounter += 2;
				}
				if ( postbyte & 0x20 )
				{
					p6809->y.B.msb = cpuMemRead8(pCPU,p6809->s.Reg++);
					p6809->y.B.lsb = cpuMemRead8(pCPU,p6809->s.Reg++);
					p6809->cpu.CycleCounter += 2;
				}
				if ( postbyte & 0x40 )
				{
					p6809->u.B.msb=cpuMemRead8(pCPU,p6809->s.Reg++);
					p6809->u.B.lsb=cpuMemRead8(pCPU,p6809->s.Reg++);
					p6809->cpu.CycleCounter += 2;
				}
				if ( postbyte & 0x80 )
				{
					p6809->pc.B.msb = cpuMemRead8(pCPU,p6809->s.Reg++);
					p6809->pc.B.lsb = cpuMemRead8(pCPU,p6809->s.Reg++);
					p6809->cpu.CycleCounter += 2;
				}
				p6809->cpu.CycleCounter += 5;
			break;

			case PSHU_M: //36
				postbyte = cpuMemRead8(pCPU,p6809->pc.Reg++);
				if ( postbyte & 0x80 )
				{
					cpuMemWrite8(pCPU,p6809->pc.B.lsb, --p6809->u.Reg);
					cpuMemWrite8(pCPU,p6809->pc.B.msb, --p6809->u.Reg);
					p6809->cpu.CycleCounter += 2;
				}
				if ( postbyte & 0x40 )
				{
					cpuMemWrite8(pCPU,p6809->s.B.lsb, --p6809->u.Reg);
					cpuMemWrite8(pCPU,p6809->s.B.msb, --p6809->u.Reg);
					p6809->cpu.CycleCounter += 2;
				}
				if ( postbyte & 0x20 )
				{
					cpuMemWrite8(pCPU,p6809->y.B.lsb, --p6809->u.Reg);
					cpuMemWrite8(pCPU,p6809->y.B.msb, --p6809->u.Reg);
					p6809->cpu.CycleCounter += 2;
				}
				if ( postbyte & 0x10 )
				{
					cpuMemWrite8(pCPU,p6809->x.B.lsb, --p6809->u.Reg);
					cpuMemWrite8(pCPU,p6809->x.B.msb, --p6809->u.Reg);
					p6809->cpu.CycleCounter += 2;
				}
				if ( postbyte & 0x08 )
				{
					cpuMemWrite8(pCPU,p6809->dp.B.msb, --p6809->u.Reg);
					p6809->cpu.CycleCounter += 1;
				}
				if ( postbyte & 0x04 )
				{
					cpuMemWrite8(pCPU,p6809->B_REG, --p6809->u.Reg);
					p6809->cpu.CycleCounter += 1;
				}
				if ( postbyte & 0x02 )
				{
					cpuMemWrite8(pCPU,p6809->A_REG, --p6809->u.Reg);
					p6809->cpu.CycleCounter += 1;
				}
				if ( postbyte & 0x01 )
				{
					cpuMemWrite8(pCPU,mc6809GetCC(pCPU), --p6809->u.Reg);
					p6809->cpu.CycleCounter += 1;
				}
				p6809->cpu.CycleCounter += 5;
			break;

			case PULU_M: //37
				postbyte = cpuMemRead8(pCPU,p6809->pc.Reg++);
				if ( postbyte & 0x01 )
				{
					mc6809SetCC(pCPU,cpuMemRead8(pCPU,p6809->u.Reg++));
					p6809->cpu.CycleCounter += 1;
				}
				if ( postbyte & 0x02 )
				{
					p6809->A_REG = cpuMemRead8(pCPU,p6809->u.Reg++);
					p6809->cpu.CycleCounter += 1;
				}
				if ( postbyte & 0x04 )
				{
					p6809->B_REG = cpuMemRead8(pCPU,p6809->u.Reg++);
					p6809->cpu.CycleCounter += 1;
				}
				if ( postbyte & 0x08)
				{
					p6809->dp.B.msb=cpuMemRead8(pCPU,p6809->u.Reg++);
					p6809->cpu.CycleCounter += 1;
				}
				if ( postbyte & 0x10 )
				{
					p6809->x.B.msb = cpuMemRead8(pCPU,p6809->u.Reg++);
					p6809->x.B.lsb = cpuMemRead8(pCPU,p6809->u.Reg++);
					p6809->cpu.CycleCounter += 2;
				}
				if ( postbyte & 0x20 )
				{
					p6809->y.B.msb = cpuMemRead8(pCPU,p6809->u.Reg++);
					p6809->y.B.lsb = cpuMemRead8(pCPU,p6809->u.Reg++);
					p6809->cpu.CycleCounter += 2;
				}
				if ( postbyte & 0x40 )
				{
					p6809->s.B.msb = cpuMemRead8(pCPU,p6809->u.Reg++);
					p6809->s.B.lsb = cpuMemRead8(pCPU,p6809->u.Reg++);
					p6809->cpu.CycleCounter += 2;
				}
				if ( postbyte & 0x80 )
				{
					p6809->pc.B.msb = cpuMemRead8(pCPU,p6809->u.Reg++);
					p6809->pc.B.lsb = cpuMemRead8(pCPU,p6809->u.Reg++);
					p6809->cpu.CycleCounter+=2;
				}
				p6809->cpu.CycleCounter += 5;
			break;

			case RTS_I: //39
				p6809->pc.B.msb = cpuMemRead8(pCPU,p6809->s.Reg++);
				p6809->pc.B.lsb = cpuMemRead8(pCPU,p6809->s.Reg++);
				p6809->cpu.CycleCounter += 5;
			break;

			case ABX_I: //3A
				p6809->x.Reg = p6809->x.Reg + p6809->B_REG;
				p6809->cpu.CycleCounter += 3;
			break;

			case RTI_I: //3B
				mc6809SetCC(pCPU,cpuMemRead8(pCPU,p6809->s.Reg++));
				p6809->cpu.CycleCounter += 6;
				if ( p6809->cc[E] )
				{
                    p6809->ActiveInterrupts &= ~IRQ;
					p6809->A_REG = cpuMemRead8(pCPU,p6809->s.Reg++);
					p6809->B_REG = cpuMemRead8(pCPU,p6809->s.Reg++);
					p6809->dp.B.msb = cpuMemRead8(pCPU,p6809->s.Reg++);
					p6809->x.B.msb = cpuMemRead8(pCPU,p6809->s.Reg++);
					p6809->x.B.lsb = cpuMemRead8(pCPU,p6809->s.Reg++);
					p6809->y.B.msb = cpuMemRead8(pCPU,p6809->s.Reg++);
					p6809->y.B.lsb = cpuMemRead8(pCPU,p6809->s.Reg++);
					p6809->u.B.msb = cpuMemRead8(pCPU,p6809->s.Reg++);
					p6809->u.B.lsb = cpuMemRead8(pCPU,p6809->s.Reg++);
					p6809->cpu.CycleCounter += 9;
				}
                else
                {
                    p6809->ActiveInterrupts &= ~FIRQ;
                }
				p6809->pc.B.msb = cpuMemRead8(pCPU,p6809->s.Reg++);
				p6809->pc.B.lsb = cpuMemRead8(pCPU,p6809->s.Reg++);
			break;

			case CWAI_I: //3C
				postbyte = cpuMemRead8(pCPU,p6809->pc.Reg++);
				p6809->ccbits = mc6809GetCC(pCPU);
				p6809->ccbits = p6809->ccbits & postbyte;
				mc6809SetCC(pCPU,p6809->ccbits);
				p6809->cpu.CycleCounter += CycleFor;
				p6809->SyncWaiting = 1;
			break;

			case MUL_I: //3D
				p6809->D_REG = p6809->A_REG * p6809->B_REG;
				p6809->cc[C] = p6809->B_REG > 0x7F;
				p6809->cc[Z] = ZTEST(p6809->D_REG);
				p6809->cpu.CycleCounter += 11;
			break;

			case RESET:	// Undocumented
				cpuMC6809Reset(pCPU);
			break;

			case SWI1_I: //3F
				p6809->cc[E]=1;
				cpuMemWrite8(pCPU,p6809->pc.B.lsb, --p6809->s.Reg);
				cpuMemWrite8(pCPU,p6809->pc.B.msb, --p6809->s.Reg);
				cpuMemWrite8(pCPU,p6809->u.B.lsb, --p6809->s.Reg);
				cpuMemWrite8(pCPU,p6809->u.B.msb, --p6809->s.Reg);
				cpuMemWrite8(pCPU,p6809->y.B.lsb, --p6809->s.Reg);
				cpuMemWrite8(pCPU,p6809->y.B.msb, --p6809->s.Reg);
				cpuMemWrite8(pCPU,p6809->x.B.lsb, --p6809->s.Reg);
				cpuMemWrite8(pCPU,p6809->x.B.msb, --p6809->s.Reg);
				cpuMemWrite8(pCPU,p6809->dp.B.msb, --p6809->s.Reg);
				cpuMemWrite8(pCPU,p6809->B_REG, --p6809->s.Reg);
				cpuMemWrite8(pCPU,p6809->A_REG, --p6809->s.Reg);
				cpuMemWrite8(pCPU,mc6809GetCC(pCPU), --p6809->s.Reg);
				p6809->pc.Reg = cpuMemRead16(&p6809->cpu,VSWI);
				p6809->cpu.CycleCounter += 19;
				p6809->cc[I]=1;
				p6809->cc[F]=1;
			break;

			case NEGA_I: //40
				temp8 = 0 - p6809->A_REG;
				p6809->cc[C] = temp8 > 0;
				p6809->cc[V] = (p6809->A_REG == 0x80); //p6809->cc[C] ^ ((p6809->A_REG^temp8)>>7);
				p6809->cc[N] = NTEST8(temp8);
				p6809->cc[Z] = ZTEST(temp8);
				p6809->A_REG = temp8;
				p6809->cpu.CycleCounter += 2;
			break;

			case COMA_I: //43
				p6809->A_REG = 0xFF - p6809->A_REG;
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cc[C] = 1;
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 2;
			break;

			case LSRA_I: //44
				p6809->cc[C] = p6809->A_REG & 1;
				p6809->A_REG = p6809->A_REG >> 1;
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cc[N] = 0;
				p6809->cpu.CycleCounter += 2;
			break;

			case RORA_I: //46
				postbyte = p6809->cc[C]<<7;
				p6809->cc[C] = p6809->A_REG & 1;
				p6809->A_REG = (p6809->A_REG >> 1) | postbyte;
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cpu.CycleCounter += 2;
			break;

			case ASRA_I: //47
				p6809->cc[C] = p6809->A_REG & 1;
				p6809->A_REG = (p6809->A_REG & 0x80) | (p6809->A_REG >> 1);
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cpu.CycleCounter += 2;
			break;

			case ASLA_I: //48 JF
				p6809->cc[C] = p6809->A_REG > 0x7F;
				p6809->cc[V] = p6809->cc[C] ^((p6809->A_REG & 0x40)>>6);
				p6809->A_REG = p6809->A_REG << 1;
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cpu.CycleCounter += 2;
			break;

			case ROLA_I: //49
				postbyte = p6809->cc[C];
				p6809->cc[C] = p6809->A_REG > 0x7F;
				p6809->cc[V] = p6809->cc[C] ^ ((p6809->A_REG & 0x40) >> 6);
				p6809->A_REG = (p6809->A_REG<<1) | postbyte;
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cpu.CycleCounter += 2;
			break;

			case DECA_I: //4A
				p6809->A_REG--;
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cc[V] = (p6809->A_REG == 0x7F);
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cpu.CycleCounter += 2;
			break;

			case INCA_I: //4C
				p6809->A_REG++;
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cc[V] = p6809->A_REG == 0x80;
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cpu.CycleCounter += 2;
			break;

			case TSTA_I: //4D
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 2;
			break;

			case CLRA_I: //4F
				p6809->A_REG = 0;
				p6809->cc[C] = 0;
				p6809->cc[V] = 0;
				p6809->cc[N] = 0;
				p6809->cc[Z] = 1;
				p6809->cpu.CycleCounter += 2;
			break;

			case NEGB_I: //50
				temp8 = 0 - p6809->B_REG;
				p6809->cc[C] = temp8 > 0;
				p6809->cc[V] = p6809->B_REG == 0x80;	
				p6809->cc[N] = NTEST8(temp8);
				p6809->cc[Z] = ZTEST(temp8);
				p6809->B_REG = temp8;
				p6809->cpu.CycleCounter += 2;
			break;

			case COMB_I: //53
				p6809->B_REG = 0xFF - p6809->B_REG;
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cc[C] = 1;
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 2;
			break;

			case LSRB_I: //54
				p6809->cc[C] = p6809->B_REG & 1;
				p6809->B_REG = p6809->B_REG >> 1;
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cc[N] = 0;
				p6809->cpu.CycleCounter += 2;
			break;

			case RORB_I: //56
				postbyte = p6809->cc[C] << 7;
				p6809->cc[C] = p6809->B_REG & 1;
				p6809->B_REG = (p6809->B_REG>>1) | postbyte;
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cpu.CycleCounter += 2;
			break;

			case ASRB_I: //57
				p6809->cc[C] = p6809->B_REG & 1;
				p6809->B_REG = (p6809->B_REG & 0x80) | (p6809->B_REG >> 1);
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cpu.CycleCounter += 2;
			break;

			case ASLB_I: //58
				p6809->cc[C] = p6809->B_REG > 0x7F;
				p6809->cc[V] =  p6809->cc[C] ^((p6809->B_REG & 0x40) >> 6);
				p6809->B_REG = p6809->B_REG << 1;
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cpu.CycleCounter += 2;
			break;

			case ROLB_I: //59
				postbyte = p6809->cc[C];
				p6809->cc[C] = p6809->B_REG > 0x7F;
				p6809->cc[V] = p6809->cc[C] ^ ((p6809->B_REG & 0x40)>>6);
				p6809->B_REG = (p6809->B_REG<<1) | postbyte;
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cpu.CycleCounter += 2;
			break;

			case DECB_I: //5A
				p6809->B_REG--;
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cc[V] = p6809->B_REG == 0x7F;
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cpu.CycleCounter += 2;
			break;

			case INCB_I: //5C
				p6809->B_REG++;
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cc[V] = p6809->B_REG == 0x80; 
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cpu.CycleCounter += 2;
			break;

			case TSTB_I: //5D
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 2;
			break;

			case CLRB_I: //5F
				p6809->B_REG = 0;
				p6809->cc[C] = 0;
				p6809->cc[N] = 0;
				p6809->cc[V] = 0;
				p6809->cc[Z] = 1;
				p6809->cpu.CycleCounter += 2;
			break;

			case NEG_X: //60
				temp16 = mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++));	
				postbyte = cpuMemRead8(pCPU,temp16);
				temp8 = 0 - postbyte;
				p6809->cc[C]= temp8 > 0;
				p6809->cc[V]= postbyte == 0x80;
				p6809->cc[N]= NTEST8(temp8);
				p6809->cc[Z]= ZTEST(temp8);
				cpuMemWrite8(pCPU,temp8,temp16);
				p6809->cpu.CycleCounter += 6;
			break;

			case COM_X: //63
				temp16 = mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++));
				temp8 = cpuMemRead8(pCPU,temp16);
				temp8 = 0xFF - temp8;
				p6809->cc[Z] = ZTEST(temp8);
				p6809->cc[N] = NTEST8(temp8);
				p6809->cc[V] = 0;
				p6809->cc[C] = 1;
				cpuMemWrite8(pCPU,temp8,temp16);
				p6809->cpu.CycleCounter += 6;
			break;

			case LSR_X: //64
				temp16 = mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++));
				temp8 = cpuMemRead8(pCPU,temp16);
				p6809->cc[C] = temp8 & 1;
				temp8 = temp8 >>1;
				p6809->cc[Z] = ZTEST(temp8);
				p6809->cc[N] = 0;
				cpuMemWrite8(pCPU,temp8,temp16);
				p6809->cpu.CycleCounter += 6;
			break;

			case ROR_X: //66
				temp16 = mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++));
				temp8 = cpuMemRead8(pCPU,temp16);
				postbyte = p6809->cc[C] << 7;
				p6809->cc[C] = (temp8 & 1);
				temp8 = (temp8 >> 1) | postbyte;
				p6809->cc[Z] = ZTEST(temp8);
				p6809->cc[N] = NTEST8(temp8);
				cpuMemWrite8(pCPU,temp8,temp16);
				p6809->cpu.CycleCounter += 6;
			break;

			case ASR_X: //67
				temp16 = mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++));
				temp8 = cpuMemRead8(pCPU,temp16);
				p6809->cc[C] = temp8 & 1;
				temp8 = (temp8 & 0x80) | (temp8 >>1);
				p6809->cc[Z]= ZTEST(temp8);
				p6809->cc[N]= NTEST8(temp8);
				cpuMemWrite8(pCPU,temp8,temp16);
				p6809->cpu.CycleCounter += 6;
			break;

			case ASL_X: //68
				temp16 = mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++));
				temp8 = cpuMemRead8(pCPU,temp16);
				p6809->cc[C] = temp8 > 0x7F;
				p6809->cc[V] = p6809->cc[C] ^ ((temp8 & 0x40)>>6);
				temp8 = temp8 << 1;
				p6809->cc[N] = NTEST8(temp8);
				p6809->cc[Z] = ZTEST(temp8);	
				cpuMemWrite8(pCPU,temp8,temp16);
				p6809->cpu.CycleCounter+=6;
			break;

			case ROL_X: //69
				temp16 = mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++));
				temp8 = cpuMemRead8(pCPU,temp16);
				postbyte = p6809->cc[C];
				p6809->cc[C]= temp8 > 0x7F;
				p6809->cc[V]= (p6809->cc[C] ^ ((temp8 & 0x40)>>6));
				temp8= ((temp8 << 1) | postbyte);
				p6809->cc[Z] = ZTEST(temp8);
				p6809->cc[N] = NTEST8(temp8);
				cpuMemWrite8(pCPU,temp8,temp16);
				p6809->cpu.CycleCounter+=6;
			break;

			case DEC_X: //6A
				temp16 = mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++));
				temp8 = cpuMemRead8(pCPU,temp16);
				temp8--;
				p6809->cc[Z] = ZTEST(temp8);
				p6809->cc[N] = NTEST8(temp8);
				p6809->cc[V] = (temp8==0x7F);
				cpuMemWrite8(pCPU,temp8,temp16);
				p6809->cpu.CycleCounter+=6;
			break;

			case INC_X: //6C
				temp16 = mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++));
				temp8 = cpuMemRead8(pCPU,temp16);
				temp8++;
				p6809->cc[V] = (temp8 == 0x80);
				p6809->cc[N] = NTEST8(temp8);
				p6809->cc[Z] = ZTEST(temp8);
				cpuMemWrite8(pCPU,temp8,temp16);
				p6809->cpu.CycleCounter+=6;
			break;

			case TST_X: //6D
				temp8 = cpuMemRead8(pCPU,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[Z]= ZTEST(temp8);
				p6809->cc[N]= NTEST8(temp8);
				p6809->cc[V]= 0;
				p6809->cpu.CycleCounter += 6;
			break;

			case JMP_X: //6E
				p6809->pc.Reg = mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++));
				p6809->cpu.CycleCounter += 3;
			break;

			case CLR_X: //6F
				cpuMemWrite8(pCPU,0,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[C]= 0;
				p6809->cc[N]= 0;
				p6809->cc[V]= 0;
				p6809->cc[Z]= 1; 
				p6809->cpu.CycleCounter += 6;
			break;

			case NEG_E: //70
				temp16 = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
				postbyte = cpuMemRead8(pCPU,temp16);
				temp8 = 0 - postbyte;
				p6809->cc[C] = temp8 > 0;
				p6809->cc[V] = postbyte == 0x80;
				p6809->cc[N] = NTEST8(temp8);
				p6809->cc[Z] = ZTEST(temp8);
				cpuMemWrite8(pCPU,temp8,temp16);
				p6809->pc.Reg+=2;
				p6809->cpu.CycleCounter+=7;
			break;

			case COM_E: //73
				temp16 = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
				temp8 = cpuMemRead8(pCPU,temp16);
				temp8 = 0xFF - temp8;
				p6809->cc[Z]= ZTEST(temp8);
				p6809->cc[N]= NTEST8(temp8);
				p6809->cc[C]= 1;
				p6809->cc[V]= 0;
				cpuMemWrite8(pCPU,temp8,temp16);
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 7;
			break;

			case LSR_E:  //74
				temp16 = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
				temp8 = cpuMemRead8(pCPU,temp16);
				p6809->cc[C] = temp8 & 1;
				temp8 = temp8 >> 1;
				p6809->cc[Z] = ZTEST(temp8);
				p6809->cc[N] = 0;
				cpuMemWrite8(pCPU,temp8,temp16);
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 7;
			break;

			case ROR_E: //76
				temp16 = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
				temp8 = cpuMemRead8(pCPU,temp16);
				postbyte = p6809->cc[C] << 7;
				p6809->cc[C] = temp8 & 1;
				temp8 = (temp8 >> 1) | postbyte;
				p6809->cc[Z] = ZTEST(temp8);
				p6809->cc[N] = NTEST8(temp8);
				cpuMemWrite8(pCPU,temp8,temp16);
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 7;
			break;

			case ASR_E: //77
				temp16 = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
				temp8 = cpuMemRead8(pCPU,temp16);
				p6809->cc[C] = temp8 & 1;
				temp8 = (temp8 & 0x80) | (temp8 >>1);
				p6809->cc[Z] = ZTEST(temp8);
				p6809->cc[N] = NTEST8(temp8);
				cpuMemWrite8(pCPU,temp8,temp16);
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 7;
			break;

			case ASL_E: //78
				temp16 = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
				temp8 = cpuMemRead8(pCPU,temp16);
				p6809->cc[C] = temp8 > 0x7F;
				p6809->cc[V] = p6809->cc[C] ^ ((temp8 & 0x40)>>6);
				temp8 = temp8 << 1;
				p6809->cc[N] = NTEST8(temp8);
				p6809->cc[Z] = ZTEST(temp8);
				cpuMemWrite8(pCPU,temp8,temp16);	
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 7;
			break;

			case ROL_E: //79
				temp16 = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
				temp8 = cpuMemRead8(pCPU,temp16);
				postbyte = p6809->cc[C];
				p6809->cc[C] = temp8 > 0x7F;
				p6809->cc[V] = p6809->cc[C] ^  ((temp8 & 0x40)>>6);
				temp8 = ((temp8<<1) | postbyte);
				p6809->cc[Z] = ZTEST(temp8);
				p6809->cc[N] = NTEST8(temp8);
				cpuMemWrite8(pCPU,temp8,temp16);
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 7;
			break;

			case DEC_E: //7A
				temp16 = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
				temp8 = cpuMemRead8(pCPU,temp16);
				temp8--;
				p6809->cc[Z] = ZTEST(temp8);
				p6809->cc[N] = NTEST8(temp8);
				p6809->cc[V] = temp8 == 0x7F;
				cpuMemWrite8(pCPU,temp8,temp16);
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 7;
			break;

			case INC_E: //7C
				temp16 = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
				temp8 = cpuMemRead8(pCPU,temp16);
				temp8++;
				p6809->cc[Z]= ZTEST(temp8);
				p6809->cc[V]= temp8==0x80;
				p6809->cc[N]= NTEST8(temp8);
				cpuMemWrite8(pCPU,temp8,temp16);
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 7;
			break;

			case TST_E: //7D
				temp8 = cpuMemRead8(pCPU,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
				p6809->cc[Z] = ZTEST(temp8);
				p6809->cc[N] = NTEST8(temp8);
				p6809->cc[V] = 0;
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 7;
			break;

			case JMP_E: //7E
				p6809->pc.Reg = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
				p6809->cpu.CycleCounter += 4;
			break;

			case CLR_E: //7F
				cpuMemWrite8(pCPU,0,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
				p6809->cc[C]= 0;
				p6809->cc[N]= 0;
				p6809->cc[V]= 0;
				p6809->cc[Z]= 1;
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 7;
			break;

			case SUBA_M: //80
				postbyte = cpuMemRead8(pCPU,p6809->pc.Reg++);
				temp16 = p6809->A_REG - postbyte;
				p6809->cc[C] = (temp16 & 0x100) >> 8; 
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp16,p6809->A_REG);
				p6809->A_REG =(temp16 & 0xFF);
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cpu.CycleCounter += 2;
			break;

			case CMPA_M: //81
				postbyte = cpuMemRead8(pCPU,p6809->pc.Reg++);
				temp8 = p6809->A_REG - postbyte;
				p6809->cc[C] = temp8 > p6809->A_REG;
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp8,p6809->A_REG);
				p6809->cc[N] = NTEST8(temp8);
				p6809->cc[Z] = ZTEST(temp8);
				p6809->cpu.CycleCounter += 2;
			break;

			case SBCA_M:  //82
				postbyte = cpuMemRead8(pCPU,p6809->pc.Reg++);
				temp16 = p6809->A_REG - postbyte - p6809->cc[C];
				p6809->cc[C]= (temp16 & 0x100) >> 8;
				p6809->cc[V]= OTEST8(p6809->cc[C],postbyte,temp16,p6809->A_REG);
				p6809->A_REG = (temp16 & 0xFF);
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cpu.CycleCounter += 2;
			break;

			case SUBD_M: //83
				temp16 = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
				temp32 = p6809->D_REG - temp16;
				p6809->cc[C] = (temp32 & 0x10000)>>16;
				p6809->cc[V] = OTEST16(p6809->cc[C],temp32,temp16,p6809->D_REG);
				p6809->D_REG = (temp32 & 0xFFFF);
				p6809->cc[Z] = ZTEST(p6809->D_REG);
				p6809->cc[N] = NTEST16(p6809->D_REG);
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 4;
			break;

			case ANDA_M: //84
				p6809->A_REG = p6809->A_REG & cpuMemRead8(pCPU,p6809->pc.Reg++);
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter+=2;
			break;

			case BITA_M: //85
				temp8 = p6809->A_REG & cpuMemRead8(pCPU,p6809->pc.Reg++);
				p6809->cc[N] = NTEST8(temp8);
				p6809->cc[Z] = ZTEST(temp8);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 2;
			break;

			case LDA_M: //86
				p6809->A_REG = cpuMemRead8(pCPU,p6809->pc.Reg++);
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 2;
			break;

			case EORA_M: //88
				p6809->A_REG = p6809->A_REG ^ cpuMemRead8(pCPU,p6809->pc.Reg++);
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 2;
			break;

			case ADCA_M: //89
				postbyte = cpuMemRead8(pCPU,p6809->pc.Reg++);
				temp16 = p6809->A_REG + postbyte + p6809->cc[C];
				p6809->cc[C] = (temp16 & 0x100)>>8;
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp16,p6809->A_REG);
				p6809->cc[H] = ((p6809->A_REG ^ temp16 ^ postbyte) & 0x10)>>4;
				p6809->A_REG = (temp16 & 0xFF);
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cpu.CycleCounter += 2;
			break;	

			case ORA_M: //8A
				p6809->A_REG = p6809->A_REG | cpuMemRead8(pCPU,p6809->pc.Reg++);
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 2;
			break;

			case ADDA_M: //8B
				postbyte = cpuMemRead8(pCPU,p6809->pc.Reg++);
				temp16 = p6809->A_REG + postbyte;
				p6809->cc[C] = (temp16 & 0x100)>>8;
				p6809->cc[H] = ((p6809->A_REG ^ postbyte ^ temp16) & 0x10) >> 4;
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp16,p6809->A_REG);
				p6809->A_REG = (temp16 & 0xFF);
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cpu.CycleCounter += 2;
			break;

			case CMPX_M: //8C
				postword = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
				temp16 = p6809->x.Reg - postword;
				p6809->cc[C] = temp16 > p6809->x.Reg;
				p6809->cc[V] = OTEST16(p6809->cc[C],postword,temp16,p6809->X_REG);
				p6809->cc[N] = NTEST16(temp16);
				p6809->cc[Z] = ZTEST(temp16);
				p6809->pc.Reg+=2;
				p6809->cpu.CycleCounter+=4;
			break;

			case BSR_R: //8D
				*spostbyte = cpuMemRead8(pCPU,p6809->pc.Reg++);
				p6809->s.Reg--;
				cpuMemWrite8(pCPU,p6809->pc.B.lsb, p6809->s.Reg--);
				cpuMemWrite8(pCPU,p6809->pc.B.msb, p6809->s.Reg);
				p6809->pc.Reg += *spostbyte;
				p6809->cpu.CycleCounter+=7;
			break;

			case LDX_M: //8E
				p6809->x.Reg = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
				p6809->cc[Z] = ZTEST(p6809->x.Reg);
				p6809->cc[N] = NTEST16(p6809->x.Reg);
				p6809->cc[V] = 0;
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 3;
			break;

			case SUBA_D: //90
				postbyte = cpuMemRead8(pCPU,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				temp16 = p6809->A_REG - postbyte;
				p6809->cc[C] =(temp16 & 0x100)>>8; 
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp16,p6809->A_REG);
				p6809->A_REG =(temp16 & 0xFF);
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cpu.CycleCounter += 4;
			break;	
 
			case CMPA_D: //91
				postbyte = cpuMemRead8(pCPU,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				temp8 = p6809->A_REG - postbyte;
				p6809->cc[C] = temp8 > p6809->A_REG;
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp8,p6809->A_REG);
				p6809->cc[N] = NTEST8(temp8);
				p6809->cc[Z] = ZTEST(temp8);
				p6809->cpu.CycleCounter += 4;
			break;

			case SBCA_D: //92
				postbyte = cpuMemRead8(pCPU,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				temp16 = p6809->A_REG - postbyte - p6809->cc[C];
				p6809->cc[C] = (temp16 & 0x100) >> 8;
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp16,p6809->A_REG);
				p6809->A_REG = (temp16 & 0xFF);
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cpu.CycleCounter += 4;
			break;

			case SUBD_D: //93
				temp16 = cpuMemRead16(&p6809->cpu,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				temp32 = p6809->D_REG - temp16;
				p6809->cc[C] = (temp32 & 0x10000) >> 16;
				p6809->cc[V] = OTEST16(p6809->cc[C],temp32,temp16,p6809->D_REG);
				p6809->D_REG = (temp32 & 0xFFFF);
				p6809->cc[Z] = ZTEST(p6809->D_REG);
				p6809->cc[N] = NTEST16(p6809->D_REG);
				p6809->cpu.CycleCounter += 6;
			break;

			case ANDA_D: //94
				p6809->A_REG = p6809->A_REG & cpuMemRead8(pCPU,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[N]= NTEST8(p6809->A_REG);
				p6809->cc[Z]= ZTEST(p6809->A_REG);
				p6809->cc[V]= 0;
				p6809->cpu.CycleCounter+=4;
			break;

			case BITA_D: //95
				temp8 = p6809->A_REG & cpuMemRead8(pCPU,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[N]= NTEST8(temp8);
				p6809->cc[Z]= ZTEST(temp8);
				p6809->cc[V]= 0;
				p6809->cpu.CycleCounter += 4;
			break;

			case LDA_D: //96
				p6809->A_REG = cpuMemRead8(pCPU,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 4;
			break;

			case STA_D: //97
				cpuMemWrite8(pCPU,p6809->A_REG,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 4;
			break;

			case EORA_D: //98
				p6809->A_REG = p6809->A_REG ^ cpuMemRead8(pCPU,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 4;
			break;

			case ADCA_D: //99
				postbyte = cpuMemRead8(pCPU,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				temp16 = p6809->A_REG + postbyte + p6809->cc[C];
				p6809->cc[C] = (temp16 & 0x100) >> 8;
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp16,p6809->A_REG);
				p6809->cc[H] = ((p6809->A_REG ^ temp16 ^ postbyte) & 0x10)>>4;
				p6809->A_REG = (temp16 & 0xFF);
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cpu.CycleCounter += 4;
			break;

			case ORA_D: //9A
				p6809->A_REG = p6809->A_REG | cpuMemRead8(pCPU,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 4;
			break;

			case ADDA_D: //9B
				postbyte = cpuMemRead8(pCPU,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				temp16 = p6809->A_REG + postbyte;
				p6809->cc[C] = (temp16 & 0x100)>>8;
				p6809->cc[H] = ((p6809->A_REG ^ postbyte ^ temp16) & 0x10)>>4;
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp16,p6809->A_REG);
				p6809->A_REG =(temp16 & 0xFF);
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cpu.CycleCounter += 4;
			break;

			case CMPX_D: //9C
				postword = cpuMemRead16(&p6809->cpu,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				temp16 = p6809->x.Reg - postword ;
				p6809->cc[C] = temp16 > p6809->x.Reg;
				p6809->cc[V] = OTEST16(p6809->cc[C],postword,temp16,p6809->X_REG);
				p6809->cc[N] = NTEST16(temp16);
				p6809->cc[Z] = ZTEST(temp16);
				p6809->cpu.CycleCounter += 6;
			break;

			case BSR_D: //9D
				temp16 = (p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++));
				p6809->s.Reg--;
				cpuMemWrite8(pCPU,p6809->pc.B.lsb, p6809->s.Reg--);
				cpuMemWrite8(pCPU,p6809->pc.B.msb, p6809->s.Reg);
				p6809->pc.Reg = temp16;
				p6809->cpu.CycleCounter += 7;
			break;

			case LDX_D: //9E
				p6809->x.Reg = cpuMemRead16(&p6809->cpu,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[Z] = ZTEST(p6809->x.Reg);	
				p6809->cc[N] = NTEST16(p6809->x.Reg);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 5;
			break;

			case STX_D: //9F
				cpuMemWrite16(&p6809->cpu,p6809->x.Reg,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[Z] = ZTEST(p6809->x.Reg);
				p6809->cc[N] = NTEST16(p6809->x.Reg);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 5;
			break;

			case SUBA_X: //A0
				postbyte = cpuMemRead8(pCPU,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
				temp16 = p6809->A_REG - postbyte;
				p6809->cc[C] =(temp16 & 0x100)>>8; 
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp16,p6809->A_REG);
				p6809->A_REG = (temp16 & 0xFF);
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cpu.CycleCounter += 4;
			break;	


			case CMPA_X: //A1
				postbyte = cpuMemRead8(pCPU,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
				temp8 = p6809->A_REG - postbyte;
				p6809->cc[C] = temp8 > p6809->A_REG;
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp8,p6809->A_REG);
				p6809->cc[N] = NTEST8(temp8);
				p6809->cc[Z] = ZTEST(temp8);
				p6809->cpu.CycleCounter += 4;
			break;

			case SBCA_X: //A2
				postbyte = cpuMemRead8(pCPU,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
				temp16 = p6809->A_REG - postbyte - p6809->cc[C];
				p6809->cc[C] = (temp16 & 0x100)>>8;
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp16,p6809->A_REG);
				p6809->A_REG = (temp16 & 0xFF);
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cpu.CycleCounter += 4;
			break;

			case SUBD_X: //A3
				temp16 = cpuMemRead16(&p6809->cpu,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
				temp32 = p6809->D_REG - temp16;
				p6809->cc[C] = (temp32 & 0x10000)>>16;
				p6809->cc[V] = OTEST16(p6809->cc[C],temp32,temp16,p6809->D_REG);
				p6809->D_REG = (temp32 & 0xFFFF);
				p6809->cc[Z] = ZTEST(p6809->D_REG);
				p6809->cc[N] = NTEST16(p6809->D_REG);
				p6809->cpu.CycleCounter += 6;
			break;

			case ANDA_X: //A4
				p6809->A_REG = p6809->A_REG & cpuMemRead8(pCPU,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 4;
			break;

			case BITA_X:  //A5
				temp8 = p6809->A_REG & cpuMemRead8(pCPU,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[N] = NTEST8(temp8);
				p6809->cc[Z] = ZTEST(temp8);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 4;
			break;

			case LDA_X: //A6
				p6809->A_REG = cpuMemRead8(pCPU,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 4;
			break;

			case STA_X: //A7
				cpuMemWrite8(pCPU,p6809->A_REG,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 4;
			break;

			case EORA_X: //A8
				p6809->A_REG = p6809->A_REG ^ cpuMemRead8(pCPU,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 4;
			break;

			case ADCA_X: //A9
				postbyte = cpuMemRead8(pCPU,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
				temp16 = p6809->A_REG + postbyte + p6809->cc[C];
				p6809->cc[C] = (temp16 & 0x100)>>8;
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp16,p6809->A_REG);
				p6809->cc[H] = ((p6809->A_REG ^ temp16 ^ postbyte) & 0x10)>>4;
				p6809->A_REG = (temp16 & 0xFF);
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cpu.CycleCounter += 4;
			break;

			case ORA_X: //AA
				p6809->A_REG = p6809->A_REG | cpuMemRead8(pCPU,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 4;
			break;

			case ADDA_X: //AB
				postbyte = cpuMemRead8(pCPU,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
				temp16 = p6809->A_REG + postbyte;
				p6809->cc[C] =(temp16 & 0x100)>>8;
				p6809->cc[H] = ((p6809->A_REG ^ postbyte ^ temp16) & 0x10)>>4;
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp16,p6809->A_REG);
				p6809->A_REG = (temp16 & 0xFF);
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cpu.CycleCounter += 4;
			break;

			case CMPX_X: //AC
				postword = cpuMemRead16(&p6809->cpu,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
				temp16 = p6809->x.Reg - postword ;
				p6809->cc[C] = temp16 > p6809->x.Reg;
				p6809->cc[V] = OTEST16(p6809->cc[C],postword,temp16,p6809->X_REG);
				p6809->cc[N] = NTEST16(temp16);
				p6809->cc[Z] = ZTEST(temp16);
				p6809->cpu.CycleCounter += 6;
			break;

			case BSR_X: //AD
				temp16 = mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++));
				p6809->s.Reg--;
				cpuMemWrite8(pCPU,p6809->pc.B.lsb, p6809->s.Reg--);
				cpuMemWrite8(pCPU,p6809->pc.B.msb, p6809->s.Reg);
				p6809->pc.Reg = temp16;
				p6809->cpu.CycleCounter += 7;
			break;

			case LDX_X: //AE
				p6809->x.Reg = cpuMemRead16(&p6809->cpu,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[Z] = ZTEST(p6809->x.Reg);
				p6809->cc[N] = NTEST16(p6809->x.Reg);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 5;
			break;

			case STX_X: //AF
				cpuMemWrite16(&p6809->cpu,p6809->x.Reg,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[Z] = ZTEST(p6809->x.Reg);
				p6809->cc[N] = NTEST16(p6809->x.Reg);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 5;
			break;

			case SUBA_E: //B0
				postbyte = cpuMemRead8(pCPU,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
				temp16 = p6809->A_REG - postbyte;
				p6809->cc[C] =(temp16 & 0x100)>>8; 
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp16,p6809->A_REG);
				p6809->A_REG = (temp16 & 0xFF);
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 5;
			break;

			case CMPA_E: //B1
				postbyte = cpuMemRead8(pCPU,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
				temp8 = p6809->A_REG - postbyte;
				p6809->cc[C] = temp8 > p6809->A_REG;
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp8,p6809->A_REG);
				p6809->cc[N] = NTEST8(temp8);
				p6809->cc[Z ]= ZTEST(temp8);
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 5;
			break;

			case SBCA_E: //B2
				postbyte = cpuMemRead8(pCPU,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
				temp16 = p6809->A_REG - postbyte - p6809->cc[C];
				p6809->cc[C] = (temp16 & 0x100)>>8;
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp16,p6809->A_REG);
				p6809->A_REG = (temp16 & 0xFF);
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 5;
			break;

			case SUBD_E: //B3
				temp16 = cpuMemRead16(&p6809->cpu,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
				temp32 = p6809->D_REG - temp16;
				p6809->cc[C] = (temp32 & 0x10000)>>16;
				p6809->cc[V] = OTEST16(p6809->cc[C],temp32,temp16,p6809->D_REG);
				p6809->D_REG = (temp32 & 0xFFFF);
				p6809->cc[Z] = ZTEST(p6809->D_REG);
				p6809->cc[N] = NTEST16(p6809->D_REG);
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 7;
			break;

			case ANDA_E: //B4
				postbyte = cpuMemRead8(pCPU,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
				p6809->A_REG = p6809->A_REG & postbyte;
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cc[V] = 0;
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 5;
			break;

			case BITA_E: //B5
				temp8 = p6809->A_REG & cpuMemRead8(pCPU,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
				p6809->cc[N] = NTEST8(temp8);
				p6809->cc[Z] = ZTEST(temp8);
				p6809->cc[V] = 0;
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 5;
			break;

			case LDA_E: //B6
				p6809->A_REG = cpuMemRead8(pCPU,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cc[V] = 0;
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 5;
			break;

			case STA_E: //B7
				cpuMemWrite8(pCPU,p6809->A_REG,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cc[V] = 0;
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 5;
			break;

			case EORA_E:  //B8
				p6809->A_REG = p6809->A_REG ^ cpuMemRead8(pCPU,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cc[V] = 0;
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 5;
			break;

			case ADCA_E: //B9
				postbyte = cpuMemRead8(pCPU,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
				temp16 = p6809->A_REG + postbyte + p6809->cc[C];
				p6809->cc[C] = (temp16 & 0x100)>>8;
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp16,p6809->A_REG);
				p6809->cc[H] = ((p6809->A_REG ^ temp16 ^ postbyte) & 0x10)>>4;
				p6809->A_REG = (temp16 & 0xFF);
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 5;
			break;

			case ORA_E: //BA
				p6809->A_REG = p6809->A_REG | cpuMemRead8(pCPU,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
				p6809->cc[N] = NTEST8(p6809->A_REG);
				p6809->cc[Z] = ZTEST(p6809->A_REG);
				p6809->cc[V] = 0;
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 5;
			break;

			case ADDA_E: //BB
				postbyte = cpuMemRead8(pCPU,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
				temp16 = p6809->A_REG + postbyte;
				p6809->cc[C]= (temp16 & 0x100) >> 8;
				p6809->cc[H]= ((p6809->A_REG ^ postbyte ^ temp16) & 0x10)>>4;
				p6809->cc[V]= OTEST8(p6809->cc[C],postbyte,temp16,p6809->A_REG);
				p6809->A_REG= (temp16 & 0xFF);
				p6809->cc[N]= NTEST8(p6809->A_REG);
				p6809->cc[Z]= ZTEST(p6809->A_REG);
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 5;
			break;

			case CMPX_E: //BC
				postword=cpuMemRead16(&p6809->cpu,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
				temp16 = p6809->x.Reg - postword;
				p6809->cc[C]= temp16 > p6809->x.Reg;
				p6809->cc[V]= OTEST16(p6809->cc[C],postword,temp16,p6809->X_REG);
				p6809->cc[N]= NTEST16(temp16);
				p6809->cc[Z]= ZTEST(temp16);
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 7;
			break;

			case BSR_E: //BD
				postword = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
				p6809->pc.Reg += 2;
				p6809->s.Reg--;
				cpuMemWrite8(pCPU,p6809->pc.B.lsb, p6809->s.Reg--);
				cpuMemWrite8(pCPU,p6809->pc.B.msb, p6809->s.Reg);
				p6809->pc.Reg = postword;
				p6809->cpu.CycleCounter += 8;
			break;

			case LDX_E: //BE
				p6809->x.Reg = cpuMemRead16(&p6809->cpu,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
				p6809->cc[Z] = ZTEST(p6809->x.Reg);
				p6809->cc[N] = NTEST16(p6809->x.Reg);
				p6809->cc[V] = 0;
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 6;
			break;

			case STX_E: //BF
				cpuMemWrite16(&p6809->cpu,p6809->x.Reg,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
				p6809->cc[Z]= ZTEST(p6809->x.Reg);
				p6809->cc[N]= NTEST16(p6809->x.Reg);
				p6809->cc[V]= 0;
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 6;
			break;

			case SUBB_M: //C0
				postbyte=cpuMemRead8(pCPU,p6809->pc.Reg++);
				temp16 = p6809->B_REG - postbyte;
				p6809->cc[C] = (temp16 & 0x100) >> 8; 
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp16,p6809->B_REG);
				p6809->B_REG = (temp16 & 0xFF);
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cpu.CycleCounter += 2;
			break;

			case CMPB_M: //C1
				postbyte = cpuMemRead8(pCPU,p6809->pc.Reg++);
				temp8 = p6809->B_REG - postbyte;
				p6809->cc[C] = temp8 > p6809->B_REG;
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp8,p6809->B_REG);
				p6809->cc[N] = NTEST8(temp8);
				p6809->cc[Z] = ZTEST(temp8);
				p6809->cpu.CycleCounter += 2;
			break;

			case SBCB_M: //C3
				postbyte = cpuMemRead8(pCPU,p6809->pc.Reg++);
				temp16 = p6809->B_REG - postbyte - p6809->cc[C];
				p6809->cc[C] = (temp16 & 0x100)>>8;
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp16,p6809->B_REG);
				p6809->B_REG = (temp16 & 0xFF);
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cpu.CycleCounter += 2;
			break;

			case ADDD_M: //C3
				temp16 = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
				temp32 = p6809->D_REG + temp16;
				p6809->cc[C] = (temp32 & 0x10000)>>16;
				p6809->cc[V] = OTEST16(p6809->cc[C],temp32,temp16,p6809->D_REG);
				p6809->D_REG = (temp32 & 0xFFFF);
				p6809->cc[Z] = ZTEST(p6809->D_REG);
				p6809->cc[N] = NTEST16(p6809->D_REG);
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 4;
			break;

			case ANDB_M: //C4
			//	postbyte=cpuMemRead8(pCPU,p6809->pc.Reg++);
				p6809->B_REG = p6809->B_REG & cpuMemRead8(pCPU,p6809->pc.Reg++);
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter+=2;
			break;

			case BITB_M: //C5
				temp8 = p6809->B_REG & cpuMemRead8(pCPU,p6809->pc.Reg++);
				p6809->cc[N] = NTEST8(temp8);
				p6809->cc[Z] = ZTEST(temp8);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 2;
			break;

			case LDB_M: //C6
				p6809->B_REG =cpuMemRead8(pCPU,p6809->pc.Reg++);
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 2;
			break;

			case EORB_M: //C8
			//	postbyte=cpuMemRead8(pCPU,p6809->pc.Reg++);
				p6809->B_REG = p6809->B_REG ^ cpuMemRead8(pCPU,p6809->pc.Reg++);
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 2;
			break;

			case ADCB_M: //C9
				postbyte = cpuMemRead8(pCPU,p6809->pc.Reg++);
				temp16 = p6809->B_REG + postbyte + p6809->cc[C];
				p6809->cc[C] = (temp16 & 0x100)>>8;
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp16,p6809->B_REG);
				p6809->cc[H] = ((p6809->B_REG ^ temp16 ^ postbyte) & 0x10) >> 4;
				p6809->B_REG = (temp16 & 0xFF);
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cpu.CycleCounter += 2;
			break;

			case ORB_M: //CA
			//	postbyte=cpuMemRead8(pCPU,p6809->pc.Reg++);
				p6809->B_REG = p6809->B_REG | cpuMemRead8(pCPU,p6809->pc.Reg++);
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 2;
			break;

			case ADDB_M: //CB
				postbyte = cpuMemRead8(pCPU,p6809->pc.Reg++);
				temp16 = p6809->B_REG + postbyte;
				p6809->cc[C] = (temp16 & 0x100)>>8;
				p6809->cc[H] = ((p6809->B_REG ^ postbyte ^ temp16) & 0x10) >> 4;
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp16,p6809->B_REG);
				p6809->B_REG = (temp16 & 0xFF);
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cpu.CycleCounter += 2;
			break;

			case LDD_M: //CC
				p6809->D_REG = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
				p6809->cc[Z] = ZTEST(p6809->D_REG);
				p6809->cc[N] = NTEST16(p6809->D_REG);
				p6809->cc[V] = 0;
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 3;
			break;

			case LDU_M: //CE
				p6809->u.Reg = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
				p6809->cc[Z] = ZTEST(p6809->u.Reg);
				p6809->cc[N] = NTEST16(p6809->u.Reg);
				p6809->cc[V] = 0;
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 3;
			break;

			case SUBB_D: //D0
				postbyte = cpuMemRead8(pCPU, (p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				temp16 = p6809->B_REG - postbyte;
				p6809->cc[C] = (temp16 & 0x100) >> 8; 
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp16,p6809->B_REG);
				p6809->B_REG = (temp16 & 0xFF);
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cpu.CycleCounter += 4;
			break;

			case CMPB_D: //D1
				postbyte = cpuMemRead8(pCPU,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				temp8 = p6809->B_REG - postbyte;
				p6809->cc[C] = temp8 > p6809->B_REG;
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp8,p6809->B_REG);
				p6809->cc[N] = NTEST8(temp8);
				p6809->cc[Z] = ZTEST(temp8);
				p6809->cpu.CycleCounter += 4;
			break;

			case SBCB_D: //D2
				postbyte = cpuMemRead8(pCPU,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				temp16 = p6809->B_REG - postbyte - p6809->cc[C];
				p6809->cc[C] = (temp16 & 0x100)>>8;
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp16,p6809->B_REG);
				p6809->B_REG = (temp16 & 0xFF);
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cpu.CycleCounter += 4;
			break;

			case ADDD_D: //D3
				temp16 = cpuMemRead16(&p6809->cpu, (p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				temp32 = p6809->D_REG + temp16;
				p6809->cc[C] = (temp32 & 0x10000)>>16;
				p6809->cc[V] = OTEST16(p6809->cc[C],temp32,temp16,p6809->D_REG);
				p6809->D_REG = (temp32 & 0xFFFF);
				p6809->cc[Z] = ZTEST(p6809->D_REG);
				p6809->cc[N] = NTEST16(p6809->D_REG);
				p6809->cpu.CycleCounter += 6;
			break;

			case ANDB_D: //D4 
			//	postbyte = cpuMemRead8(pCPU,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->B_REG = p6809->B_REG & cpuMemRead8(pCPU,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 4;
			break;

			case BITB_D: //D5
				temp8 = p6809->B_REG & cpuMemRead8(pCPU,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[N] = NTEST8(temp8);
				p6809->cc[Z] = ZTEST(temp8);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 4;
			break;

			case LDB_D: //D6
				p6809->B_REG = cpuMemRead8(pCPU, (p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 4;
			break;

			case STB_D: //D7
				cpuMemWrite8(pCPU,p6809->B_REG,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 4;
				break;

			case EORB_D: //D8	
			//	postbyte = cpuMemRead8(pCPU,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->B_REG = p6809->B_REG ^ cpuMemRead8(pCPU,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 4;
			break;

			case ADCB_D: //D9
				postbyte = cpuMemRead8(pCPU,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				temp16 = p6809->B_REG + postbyte + p6809->cc[C];
				p6809->cc[C] = (temp16 & 0x100)>>8;
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp16,p6809->B_REG);
				p6809->cc[H] = ((p6809->B_REG ^ temp16 ^ postbyte) & 0x10) >> 4;
				p6809->B_REG = (temp16 & 0xFF);
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cpu.CycleCounter += 4;
			break;

			case ORB_D: //DA
			//	postbyte = cpuMemRead8(pCPU,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->B_REG = p6809->B_REG | cpuMemRead8(pCPU,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 4;
			break;

			case ADDB_D: //DB
				postbyte = cpuMemRead8(pCPU,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				temp16 = p6809->B_REG + postbyte;
				p6809->cc[C] = (temp16 & 0x100)>>8;
				p6809->cc[H] = ((p6809->B_REG ^ postbyte ^ temp16) & 0x10) >> 4;
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp16,p6809->B_REG);
				p6809->B_REG = (temp16 & 0xFF);
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cpu.CycleCounter += 4;
			break;

			case LDD_D: //DC
				p6809->D_REG = cpuMemRead16(&p6809->cpu,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[Z] = ZTEST(p6809->D_REG);
				p6809->cc[N] = NTEST16(p6809->D_REG);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 5;
			break;

			case STD_D: //DD
				cpuMemWrite16(&p6809->cpu,p6809->D_REG,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[Z] = ZTEST(p6809->D_REG);
				p6809->cc[N] = NTEST16(p6809->D_REG);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 5;
			break;

			case LDU_D: //DE
				p6809->u.Reg = cpuMemRead16(&p6809->cpu,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[Z] = ZTEST(p6809->u.Reg);
				p6809->cc[N] = NTEST16(p6809->u.Reg);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 5;
			break;

			case STU_D: //DF
				cpuMemWrite16(&p6809->cpu,p6809->u.Reg,(p6809->dp.Reg | cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[Z] = ZTEST(p6809->u.Reg);
				p6809->cc[N] = NTEST16(p6809->u.Reg);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 5;
			break;

			case SUBB_X: //E0
				postbyte = cpuMemRead8(pCPU,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
				temp16 = p6809->B_REG - postbyte;
				p6809->cc[C] = (temp16 & 0x100) >> 8; 
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp16,p6809->B_REG);
				p6809->B_REG = (temp16 & 0xFF);
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cpu.CycleCounter += 4;
			break;

			case CMPB_X: //E1
				postbyte = cpuMemRead8(pCPU,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
				temp8 = p6809->B_REG - postbyte;
				p6809->cc[C] = temp8 > p6809->B_REG;
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp8,p6809->B_REG);
				p6809->cc[N] = NTEST8(temp8);
				p6809->cc[Z] = ZTEST(temp8);
				p6809->cpu.CycleCounter += 4;
			break;

			case SBCB_X: //E2
				postbyte = cpuMemRead8(pCPU,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
				temp16 = p6809->B_REG - postbyte - p6809->cc[C];
				p6809->cc[C] = (temp16 & 0x100)>>8;
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp16,p6809->B_REG);
				p6809->B_REG = (temp16 & 0xFF);
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cpu.CycleCounter += 4;
			break;

			case ADDD_X: //E3 
				temp16 = cpuMemRead16(&p6809->cpu,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
				temp32 = p6809->D_REG + temp16;
				p6809->cc[C] = (temp32 & 0x10000)>>16;
				p6809->cc[V] = OTEST16(p6809->cc[C],temp32,temp16,p6809->D_REG);
				p6809->D_REG = (temp32 & 0xFFFF);
				p6809->cc[Z] = ZTEST(p6809->D_REG);
				p6809->cc[N] = NTEST16(p6809->D_REG);
				p6809->cpu.CycleCounter += 6;
			break;

			case ANDB_X: //E4
			//	postbyte=cpuMemRead8(pCPU,mc6809CalculateEA(cpuMemRead8(pCPU,pc.Reg++)));
				p6809->B_REG = p6809->B_REG & cpuMemRead8(pCPU,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter+=4;
			break;

			case BITB_X: //E5 
				temp8 = p6809->B_REG & cpuMemRead8(pCPU,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[N] = NTEST8(temp8);
				p6809->cc[Z] = ZTEST(temp8);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 4;
			break;

			case LDB_X: //E6
				p6809->B_REG = cpuMemRead8(pCPU,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 4;
			break;

			case STB_X: //E7
			//	postbyte=cpuMemRead8(pCPU,p6809->pc.Reg++);
				cpuMemWrite8(pCPU,p6809->B_REG,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 4;
			break;

			case EORB_X: //E8
			//	postbyte = cpuMemRead8(pCPU,p6809->pc.Reg++);
			//	temp16 = mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++));
				temp8 = cpuMemRead8(pCPU,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->B_REG = p6809->B_REG ^ temp8;
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter+=4;
			break;

			case ADCB_X: //E9
				postbyte = cpuMemRead8(pCPU,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
				temp16 = p6809->B_REG + postbyte + p6809->cc[C];
				p6809->cc[C] = (temp16 & 0x100)>>8;
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp16,p6809->B_REG);
				p6809->cc[H] = ((p6809->B_REG ^ temp16 ^ postbyte) & 0x10)>>4;
				p6809->B_REG = (temp16 & 0xFF);
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cpu.CycleCounter += 4;
			break;

			case ORB_X: //EA 
			//	postbyte = cpuMemRead8(pCPU,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->B_REG = p6809->B_REG | cpuMemRead8(pCPU,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 4;
				break;

			case ADDB_X: //EB
				postbyte = cpuMemRead8(pCPU,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
				temp16 = p6809->B_REG + postbyte;
				p6809->cc[C] =(temp16 & 0x100)>>8;
				p6809->cc[H] = ((p6809->B_REG ^ postbyte ^ temp16) & 0x10) >> 4;
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp16,p6809->B_REG);
				p6809->B_REG = (temp16 & 0xFF);
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cpu.CycleCounter += 4;
			break;

			case LDD_X: //EC
			//	postbyte = cpuMemRead8(pCPU,p6809->pc.Reg++);
				temp16 = mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++));
				p6809->D_REG = cpuMemRead16(&p6809->cpu,temp16);
				p6809->cc[Z] = ZTEST(p6809->D_REG);
				p6809->cc[N] = NTEST16(p6809->D_REG);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 5;
			break;

			case STD_X: //ED
			//	postbyte = cpuMemRead8(pCPU,p6809->pc.Reg++);
			//	temp16 = mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++));
				cpuMemWrite16(&p6809->cpu,p6809->D_REG,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[Z] = ZTEST(p6809->D_REG);
				p6809->cc[N] = NTEST16(p6809->D_REG);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 5;
			break;

			case LDU_X: //EE
				p6809->u.Reg = cpuMemRead16(&p6809->cpu,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[Z] = ZTEST(p6809->u.Reg);
				p6809->cc[N] = NTEST16(p6809->u.Reg);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 5;
			break;

			case STU_X: //EF
			//	postbyte=cpuMemRead8(pCPU,p6809->pc.Reg++);
			//	temp16=mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++));
				cpuMemWrite16(&p6809->cpu,p6809->u.Reg,mc6809CalculateEA(pCPU,cpuMemRead8(pCPU,p6809->pc.Reg++)));
				p6809->cc[Z] = ZTEST(p6809->u.Reg);
				p6809->cc[N] = NTEST16(p6809->u.Reg);
				p6809->cc[V] = 0;
				p6809->cpu.CycleCounter += 5;
			break;

			case SUBB_E: //F0
				postbyte = cpuMemRead8(pCPU,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
				temp16 = p6809->B_REG - postbyte;
				p6809->cc[C] = (temp16 & 0x100)>>8; 
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp16,p6809->B_REG);
				p6809->B_REG = (temp16 & 0xFF);
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 5;
			break;

			case CMPB_E: //F1
				postbyte=cpuMemRead8(pCPU,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
				temp8 = p6809->B_REG - postbyte;
				p6809->cc[C] = temp8 > p6809->B_REG;
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp8,p6809->B_REG);
				p6809->cc[N] = NTEST8(temp8);
				p6809->cc[Z] = ZTEST(temp8);
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 5;
			break;

			case SBCB_E: //F2
				postbyte = cpuMemRead8(pCPU,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
				temp16 = p6809->B_REG - postbyte - p6809->cc[C];
				p6809->cc[C] = (temp16 & 0x100)>>8;
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp16,p6809->B_REG);
				p6809->B_REG = (temp16 & 0xFF);
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 5;
			break;

			case ADDD_E: //F3
				temp16 = cpuMemRead16(&p6809->cpu,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
				temp32 = p6809->D_REG + temp16;
				p6809->cc[C] =(temp32 & 0x10000)>>16;
				p6809->cc[V] = OTEST16(p6809->cc[C],temp32,temp16,p6809->D_REG);
				p6809->D_REG = (temp32 & 0xFFFF);
				p6809->cc[Z] = ZTEST(p6809->D_REG);
				p6809->cc[N] = NTEST16(p6809->D_REG);
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 7;
			break;

			case ANDB_E:  //F4
			//	postbyte = cpuMemRead8(pCPU,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
				p6809->B_REG = p6809->B_REG & cpuMemRead8(pCPU,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cc[V] = 0;
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 5;
			break;

			case BITB_E: //F5
				temp8 = p6809->B_REG & cpuMemRead8(pCPU,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
				p6809->cc[N] = NTEST8(temp8);
				p6809->cc[Z] = ZTEST(temp8);
				p6809->cc[V] = 0;
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 5;
			break;

			case LDB_E: //F6
				p6809->B_REG = cpuMemRead8(pCPU,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cc[N]= NTEST8(p6809->B_REG);
				p6809->cc[V] = 0;
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 5;
			break;

			case STB_E: //F7 
				cpuMemWrite8(pCPU,p6809->B_REG,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cc[V] = 0;
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 5;
			break;

			case EORB_E: //F8
			//	postbyte=cpuMemRead8(pCPU,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
				p6809->B_REG = p6809->B_REG ^ cpuMemRead8(pCPU,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cc[V] = 0;
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 5;
			break;

			case ADCB_E: //F9
				postbyte = cpuMemRead8(pCPU,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
				temp16 = p6809->B_REG + postbyte + p6809->cc[C];
				p6809->cc[C] = (temp16 & 0x100)>>8;
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp16,p6809->B_REG);
				p6809->cc[H] = ((p6809->B_REG ^ temp16 ^ postbyte) & 0x10) >> 4;
				p6809->B_REG = (temp16 & 0xFF);
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 5;
			break;

			case ORB_E: //FA
			//	postbyte=cpuMemRead8(pCPU,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
				p6809->B_REG = p6809->B_REG | cpuMemRead8(pCPU,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->cc[V] = 0;
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 5;
			break;

			case ADDB_E: //FB
				postbyte = cpuMemRead8(pCPU,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
				temp16 = p6809->B_REG + postbyte;
				p6809->cc[C] = (temp16 & 0x100) >> 8;
				p6809->cc[H] = ((p6809->B_REG ^ postbyte ^ temp16) & 0x10) >> 4;
				p6809->cc[V] = OTEST8(p6809->cc[C],postbyte,temp16,p6809->B_REG);
				p6809->B_REG = (temp16 & 0xFF);
				p6809->cc[N] = NTEST8(p6809->B_REG);
				p6809->cc[Z] = ZTEST(p6809->B_REG);
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 5;
			break;

			case LDD_E: //FC
				p6809->D_REG = cpuMemRead16(&p6809->cpu,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
				p6809->cc[Z] = ZTEST(p6809->D_REG);
				p6809->cc[N] = NTEST16(p6809->D_REG);
				p6809->cc[V] = 0;
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 6;
			break;

			case STD_E: //FD
				cpuMemWrite16(&p6809->cpu,p6809->D_REG,cpuMemRead16(&p6809->cpu,p6809->pc.Reg));
				p6809->cc[Z] = ZTEST(p6809->D_REG);
				p6809->cc[N] = NTEST16(p6809->D_REG);
				p6809->cc[V] = 0;
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 6;
			break;

			case LDU_E: //FE
				postword = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
				//assert(postword <= 0xFFFF);
				p6809->u.Reg = cpuMemRead16(&p6809->cpu,postword);
				p6809->cc[Z] = ZTEST(p6809->u.Reg);
				p6809->cc[N] = NTEST16(p6809->u.Reg);
				p6809->cc[V] = 0;
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 6;
			break;

			case STU_E: //FF
				postword = cpuMemRead16(&p6809->cpu,p6809->pc.Reg);
				//assert(postword <= 0xFFFF);
				cpuMemWrite16(&p6809->cpu,p6809->u.Reg,postword);
				p6809->cc[Z] = ZTEST(p6809->u.Reg);
				p6809->cc[N] = NTEST16(p6809->u.Reg);
				p6809->cc[V] = 0;
				p6809->pc.Reg += 2;
				p6809->cpu.CycleCounter += 6;
			break;

			default:
				//MessageBox(0,"Unhandled Op","Ok",0);
			break;
		}//End Switch
	}//End While

	return (endCycle - p6809->cpu.CycleCounter);
}

/********************************************************************************/

#pragma mark -
#pragma mark --- exported plug-in functions ---

/********************************************************************************/

XAPI cpu_t * vcccpuModuleCreate()
{
	cpu6809_t *		p6809;
	
	p6809 = calloc(1,sizeof(cpu6809_t));
	assert(p6809 != NULL);
	if ( p6809 != NULL )
	{
		p6809->cpu.device.id		= EMU_DEVICE_ID;
		p6809->cpu.device.idModule	= EMU_CPU_ID;
		p6809->cpu.device.idDevice	= VCC_MC6809_ID;
		strncpy(p6809->cpu.device.Name,"MC6809",sizeof(p6809->cpu.device.Name)-1);
		
		p6809->cpu.device.pfnDestroy	= mc6809EmuDevDestroy;
		p6809->cpu.device.pfnSave		= mc6809EmuDevConfSave;
		p6809->cpu.device.pfnLoad		= mc6809EmuDevConfLoad;
		
		p6809->cpu.cpuInit				= cpuMC6809Init;
		p6809->cpu.cpuExec				= cpuMC6809Exec;
		p6809->cpu.cpuReset				= cpuMC6809Reset;
		p6809->cpu.cpuAssertInterrupt	= cpuMC6809AssertInterrupt;
		p6809->cpu.cpuDeAssertInterrupt	= cpuMC6809DeAssertInterrupt;
		p6809->cpu.cpuForcePC			= cpuMC6809ForcePC;
		p6809->cpu.cpuDestructor		= NULL;
		
		cpuInit(&p6809->cpu);
		
		return &p6809->cpu;
	}
	
	assert(0 && "out of memory");
	
	return NULL;
}

/*****************************************************************/

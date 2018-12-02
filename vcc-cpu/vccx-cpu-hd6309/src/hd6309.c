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

/**************************************************************************/
/*
	HD6309 cpu module
 
	TODO: save/load of CPU state
	TODO: check out claimed unemulated functions - search for 'WriteLog'
 */
/**************************************************************************/

#include "hd6309.h"
#include "hd6309defs.h"

#include "xDebug.h"

/**************************************************************************/

// turn into callbacks?
void InvalidInsHandler(cpu_t * pCPU);
void DivbyZero(cpu_t * pCPU);
void ErrorVector(cpu_t * pCPU);
int performDivQ(cpu_t * pCPU,short divisor);

/********************************************************************************/

#pragma mark -
#pragma mark --- local macros ---

/********************************************************************************/

#define ASSERT_HD6309(pInstance)	assert(pInstance != NULL);	\
									assert(pInstance->cpu.device.id == EMU_DEVICE_ID); \
									assert(pInstance->cpu.device.idModule == EMU_CPU_ID); \
									assert(pInstance->cpu.device.idDevice == VCC_HD6309_ID)

#define NTEST8(r)			((r) > 0x7F)
#define NTEST16(r)			((r) > 0x7FFF)
#define NTEST32(r)			((r) > 0x7FFFFFFF)
#define OVERFLOW8(c,a,b,r)	((c) ^ ((((a)^(b)^(r))>>7) &1))
#define OVERFLOW16(c,a,b,r)	((c) ^ ((((a)^(b)^(r))>>15)&1))
#define ZTEST(r)			(!(r))

#define DPADDRESS(p6309,r)	(p6309->dp.Reg | cpuMemRead8(pCPU,r))
#define IMMADDRESS(p6309,r)	cpuMemRead16(&p6309->cpu,r)
#define INDADDRESS(pCPU,r)	CalculateEA(pCPU,cpuMemRead8(pCPU,r))

#define M65		0
#define M64		1
#define M32		2
#define M21		3
#define M54		4
#define M97		5
#define M85		6
#define M51		7
#define M31		8
#define M1110	9
#define M76		10
#define M75		11
#define M43		12
#define M87		13
#define M86		14
#define M98		15
#define M2726	16
#define M3635	17
#define M3029	18
#define M2827	19
#define M3726	20
#define M3130	21

#define D_REG	q.Word.lsw
#define W_REG	q.Word.msw
#define PC_REG	pc.Reg
#define X_REG	x.Reg
#define Y_REG	y.Reg
#define U_REG	u.Reg
#define S_REG	s.Reg
#define A_REG	q.Byte.lswmsb
#define B_REG	q.Byte.lswlsb
#define E_REG	q.Byte.mswmsb
#define F_REG	q.Byte.mswlsb	
#define Q_REG	q.Reg
#define V_REG	v.Reg
#define O_REG	z.Reg

/********************************************************************************/

#pragma mark -
#pragma mark --- local definitions ---

/********************************************************************************/

typedef union
{
	unsigned short Reg;
	struct
	{
		unsigned char lsb,msb;
	} B;
} cpuregister;

typedef union
{
	unsigned int Reg;
	struct
	{
		unsigned short msw,lsw;
	} Word;
	struct
	{
		unsigned char mswlsb,mswmsb,lswlsb,lswmsb;	//Might be backwards
	} Byte;
} wideregister;

typedef struct cpu6309_t
{
	cpu_t	cpu;
	
	wideregister		q;
	cpuregister			pc,x,y,u,s,dp,v,z;
	unsigned char		ccbits;
	unsigned char		mdbits;
    unsigned int		cc[8];
    unsigned int		md[8];
    unsigned char *		ureg8[8];
    unsigned short *	xfreg16[8];
    
	int		SyncWaiting;
	int		PendingInterrupts;
	int		IRQWaiter;
	int		InInterrupt;
    
	unsigned int		InsCycles[2][25];
} cpu6309_t;

/********************************************************************************/

#pragma mark -
#pragma mark --- forward declarations ---

/********************************************************************************/

/********************************************************************************/

#pragma mark -
#pragma mark --- local functions ---

/********************************************************************************/

static unsigned short CalculateEA(cpu_t * pCPU, unsigned char postbyte)
{
	static unsigned short int	ea		= 0;
	static signed char			byte	= 0;
	static unsigned char		Register;
	cpu6309_t *					p6309	= (cpu6309_t *)pCPU;
	
	assert(p6309 != NULL);
	
	Register= ((postbyte >> 5) & 3) + 1;
	
	if ( postbyte & 0x80 ) 
	{
		switch ( postbyte & 0x1F )
		{
			case 0:
				ea=(*p6309->xfreg16[Register]);
				(*p6309->xfreg16[Register])++;
				p6309->cpu.CycleCounter+=2;
				break;
				
			case 1:
				ea=(*p6309->xfreg16[Register]);
				(*p6309->xfreg16[Register])+=2;
				p6309->cpu.CycleCounter+=3;
				break;
				
			case 2:
				(*p6309->xfreg16[Register])-=1;
				ea=(*p6309->xfreg16[Register]);
				p6309->cpu.CycleCounter+=2;
				break;
				
			case 3:
				(*p6309->xfreg16[Register])-=2;
				ea=(*p6309->xfreg16[Register]);
				p6309->cpu.CycleCounter+=3;
				break;
				
			case 4:
				ea=(*p6309->xfreg16[Register]);
				break;
				
			case 5:
				ea = (*p6309->xfreg16[Register])+((signed char)p6309->B_REG);
				p6309->cpu.CycleCounter+=1;
				break;
				
			case 6:
				ea=(*p6309->xfreg16[Register])+((signed char)p6309->A_REG);
				p6309->cpu.CycleCounter+=1;
				break;
				
			case 7:
				ea=(*p6309->xfreg16[Register])+((signed char)p6309->E_REG);
				p6309->cpu.CycleCounter+=1;
				break;
				
			case 8:
				ea=(*p6309->xfreg16[Register])+(signed char)cpuMemRead8(pCPU,p6309->PC_REG++);
				p6309->cpu.CycleCounter+=1;
				break;
				
			case 9:
				ea=(*p6309->xfreg16[Register])+IMMADDRESS(p6309,p6309->PC_REG);
				p6309->cpu.CycleCounter+=4;
				p6309->PC_REG+=2;
				break;
				
			case 10:
				ea=(*p6309->xfreg16[Register])+((signed char)p6309->F_REG);
				p6309->cpu.CycleCounter+=1;
				break;
				
			case 11:
				ea=(*p6309->xfreg16[Register])+p6309->D_REG; //Changed to unsigned 03/14/2005 NG Was signed
				p6309->cpu.CycleCounter+=4;
				break;
				
			case 12:
				ea=(signed short)p6309->PC_REG+(signed char)cpuMemRead8(pCPU,p6309->PC_REG)+1; 
				p6309->cpu.CycleCounter+=1;
				p6309->PC_REG++;
				break;
				
			case 13: //MM
				ea=p6309->PC_REG+IMMADDRESS(p6309,p6309->PC_REG)+2; 
				p6309->cpu.CycleCounter+=5;
				p6309->PC_REG+=2;
				break;
				
			case 14:
				ea=(*p6309->xfreg16[Register])+p6309->W_REG; 
				p6309->cpu.CycleCounter+=4;
				break;
				
			case 15: //01111
				byte=(postbyte>>5)&3;
				switch (byte)
			{
				case 0:
					ea=p6309->W_REG;
					break;
				case 1:
					ea=p6309->W_REG+IMMADDRESS(p6309,p6309->PC_REG);
					p6309->PC_REG+=2;
					break;
				case 2:
					ea=p6309->W_REG;
					p6309->W_REG+=2;
					break;
				case 3:
					p6309->W_REG-=2;
					ea=p6309->W_REG;
					break;
			}
				break;
				
			case 16: //10000
				byte=(postbyte>>5)&3;
				switch (byte)
			{
				case 0:
					ea=cpuMemRead16(&p6309->cpu,p6309->W_REG);
					break;
				case 1:
					ea=cpuMemRead16(&p6309->cpu,p6309->W_REG+IMMADDRESS(p6309,p6309->PC_REG));
					p6309->PC_REG+=2;
					break;
				case 2:
					ea=cpuMemRead16(&p6309->cpu,p6309->W_REG);
					p6309->W_REG+=2;
					break;
				case 3:
					p6309->W_REG-=2;
					ea=cpuMemRead16(&p6309->cpu,p6309->W_REG);
					break;
			}
				break;
				
				
			case 17: //10001
				ea=(*p6309->xfreg16[Register]);
				(*p6309->xfreg16[Register])+=2;
				ea=cpuMemRead16(&p6309->cpu,ea);
				p6309->cpu.CycleCounter+=6;
				break;
				
			case 18: //10010
				p6309->cpu.CycleCounter+=6;
				break;
				
			case 19: //10011
				(*p6309->xfreg16[Register])-=2;
				ea=(*p6309->xfreg16[Register]);
				ea=cpuMemRead16(&p6309->cpu,ea);
				p6309->cpu.CycleCounter+=6;
				break;
				
			case 20: //10100
				ea=(*p6309->xfreg16[Register]);
				ea=cpuMemRead16(&p6309->cpu,ea);
				p6309->cpu.CycleCounter+=3;
				break;
				
			case 21: //10101
				ea=(*p6309->xfreg16[Register])+((signed char)p6309->B_REG);
				ea=cpuMemRead16(&p6309->cpu,ea);
				p6309->cpu.CycleCounter+=4;
				break;
				
			case 22: //10110
				ea=(*p6309->xfreg16[Register])+((signed char)p6309->A_REG);
				ea=cpuMemRead16(&p6309->cpu,ea);
				p6309->cpu.CycleCounter+=4;
				break;
				
			case 23: //10111
				ea=(*p6309->xfreg16[Register])+((signed char)p6309->E_REG);
				ea=cpuMemRead16(&p6309->cpu,ea);
				p6309->cpu.CycleCounter+=4;
				break;
				
			case 24: //11000
				ea=(*p6309->xfreg16[Register])+(signed char)cpuMemRead8(pCPU,p6309->PC_REG++);
				ea=cpuMemRead16(&p6309->cpu,ea);
				p6309->cpu.CycleCounter+=4;
				break;
				
			case 25: //11001
				ea=(*p6309->xfreg16[Register])+IMMADDRESS(p6309,p6309->PC_REG);
				ea=cpuMemRead16(&p6309->cpu,ea);
				p6309->cpu.CycleCounter+=7;
				p6309->PC_REG+=2;
				break;
			case 26: //11010
				ea=(*p6309->xfreg16[Register])+((signed char)p6309->F_REG);
				ea=cpuMemRead16(&p6309->cpu,ea);
				p6309->cpu.CycleCounter+=4;
				break;
				
			case 27: //11011
				ea=(*p6309->xfreg16[Register])+p6309->D_REG;
				ea=cpuMemRead16(&p6309->cpu,ea);
				p6309->cpu.CycleCounter+=7;
				break;
				
			case 28: //11100
				ea=(signed short)p6309->PC_REG+(signed char)cpuMemRead8(pCPU,p6309->PC_REG)+1; 
				ea=cpuMemRead16(&p6309->cpu,ea);
				p6309->cpu.CycleCounter+=4;
				p6309->PC_REG++;
				break;
				
			case 29: //11101
				ea=p6309->PC_REG+IMMADDRESS(p6309,p6309->PC_REG)+2; 
				ea=cpuMemRead16(&p6309->cpu,ea);
				p6309->cpu.CycleCounter+=8;
				p6309->PC_REG+=2;
				break;
				
			case 30: //11110
				ea=(*p6309->xfreg16[Register])+p6309->W_REG; 
				ea=cpuMemRead16(&p6309->cpu,ea);
				p6309->cpu.CycleCounter+=7;
				break;
				
			case 31: //11111
				ea=IMMADDRESS(p6309,p6309->PC_REG);
				ea=cpuMemRead16(&p6309->cpu,ea);
				p6309->cpu.CycleCounter+=8;
				p6309->PC_REG+=2;
				break;
				
		} //END Switch
	}
	else 
	{
		byte= (postbyte & 31);
		byte= (byte << 3);
		byte= byte /8;
		ea= *p6309->xfreg16[Register]+byte; //Was signed
		p6309->cpu.CycleCounter+=1;
	}
	return(ea);
}


static void setcc(cpu_t * pCPU, unsigned char bincc)
{
	unsigned char	bit;
	cpu6309_t *		p6309 = (cpu6309_t *)pCPU;
	
	assert(p6309 != NULL);
	
	for (bit=0; bit<=7; bit++)
	{
		p6309->cc[bit] = !!(bincc & (1<<bit));
	}
}

static unsigned char getcc(cpu_t * pCPU)
{
	unsigned char bincc=0,bit=0;
	cpu6309_t *	p6309 = (cpu6309_t *)pCPU;
	
	assert(p6309 != NULL);
	
	for (bit=0;bit<=7;bit++)
		if ( p6309->cc[bit] )
			bincc=bincc | (1<<bit);
	
	return (bincc);
}

static void setmd(cpu_t * pCPU, unsigned char binmd)
{
	unsigned char bit;
	cpu6309_t *	p6309 = (cpu6309_t *)pCPU;
	
	assert(p6309 != NULL);
	
	for (bit=0;bit<=7;bit++)
		p6309->md[bit] = !!(binmd & (1<<bit));
}

static unsigned char getmd(cpu_t * pCPU)
{
	unsigned char binmd=0,bit=0;
	cpu6309_t *	p6309 = (cpu6309_t *)pCPU;
	
	assert(p6309 != NULL);
	
	for (bit=0;bit<=7;bit++)
		if ( p6309->md[bit] )
			binmd=binmd | (1<<bit);
	return(binmd);
}

void InvalidInsHandler(cpu_t * pCPU)
{	
	cpu6309_t *	p6309 = (cpu6309_t *)pCPU;
	
	assert(p6309 != NULL);
	
	p6309->md[ILLEGAL ]= 1;
	p6309->mdbits = getmd(pCPU);
	ErrorVector(pCPU);
}

void DivbyZero(cpu_t * pCPU)
{
	cpu6309_t *	p6309 = (cpu6309_t *)pCPU;
	
	assert(p6309 != NULL);
	
	p6309->md[ZERODIV] = 1;
	p6309->mdbits=getmd(pCPU);
	ErrorVector(pCPU);
}

void ErrorVector(cpu_t * pCPU)
{
	cpu6309_t *	p6309 = (cpu6309_t *)pCPU;
	
	assert(p6309 != NULL);
	
	p6309->cc[E]=1;
	cpuMemWrite8(pCPU, p6309->pc.B.lsb, --p6309->S_REG);
	cpuMemWrite8(pCPU, p6309->pc.B.msb, --p6309->S_REG);
	cpuMemWrite8(pCPU, p6309->u.B.lsb, --p6309->S_REG);
	cpuMemWrite8(pCPU, p6309->u.B.msb, --p6309->S_REG);
	cpuMemWrite8(pCPU, p6309->y.B.lsb, --p6309->S_REG);
	cpuMemWrite8(pCPU, p6309->y.B.msb, --p6309->S_REG);
	cpuMemWrite8(pCPU, p6309->x.B.lsb, --p6309->S_REG);
	cpuMemWrite8(pCPU, p6309->x.B.msb, --p6309->S_REG);
	cpuMemWrite8(pCPU, p6309->dp.B.msb, --p6309->S_REG);
	if ( p6309->md[NATIVE6309] )
	{
		cpuMemWrite8(pCPU,(p6309->F_REG),--p6309->S_REG);
		cpuMemWrite8(pCPU,(p6309->E_REG),--p6309->S_REG);
		p6309->cpu.CycleCounter+=2;
	}
	cpuMemWrite8(pCPU,p6309->B_REG,--p6309->S_REG);
	cpuMemWrite8(pCPU,p6309->A_REG,--p6309->S_REG);
	cpuMemWrite8(pCPU,getcc(pCPU),--p6309->S_REG);
	p6309->PC_REG=cpuMemRead16(&p6309->cpu,VTRAP);
	p6309->cpu.CycleCounter+=(12 + p6309->InsCycles[p6309->md[NATIVE6309]][M54]);	// One for each byte +overhead? Guessing from PSHS
}

#if 0 // unused
static unsigned char GetSorceReg(cpu_t * pCPU, unsigned char Tmp)
{
	unsigned char Source=(Tmp>>4);
	unsigned char Dest= Tmp & 15;
	//cpu6309_t *	p6309 = (cpu6309_t *)pCPU;
	
	//assert(p6309 != NULL);
	
	//unsigned char Translate[]={0,0};
	if ( (Source & 8) == (Dest & 8) ) //like size registers
		return(Source );
	
	return(0);
}
#endif

static void cpu_firq(cpu_t * pCPU)
{
	cpu6309_t *	p6309 = (cpu6309_t *)pCPU;
	
	assert(p6309 != NULL);
	
	if ( !p6309->cc[F] )
	{
		p6309->InInterrupt=1; //Flag to indicate FIRQ has been asserted
		switch ( p6309->md[FIRQMODE] )
		{
			case 0:
				p6309->cc[E]=0; // Turn E flag off
				cpuMemWrite8(pCPU, p6309->pc.B.lsb,--p6309->S_REG);
				cpuMemWrite8(pCPU, p6309->pc.B.msb,--p6309->S_REG);
				cpuMemWrite8(pCPU,getcc(pCPU),--p6309->S_REG);
				p6309->cc[I]=1;
				p6309->cc[F]=1;
				p6309->PC_REG=cpuMemRead16(&p6309->cpu,VFIRQ);
				break;
				
			case 1:		//6309
				p6309->cc[E]=1;
				cpuMemWrite8(pCPU,p6309->pc.B.lsb, --p6309->S_REG);
				cpuMemWrite8(pCPU,p6309->pc.B.msb, --p6309->S_REG);
				cpuMemWrite8(pCPU,p6309->u.B.lsb, --p6309->S_REG);
				cpuMemWrite8(pCPU,p6309->u.B.msb, --p6309->S_REG);
				cpuMemWrite8(pCPU,p6309->y.B.lsb, --p6309->S_REG);
				cpuMemWrite8(pCPU,p6309->y.B.msb, --p6309->S_REG);
				cpuMemWrite8(pCPU,p6309->x.B.lsb, --p6309->S_REG);
				cpuMemWrite8(pCPU,p6309->x.B.msb, --p6309->S_REG);
				cpuMemWrite8(pCPU,p6309->dp.B.msb, --p6309->S_REG);
				if ( p6309->md[NATIVE6309] )
				{
					cpuMemWrite8(pCPU,(p6309->F_REG), --p6309->S_REG);
					cpuMemWrite8(pCPU,(p6309->E_REG), --p6309->S_REG);
				}
				cpuMemWrite8(pCPU,p6309->B_REG, --p6309->S_REG);
				cpuMemWrite8(pCPU,p6309->A_REG, --p6309->S_REG);
				cpuMemWrite8(pCPU,getcc(pCPU), --p6309->S_REG);
				p6309->cc[I]=1;
				p6309->cc[F]=1;
				p6309->PC_REG=cpuMemRead16(&p6309->cpu,VFIRQ);
				break;
		}
	}
	p6309->PendingInterrupts = p6309->PendingInterrupts & ~FIRQ;
}

static void cpu_irq(cpu_t * pCPU)
{
	cpu6309_t *	p6309 = (cpu6309_t *)pCPU;
	
	assert(p6309 != NULL);
	
	if ( p6309->InInterrupt ) //If FIRQ is running postpone the IRQ
		return;	
	
	if ( (!p6309->cc[I]) )
	{
		p6309->cc[E] = 1;
		cpuMemWrite8(pCPU, p6309->pc.B.lsb, --p6309->S_REG);
		cpuMemWrite8(pCPU, p6309->pc.B.msb, --p6309->S_REG);
		cpuMemWrite8(pCPU, p6309->u.B.lsb, --p6309->S_REG);
		cpuMemWrite8(pCPU, p6309->u.B.msb, --p6309->S_REG);
		cpuMemWrite8(pCPU, p6309->y.B.lsb, --p6309->S_REG);
		cpuMemWrite8(pCPU, p6309->y.B.msb, --p6309->S_REG);
		cpuMemWrite8(pCPU, p6309->x.B.lsb, --p6309->S_REG);
		cpuMemWrite8(pCPU, p6309->x.B.msb, --p6309->S_REG);
		cpuMemWrite8(pCPU, p6309->dp.B.msb, --p6309->S_REG);
		if ( p6309->md[NATIVE6309] )
		{
			cpuMemWrite8(pCPU,(p6309->F_REG), --p6309->S_REG);
			cpuMemWrite8(pCPU,(p6309->E_REG), --p6309->S_REG);
		}
		cpuMemWrite8(pCPU,p6309->B_REG, --p6309->S_REG);
		cpuMemWrite8(pCPU,p6309->A_REG, --p6309->S_REG);
		cpuMemWrite8(pCPU,getcc(pCPU), --p6309->S_REG);
		p6309->PC_REG=cpuMemRead16(&p6309->cpu,VIRQ);
		p6309->cc[I]=1; 
	} //Fi I test
	p6309->PendingInterrupts = p6309->PendingInterrupts & ~IRQ;
}

static void cpu_nmi(cpu_t * pCPU)
{
	cpu6309_t *	p6309 = (cpu6309_t *)pCPU;
	
	assert(p6309 != NULL);
	
	p6309->cc[E]=1;
	cpuMemWrite8(pCPU, p6309->pc.B.lsb, --p6309->S_REG);
	cpuMemWrite8(pCPU, p6309->pc.B.msb, --p6309->S_REG);
	cpuMemWrite8(pCPU, p6309->u.B.lsb, --p6309->S_REG);
	cpuMemWrite8(pCPU, p6309->u.B.msb, --p6309->S_REG);
	cpuMemWrite8(pCPU, p6309->y.B.lsb, --p6309->S_REG);
	cpuMemWrite8(pCPU, p6309->y.B.msb, --p6309->S_REG);
	cpuMemWrite8(pCPU, p6309->x.B.lsb, --p6309->S_REG);
	cpuMemWrite8(pCPU, p6309->x.B.msb, --p6309->S_REG);
	cpuMemWrite8(pCPU, p6309->dp.B.msb, --p6309->S_REG);
	if ( p6309->md[NATIVE6309] )
	{
		cpuMemWrite8(pCPU,(p6309->F_REG), --p6309->S_REG);
		cpuMemWrite8(pCPU,(p6309->E_REG), --p6309->S_REG);
	}
	cpuMemWrite8(pCPU,p6309->B_REG, --p6309->S_REG);
	cpuMemWrite8(pCPU,p6309->A_REG, --p6309->S_REG);
	cpuMemWrite8(pCPU,getcc(pCPU), --p6309->S_REG);
	p6309->cc[I]=1;
	p6309->cc[F]=1;
	p6309->PC_REG=cpuMemRead16(&p6309->cpu,VNMI);
	p6309->PendingInterrupts = p6309->PendingInterrupts & ~NMI;
}

/********************************************************************************/

#pragma mark -
#pragma mark --- cpu_t plug-in API ---

/**************************************************************************/

void cpuHD6309Init(cpu_t * pCPU)
{	
	cpu6309_t *	p6309 = (cpu6309_t *)pCPU;
	
	assert(p6309 != NULL);
	
	emuDevRegisterDevice(&p6309->cpu.device);

	//Call this first or RESET will core!
	// reg pointers for TFR and EXG and LEA ops
	p6309->xfreg16[0] = &p6309->D_REG;
	p6309->xfreg16[1] = &p6309->X_REG;
	p6309->xfreg16[2] = &p6309->Y_REG;
	p6309->xfreg16[3] = &p6309->U_REG;
	p6309->xfreg16[4] = &p6309->S_REG;
	p6309->xfreg16[5] = &p6309->PC_REG;
	p6309->xfreg16[6] = &p6309->W_REG;
	p6309->xfreg16[7] = &p6309->V_REG;

	p6309->ureg8[0]=(unsigned char*)&p6309->A_REG;		
	p6309->ureg8[1]=(unsigned char*)&p6309->B_REG;		
	p6309->ureg8[2]=(unsigned char*)&p6309->ccbits;
	p6309->ureg8[3]=(unsigned char*)&p6309->dp.B.msb;
	p6309->ureg8[4]=(unsigned char*)&p6309->O_REG;
	p6309->ureg8[5]=(unsigned char*)&p6309->O_REG;
	p6309->ureg8[6]=(unsigned char*)&p6309->E_REG;
	p6309->ureg8[7]=(unsigned char*)&p6309->F_REG;
	//This handles the disparity between 6309 and 6809 Instruction timing
	p6309->InsCycles[0][M65]=6;	//6-5
	p6309->InsCycles[1][M65]=5;
	p6309->InsCycles[0][M64]=6;	//6-4
	p6309->InsCycles[1][M64]=4;
	p6309->InsCycles[0][M32]=3;	//3-2
	p6309->InsCycles[1][M32]=2;
	p6309->InsCycles[0][M21]=2;	//2-1
	p6309->InsCycles[1][M21]=1;
	p6309->InsCycles[0][M54]=5;	//5-4
	p6309->InsCycles[1][M54]=4;
	p6309->InsCycles[0][M97]=9;	//9-7
	p6309->InsCycles[1][M97]=7;
	p6309->InsCycles[0][M85]=8;	//8-5
	p6309->InsCycles[1][M85]=5;
	p6309->InsCycles[0][M51]=5;	//5-1
	p6309->InsCycles[1][M51]=1;
	p6309->InsCycles[0][M31]=3;	//3-1
	p6309->InsCycles[1][M31]=1;
	p6309->InsCycles[0][M1110]=11;	//11-10
	p6309->InsCycles[1][M1110]=10;
	p6309->InsCycles[0][M76]=7;	//7-6
	p6309->InsCycles[1][M76]=6;
	p6309->InsCycles[0][M75]=7;	//7-5
	p6309->InsCycles[1][M75]=5;
	p6309->InsCycles[0][M43]=4;	//4-3
	p6309->InsCycles[1][M43]=3;
	p6309->InsCycles[0][M87]=8;	//8-7
	p6309->InsCycles[1][M87]=7;
	p6309->InsCycles[0][M86]=8;	//8-6
	p6309->InsCycles[1][M86]=6;
	p6309->InsCycles[0][M98]=9;	//9-8
	p6309->InsCycles[1][M98]=8;
	p6309->InsCycles[0][M2726]=27;	//27-26
	p6309->InsCycles[1][M2726]=26;
	p6309->InsCycles[0][M3635]=36;	//36-25
	p6309->InsCycles[1][M3635]=35;	
	p6309->InsCycles[0][M3029]=30;	//30-29
	p6309->InsCycles[1][M3029]=29;	
	p6309->InsCycles[0][M2827]=28;	//28-27
	p6309->InsCycles[1][M2827]=27;	
	p6309->InsCycles[0][M3726]=37;	//37-26
	p6309->InsCycles[1][M3726]=26;		
	p6309->InsCycles[0][M3130]=31;	//31-30
	p6309->InsCycles[1][M3130]=30;		
	p6309->cc[I]=1;
	p6309->cc[F]=1;
}

/**************************************************************************/
/**
 Reset CPU
 
 Reset vector needs to be set
 */
void cpuHD6309Reset(cpu_t * pCPU)
{
	char index;
	cpu6309_t *	p6309 = (cpu6309_t *)pCPU;
	
	assert(p6309 != NULL);
	
	for(index=0;index<=6;index++)		//Set all register to 0 except V
		*p6309->xfreg16[index] = 0;
	for(index=0;index<=7;index++)
		*p6309->ureg8[index]=0;
	for(index=0;index<=7;index++)
		p6309->cc[index]=0;
	for(index=0;index<=7;index++)
		p6309->md[index]=0;
	p6309->mdbits = getmd(pCPU);
	p6309->dp.Reg=0;
	p6309->cc[I]=1;
	p6309->cc[F]=1;
	p6309->SyncWaiting=0;
	
	p6309->PC_REG = cpuMemRead16(&p6309->cpu,VRESET);	//PC gets its reset vector
}

/**************************************************************************/

int cpuHD6309Exec(cpu_t * pCPU, int CycleFor)
{
	uint8_t			msn,lsn;
	uint16_t		temp16;
	int16_t			stemp16;
	uint32_t		temp32;
	uint8_t			temp8;
	unsigned char	Source=0;
	unsigned char	Dest=0;
	unsigned char	postbyte=0;
	short unsigned	postword=0;
	signed char *	spostbyte=(signed char *)&postbyte;
	signed short *	spostword=(signed short *)&postword;
	cpu6309_t *	p6309 = (cpu6309_t *)pCPU;
	
	assert(p6309 != NULL);
		
	p6309->cpu.CycleCounter=0;
    
while ( p6309->cpu.CycleCounter < CycleFor ) 
{

	if ( p6309->PendingInterrupts )
	{
		if ( p6309->PendingInterrupts & NMI )
			cpu_nmi(pCPU);

		if ( p6309->PendingInterrupts & FIRQ )
			cpu_firq(pCPU);

		if ( p6309->PendingInterrupts & IRQ )
		{
			if ( p6309->IRQWaiter == 0 )	// This is needed to fix a subtle timing problem
				cpu_irq(pCPU);		// It allows the CPU to see $FF03 bit 7 high before
			else				// The IRQ is asserted.
				p6309->IRQWaiter -= 1;
		}
	}

	if ( p6309->SyncWaiting == 1 )	//Abort the run nothing happens asyncronously from the CPU
		return(0);

	switch ( cpuMemRead8(pCPU,p6309->PC_REG++) )
	{

	case NEG_D: //0
		temp16 = DPADDRESS(p6309,p6309->PC_REG++);
		postbyte=cpuMemRead8(pCPU,temp16);
		temp8=0-postbyte;
		p6309->cc[C] =temp8>0;
		p6309->cc[V] = (postbyte==0x80);
		p6309->cc[N] = NTEST8(temp8);
		p6309->cc[Z] = ZTEST(temp8);
		cpuMemWrite8(pCPU,temp8,temp16);
		p6309->cpu.CycleCounter += p6309->InsCycles[p6309->md[NATIVE6309]][M65];
		break;

	case OIM_D://1 6309
		postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
		temp16 = DPADDRESS(p6309,p6309->PC_REG++);
		postbyte|= cpuMemRead8(pCPU,temp16);
		cpuMemWrite8(pCPU,postbyte,temp16);
		p6309->cc[N] = NTEST8(postbyte);
		p6309->cc[Z] = ZTEST(postbyte);
		p6309->cc[V] = 0;
		p6309->cpu.CycleCounter += 6;
		break;

	case AIM_D://2 Phase 2 6309
		postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
		temp16 = DPADDRESS(p6309,p6309->PC_REG++);
		postbyte&= cpuMemRead8(pCPU,temp16);
		cpuMemWrite8(pCPU,postbyte,temp16);
		p6309->cc[N] = NTEST8(postbyte);
		p6309->cc[Z] = ZTEST(postbyte);
		p6309->cc[V] = 0;
		p6309->cpu.CycleCounter += 6;
		break;

	case COM_D:
		temp16 = DPADDRESS(p6309,p6309->PC_REG++);
		temp8=cpuMemRead8(pCPU,temp16);
		temp8=0xFF-temp8;
		p6309->cc[Z] = ZTEST(temp8);
		p6309->cc[N] = NTEST8(temp8); 
		p6309->cc[C] = 1;
		p6309->cc[V] = 0;
		cpuMemWrite8(pCPU,temp8,temp16);
		p6309->cpu.CycleCounter += p6309->InsCycles[p6309->md[NATIVE6309]][M65];
		break;

	case LSR_D: //S2
		temp16 = DPADDRESS(p6309,p6309->PC_REG++);
		temp8 = cpuMemRead8(pCPU,temp16);
		p6309->cc[C] = temp8 & 1;
		temp8 = temp8 >>1;
		p6309->cc[Z] = ZTEST(temp8);
		p6309->cc[N] = 0;
		cpuMemWrite8(pCPU,temp8,temp16);
		p6309->cpu.CycleCounter += p6309->InsCycles[p6309->md[NATIVE6309]][M65];
		break;

	case EIM_D: //6309 Untested
		postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
		temp16 = DPADDRESS(p6309,p6309->PC_REG++);
		postbyte^= cpuMemRead8(pCPU,temp16);
		cpuMemWrite8(pCPU,postbyte,temp16);
		p6309->cc[N] = NTEST8(postbyte);
		p6309->cc[Z] = ZTEST(postbyte);
		p6309->cc[V] = 0;
		p6309->cpu.CycleCounter+=6;
		break;

	case ROR_D: //S2
		temp16 = DPADDRESS(p6309,p6309->PC_REG++);
		temp8=cpuMemRead8(pCPU,temp16);
		postbyte= p6309->cc[C]<<7;
		p6309->cc[C] = temp8 & 1;
		temp8 = (temp8 >> 1)| postbyte;
		p6309->cc[Z] = ZTEST(temp8);
		p6309->cc[N] = NTEST8(temp8);
		cpuMemWrite8(pCPU,temp8,temp16);
		p6309->cpu.CycleCounter += p6309->InsCycles[p6309->md[NATIVE6309]][M65];
		break;

	case ASR_D: //7
		temp16 = DPADDRESS(p6309,p6309->PC_REG++);
		temp8=cpuMemRead8(pCPU,temp16);
		p6309->cc[C] = temp8 & 1;
		temp8 = (temp8 & 0x80) | (temp8 >>1);
		p6309->cc[Z] = ZTEST(temp8);
		p6309->cc[N] = NTEST8(temp8);
		cpuMemWrite8(pCPU,temp8,temp16);
		p6309->cpu.CycleCounter += p6309->InsCycles[p6309->md[NATIVE6309]][M65];
		break;

	case ASL_D: //8 
		temp16 = DPADDRESS(p6309,p6309->PC_REG++);
		temp8=cpuMemRead8(pCPU,temp16);
		p6309->cc[C] = (temp8 & 0x80) >>7;
		p6309->cc[V] = p6309->cc[C] ^ ((temp8 & 0x40) >> 6);
		temp8 = temp8 <<1;
		p6309->cc[N] = NTEST8(temp8);
		p6309->cc[Z] = ZTEST(temp8);
		cpuMemWrite8(pCPU,temp8,temp16);
		p6309->cpu.CycleCounter += p6309->InsCycles[p6309->md[NATIVE6309]][M65];
		break;

	case ROL_D:	//9
		temp16 = DPADDRESS(p6309,p6309->PC_REG++);
		temp8 = cpuMemRead8(pCPU,temp16);
		postbyte = p6309->cc[C];
		p6309->cc[C] =(temp8 & 0x80)>>7;
		p6309->cc[V] = p6309->cc[C] ^ ((temp8 & 0x40) >>6);
		temp8 = (temp8<<1) | postbyte;
		p6309->cc[Z] = ZTEST(temp8);
		p6309->cc[N] = NTEST8(temp8);
		cpuMemWrite8(pCPU,temp8,temp16);
		p6309->cpu.CycleCounter += p6309->InsCycles[p6309->md[NATIVE6309]][M65];
		break;

	case DEC_D: //A
		temp16 = DPADDRESS(p6309,p6309->PC_REG++);
		temp8 = cpuMemRead8(pCPU,temp16)-1;
		p6309->cc[Z] = ZTEST(temp8);
		p6309->cc[N] = NTEST8(temp8);
		p6309->cc[V] = temp8==0x7F;
		cpuMemWrite8(pCPU,temp8,temp16);
		p6309->cpu.CycleCounter += p6309->InsCycles[p6309->md[NATIVE6309]][M65];
		break;

	case TIM_D:	//B 6309 Untested wcreate
		postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
		temp8=cpuMemRead8(pCPU,DPADDRESS(p6309,p6309->PC_REG++));
		postbyte&=temp8;
		p6309->cc[N] = NTEST8(postbyte);
		p6309->cc[Z] = ZTEST(postbyte);
		p6309->cc[V] = 0;
		p6309->cpu.CycleCounter += 6;
		break;

	case INC_D: //C
		temp16=(DPADDRESS(p6309,p6309->PC_REG++));
		temp8 = cpuMemRead8(pCPU,temp16)+1;
		p6309->cc[Z] = ZTEST(temp8);
		p6309->cc[V] = temp8==0x80;
		p6309->cc[N] = NTEST8(temp8);
		cpuMemWrite8(pCPU,temp8,temp16);
		p6309->cpu.CycleCounter += p6309->InsCycles[p6309->md[NATIVE6309]][M65];
		break;

	case TST_D: //D
		temp8 = cpuMemRead8(pCPU,DPADDRESS(p6309,p6309->PC_REG++));
		p6309->cc[Z] = ZTEST(temp8);
		p6309->cc[N] = NTEST8(temp8);
		p6309->cc[V] = 0;
		p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M64];
		break;

	case JMP_D:	//E
		p6309->PC_REG= ((p6309->dp.Reg |cpuMemRead8(pCPU,p6309->PC_REG)));
		p6309->cpu.CycleCounter += p6309->InsCycles[p6309->md[NATIVE6309]][M32];
		break;

	case CLR_D:	//F
		cpuMemWrite8(pCPU,0,DPADDRESS(p6309,p6309->PC_REG++));
		p6309->cc[Z] = 1;
		p6309->cc[N] = 0;
		p6309->cc[V] = 0;
		p6309->cc[C] = 0;
		p6309->cpu.CycleCounter += p6309->InsCycles[p6309->md[NATIVE6309]][M65];
		break;

	case Page2:
		switch (cpuMemRead8(pCPU,p6309->PC_REG++)) 
		{

		case LBEQ_R: //1027
			if (p6309->cc[Z])
			{
				*spostword=IMMADDRESS(p6309,p6309->PC_REG);
				p6309->PC_REG+=*spostword;
				p6309->cpu.CycleCounter+=1;
			}
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=5;
			break;

		case LBRN_R: //1021
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=5;
			break;

		case LBHI_R: //1022
			if  (!(p6309->cc[C] | p6309->cc[Z]))
			{
				*spostword=IMMADDRESS(p6309,p6309->PC_REG);
				p6309->PC_REG+=*spostword;
				p6309->cpu.CycleCounter+=1;
			}
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=5;
			break;

		case LBLS_R: //1023
			if (p6309->cc[C] | p6309->cc[Z])
			{
				*spostword=IMMADDRESS(p6309,p6309->PC_REG);
				p6309->PC_REG+=*spostword;
				p6309->cpu.CycleCounter+=1;
			}
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=5;
			break;

		case LBHS_R: //1024
			if (!p6309->cc[C])
			{
				*spostword=IMMADDRESS(p6309,p6309->PC_REG);
				p6309->PC_REG+=*spostword;
				p6309->cpu.CycleCounter+=1;
			}
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=6;
			break;

		case LBCS_R: //1025
			if (p6309->cc[C])
			{
				*spostword=IMMADDRESS(p6309,p6309->PC_REG);
				p6309->PC_REG+=*spostword;
				p6309->cpu.CycleCounter+=1;
			}
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=5;
			break;

		case LBNE_R: //1026
			if (!p6309->cc[Z])
			{
				*spostword=IMMADDRESS(p6309,p6309->PC_REG);
				p6309->PC_REG+=*spostword;
				p6309->cpu.CycleCounter+=1;
			}
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=5;
			break;

		case LBVC_R: //1028
			if ( !p6309->cc[V])
			{
				*spostword=IMMADDRESS(p6309,p6309->PC_REG);
				p6309->PC_REG+=*spostword;
				p6309->cpu.CycleCounter+=1;
			}
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=5;
			break;

		case LBVS_R: //1029
			if ( p6309->cc[V])
			{
				*spostword=IMMADDRESS(p6309,p6309->PC_REG);
				p6309->PC_REG+=*spostword;
				p6309->cpu.CycleCounter+=1;
			}
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=5;
			break;

		case LBPL_R: //102A
		if (!p6309->cc[N])
			{
				*spostword=IMMADDRESS(p6309,p6309->PC_REG);
				p6309->PC_REG+=*spostword;
				p6309->cpu.CycleCounter+=1;
			}
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=5;
			break;

		case LBMI_R: //102B
		if ( p6309->cc[N])
			{
				*spostword=IMMADDRESS(p6309,p6309->PC_REG);
				p6309->PC_REG+=*spostword;
				p6309->cpu.CycleCounter+=1;
			}
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=5;
			break;

		case LBGE_R: //102C
			if (! (p6309->cc[N] ^ p6309->cc[V]))
			{
				*spostword=IMMADDRESS(p6309,p6309->PC_REG);
				p6309->PC_REG+=*spostword;
				p6309->cpu.CycleCounter+=1;
			}
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=5;
			break;

		case LBLT_R: //102D
			if ( p6309->cc[V] ^ p6309->cc[N])
			{
				*spostword=IMMADDRESS(p6309,p6309->PC_REG);
				p6309->PC_REG+=*spostword;
				p6309->cpu.CycleCounter+=1;
			}
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=5;
			break;

		case LBGT_R: //102E
			if ( !( p6309->cc[Z] | (p6309->cc[N]^p6309->cc[V] ) ))
			{
				*spostword=IMMADDRESS(p6309,p6309->PC_REG);
				p6309->PC_REG+=*spostword;
				p6309->cpu.CycleCounter+=1;
			}
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=5;
			break;

		case LBLE_R:	//102F
			if ( p6309->cc[Z] | (p6309->cc[N]^p6309->cc[V]) )
			{
				*spostword=IMMADDRESS(p6309,p6309->PC_REG);
				p6309->PC_REG+=*spostword;
				p6309->cpu.CycleCounter+=1;
			}
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=5;
			break;

		case ADDR: //1030 6309 CC? NITRO 8 bit code
			temp8=cpuMemRead8(pCPU,p6309->PC_REG++);
			Source= temp8>>4;
			Dest=temp8 & 15;

			if ( (Source>7) & (Dest>7) )
			{
				temp16= *p6309->ureg8[Source & 7] + *p6309->ureg8[Dest & 7];
				p6309->cc[C] = (temp16 & 0x100)>>8;
				p6309->cc[V] = OVERFLOW8(p6309->cc[C],*p6309->ureg8[Source & 7],*p6309->ureg8[Dest & 7],temp16);
				*p6309->ureg8[Dest & 7]=(temp16 & 0xFF);
				p6309->cc[N] = NTEST8(*p6309->ureg8[Dest & 7]);	
				p6309->cc[Z] = ZTEST(*p6309->ureg8[Dest & 7]);
			}
			else
			{
				temp32= *p6309->xfreg16[Source] + *p6309->xfreg16[Dest];
				p6309->cc[C] = (temp32 & 0x10000)>>16;
				p6309->cc[V] = OVERFLOW16(p6309->cc[C],*p6309->xfreg16[Source],*p6309->xfreg16[Dest],temp32);
				*p6309->xfreg16[Dest]=(temp32 & 0xFFFF);
				p6309->cc[N] = NTEST16(*p6309->xfreg16[Dest]);
				p6309->cc[Z] = ZTEST(*p6309->xfreg16[Dest]);
			}
			p6309->cc[H] =0;
			p6309->cpu.CycleCounter+=4;
		break;

		case ADCR: //1031 6309
			assert(0);
			//WriteLog("Hitting UNEMULATED INS ADCR",TOCONS);
			p6309->cpu.CycleCounter+=4;
		break;

		case SUBR: //1032 6309
			temp8=cpuMemRead8(pCPU,p6309->PC_REG++);
			Source=temp8>>4; 
			Dest=temp8 & 15;

			switch (Dest)
			{
				case 0:
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
				case 6:
				case 7:
					if ((Source==12) | (Source==13))
						postword=0;
					else
						postword=*p6309->xfreg16[Source];

					temp32=  *p6309->xfreg16[Dest] - postword;
					p6309->cc[C] =(temp32 & 0x10000)>>16;
					p6309->cc[V] =!!(((*p6309->xfreg16[Dest])^postword^temp32^(temp32>>1)) & 0x8000);
					p6309->cc[N] =(temp32 & 0x8000)>>15;	
					p6309->cc[Z] = !temp32;
					*p6309->xfreg16[Dest]=temp32;
				break;

				case 8:
				case 9:
				case 10:
				case 11:
				case 14:
				case 15:
					if (Source>=8)
					{
						temp8= *p6309->ureg8[Dest&7] - *p6309->ureg8[Source&7];
						p6309->cc[C] = temp8 > *p6309->ureg8[Dest&7];
						p6309->cc[V] = p6309->cc[C] ^ ( ((*p6309->ureg8[Dest&7])^temp8^(*p6309->ureg8[Source&7]))>>7);
						p6309->cc[N] = temp8>>7;
						p6309->cc[Z] = !temp8;
						*p6309->ureg8[Dest&7]=temp8;
					}
				break;

			}
			p6309->cpu.CycleCounter+=4;
		break;

		case SBCR: //1033 6309
				assert(0);
			//WriteLog("Hitting UNEMULATED INS SBCR",TOCONS);
			p6309->cpu.CycleCounter+=4;
		break;

		case ANDR: //1034 6309 Untested wcreate
			temp8=cpuMemRead8(pCPU,p6309->PC_REG++);
			Source=temp8>>4;
			Dest=temp8 & 15;

			if ( (Source >=8) & (Dest >=8) )
			{
				(*p6309->ureg8[(Dest & 7)])&=(*p6309->ureg8[(Source & 7)]);
				p6309->cc[N] = (*p6309->ureg8[(Dest & 7)]) >>7;
				p6309->cc[Z] = !(*p6309->ureg8[(Dest & 7)]);
			}
			else
			{
				(*p6309->xfreg16[Dest])&=(*p6309->xfreg16[Source]);
				p6309->cc[N] = (*p6309->xfreg16[Dest]) >>15;
				p6309->cc[Z] = !(*p6309->xfreg16[Dest]);
			}
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=4;
		break;

		case ORR: //1035 6309
			temp8=cpuMemRead8(pCPU,p6309->PC_REG++);
			Source=temp8>>4;
			Dest=temp8 & 15;

			if ( (Source >=8) & (Dest >=8) )
			{
				(*p6309->ureg8[(Dest & 7)])|=(*p6309->ureg8[(Source & 7)]);
				p6309->cc[N] = (*p6309->ureg8[(Dest & 7)]) >>7;
				p6309->cc[Z] = !(*p6309->ureg8[(Dest & 7)]);
			}
			else
			{
				(*p6309->xfreg16[Dest])|=(*p6309->xfreg16[Source]);
				p6309->cc[N] = (*p6309->xfreg16[Dest]) >>15;
				p6309->cc[Z] = !(*p6309->xfreg16[Dest]);
			}
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=4;
		break;

		case EORR: //1036 6309
			temp8=cpuMemRead8(pCPU,p6309->PC_REG++);
			Source=temp8>>4;
			Dest=temp8 & 15;

			if ( (Source >=8) & (Dest >=8) )
			{
				(*p6309->ureg8[(Dest & 7)])^=(*p6309->ureg8[(Source & 7)]);
				p6309->cc[N] = (*p6309->ureg8[(Dest & 7)]) >>7;
				p6309->cc[Z] = !(*p6309->ureg8[(Dest & 7)]);
			}
			else
			{
				(*p6309->xfreg16[Dest])^=(*p6309->xfreg16[Source]);
				p6309->cc[N] = (*p6309->xfreg16[Dest]) >>15;
				p6309->cc[Z] = !(*p6309->xfreg16[Dest]);
			}
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=4;
		break;

		case CMPR: //1037 6309
			temp8=cpuMemRead8(pCPU,p6309->PC_REG++);
			Source=temp8>>4; 
			Dest=temp8 & 15;
			switch (Dest)
			{
				case 0:
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
				case 6:
				case 7:
					if ((Source==12) | (Source==13))
						postword=0;
					else
						postword=*p6309->xfreg16[Source];

					temp16=   (*p6309->xfreg16[Dest]) - postword;
					p6309->cc[C] = temp16 > *p6309->xfreg16[Dest];
					p6309->cc[V] = p6309->cc[C]^(( (*p6309->xfreg16[Dest])^temp16^postword)>>15);
					p6309->cc[N] = (temp16 >> 15);
					p6309->cc[Z] = !temp16;
				break;

				case 8:
				case 9:
				case 10:
				case 11:
				case 14:
				case 15:
					if (Source>=8)
					{
						temp8= *p6309->ureg8[Dest&7] - *p6309->ureg8[Source&7];
						p6309->cc[C] = temp8 > *p6309->ureg8[Dest&7];
						p6309->cc[V] = p6309->cc[C] ^ ( ((*p6309->ureg8[Dest&7])^temp8^(*p6309->ureg8[Source&7]))>>7);
						p6309->cc[N] = temp8>>7;
						p6309->cc[Z] = !temp8;
					}
				break;

			}
			p6309->cpu.CycleCounter+=4;
		break;

		case PSHSW: //1038 DONE 6309
			cpuMemWrite8(pCPU,(p6309->F_REG),--p6309->S_REG);
			cpuMemWrite8(pCPU,(p6309->E_REG),--p6309->S_REG);
			p6309->cpu.CycleCounter+=6;
		break;

		case PULSW:	//1039 6309 Untested wcreate
			p6309->E_REG=cpuMemRead8(pCPU, p6309->S_REG++);
			p6309->F_REG=cpuMemRead8(pCPU, p6309->S_REG++);
			p6309->cpu.CycleCounter+=6;
		break;

		case PSHUW: //103A 6309 Untested
			cpuMemWrite8(pCPU,(p6309->F_REG),--p6309->U_REG);
			cpuMemWrite8(pCPU,(p6309->E_REG),--p6309->U_REG);
			p6309->cpu.CycleCounter+=6;
		break;

		case PULUW: //103B 6309 Untested
			p6309->E_REG=cpuMemRead8(pCPU, p6309->U_REG++);
			p6309->F_REG=cpuMemRead8(pCPU, p6309->U_REG++);
			p6309->cpu.CycleCounter+=6;
		break;

		case SWI2_I: //103F
			p6309->cc[E]=1;
			cpuMemWrite8(pCPU, p6309->pc.B.lsb,--p6309->S_REG);
			cpuMemWrite8(pCPU, p6309->pc.B.msb,--p6309->S_REG);
			cpuMemWrite8(pCPU, p6309->u.B.lsb,--p6309->S_REG);
			cpuMemWrite8(pCPU, p6309->u.B.msb,--p6309->S_REG);
			cpuMemWrite8(pCPU, p6309->y.B.lsb,--p6309->S_REG);
			cpuMemWrite8(pCPU, p6309->y.B.msb,--p6309->S_REG);
			cpuMemWrite8(pCPU, p6309->x.B.lsb,--p6309->S_REG);
			cpuMemWrite8(pCPU, p6309->x.B.msb,--p6309->S_REG);
			cpuMemWrite8(pCPU, p6309->dp.B.msb,--p6309->S_REG);
			if (p6309->md[NATIVE6309])
			{
				cpuMemWrite8(pCPU,(p6309->F_REG),--p6309->S_REG);
				cpuMemWrite8(pCPU,(p6309->E_REG),--p6309->S_REG);
				p6309->cpu.CycleCounter+=2;
			}
			cpuMemWrite8(pCPU,p6309->B_REG,--p6309->S_REG);
			cpuMemWrite8(pCPU,p6309->A_REG,--p6309->S_REG);
			cpuMemWrite8(pCPU,getcc(pCPU),--p6309->S_REG);
			p6309->PC_REG=cpuMemRead16(&p6309->cpu,VSWI2);
			p6309->cpu.CycleCounter+=20;
		break;

		case NEGD_I: //1040 Phase 5 6309
			temp16= 0-p6309->D_REG;
			p6309->cc[C] = temp16>0;
			p6309->cc[V] = p6309->D_REG==0x8000;
			p6309->cc[N] = NTEST16(temp16);
			p6309->cc[Z] = ZTEST(temp16);
			p6309->D_REG= temp16;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M32];
		break;

		case COMD_I: //1043 6309
			p6309->D_REG = 0xFFFF- p6309->D_REG;
			p6309->cc[Z] = ZTEST(p6309->D_REG);
			p6309->cc[N] = NTEST16(p6309->D_REG);
			p6309->cc[C] = 1;
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M32];
		break;

		case LSRD_I: //1044 6309
			p6309->cc[C] = p6309->D_REG & 1;
			p6309->D_REG = p6309->D_REG>>1;
			p6309->cc[Z] = ZTEST(p6309->D_REG);
			p6309->cc[N] = 0;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M32];
		break;

		case RORD_I: //1046 6309 Untested
			postword=p6309->cc[C]<<15;
			p6309->cc[C] = p6309->D_REG & 1;
			p6309->D_REG = (p6309->D_REG>>1) | postword;
			p6309->cc[Z] = ZTEST(p6309->D_REG);
			p6309->cc[N] = NTEST16(p6309->D_REG);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M32];
		break;

		case ASRD_I: //1047 6309 Untested TESTED NITRO MULTIVUE
			p6309->cc[C] = p6309->D_REG & 1;
			p6309->D_REG = (p6309->D_REG & 0x8000) | (p6309->D_REG >> 1);
			p6309->cc[Z] = ZTEST(p6309->D_REG);
			p6309->cc[N] = NTEST16(p6309->D_REG);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M32];
		break;

		case ASLD_I: //1048 6309
			p6309->cc[C] = p6309->D_REG >>15;
			p6309->cc[V] =  p6309->cc[C] ^((p6309->D_REG & 0x4000)>>14);
			p6309->D_REG = p6309->D_REG<<1;
			p6309->cc[N] = NTEST16(p6309->D_REG);
			p6309->cc[Z] = ZTEST(p6309->D_REG);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M32];
		break;

		case ROLD_I: //1049 6309 Untested
			postword=p6309->cc[C];
			p6309->cc[C] = p6309->D_REG >>15;
			p6309->cc[V] = p6309->cc[C] ^ ((p6309->D_REG & 0x4000)>>14);
			p6309->D_REG= (p6309->D_REG<<1) | postword;
			p6309->cc[Z] = ZTEST(p6309->D_REG);
			p6309->cc[N] = NTEST16(p6309->D_REG);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M32];
		break;

		case DECD_I: //104A 6309
			p6309->D_REG--;
			p6309->cc[Z] = ZTEST(p6309->D_REG);
			p6309->cc[V] = p6309->D_REG==0x7FFF;
			p6309->cc[N] = NTEST16(p6309->D_REG);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M32];
		break;

		case INCD_I: //104C 6309
			p6309->D_REG++;
			p6309->cc[Z] = ZTEST(p6309->D_REG);
			p6309->cc[V] = p6309->D_REG==0x8000;
			p6309->cc[N] = NTEST16(p6309->D_REG);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M32];
		break;

		case TSTD_I: //104D 6309
			p6309->cc[Z] = ZTEST(p6309->D_REG);
			p6309->cc[N] = NTEST16(p6309->D_REG);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M32];
		break;

		case CLRD_I: //104F 6309
			p6309->D_REG= 0;
			p6309->cc[C] = 0;
			p6309->cc[V] = 0;
			p6309->cc[N] = 0;
			p6309->cc[Z] = 1;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M32];
		break;

		case COMW_I: //1053 6309 Untested
			p6309->W_REG= 0xFFFF- p6309->W_REG;
			p6309->cc[Z] = ZTEST(p6309->W_REG);
			p6309->cc[N] = NTEST16(p6309->W_REG);
			p6309->cc[C] = 1;
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M32];
			break;

		case LSRW_I: //1054 6309 Untested
			p6309->cc[C] = p6309->W_REG & 1;
			p6309->W_REG= p6309->W_REG>>1;
			p6309->cc[Z] = ZTEST(p6309->W_REG);
			p6309->cc[N] = 0;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M32];
		break;

		case RORW_I: //1056 6309 Untested
			postword=p6309->cc[C]<<15;
			p6309->cc[C] = p6309->W_REG & 1;
			p6309->W_REG= (p6309->W_REG>>1) | postword;
			p6309->cc[Z] = ZTEST(p6309->W_REG);
			p6309->cc[N] = NTEST16(p6309->W_REG);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M32];
		break;

		case ROLW_I: //1059 6309
			postword=p6309->cc[C];
			p6309->cc[C] = p6309->W_REG >>15;
			p6309->cc[V] = p6309->cc[C] ^ ((p6309->W_REG & 0x4000)>>14);
			p6309->W_REG= ( p6309->W_REG<<1) | postword;
			p6309->cc[Z] = ZTEST(p6309->W_REG);
			p6309->cc[N] = NTEST16(p6309->W_REG);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M32];
		break;

		case DECW_I: //105A 6309
			p6309->W_REG--;
			p6309->cc[Z] = ZTEST(p6309->W_REG);
			p6309->cc[V] = p6309->W_REG==0x7FFF;
			p6309->cc[N] = NTEST16(p6309->W_REG);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M32];
		break;

		case INCW_I: //105C 6309
			p6309->W_REG++;
			p6309->cc[Z] = ZTEST(p6309->W_REG);
			p6309->cc[V] = p6309->W_REG==0x8000;
			p6309->cc[N] = NTEST16(p6309->W_REG);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M32];
		break;

		case TSTW_I: //105D Untested 6309 wcreate
			p6309->cc[Z] = ZTEST(p6309->W_REG);
			p6309->cc[N] = NTEST16(p6309->W_REG);
			p6309->cc[V] = 0;	
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M32];
		break;

		case CLRW_I: //105F 6309
			p6309->W_REG = 0;
			p6309->cc[C] = 0;
			p6309->cc[V] = 0;
			p6309->cc[N] = 0;
			p6309->cc[Z] = 1;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M32];
		break;

		case SUBW_M: //1080 6309 CHECK
			postword=IMMADDRESS(p6309,p6309->PC_REG);
			temp16= p6309->W_REG-postword;
			p6309->cc[C] = temp16 > p6309->W_REG;
			p6309->cc[V] = OVERFLOW16(p6309->cc[C],temp16,p6309->W_REG,postword);
			p6309->cc[N] = NTEST16(temp16);
			p6309->cc[Z] = ZTEST(temp16);
			p6309->W_REG= temp16;
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
		break;

		case CMPW_M: //1081 6309 CHECK
			postword=IMMADDRESS(p6309,p6309->PC_REG);
			temp16= p6309->W_REG-postword;
			p6309->cc[C] = temp16 > p6309->W_REG;
			p6309->cc[V] = OVERFLOW16(p6309->cc[C],temp16,p6309->W_REG,postword);
			p6309->cc[N] = NTEST16(temp16);
			p6309->cc[Z] = ZTEST(temp16);
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
		break;

		case SBCD_M: //1082 P6309
			postword=IMMADDRESS(p6309,p6309->PC_REG);
			temp32=p6309->D_REG-postword-p6309->cc[C];
			p6309->cc[C] = (temp32 & 0x10000)>>16;
			p6309->cc[V] = OVERFLOW16(p6309->cc[C],temp32,p6309->D_REG,postword);
			p6309->D_REG= temp32;
			p6309->cc[N] = NTEST16(p6309->D_REG);
			p6309->cc[Z] = ZTEST(p6309->D_REG);
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
		break;

		case CMPD_M: //1083
			postword=IMMADDRESS(p6309,p6309->PC_REG);
			temp16 = p6309->D_REG-postword;
			p6309->cc[C] = temp16 > p6309->D_REG;
			p6309->cc[V] = OVERFLOW16(p6309->cc[C],postword,temp16,p6309->D_REG);
			p6309->cc[N] = NTEST16(temp16);
			p6309->cc[Z] = ZTEST(temp16);
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
			break;

		case ANDD_M: //1084 6309
			p6309->D_REG &= IMMADDRESS(p6309,p6309->PC_REG);
			p6309->cc[N] = NTEST16(p6309->D_REG);
			p6309->cc[Z] = ZTEST(p6309->D_REG);
			p6309->cc[V] = 0;
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
		break;

		case BITD_M: //1085 6309 Untested
			temp16= p6309->D_REG & IMMADDRESS(p6309,p6309->PC_REG);
			p6309->cc[N] = NTEST16(temp16);
			p6309->cc[Z] = ZTEST(temp16);
			p6309->cc[V] = 0;
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
		break;

		case LDW_M: //1086 6309
			p6309->W_REG=IMMADDRESS(p6309,p6309->PC_REG);
			p6309->cc[Z] = ZTEST(p6309->W_REG);
			p6309->cc[N] = NTEST16(p6309->W_REG);
			p6309->cc[V] = 0;
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
		break;

		case EORD_M: //1088 6309 Untested
			p6309->D_REG ^= IMMADDRESS(p6309,p6309->PC_REG);
			p6309->cc[N] = NTEST16(p6309->D_REG);
			p6309->cc[Z] = ZTEST(p6309->D_REG);
			p6309->cc[V] = 0;
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
		break;

		case ADCD_M: //1089 6309
			postword=IMMADDRESS(p6309,p6309->PC_REG);
			temp32= p6309->D_REG + postword + p6309->cc[C];
			p6309->cc[C] = (temp32 & 0x10000)>>16;
			p6309->cc[V] = OVERFLOW16(p6309->cc[C],postword,temp32,p6309->D_REG);
			p6309->cc[H] = ((p6309->D_REG ^ temp32 ^ postword) & 0x100)>>8;
			p6309->D_REG = temp32;
			p6309->cc[N] = NTEST16(p6309->D_REG);
			p6309->cc[Z] = ZTEST(p6309->D_REG);
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
		break;

		case ORD_M: //108A 6309 Untested
			p6309->D_REG |= IMMADDRESS(p6309,p6309->PC_REG);
			p6309->cc[N] = NTEST16(p6309->D_REG);
			p6309->cc[Z] = ZTEST(p6309->D_REG);
			p6309->cc[V] = 0;
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
		break;

		case ADDW_M: //108B Phase 5 6309
			temp16=IMMADDRESS(p6309,p6309->PC_REG);
			temp32= p6309->W_REG+ temp16;
			p6309->cc[C] = (temp32 & 0x10000)>>16;
			p6309->cc[V] = OVERFLOW16(p6309->cc[C],temp32,temp16,p6309->W_REG);
			p6309->W_REG = temp32;
			p6309->cc[Z] = ZTEST(p6309->W_REG);
			p6309->cc[N] = NTEST16(p6309->W_REG);
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
		break;

		case CMPY_M: //108C
			postword=IMMADDRESS(p6309,p6309->PC_REG);
			temp16 = p6309->Y_REG-postword;
			p6309->cc[C] = temp16 > p6309->Y_REG;
			p6309->cc[V] = OVERFLOW16(p6309->cc[C],postword,temp16,p6309->Y_REG);
			p6309->cc[N] = NTEST16(temp16);
			p6309->cc[Z] = ZTEST(temp16);
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
			break;
	
		case LDY_M: //108E
			p6309->Y_REG = IMMADDRESS(p6309,p6309->PC_REG);
			p6309->cc[Z] = ZTEST(p6309->Y_REG);
			p6309->cc[N] = NTEST16(p6309->Y_REG);
			p6309->cc[V] = 0;
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
			break;

		case SUBW_D: //1090 Untested 6309
			temp16= cpuMemRead16(&p6309->cpu,DPADDRESS(p6309,p6309->PC_REG++));
			temp32= p6309->W_REG-temp16;
			p6309->cc[C] = (temp32 & 0x10000)>>16;
			p6309->cc[V] = OVERFLOW16(p6309->cc[C],temp32,temp16,p6309->W_REG);
			p6309->W_REG = temp32;
			p6309->cc[Z] = ZTEST(p6309->W_REG);
			p6309->cc[N] = NTEST16(p6309->W_REG);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M75];
		break;

		case CMPW_D: //1091 6309 Untested
			postword=cpuMemRead16(&p6309->cpu,DPADDRESS(p6309,p6309->PC_REG++));
			temp16= p6309->W_REG - postword ;
			p6309->cc[C] = temp16 > p6309->W_REG;
			p6309->cc[V] = OVERFLOW16(p6309->cc[C],postword,temp16,p6309->W_REG); 
			p6309->cc[N] = NTEST16(temp16);
			p6309->cc[Z] = ZTEST(temp16);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M75];
		break;

		case SBCD_D: //1092 6309
				assert(0);
			//WriteLog("Hitting UNEMULATED INS SBCD_D",TOCONS);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M75];
		break;

		case CMPD_D: //1093
			postword=cpuMemRead16(&p6309->cpu,DPADDRESS(p6309,p6309->PC_REG++));
			temp16= p6309->D_REG - postword ;
			p6309->cc[C] = temp16 > p6309->D_REG;
			p6309->cc[V] = OVERFLOW16(p6309->cc[C],postword,temp16,p6309->D_REG); 
			p6309->cc[N] = NTEST16(temp16);
			p6309->cc[Z] = ZTEST(temp16);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M75];
			break;

		case ANDD_D: //1094 6309 Untested
			postword=cpuMemRead16(&p6309->cpu,DPADDRESS(p6309,p6309->PC_REG++));
			p6309->D_REG&=postword;
			p6309->cc[N] = NTEST16(p6309->D_REG);
			p6309->cc[Z] = ZTEST(p6309->D_REG);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M75];
		break;

		case BITD_D: //1095 6309 Untested
			temp16= p6309->D_REG & cpuMemRead16(&p6309->cpu,DPADDRESS(p6309,p6309->PC_REG++));
			p6309->cc[N] = NTEST16(temp16);
			p6309->cc[Z] = ZTEST(temp16);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M75];
		break;

		case LDW_D: //1096 6309
			p6309->W_REG = cpuMemRead16(&p6309->cpu,DPADDRESS(p6309,p6309->PC_REG++));
			p6309->cc[Z] = ZTEST(p6309->W_REG);
			p6309->cc[N] = NTEST16(p6309->W_REG);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M65];
		break;

		case STW_D: //1097 6309
			cpuMemWrite16(&p6309->cpu,p6309->W_REG,DPADDRESS(p6309,p6309->PC_REG++));
			p6309->cc[Z] = ZTEST(p6309->W_REG);
			p6309->cc[N] = NTEST16(p6309->W_REG);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M65];
		break;

		case EORD_D: //1098 6309 Untested
			p6309->D_REG^=cpuMemRead16(&p6309->cpu,DPADDRESS(p6309,p6309->PC_REG++));
			p6309->cc[N] = NTEST16(p6309->D_REG);
			p6309->cc[Z] = ZTEST(p6309->D_REG);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M75];
		break;

		case ADCD_D: //1099 6309
				assert(0);
			//WriteLog("Hitting UNEMULATED INS ADCD_D",TOCONS);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M75];
		
		break;

		case ORD_D: //109A 6309 Untested
			p6309->D_REG|=cpuMemRead16(&p6309->cpu,DPADDRESS(p6309,p6309->PC_REG++));
			p6309->cc[N] = NTEST16(p6309->D_REG);
			p6309->cc[Z] = ZTEST(p6309->D_REG);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M75];
		break;

		case ADDW_D: //109B 6309
			temp16=cpuMemRead16(&p6309->cpu, DPADDRESS(p6309,p6309->PC_REG++));
			temp32= p6309->W_REG+ temp16;
			p6309->cc[C] =(temp32 & 0x10000)>>16;
			p6309->cc[V] = OVERFLOW16(p6309->cc[C],temp32,temp16,p6309->W_REG);
			p6309->W_REG = temp32;
			p6309->cc[Z] = ZTEST(p6309->W_REG);
			p6309->cc[N] = NTEST16(p6309->W_REG);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M75];
		break;

		case CMPY_D:	//109C
			postword=cpuMemRead16(&p6309->cpu,DPADDRESS(p6309,p6309->PC_REG++));
			temp16= p6309->Y_REG - postword ;
			p6309->cc[C] = temp16 > p6309->Y_REG;
			p6309->cc[V] = OVERFLOW16(p6309->cc[C],postword,temp16,p6309->Y_REG);
			p6309->cc[N] = NTEST16(temp16);
			p6309->cc[Z] = ZTEST(temp16);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M75];
			break;

		case LDY_D: //109E
			p6309->Y_REG=cpuMemRead16(&p6309->cpu,DPADDRESS(p6309,p6309->PC_REG++));
			p6309->cc[Z] = ZTEST(p6309->Y_REG);	
			p6309->cc[N] = NTEST16(p6309->Y_REG);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M65];
			break;
	
		case STY_D: //109F
			cpuMemWrite16(&p6309->cpu,p6309->Y_REG,DPADDRESS(p6309,p6309->PC_REG++));
			p6309->cc[Z] = ZTEST(p6309->Y_REG);
			p6309->cc[N] = NTEST16(p6309->Y_REG);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M65];
		break;

		case SUBW_X: //10A0 6309 MODDED
			temp16=cpuMemRead16(&p6309->cpu,INDADDRESS(pCPU,p6309->PC_REG++));
			temp32=p6309->W_REG-temp16;
			p6309->cc[C] = (temp32 & 0x10000)>>16;
			p6309->cc[V] = OVERFLOW16(p6309->cc[C],temp32,temp16,p6309->W_REG);
			p6309->W_REG= temp32;
			p6309->cc[Z] = ZTEST(p6309->W_REG);
			p6309->cc[N] = NTEST16(p6309->W_REG);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
		break;

		case CMPW_X: //10A1 6309
			postword=cpuMemRead16(&p6309->cpu,INDADDRESS(pCPU,p6309->PC_REG++));
			temp16= p6309->W_REG - postword ;
			p6309->cc[C] = temp16 > p6309->W_REG;
			p6309->cc[V] = OVERFLOW16(p6309->cc[C],postword,temp16,p6309->W_REG);
			p6309->cc[N] = NTEST16(temp16);
			p6309->cc[Z] = ZTEST(temp16);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
		break;

		case SBCD_X: //10A2 6309
			postword=cpuMemRead16(&p6309->cpu,INDADDRESS(pCPU,p6309->PC_REG++));
			temp32=p6309->D_REG-postword-p6309->cc[C];
			p6309->cc[C] = (temp32 & 0x10000)>>16;
			p6309->cc[V] = OVERFLOW16(p6309->cc[C],postword,temp32,p6309->D_REG);
			p6309->D_REG= temp32;
			p6309->cc[N] = NTEST16(p6309->D_REG);
			p6309->cc[Z] = ZTEST(p6309->D_REG);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
		break;

		case CMPD_X: //10A3
			postword=cpuMemRead16(&p6309->cpu,INDADDRESS(pCPU,p6309->PC_REG++));
			temp16= p6309->D_REG - postword ;
			p6309->cc[C] = temp16 > p6309->D_REG;
			p6309->cc[V] = OVERFLOW16(p6309->cc[C],postword,temp16,p6309->D_REG);
			p6309->cc[N] = NTEST16(temp16);
			p6309->cc[Z] = ZTEST(temp16);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
		break;

		case ANDD_X: //10A4 6309
			p6309->D_REG&=cpuMemRead16(&p6309->cpu,INDADDRESS(pCPU,p6309->PC_REG++));
			p6309->cc[N] = NTEST16(p6309->D_REG);
			p6309->cc[Z] = ZTEST(p6309->D_REG);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
		break;

		case BITD_X: //10A5 6309 Untested
			temp16= p6309->D_REG & cpuMemRead16(&p6309->cpu,INDADDRESS(pCPU,p6309->PC_REG++));
			p6309->cc[N] = NTEST16(temp16);
			p6309->cc[Z] = ZTEST(temp16);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
		break;

		case LDW_X: //10A6 6309
			p6309->W_REG=cpuMemRead16(&p6309->cpu,INDADDRESS(pCPU,p6309->PC_REG++));
			p6309->cc[Z] = ZTEST(p6309->W_REG);
			p6309->cc[N] = NTEST16(p6309->W_REG);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=6;
		break;

		case STW_X: //10A7 6309
			cpuMemWrite16(&p6309->cpu,p6309->W_REG,INDADDRESS(pCPU,p6309->PC_REG++));
			p6309->cc[Z] = ZTEST(p6309->W_REG);
			p6309->cc[N] = NTEST16(p6309->W_REG);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=6;
		break;

		case EORD_X: //10A8 6309 Untested TESTED NITRO 
			p6309->D_REG ^= cpuMemRead16(&p6309->cpu,INDADDRESS(pCPU,p6309->PC_REG++));
			p6309->cc[N] = NTEST16(p6309->D_REG);
			p6309->cc[Z] = ZTEST(p6309->D_REG);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
		break;

		case ADCD_X: //10A9 6309
			postword=cpuMemRead16(&p6309->cpu,INDADDRESS(pCPU,p6309->PC_REG++));
			temp32 = p6309->D_REG + postword + p6309->cc[C];
			p6309->cc[C] = (temp32 & 0x10000)>>16;
			p6309->cc[V] = OVERFLOW16(p6309->cc[C],postword,temp32,p6309->D_REG);
			p6309->cc[H] = (((p6309->D_REG ^ temp32 ^ postword) & 0x100)>>8);
			p6309->D_REG = temp32;
			p6309->cc[N] = NTEST16(p6309->D_REG);
			p6309->cc[Z] = ZTEST(p6309->D_REG);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
		break;

		case ORD_X: //10AA 6309 Untested wcreate
			p6309->D_REG |= cpuMemRead16(&p6309->cpu,INDADDRESS(pCPU,p6309->PC_REG++));
			p6309->cc[N] = NTEST16(p6309->D_REG);
			p6309->cc[Z] = ZTEST(p6309->D_REG);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
		break;

		case ADDW_X: //10AB 6309 Untested TESTED NITRO CHECK no Half carry?
				assert(0);
			//WriteLog(check)
			temp16=cpuMemRead16(&p6309->cpu,INDADDRESS(pCPU,p6309->PC_REG++));
			temp32= p6309->W_REG+ temp16;
			p6309->cc[C] =(temp32 & 0x10000)>>16;
			p6309->cc[V] = OVERFLOW16(p6309->cc[C],temp32,temp16,p6309->W_REG);
			p6309->W_REG= temp32;
			p6309->cc[Z] = ZTEST(p6309->W_REG);
			p6309->cc[N] = NTEST16(p6309->W_REG);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
		break;

		case CMPY_X: //10AC
			postword=cpuMemRead16(&p6309->cpu,INDADDRESS(pCPU,p6309->PC_REG++));
			temp16= p6309->Y_REG - postword ;
			p6309->cc[C] = temp16 > p6309->Y_REG;
			p6309->cc[V] = OVERFLOW16(p6309->cc[C],postword,temp16,p6309->Y_REG);
			p6309->cc[N] = NTEST16(temp16);
			p6309->cc[Z] = ZTEST(temp16);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
			break;

		case LDY_X: //10AE
			p6309->Y_REG=cpuMemRead16(&p6309->cpu,INDADDRESS(pCPU,p6309->PC_REG++));
			p6309->cc[Z] = ZTEST(p6309->Y_REG);
			p6309->cc[N] = NTEST16(p6309->Y_REG);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=6;
			break;

		case STY_X: //10AF
			cpuMemWrite16(&p6309->cpu,p6309->Y_REG,INDADDRESS(pCPU,p6309->PC_REG++));
			p6309->cc[Z] = ZTEST(p6309->Y_REG);
			p6309->cc[N] = NTEST16(p6309->Y_REG);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=6;
			break;

		case SUBW_E: //10B0 6309 Untested
			temp16=cpuMemRead16(&p6309->cpu,IMMADDRESS(p6309,p6309->PC_REG));
			temp32=p6309->W_REG-temp16;
			p6309->cc[C] = (temp32 & 0x10000)>>16;
			p6309->cc[V] = OVERFLOW16(p6309->cc[C],temp32,temp16,p6309->W_REG);
			p6309->W_REG= temp32;
			p6309->cc[Z] = ZTEST(p6309->W_REG);
			p6309->cc[N] = NTEST16(p6309->W_REG);
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M86];
		break;

		case CMPW_E: //10B1 6309 Untested
			postword=cpuMemRead16(&p6309->cpu,IMMADDRESS(p6309,p6309->PC_REG));
			temp16 = p6309->W_REG-postword;
			p6309->cc[C] = temp16 > p6309->W_REG;
			p6309->cc[V] = OVERFLOW16(p6309->cc[C],postword,temp16,p6309->W_REG);
			p6309->cc[N] = NTEST16(temp16);
			p6309->cc[Z] = ZTEST(temp16);
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M86];
		break;

		case SBCD_E: //10B2 6309
				assert(0);
			//WriteLog("Hitting UNEMULATED INS SBCD_E",TOCONS);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M86];
		break;

		case CMPD_E: //10B3
			postword=cpuMemRead16(&p6309->cpu,IMMADDRESS(p6309,p6309->PC_REG));
			temp16 = p6309->D_REG-postword;
			p6309->cc[C] = temp16 > p6309->D_REG;
			p6309->cc[V] = OVERFLOW16(p6309->cc[C],postword,temp16,p6309->D_REG);
			p6309->cc[N] = NTEST16(temp16);
			p6309->cc[Z] = ZTEST(temp16);
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M86];
			break;

		case ANDD_E: //10B4 6309 Untested
			p6309->D_REG &= cpuMemRead16(&p6309->cpu,IMMADDRESS(p6309,p6309->PC_REG));
			p6309->cc[N] = NTEST16(p6309->D_REG);
			p6309->cc[Z] = ZTEST(p6309->D_REG);
			p6309->cc[V] = 0;
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M86];
		break;

		case BITD_E: //10B5 6309 Untested CHECK NITRO
			temp16= p6309->D_REG & cpuMemRead16(&p6309->cpu,IMMADDRESS(p6309,p6309->PC_REG));
			p6309->cc[N] = NTEST16(temp16);
			p6309->cc[Z] = ZTEST(temp16);
			p6309->cc[V] = 0;
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M86];
		break;

		case LDW_E: //10B6 6309
			p6309->W_REG=cpuMemRead16(&p6309->cpu,IMMADDRESS(p6309,p6309->PC_REG));
			p6309->cc[Z] = ZTEST(p6309->W_REG);
			p6309->cc[N] = NTEST16(p6309->W_REG);
			p6309->cc[V] = 0;
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
		break;

		case STW_E: //10B7 6309
			cpuMemWrite16(&p6309->cpu,p6309->W_REG,IMMADDRESS(p6309,p6309->PC_REG));
			p6309->cc[Z] = ZTEST(p6309->W_REG);
			p6309->cc[N] = NTEST16(p6309->W_REG);
			p6309->cc[V] = 0;
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
		break;

		case EORD_E: //10B8 6309 Untested
			p6309->D_REG ^= cpuMemRead16(&p6309->cpu,IMMADDRESS(p6309,p6309->PC_REG));
			p6309->cc[N] = NTEST16(p6309->D_REG);
			p6309->cc[Z] = ZTEST(p6309->D_REG);
			p6309->cc[V] = 0;
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M86];
		break;

		case ADCD_E: //10B9 6309
			//WriteLog("Hitting UNEMULATED INS ADCD_E",TOCONS);
				assert(0);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M86];
		break;

		case ORD_E: //10BA 6309 Untested
			p6309->D_REG |= cpuMemRead16(&p6309->cpu,IMMADDRESS(p6309,p6309->PC_REG));
			p6309->cc[N] = NTEST16(p6309->D_REG);
			p6309->cc[Z] = ZTEST(p6309->D_REG);
			p6309->cc[V] = 0;
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M86];
		break;

		case ADDW_E: //10BB 6309 Untested
			temp16=cpuMemRead16(&p6309->cpu,IMMADDRESS(p6309,p6309->PC_REG));
			temp32= p6309->W_REG+ temp16;
			p6309->cc[C] =(temp32 & 0x10000)>>16;
			p6309->cc[V] = OVERFLOW16(p6309->cc[C],temp32,temp16,p6309->W_REG);
			p6309->W_REG= temp32;
			p6309->cc[Z] = ZTEST(p6309->W_REG);
			p6309->cc[N] = NTEST16(p6309->W_REG);
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M86];
		break;

		case CMPY_E: //10BC
			postword=cpuMemRead16(&p6309->cpu,IMMADDRESS(p6309,p6309->PC_REG));
			temp16 = p6309->Y_REG-postword;
			p6309->cc[C] = temp16 > p6309->Y_REG;
			p6309->cc[V] = OVERFLOW16(p6309->cc[C],postword,temp16,p6309->Y_REG);
			p6309->cc[N] = NTEST16(temp16);
			p6309->cc[Z] = ZTEST(temp16);
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M86];
			break;

		case LDY_E: //10BE
			p6309->Y_REG=cpuMemRead16(&p6309->cpu,IMMADDRESS(p6309,p6309->PC_REG));
			p6309->cc[Z] = ZTEST(p6309->Y_REG);
			p6309->cc[N] = NTEST16(p6309->Y_REG);
			p6309->cc[V] = 0;
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
			break;

		case STY_E: //10BF
			cpuMemWrite16(&p6309->cpu,p6309->Y_REG,IMMADDRESS(p6309,p6309->PC_REG));
			p6309->cc[Z] = ZTEST(p6309->Y_REG);
			p6309->cc[N] = NTEST16(p6309->Y_REG);
			p6309->cc[V] = 0;
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
			break;

		case LDS_I:  //10CE
			p6309->S_REG=IMMADDRESS(p6309,p6309->PC_REG);
			p6309->cc[Z] = ZTEST(p6309->S_REG);
			p6309->cc[N] = NTEST16(p6309->S_REG);
			p6309->cc[V] = 0;
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=4;
			break;

		case LDQ_D: //10DC 6309
			p6309->Q_REG = cpuMemRead32(pCPU,DPADDRESS(p6309,p6309->PC_REG++));
			p6309->cc[Z] = ZTEST(p6309->Q_REG);
			p6309->cc[N] = NTEST32(p6309->Q_REG);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M87];
		break;

		case STQ_D: //10DD 6309
			cpuMemWrite32(pCPU,p6309->Q_REG,DPADDRESS(p6309,p6309->PC_REG++));
			p6309->cc[Z] = ZTEST(p6309->Q_REG);
			p6309->cc[N] = NTEST32(p6309->Q_REG);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M87];
		break;

		case LDS_D: //10DE
			p6309->S_REG=cpuMemRead16(&p6309->cpu,DPADDRESS(p6309,p6309->PC_REG++));
			p6309->cc[Z] = ZTEST(p6309->S_REG);
			p6309->cc[N] = NTEST16(p6309->S_REG);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M65];
			break;

		case STS_D: //10DF 6309
			cpuMemWrite16(&p6309->cpu,p6309->S_REG,DPADDRESS(p6309,p6309->PC_REG++));
			p6309->cc[Z] = ZTEST(p6309->S_REG);
			p6309->cc[N] = NTEST16(p6309->S_REG);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M65];
			break;

		case LDQ_X: //10EC 6309
			p6309->Q_REG=cpuMemRead32(pCPU,INDADDRESS(pCPU,p6309->PC_REG++));
			p6309->cc[Z] = ZTEST(p6309->Q_REG);
			p6309->cc[N] = NTEST32(p6309->Q_REG);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=8;
			break;

		case STQ_X: //10ED 6309 DONE
			cpuMemWrite32(pCPU,p6309->Q_REG,INDADDRESS(pCPU,p6309->PC_REG++));
			p6309->cc[Z] = ZTEST(p6309->Q_REG);
			p6309->cc[N] = NTEST32(p6309->Q_REG);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=8;
			break;


		case LDS_X: //10EE
			p6309->S_REG=cpuMemRead16(&p6309->cpu,INDADDRESS(pCPU,p6309->PC_REG++));
			p6309->cc[Z] = ZTEST(p6309->S_REG);
			p6309->cc[N] = NTEST16(p6309->S_REG);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=6;
			break;

		case STS_X: //10EF 6309
			cpuMemWrite16(&p6309->cpu,p6309->S_REG,INDADDRESS(pCPU,p6309->PC_REG++));
			p6309->cc[Z] = ZTEST(p6309->S_REG);
			p6309->cc[N] = NTEST16(p6309->S_REG);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=6;
			break;

		case LDQ_E: //10FC 6309
			p6309->Q_REG=cpuMemRead32(pCPU,IMMADDRESS(p6309,p6309->PC_REG));
			p6309->cc[Z] = ZTEST(p6309->Q_REG);
			p6309->cc[N] = NTEST32(p6309->Q_REG);
			p6309->cc[V] = 0;
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M98];
		break;

		case STQ_E: //10FD 6309
			cpuMemWrite32(pCPU,p6309->Q_REG,IMMADDRESS(p6309,p6309->PC_REG));
			p6309->cc[Z] = ZTEST(p6309->Q_REG);
			p6309->cc[N] = NTEST32(p6309->Q_REG);
			p6309->cc[V] = 0;
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M98];
		break;

		case LDS_E: //10FE
			p6309->S_REG=cpuMemRead16(&p6309->cpu,IMMADDRESS(p6309,p6309->PC_REG));
			p6309->cc[Z] = ZTEST(p6309->S_REG);
			p6309->cc[N] = NTEST16(p6309->S_REG);
			p6309->cc[V] = 0;
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
			break;

		case STS_E: //10FF 6309
			cpuMemWrite16(&p6309->cpu,p6309->S_REG,IMMADDRESS(p6309,p6309->PC_REG));
			p6309->cc[Z] = ZTEST(p6309->S_REG);
			p6309->cc[N] = NTEST16(p6309->S_REG);
			p6309->cc[V] = 0;
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
			break;

		default:
			InvalidInsHandler(pCPU);
			break;
	} //Page 2 switch END
	break;
case	Page3:

		switch (cpuMemRead8(pCPU,p6309->PC_REG++))
		{
		case BAND: //1130 6309
				assert(0);
			//WriteLog("Hitting UNEMULATED INS BAND",TOCONS);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
		break;

		case BIAND: //1131 6309
				assert(0);
			//WriteLog("Hitting UNEMULATED INS BIAND",TOCONS);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
		break;

		case BOR: //1132 6309
				assert(0);
			//WriteLog("Hitting UNEMULATED INS BOR",TOCONS);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
		break;

		case BIOR: //1133 6309
				assert(0);
			//WriteLog("Hitting UNEMULATED INS BIOR",TOCONS);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
		break;

		case BEOR: //1134 6309
				assert(0);
			//WriteLog("Hitting UNEMULATED INS BEOR",TOCONS);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
		break;

		case BIEOR: //1135 6309
				assert(0);
			//WriteLog("Hitting UNEMULATED INS BIEOR",TOCONS);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
		break;

		case LDBT: //1136 6309
				assert(0);
			//WriteLog("Hitting UNEMULATED INS LDBT",TOCONS);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
		break;

		case STBT: //1137 6309
				assert(0);
			//WriteLog("Hitting UNEMULATED INS STBT",TOCONS);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M87];
		break;

		case TFM1: //1138 TFM R+,R+ 6309
			postbyte=cpuMemRead8(pCPU,p6309->PC_REG);
			Source=postbyte>>4;
			Dest=postbyte&15;
			if ((p6309->W_REG)>0)
			{
				temp8=cpuMemRead8(pCPU,*p6309->xfreg16[Source]);
				cpuMemWrite8(pCPU,temp8,*p6309->xfreg16[Dest]);
				(*p6309->xfreg16[Dest])++;
				(*p6309->xfreg16[Source])++;
				p6309->W_REG--;
				p6309->cpu.CycleCounter+=3;
				p6309->PC_REG-=2;
			}
			else
			{
				p6309->cpu.CycleCounter+=6;
				p6309->PC_REG++;
			}
		break;

		case TFM2: //1139 TFM R-,R- Phase 3
			postbyte=cpuMemRead8(pCPU,p6309->PC_REG);
			Source=postbyte>>4;
			Dest=postbyte&15;

			if (p6309->W_REG>0)
			{
				temp8=cpuMemRead8(pCPU,*p6309->xfreg16[Source]);
				cpuMemWrite8(pCPU,temp8,*p6309->xfreg16[Dest]);
				(*p6309->xfreg16[Dest])--;
				(*p6309->xfreg16[Source])--;
				p6309->W_REG--;
				p6309->cpu.CycleCounter+=3;
				p6309->PC_REG-=2;
			}
			else
			{
				p6309->cpu.CycleCounter+=6;
				p6309->PC_REG++;
			}			
		break;

		case TFM3: //113A 6309
			//WriteLog("Hitting TFM3",TOCONS);
				assert(0);
			p6309->cpu.CycleCounter+=6;
		break;

		case TFM4: //113B TFM R,R+ 6309 
			postbyte=cpuMemRead8(pCPU,p6309->PC_REG);
			Source=postbyte>>4;
			Dest=postbyte&15;

			if (p6309->W_REG>0)
			{
				temp8=cpuMemRead8(pCPU,*p6309->xfreg16[Source]);
				cpuMemWrite8(pCPU,temp8,*p6309->xfreg16[Dest]);
				(*p6309->xfreg16[Dest])++;
				p6309->W_REG--;
				p6309->PC_REG-=2; //Hit the same instruction on the next loop if not done copying
				p6309->cpu.CycleCounter+=3;
			}
			else
			{
				p6309->cpu.CycleCounter+=6;
				p6309->PC_REG++;
			}
		break;

		case BITMD_M: //113C  6309
				assert(0);
			//WriteLog("Hitting bitmd_m",TOCONS);
			p6309->cpu.CycleCounter+=4;
		break;

		case LDMD_M: //113D DONE 6309
			p6309->mdbits = cpuMemRead8(pCPU,p6309->PC_REG++);
			setmd(pCPU,p6309->mdbits);
			p6309->cpu.CycleCounter+=5;
		break;

		case SWI3_I: //113F
			p6309->cc[E]=1;
			cpuMemWrite8(pCPU, p6309->pc.B.lsb,--p6309->S_REG);
			cpuMemWrite8(pCPU, p6309->pc.B.msb,--p6309->S_REG);
			cpuMemWrite8(pCPU, p6309->u.B.lsb,--p6309->S_REG);
			cpuMemWrite8(pCPU, p6309->u.B.msb,--p6309->S_REG);
			cpuMemWrite8(pCPU, p6309->y.B.lsb,--p6309->S_REG);
			cpuMemWrite8(pCPU, p6309->y.B.msb,--p6309->S_REG);
			cpuMemWrite8(pCPU, p6309->x.B.lsb,--p6309->S_REG);
			cpuMemWrite8(pCPU, p6309->x.B.msb,--p6309->S_REG);
			cpuMemWrite8(pCPU, p6309->dp.B.msb,--p6309->S_REG);
			if (p6309->md[NATIVE6309])
			{
				cpuMemWrite8(pCPU,(p6309->F_REG),--p6309->S_REG);
				cpuMemWrite8(pCPU,(p6309->E_REG),--p6309->S_REG);
				p6309->cpu.CycleCounter+=2;
			}
			cpuMemWrite8(pCPU,p6309->B_REG,--p6309->S_REG);
			cpuMemWrite8(pCPU,p6309->A_REG,--p6309->S_REG);
			cpuMemWrite8(pCPU,getcc(pCPU),--p6309->S_REG);
			p6309->PC_REG=cpuMemRead16(&p6309->cpu,VSWI3);
			p6309->cpu.CycleCounter+=20;
			break;

		case COME_I: //1143 6309 Untested
			p6309->E_REG = 0xFF- p6309->E_REG;
			p6309->cc[Z] = ZTEST(p6309->E_REG);
			p6309->cc[N] = NTEST8(p6309->E_REG);
			p6309->cc[C] = 1;
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M32];
		break;

		case DECE_I: //114A 6309
			p6309->E_REG--;
			p6309->cc[Z] = ZTEST(p6309->E_REG);
			p6309->cc[N] = NTEST8(p6309->E_REG);
			p6309->cc[V] = p6309->E_REG==0x7F;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M32];
		break;

		case INCE_I: //114C 6309
			p6309->E_REG++;
			p6309->cc[Z] = ZTEST(p6309->E_REG);
			p6309->cc[N] = NTEST8(p6309->E_REG);
			p6309->cc[V] = p6309->E_REG==0x80;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M32];
		break;

		case TSTE_I: //114D 6309 Untested TESTED NITRO
			p6309->cc[Z] = ZTEST(p6309->E_REG);
			p6309->cc[N] = NTEST8(p6309->E_REG);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M32];
		break;

		case CLRE_I: //114F 6309
			p6309->E_REG= 0;
			p6309->cc[C] = 0;
			p6309->cc[V] = 0;
			p6309->cc[N] = 0;
			p6309->cc[Z] = 1;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M32];
		break;

		case COMF_I: //1153 6309 Untested
			p6309->F_REG= 0xFF- p6309->F_REG;
			p6309->cc[Z] = ZTEST(p6309->F_REG);
			p6309->cc[N] = NTEST8(p6309->F_REG);
			p6309->cc[C] = 1;
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M32];
		break;

		case DECF_I: //115A 6309
			p6309->F_REG--;
			p6309->cc[Z] = ZTEST(p6309->F_REG);
			p6309->cc[N] = NTEST8(p6309->F_REG);
			p6309->cc[V] = p6309->F_REG==0x7F;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M21];
		break;

		case INCF_I: //115C 6309 Untested
			p6309->F_REG++;
			p6309->cc[Z] = ZTEST(p6309->F_REG);
			p6309->cc[N] = NTEST8(p6309->F_REG);
			p6309->cc[V] = p6309->F_REG==0x80;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M32];
		break;

		case TSTF_I: //115D 6309
			p6309->cc[Z] = ZTEST(p6309->F_REG);
			p6309->cc[N] = NTEST8(p6309->F_REG);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M32];
		break;

		case CLRF_I: //115F 6309 Untested wcreate
			p6309->F_REG= 0;
			p6309->cc[C] = 0;
			p6309->cc[V] = 0;
			p6309->cc[N] = 0;
			p6309->cc[Z] = 1;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M32];
		break;

		case SUBE_M: //1180 6309 Untested
			postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
			temp16 = p6309->E_REG - postbyte;
			p6309->cc[C] =(temp16 & 0x100)>>8; 
			p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->E_REG);
			p6309->E_REG= (unsigned char)temp16;
			p6309->cc[Z] = ZTEST(p6309->E_REG);
			p6309->cc[N] = NTEST8(p6309->E_REG);
			p6309->cpu.CycleCounter+=3;
		break;

		case CMPE_M: //1181 6309
			postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
			temp8= p6309->E_REG-postbyte;
			p6309->cc[C] = temp8 > p6309->E_REG;
			p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp8,p6309->E_REG);
			p6309->cc[N] = NTEST8(temp8);
			p6309->cc[Z] = ZTEST(temp8);
			p6309->cpu.CycleCounter+=3;
		break;

		case CMPU_M: //1183
			postword=IMMADDRESS(p6309,p6309->PC_REG);
			temp16 = p6309->U_REG-postword;
			p6309->cc[C] = temp16 > p6309->U_REG;
			p6309->cc[V] = OVERFLOW16(p6309->cc[C],postword,temp16,p6309->U_REG);
			p6309->cc[N] = NTEST16(temp16);
			p6309->cc[Z] = ZTEST(temp16);
			p6309->PC_REG+=2;	
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
		break;

		case LDE_M: //1186 6309
			p6309->E_REG= cpuMemRead8(pCPU,p6309->PC_REG++);
			p6309->cc[Z] = ZTEST(p6309->E_REG);
			p6309->cc[N] = NTEST8(p6309->E_REG);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=3;
		break;

		case ADDE_M: //118B 6309
			postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
			temp16=p6309->E_REG+postbyte;
			p6309->cc[C] =(temp16 & 0x100)>>8;
			p6309->cc[H] = ((p6309->E_REG ^ postbyte ^ temp16) & 0x10)>>4;
			p6309->cc[V] =OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->E_REG);
			p6309->E_REG= (unsigned char)temp16;
			p6309->cc[N] = NTEST8(p6309->E_REG);
			p6309->cc[Z] = ZTEST(p6309->E_REG);
			p6309->cpu.CycleCounter+=3;
		break;

		case CMPS_M: //118C
			postword=IMMADDRESS(p6309,p6309->PC_REG);
			temp16 = p6309->S_REG-postword;
			p6309->cc[C] = temp16 > p6309->S_REG;
			p6309->cc[V] = OVERFLOW16(p6309->cc[C],postword,temp16,p6309->S_REG);
			p6309->cc[N] = NTEST16(temp16);
			p6309->cc[Z] = ZTEST(temp16);
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
			break;

		case DIVD_M: //118D 6309 NITRO
			*spostbyte=(signed char)cpuMemRead8(pCPU,p6309->PC_REG++);
			if (*spostbyte)
			{	
				*spostword=p6309->D_REG;
				stemp16= (signed short)p6309->D_REG / *spostbyte;
				p6309->A_REG = (signed short)p6309->D_REG % *spostbyte;
				p6309->B_REG=(unsigned char)stemp16;

				p6309->cc[Z] = ZTEST(p6309->B_REG);
				p6309->cc[N] = NTEST16(p6309->D_REG);
				p6309->cc[C] = p6309->B_REG & 1;
				p6309->cc[V] =(stemp16 >127) | (stemp16 <-128);

				if ( (stemp16 > 255) | (stemp16 < -256) ) //Abort
				{
					p6309->D_REG=abs (*spostword);
					p6309->cc[N] = NTEST16(p6309->D_REG);
					p6309->cc[Z] = ZTEST(p6309->D_REG);
				}
				p6309->cpu.CycleCounter+=25;
			}
			else
			{
				p6309->cpu.CycleCounter+=17;
				DivbyZero(pCPU);				
			}
		break;

		case DIVQ_M: //118E 6309
            if (performDivQ(pCPU,(short)IMMADDRESS(p6309,p6309->PC_REG)))
            {
                p6309->cpu.CycleCounter+=36;
                p6309->PC_REG += 2; //PC_REG+2 + 2 more for a total of 4
            }
            else
            {
                p6309->cpu.CycleCounter+=17;
            }
            break;
		break;

		case MULD_M: //118F Phase 5 6309
			p6309->Q_REG=  p6309->D_REG * IMMADDRESS(p6309,p6309->PC_REG);
			p6309->cc[C] = 0; 
			p6309->cc[Z] = ZTEST(p6309->D_REG);
			p6309->cc[V] = 0;
			p6309->cc[N] = NTEST16(p6309->D_REG);
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=28;
		break;

		case SUBE_D: //1190 6309 Untested HERE
			postbyte=cpuMemRead8(pCPU, DPADDRESS(p6309,p6309->PC_REG++));
			temp16 = p6309->E_REG - postbyte;
			p6309->cc[C] =(temp16 & 0x100)>>8; 
			p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->E_REG);
			p6309->E_REG= (unsigned char)temp16;
			p6309->cc[Z] = ZTEST(p6309->E_REG);
			p6309->cc[N] = NTEST8(p6309->E_REG);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
		break;

		case CMPE_D: //1191 6309 Untested
			postbyte=cpuMemRead8(pCPU,DPADDRESS(p6309,p6309->PC_REG++));
			temp8= p6309->E_REG-postbyte;
			p6309->cc[C] = temp8 > p6309->E_REG;
			p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp8,p6309->E_REG);
			p6309->cc[N] = NTEST8(temp8);
			p6309->cc[Z] = ZTEST(temp8);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
		break;

		case CMPU_D: //1193
			postword=cpuMemRead16(&p6309->cpu,DPADDRESS(p6309,p6309->PC_REG++));
			temp16= p6309->U_REG - postword ;
			p6309->cc[C] = temp16 > p6309->U_REG;
			p6309->cc[V] = OVERFLOW16(p6309->cc[C],postword,temp16,p6309->U_REG);
			p6309->cc[N] = NTEST16(temp16);
			p6309->cc[Z] = ZTEST(temp16);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M75];
			break;

		case LDE_D: //1196 6309
			p6309->E_REG= cpuMemRead8(pCPU,DPADDRESS(p6309,p6309->PC_REG++));
			p6309->cc[Z] = ZTEST(p6309->E_REG);
			p6309->cc[N] = NTEST8(p6309->E_REG);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
		break;

		case STE_D: //1197 Phase 5 6309
			cpuMemWrite8(pCPU, p6309->E_REG,DPADDRESS(p6309,p6309->PC_REG++));
			p6309->cc[Z] = ZTEST(p6309->E_REG);
			p6309->cc[N] = NTEST8(p6309->E_REG);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
		break;

		case ADDE_D: //119B 6309 Untested
			postbyte=cpuMemRead8(pCPU,DPADDRESS(p6309,p6309->PC_REG++));
			temp16=p6309->E_REG+postbyte;
			p6309->cc[C] = (temp16 & 0x100)>>8;
			p6309->cc[H] = ((p6309->E_REG ^ postbyte ^ temp16) & 0x10)>>4;
			p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->E_REG);
			p6309->E_REG= (unsigned char)temp16;
			p6309->cc[N] = NTEST8(p6309->E_REG);
			p6309->cc[Z] =ZTEST(p6309->E_REG);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
		break;

		case CMPS_D: //119C
			postword=cpuMemRead16(&p6309->cpu,DPADDRESS(p6309,p6309->PC_REG++));
			temp16= p6309->S_REG - postword ;
			p6309->cc[C] = temp16 > p6309->S_REG;
			p6309->cc[V] = OVERFLOW16(p6309->cc[C],postword,temp16,p6309->S_REG);
			p6309->cc[N] = NTEST16(temp16);
			p6309->cc[Z] = ZTEST(temp16);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M75];
			break;

		case DIVD_D: //119D 6309 02292008
			*spostbyte=(signed char)cpuMemRead8(pCPU,DPADDRESS(p6309,p6309->PC_REG++));
			if (*spostbyte)
			{	
				*spostword=p6309->D_REG;
				stemp16= (signed short)p6309->D_REG / *spostbyte;
				p6309->A_REG = (signed short)p6309->D_REG % *spostbyte;
				p6309->B_REG=(unsigned char)stemp16;

				p6309->cc[Z] = ZTEST(p6309->B_REG);
				p6309->cc[N] = NTEST16(p6309->D_REG);
				p6309->cc[C] = p6309->B_REG & 1;
				p6309->cc[V] =(stemp16 >127) | (stemp16 <-128);

				if ( (stemp16 > 255) | (stemp16 < -256) ) //Abort
				{
					p6309->D_REG=abs (*spostword);
					p6309->cc[N] = NTEST16(p6309->D_REG);
					p6309->cc[Z] = ZTEST(p6309->D_REG);
				}
				p6309->cpu.CycleCounter+=27;
			}
			else
			{
				p6309->cpu.CycleCounter+=19;
				DivbyZero(pCPU);				
			}
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M2726];
		break;

		case DIVQ_D: //119E 6309
            if (performDivQ(pCPU,cpuMemRead16(&p6309->cpu,DPADDRESS(p6309,p6309->PC_REG))))
            {
                p6309->cpu.CycleCounter += 35;
                p6309->PC_REG += 1;
            }
            else
            {
                p6309->cpu.CycleCounter += 17;
            }
		break;

		case MULD_D: //119F 6309 02292008
			p6309->Q_REG=  p6309->D_REG * cpuMemRead16(&p6309->cpu,DPADDRESS(p6309,p6309->PC_REG++));
			p6309->cc[C] = 0;
			p6309->cc[Z] = ZTEST(p6309->D_REG);
			p6309->cc[V] = 0;
			p6309->cc[N] = NTEST16(p6309->D_REG);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M3029];
		break;

		case SUBE_X: //11A0 6309 Untested
			postbyte=cpuMemRead8(pCPU,INDADDRESS(pCPU,p6309->PC_REG++));
			temp16 = p6309->E_REG - postbyte;
			p6309->cc[C] =(temp16 & 0x100)>>8; 
			p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->E_REG);
			p6309->E_REG= (unsigned char)temp16;
			p6309->cc[Z] = ZTEST(p6309->E_REG);
			p6309->cc[N] = NTEST8(p6309->E_REG);
			p6309->cpu.CycleCounter+=5;
		break;

		case CMPE_X: //11A1 6309
			postbyte=cpuMemRead8(pCPU,INDADDRESS(pCPU,p6309->PC_REG++));
			temp8= p6309->E_REG-postbyte;
			p6309->cc[C] = temp8 > p6309->E_REG;
			p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp8,p6309->E_REG);
			p6309->cc[N] = NTEST8(temp8);
			p6309->cc[Z] = ZTEST(temp8);
			p6309->cpu.CycleCounter+=5;
		break;

		case CMPU_X: //11A3
			postword=cpuMemRead16(&p6309->cpu,INDADDRESS(pCPU,p6309->PC_REG++));
			temp16= p6309->U_REG - postword ;
			p6309->cc[C] = temp16 > p6309->U_REG;
			p6309->cc[V] = OVERFLOW16(p6309->cc[C],postword,temp16,p6309->U_REG);
			p6309->cc[N] = NTEST16(temp16);
			p6309->cc[Z] = ZTEST(temp16);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
			break;

		case LDE_X: //11A6 6309
			p6309->E_REG= cpuMemRead8(pCPU,INDADDRESS(pCPU,p6309->PC_REG++));
			p6309->cc[Z] = ZTEST(p6309->E_REG);
			p6309->cc[N] = NTEST8(p6309->E_REG);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=5;
		break;

		case STE_X: //11A7 6309
			cpuMemWrite8(pCPU,p6309->E_REG,INDADDRESS(pCPU,p6309->PC_REG++));
			p6309->cc[Z] = ZTEST(p6309->E_REG);
			p6309->cc[N] = NTEST8(p6309->E_REG);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=5;
		break;

		case ADDE_X: //11AB 6309 Untested
			postbyte=cpuMemRead8(pCPU,INDADDRESS(pCPU,p6309->PC_REG++));
			temp16=p6309->E_REG+postbyte;
			p6309->cc[C] =(temp16 & 0x100)>>8;
			p6309->cc[H] = ((p6309->E_REG ^ postbyte ^ temp16) & 0x10)>>4;
			p6309->cc[V] =OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->E_REG);
			p6309->E_REG= (unsigned char)temp16;
			p6309->cc[N] = NTEST8(p6309->E_REG);
			p6309->cc[Z] = ZTEST(p6309->E_REG);
			p6309->cpu.CycleCounter+=5;
		break;

		case CMPS_X:  //11AC
			postword=cpuMemRead16(&p6309->cpu,INDADDRESS(pCPU,p6309->PC_REG++));
			temp16= p6309->S_REG - postword ;
			p6309->cc[C] = temp16 > p6309->S_REG;
			p6309->cc[V] = OVERFLOW16(p6309->cc[C],postword,temp16,p6309->S_REG);
			p6309->cc[N] = NTEST16(temp16);
			p6309->cc[Z] = ZTEST(temp16);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
			break;

		case DIVD_X: //11AD wcreate  6309
			*spostbyte=(signed char)cpuMemRead8(pCPU,INDADDRESS(pCPU,p6309->PC_REG++));
			if (*spostbyte)
			{	
				*spostword=p6309->D_REG;
				stemp16= (signed short)p6309->D_REG / *spostbyte;
				p6309->A_REG = (signed short)p6309->D_REG % *spostbyte;
				p6309->B_REG=(unsigned char)stemp16;

				p6309->cc[Z] = ZTEST(p6309->B_REG);
				p6309->cc[N] = NTEST16(p6309->D_REG);	//cc[N] = NTEST8(p6309->B_REG);
				p6309->cc[C] = p6309->B_REG & 1;
				p6309->cc[V] =(stemp16 >127) | (stemp16 <-128);

				if ( (stemp16 > 255) | (stemp16 < -256) ) //Abort
				{
					p6309->D_REG=abs (*spostword);
					p6309->cc[N] = NTEST16(p6309->D_REG);
					p6309->cc[Z] = ZTEST(p6309->D_REG);
				}
				p6309->cpu.CycleCounter+=27;
			}
			else
			{
				p6309->cpu.CycleCounter+=19;
				DivbyZero(pCPU);				
			}
		break;

		case DIVQ_X: //11AE 6309
            if (performDivQ(pCPU,cpuMemRead16(&p6309->cpu,cpuMemRead16(pCPU,p6309->PC_REG++))))
            {
                p6309->cpu.CycleCounter += 36;
            }
            else
            {
                p6309->cpu.CycleCounter += 17;
            }
		break;

		case MULD_X: //11AF 6309 CHECK
			p6309->Q_REG=  p6309->D_REG * cpuMemRead16(&p6309->cpu,INDADDRESS(pCPU,p6309->PC_REG++));
			p6309->cc[C] = 0;
			p6309->cc[Z] = ZTEST(p6309->D_REG);
			p6309->cc[V] = 0;
			p6309->cc[N] = NTEST16(p6309->D_REG);
			p6309->cpu.CycleCounter+=30;
		break;

		case SUBE_E: //11B0 6309 Untested
			postbyte=cpuMemRead8(pCPU,IMMADDRESS(p6309,p6309->PC_REG));
			temp16 = p6309->E_REG - postbyte;
			p6309->cc[C] =(temp16 & 0x100)>>8; 
			p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->E_REG);
			p6309->E_REG= (unsigned char)temp16;
			p6309->cc[Z] = ZTEST(p6309->E_REG);
			p6309->cc[N] = NTEST8(p6309->E_REG);
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M65];
		break;

		case CMPE_E: //11B1 6309 Untested
			postbyte=cpuMemRead8(pCPU,IMMADDRESS(p6309,p6309->PC_REG));
			temp8= p6309->E_REG-postbyte;
			p6309->cc[C] = temp8 > p6309->E_REG;
			p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp8,p6309->E_REG);
			p6309->cc[N] = NTEST8(temp8);
			p6309->cc[Z] = ZTEST(temp8);
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M65];
		break;

		case CMPU_E: //11B3
			postword=cpuMemRead16(&p6309->cpu,IMMADDRESS(p6309,p6309->PC_REG));
			temp16 = p6309->U_REG-postword;
			p6309->cc[C] = temp16 > p6309->U_REG;
			p6309->cc[V] = OVERFLOW16(p6309->cc[C],postword,temp16,p6309->U_REG);
			p6309->cc[N] = NTEST16(temp16);
			p6309->cc[Z] = ZTEST(temp16);
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M86];
			break;

		case LDE_E: //11B6 6309
			p6309->E_REG= cpuMemRead8(pCPU,IMMADDRESS(p6309,p6309->PC_REG));
			p6309->cc[Z] = ZTEST(p6309->E_REG);
			p6309->cc[N] = NTEST8(p6309->E_REG);
			p6309->cc[V] = 0;
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M65];
		break;

		case STE_E: //11B7 6309
			cpuMemWrite8(pCPU,p6309->E_REG,IMMADDRESS(p6309,p6309->PC_REG));
			p6309->cc[Z] = ZTEST(p6309->E_REG);
			p6309->cc[N] = NTEST8(p6309->E_REG);
			p6309->cc[V] = 0;
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M65];
		break;

		case ADDE_E: //11BB 6309 Untested
			postbyte=cpuMemRead8(pCPU,IMMADDRESS(p6309,p6309->PC_REG));
			temp16=p6309->E_REG+postbyte;
			p6309->cc[C] = (temp16 & 0x100)>>8;
			p6309->cc[H] = ((p6309->E_REG ^ postbyte ^ temp16) & 0x10)>>4;
			p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->E_REG);
			p6309->E_REG= (unsigned char)temp16;
			p6309->cc[N] = NTEST8(p6309->E_REG);
			p6309->cc[Z] = ZTEST(p6309->E_REG);
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M65];
		break;

		case CMPS_E: //11BC
			postword=cpuMemRead16(&p6309->cpu,IMMADDRESS(p6309,p6309->PC_REG));
			temp16 = p6309->S_REG-postword;
			p6309->cc[C] = temp16 > p6309->S_REG;
			p6309->cc[V] = OVERFLOW16(p6309->cc[C],postword,temp16,p6309->S_REG);
			p6309->cc[N] = NTEST16(temp16);
			p6309->cc[Z] = ZTEST(temp16);
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M86];
			break;

		case DIVD_E: //11BD 6309 02292008 Untested
			*spostbyte=(signed char)cpuMemRead8(pCPU,IMMADDRESS(p6309,p6309->PC_REG));
			if (*spostbyte)
			{	
				*spostword=p6309->D_REG;
				stemp16= (signed short)p6309->D_REG / *spostbyte;
				p6309->A_REG = (signed short)p6309->D_REG % *spostbyte;
				p6309->B_REG=(unsigned char)stemp16;

				p6309->cc[Z] = ZTEST(p6309->B_REG);
				p6309->cc[N] = NTEST16(p6309->D_REG);
				p6309->cc[C] = p6309->B_REG & 1;
				p6309->cc[V] =(stemp16 >127) | (stemp16 <-128);

				if ( (stemp16 > 255) | (stemp16 < -256) ) //Abort
				{
					p6309->D_REG=abs (*spostword);
					p6309->cc[N] = NTEST16(p6309->D_REG);
					p6309->cc[Z] = ZTEST(p6309->D_REG);
				}
				p6309->cpu.CycleCounter+=25;
			}
			else
			{
				p6309->cpu.CycleCounter+=17;
				DivbyZero(pCPU);				
			}
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M2827];
		break;

		case DIVQ_E: //11BE Phase 5 6309 CHECK
            temp16 = IMMADDRESS(p6309,p6309->PC_REG);
            if (performDivQ(pCPU,cpuMemRead16(pCPU,temp16)))
            {
                p6309->cpu.CycleCounter += 37;
                p6309->PC_REG += 2;
            }
            else
            {
                p6309->cpu.CycleCounter += 17;
            }
		break;

		case MULD_E: //11BF 6309
			p6309->Q_REG=  p6309->D_REG * cpuMemRead16(&p6309->cpu,IMMADDRESS(p6309,p6309->PC_REG));
			p6309->cc[C] = 0;
			p6309->cc[Z] = ZTEST(p6309->D_REG);
			p6309->cc[V] = 0;
			p6309->cc[N] = NTEST16(p6309->D_REG);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M3130];
		break;

		case SUBF_M: //11C0 6309 Untested
			postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
			temp16 = p6309->F_REG - postbyte;
			p6309->cc[C] = (temp16 & 0x100)>>8; 
			p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->F_REG);
			p6309->F_REG= (unsigned char)temp16;
			p6309->cc[Z] = ZTEST(p6309->F_REG);
			p6309->cc[N] = NTEST8(p6309->F_REG);
			p6309->cpu.CycleCounter+=3;
		break;

		case CMPF_M: //11C1 6309
			postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
			temp8= p6309->F_REG-postbyte;
			p6309->cc[C] = temp8 > p6309->F_REG;
			p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp8,p6309->F_REG);
			p6309->cc[N] = NTEST8(temp8);
			p6309->cc[Z] = ZTEST(temp8);
			p6309->cpu.CycleCounter+=3;
		break;

		case LDF_M: //11C6 6309
			p6309->F_REG= cpuMemRead8(pCPU,p6309->PC_REG++);
			p6309->cc[Z] = ZTEST(p6309->F_REG);
			p6309->cc[N] = NTEST8(p6309->F_REG);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=3;
		break;

		case ADDF_M: //11CB 6309 Untested
			postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
			temp16=p6309->F_REG+postbyte;
			p6309->cc[C] = (temp16 & 0x100)>>8;
			p6309->cc[H] = ((p6309->F_REG ^ postbyte ^ temp16) & 0x10)>>4;
			p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->F_REG);
			p6309->F_REG= (unsigned char)temp16;
			p6309->cc[N] = NTEST8(p6309->F_REG);
			p6309->cc[Z] = ZTEST(p6309->F_REG);
			p6309->cpu.CycleCounter+=3;
		break;

		case SUBF_D: //11D0 6309 Untested
			postbyte=cpuMemRead8(pCPU, DPADDRESS(p6309,p6309->PC_REG++));
			temp16 = p6309->F_REG - postbyte;
			p6309->cc[C] =(temp16 & 0x100)>>8; 
			p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->F_REG);
			p6309->F_REG= (unsigned char)temp16;
			p6309->cc[Z] = ZTEST(p6309->F_REG);
			p6309->cc[N] = NTEST8(p6309->F_REG);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
		break;

		case CMPF_D: //11D1 6309 Untested
			postbyte=cpuMemRead8(pCPU,DPADDRESS(p6309,p6309->PC_REG++));
			temp8= p6309->F_REG-postbyte;
			p6309->cc[C] = temp8 > p6309->F_REG;
			p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp8,p6309->F_REG);
			p6309->cc[N] = NTEST8(temp8);
			p6309->cc[Z] = ZTEST(temp8);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
		break;

		case LDF_D: //11D6 6309 Untested wcreate
			p6309->F_REG= cpuMemRead8(pCPU,DPADDRESS(p6309,p6309->PC_REG++));
			p6309->cc[Z] = ZTEST(p6309->F_REG);
			p6309->cc[N] = NTEST8(p6309->F_REG);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
		break;

		case STF_D: //11D7 Phase 5 6309
			cpuMemWrite8(pCPU,p6309->F_REG,DPADDRESS(p6309,p6309->PC_REG++));
			p6309->cc[Z] = ZTEST(p6309->F_REG);
			p6309->cc[N] = NTEST8(p6309->F_REG);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
		break;

		case ADDF_D: //11D8 6309 Untested
			postbyte=cpuMemRead8(pCPU,DPADDRESS(p6309,p6309->PC_REG++));
			temp16=p6309->F_REG+postbyte;
			p6309->cc[C] =(temp16 & 0x100)>>8;
			p6309->cc[H] = ((p6309->F_REG ^ postbyte ^ temp16) & 0x10)>>4;
			p6309->cc[V] =OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->F_REG);
			p6309->F_REG= (unsigned char)temp16;
			p6309->cc[N] = NTEST8(p6309->F_REG);
			p6309->cc[Z] = ZTEST(p6309->F_REG);
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
		break;

		case SUBF_X: //11E0 6309 Untested
			postbyte=cpuMemRead8(pCPU,INDADDRESS(pCPU,p6309->PC_REG++));
			temp16 = p6309->F_REG - postbyte;
			p6309->cc[C] =(temp16 & 0x100)>>8; 
			p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->F_REG);
			p6309->F_REG= (unsigned char)temp16;
			p6309->cc[Z] = ZTEST(p6309->F_REG);
			p6309->cc[N] = NTEST8(p6309->F_REG);
			p6309->cpu.CycleCounter+=5;
		break;

		case CMPF_X: //11E1 6309 Untested
			postbyte=cpuMemRead8(pCPU,INDADDRESS(pCPU,p6309->PC_REG++));
			temp8= p6309->F_REG-postbyte;
			p6309->cc[C] = temp8 > p6309->F_REG;
			p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp8,p6309->F_REG);
			p6309->cc[N] = NTEST8(temp8);
			p6309->cc[Z] = ZTEST(temp8);
			p6309->cpu.CycleCounter+=5;
		break;

		case LDF_X: //11E6 6309
			p6309->F_REG=cpuMemRead8(pCPU,INDADDRESS(pCPU,p6309->PC_REG++));
			p6309->cc[Z] = ZTEST(p6309->F_REG);
			p6309->cc[N] = NTEST8(p6309->F_REG);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=5;
		break;

		case STF_X: //11E7 Phase 5 6309
			cpuMemWrite8(pCPU,p6309->F_REG,INDADDRESS(pCPU,p6309->PC_REG++));
			p6309->cc[Z] = ZTEST(p6309->F_REG);
			p6309->cc[N] = NTEST8(p6309->F_REG);
			p6309->cc[V] = 0;
			p6309->cpu.CycleCounter+=5;
		break;

		case ADDF_X: //11EB 6309 Untested
			postbyte=cpuMemRead8(pCPU,INDADDRESS(pCPU,p6309->PC_REG++));
			temp16=p6309->F_REG+postbyte;
			p6309->cc[C] =(temp16 & 0x100)>>8;
			p6309->cc[H] = ((p6309->F_REG ^ postbyte ^ temp16) & 0x10)>>4;
			p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->F_REG);
			p6309->F_REG= (unsigned char)temp16;
			p6309->cc[N] = NTEST8(p6309->F_REG);
			p6309->cc[Z] = ZTEST(p6309->F_REG);
			p6309->cpu.CycleCounter+=5;
		break;

		case SUBF_E: //11F0 6309 Untested
			postbyte=cpuMemRead8(pCPU,IMMADDRESS(p6309,p6309->PC_REG));
			temp16 = p6309->F_REG - postbyte;
			p6309->cc[C] = (temp16 & 0x100)>>8; 
			p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->F_REG);
			p6309->F_REG= (unsigned char)temp16;
			p6309->cc[Z] = ZTEST(p6309->F_REG);
			p6309->cc[N] = NTEST8(p6309->F_REG);
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M65];
		break;

		case CMPF_E: //11F1 6309 Untested
			postbyte=cpuMemRead8(pCPU,IMMADDRESS(p6309,p6309->PC_REG));
			temp8= p6309->F_REG-postbyte;
			p6309->cc[C] = temp8 > p6309->F_REG;
			p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp8,p6309->F_REG);
			p6309->cc[N] = NTEST8(temp8);
			p6309->cc[Z] = ZTEST(temp8);
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M65];
		break;

		case LDF_E: //11F6 6309
			p6309->F_REG= cpuMemRead8(pCPU,IMMADDRESS(p6309,p6309->PC_REG));
			p6309->cc[Z] = ZTEST(p6309->F_REG);
			p6309->cc[N] = NTEST8(p6309->F_REG);
			p6309->cc[V] = 0;
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M65];
		break;

		case STF_E: //11F7 Phase 5 6309
			cpuMemWrite8(pCPU,p6309->F_REG,IMMADDRESS(p6309,p6309->PC_REG));
			p6309->cc[Z] = ZTEST(p6309->F_REG);
			p6309->cc[N] = NTEST8(p6309->F_REG);
			p6309->cc[V] = 0;
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M65];
		break;

		case ADDF_E: //11FB 6309 Untested
			postbyte=cpuMemRead8(pCPU,IMMADDRESS(p6309,p6309->PC_REG));
			temp16=p6309->F_REG+postbyte;
			p6309->cc[C] = (temp16 & 0x100)>>8;
			p6309->cc[H] = ((p6309->F_REG ^ postbyte ^ temp16) & 0x10)>>4;
			p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->F_REG);
			p6309->F_REG= (unsigned char)temp16;
			p6309->cc[N] = NTEST8(p6309->F_REG);
			p6309->cc[Z] = ZTEST(p6309->F_REG);
			p6309->PC_REG+=2;
			p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M65];
		break;

		default:
			InvalidInsHandler(pCPU);
		break;

	} //Page 3 switch END
	break;

case NOP_I:	//12
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M21];
	break;

case SYNC_I: //13
	p6309->cpu.CycleCounter=CycleFor;
	p6309->SyncWaiting=1;
	break;

case SEXW_I: //14 6309 CHECK
	if (p6309->W_REG & 32768)
		p6309->D_REG=0xFFFF;
	else
		p6309->D_REG=0;
	p6309->cc[Z] = ZTEST(p6309->Q_REG);
	p6309->cc[N] = NTEST16(p6309->D_REG);
	p6309->cpu.CycleCounter+=4;
	break;

case LBRA_R: //16
	*spostword=IMMADDRESS(p6309,p6309->PC_REG);
	p6309->PC_REG+=2;
	p6309->PC_REG+=*spostword;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
	break;

case	LBSR_R: //17
	*spostword=IMMADDRESS(p6309,p6309->PC_REG);
	p6309->PC_REG+=2;
	p6309->S_REG--;
	cpuMemWrite8(pCPU,p6309->pc.B.lsb,p6309->S_REG--);
	cpuMemWrite8(pCPU,p6309->pc.B.msb,p6309->S_REG);
	p6309->PC_REG+=*spostword;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M97];
	break;

case DAA_I: //19

	msn=(p6309->A_REG & 0xF0) ;
	lsn=(p6309->A_REG & 0xF);
	temp8=0;
	if ( p6309->cc[H] ||  (lsn >9) )
		temp8 |= 0x06;

	if ( (msn>0x80) && (lsn>9)) 
		temp8|=0x60;
	
	if ( (msn>0x90) || p6309->cc[C] )
		temp8|=0x60;

	temp16= p6309->A_REG+temp8;
	p6309->cc[C]|= ((temp16 & 0x100)>>8);
	p6309->A_REG= (unsigned char)temp16;
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M21];
	break;

case ORCC_M: //1A
	postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
	temp8 = getcc(pCPU);
	temp8 = (temp8 | postbyte);
	setcc(pCPU,temp8);
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M32];
	break;

case ANDCC_M: //1C
	postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
	temp8=getcc(pCPU);
	temp8 = (temp8 & postbyte);
	setcc(pCPU,temp8);
	p6309->cpu.CycleCounter+=3;
	break;

case SEX_I: //1D
	p6309->A_REG= 0-(p6309->B_REG>>7);
	p6309->cc[Z] = ZTEST(p6309->D_REG);
	p6309->cc[N] = p6309->D_REG >> 15;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M21];
	break;

case EXG_M: //1E
	postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
	//Source= postbyte>>4;
	//Dest=postbyte & 15;

	p6309->ccbits = getcc(pCPU);
	if ( ((postbyte & 0x80)>>4)==(postbyte & 0x08)) //Verify like size registers
	{
		if (postbyte & 0x08) //8 bit EXG
		{
			temp8= (*p6309->ureg8[((postbyte & 0x70) >> 4)]); //
			(*p6309->ureg8[((postbyte & 0x70) >> 4)]) = (*p6309->ureg8[postbyte & 0x07]);
			(*p6309->ureg8[postbyte & 0x07])=temp8;
		}
		else // 16 bit EXG
		{
			temp16=(*p6309->xfreg16[((postbyte & 0x70) >> 4)]);
			(*p6309->xfreg16[((postbyte & 0x70) >> 4)])=(*p6309->xfreg16[postbyte & 0x07]);
			(*p6309->xfreg16[postbyte & 0x07])=temp16;
		}
	}
	setcc(pCPU,p6309->ccbits);
	p6309->cpu.CycleCounter += p6309->InsCycles[p6309->md[NATIVE6309]][M85];
	break;

case TFR_M: //1F
	postbyte = cpuMemRead8(pCPU,p6309->PC_REG++);
	Source = postbyte>>4;
	Dest = postbyte & 15;

	switch (Dest)
	{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
			*p6309->xfreg16[Dest]=0xFFFF;
			if ( (Source==12) | (Source==13) )
				*p6309->xfreg16[Dest]=0;
			else
				if (Source<=7)
					*p6309->xfreg16[Dest]=*p6309->xfreg16[Source];
		break;

		case 8:
		case 9:
		case 10:
		case 11:
		case 14:
		case 15:
			p6309->ccbits=getcc(pCPU);
			*p6309->ureg8[Dest&7]=0xFF;
			if ( (Source==12) | (Source==13) )
				*p6309->ureg8[Dest&7]=0;
			else
				if (Source>7)
					*p6309->ureg8[Dest&7]=*p6309->ureg8[Source&7];
			setcc(pCPU,p6309->ccbits);
		break;
	}
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M64];
	break;

case BRA_R: //20
	*spostbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
	p6309->PC_REG+=*spostbyte;
	p6309->cpu.CycleCounter+=3;
	break;

case BRN_R: //21
	p6309->cpu.CycleCounter+=3;
	p6309->PC_REG++;
	break;

case BHI_R: //22
	if  (!(p6309->cc[C] | p6309->cc[Z]))
		p6309->PC_REG+=(signed char)cpuMemRead8(pCPU,p6309->PC_REG);
	p6309->PC_REG++;
	p6309->cpu.CycleCounter+=3;
	break;

case BLS_R: //23
	if (p6309->cc[C] | p6309->cc[Z])
		p6309->PC_REG+=(signed char)cpuMemRead8(pCPU,p6309->PC_REG);
	p6309->PC_REG++;
	p6309->cpu.CycleCounter+=3;
	break;

case BHS_R: //24
	if (!p6309->cc[C])
		p6309->PC_REG+=(signed char)cpuMemRead8(pCPU,p6309->PC_REG);
	p6309->PC_REG++;
	p6309->cpu.CycleCounter+=3;
	break;

case BLO_R: //25
	if (p6309->cc[C])
		p6309->PC_REG+=(signed char)cpuMemRead8(pCPU,p6309->PC_REG);
	p6309->PC_REG++;
	p6309->cpu.CycleCounter+=3;
	break;

case BNE_R: //26
	if (!p6309->cc[Z])
		p6309->PC_REG+=(signed char)cpuMemRead8(pCPU,p6309->PC_REG);
	p6309->PC_REG++;
	p6309->cpu.CycleCounter+=3;
	break;

case BEQ_R: //27
	if (p6309->cc[Z])
		p6309->PC_REG+=(signed char)cpuMemRead8(pCPU,p6309->PC_REG);
	p6309->PC_REG++;
	p6309->cpu.CycleCounter+=3;
	break;

case BVC_R: //28
	if (!p6309->cc[V])
		p6309->PC_REG+=(signed char)cpuMemRead8(pCPU,p6309->PC_REG);
	p6309->PC_REG++;
	p6309->cpu.CycleCounter+=3;
	break;

case BVS_R: //29
	if ( p6309->cc[V])
		p6309->PC_REG+=(signed char)cpuMemRead8(pCPU,p6309->PC_REG);
	p6309->PC_REG++;
	p6309->cpu.CycleCounter+=3;
	break;

case BPL_R: //2A
	if (!p6309->cc[N])
		p6309->PC_REG+=(signed char)cpuMemRead8(pCPU,p6309->PC_REG);
	p6309->PC_REG++;
	p6309->cpu.CycleCounter+=3;
	break;

case BMI_R: //2B
	if ( p6309->cc[N])
		p6309->PC_REG+=(signed char)cpuMemRead8(pCPU,p6309->PC_REG);
	p6309->PC_REG++;
	p6309->cpu.CycleCounter+=3;
	break;

case BGE_R: //2C
	if (! (p6309->cc[N] ^ p6309->cc[V]))
		p6309->PC_REG+=(signed char)cpuMemRead8(pCPU,p6309->PC_REG);
	p6309->PC_REG++;
	p6309->cpu.CycleCounter+=3;
	break;

case BLT_R: //2D
	if ( p6309->cc[V] ^ p6309->cc[N])
		p6309->PC_REG+=(signed char)cpuMemRead8(pCPU,p6309->PC_REG);
	p6309->PC_REG++;
	p6309->cpu.CycleCounter+=3;
	break;

case BGT_R: //2E
	if ( !( p6309->cc[Z] | (p6309->cc[N]^p6309->cc[V] ) ))
		p6309->PC_REG+=(signed char)cpuMemRead8(pCPU,p6309->PC_REG);
	p6309->PC_REG++;
	p6309->cpu.CycleCounter+=3;
	break;

case BLE_R: //2F
	if ( p6309->cc[Z] | (p6309->cc[N]^p6309->cc[V]) )
		p6309->PC_REG+=(signed char)cpuMemRead8(pCPU,p6309->PC_REG);
	p6309->PC_REG++;
	p6309->cpu.CycleCounter+=3;
	break;

case LEAX_X: //30
	p6309->X_REG=INDADDRESS(pCPU,p6309->PC_REG++);
	p6309->cc[Z] = ZTEST(p6309->X_REG);
	p6309->cpu.CycleCounter+=4;
	break;

case LEAY_X: //31
	p6309->Y_REG=INDADDRESS(pCPU,p6309->PC_REG++);
	p6309->cc[Z] = ZTEST(p6309->Y_REG);
	p6309->cpu.CycleCounter+=4;
	break;

case LEAS_X: //32
	p6309->S_REG=INDADDRESS(pCPU,p6309->PC_REG++);
	p6309->cpu.CycleCounter+=4;
	break;

case LEAU_X: //33
	p6309->U_REG=INDADDRESS(pCPU,p6309->PC_REG++);
	p6309->cpu.CycleCounter+=4;
	break;

case PSHS_M: //34
	postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
	if (postbyte & 0x80)
	{
		cpuMemWrite8(pCPU, p6309->pc.B.lsb,--p6309->S_REG);
		cpuMemWrite8(pCPU, p6309->pc.B.msb,--p6309->S_REG);
		p6309->cpu.CycleCounter+=2;
	}
	if (postbyte & 0x40)
	{
		cpuMemWrite8(pCPU, p6309->u.B.lsb,--p6309->S_REG);
		cpuMemWrite8(pCPU, p6309->u.B.msb,--p6309->S_REG);
		p6309->cpu.CycleCounter+=2;
	}
	if (postbyte & 0x20)
	{
		cpuMemWrite8(pCPU, p6309->y.B.lsb,--p6309->S_REG);
		cpuMemWrite8(pCPU, p6309->y.B.msb,--p6309->S_REG);
		p6309->cpu.CycleCounter+=2;
	}
	if (postbyte & 0x10)
	{
		cpuMemWrite8(pCPU, p6309->x.B.lsb,--p6309->S_REG);
		cpuMemWrite8(pCPU, p6309->x.B.msb,--p6309->S_REG);
		p6309->cpu.CycleCounter+=2;
	}
	if (postbyte & 0x08)
	{
		cpuMemWrite8(pCPU, p6309->dp.B.msb,--p6309->S_REG);
		p6309->cpu.CycleCounter+=1;
	}
	if (postbyte & 0x04)
	{
		cpuMemWrite8(pCPU,p6309->B_REG,--p6309->S_REG);
		p6309->cpu.CycleCounter+=1;
	}
	if (postbyte & 0x02)
	{
		cpuMemWrite8(pCPU,p6309->A_REG,--p6309->S_REG);
		p6309->cpu.CycleCounter+=1;
	}
	if (postbyte & 0x01)
	{
		cpuMemWrite8(pCPU,getcc(pCPU),--p6309->S_REG);
		p6309->cpu.CycleCounter+=1;
	}

	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
	break;

case PULS_M: //35
	postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
	if (postbyte & 0x01)
	{
		setcc(pCPU,cpuMemRead8(pCPU,p6309->S_REG++));
		p6309->cpu.CycleCounter+=1;
	}
	if (postbyte & 0x02)
	{
		p6309->A_REG=cpuMemRead8(pCPU,p6309->S_REG++);
		p6309->cpu.CycleCounter+=1;
	}
	if (postbyte & 0x04)
	{
		p6309->B_REG=cpuMemRead8(pCPU,p6309->S_REG++);
		p6309->cpu.CycleCounter+=1;
	}
	if (postbyte & 0x08)
	{
		p6309->dp.B.msb=cpuMemRead8(pCPU,p6309->S_REG++);
		p6309->cpu.CycleCounter+=1;
	}
	if (postbyte & 0x10)
	{
		p6309->x.B.msb=cpuMemRead8(pCPU, p6309->S_REG++);
		p6309->x.B.lsb=cpuMemRead8(pCPU, p6309->S_REG++);
		p6309->cpu.CycleCounter+=2;
	}
	if (postbyte & 0x20)
	{
		p6309->y.B.msb=cpuMemRead8(pCPU, p6309->S_REG++);
		p6309->y.B.lsb=cpuMemRead8(pCPU, p6309->S_REG++);
		p6309->cpu.CycleCounter+=2;
	}
	if (postbyte & 0x40)
	{
		p6309->u.B.msb=cpuMemRead8(pCPU, p6309->S_REG++);
		p6309->u.B.lsb=cpuMemRead8(pCPU, p6309->S_REG++);
		p6309->cpu.CycleCounter+=2;
	}
	if (postbyte & 0x80)
	{
		p6309->pc.B.msb=cpuMemRead8(pCPU, p6309->S_REG++);
		p6309->pc.B.lsb=cpuMemRead8(pCPU, p6309->S_REG++);
		p6309->cpu.CycleCounter+=2;
	}
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
	break;

case PSHU_M: //36
	postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
	if (postbyte & 0x80)
	{
		cpuMemWrite8(pCPU, p6309->pc.B.lsb,--p6309->U_REG);
		cpuMemWrite8(pCPU, p6309->pc.B.msb,--p6309->U_REG);
		p6309->cpu.CycleCounter+=2;
	}
	if (postbyte & 0x40)
	{
		cpuMemWrite8(pCPU, p6309->s.B.lsb,--p6309->U_REG);
		cpuMemWrite8(pCPU, p6309->s.B.msb,--p6309->U_REG);
		p6309->cpu.CycleCounter+=2;
	}
	if (postbyte & 0x20)
	{
		cpuMemWrite8(pCPU, p6309->y.B.lsb,--p6309->U_REG);
		cpuMemWrite8(pCPU, p6309->y.B.msb,--p6309->U_REG);
		p6309->cpu.CycleCounter+=2;
	}
	if (postbyte & 0x10)
	{
		cpuMemWrite8(pCPU, p6309->x.B.lsb,--p6309->U_REG);
		cpuMemWrite8(pCPU, p6309->x.B.msb,--p6309->U_REG);
		p6309->cpu.CycleCounter+=2;
	}
	if (postbyte & 0x08)
	{
		cpuMemWrite8(pCPU, p6309->dp.B.msb,--p6309->U_REG);
		p6309->cpu.CycleCounter+=1;
	}
	if (postbyte & 0x04)
	{
		cpuMemWrite8(pCPU,p6309->B_REG,--p6309->U_REG);
		p6309->cpu.CycleCounter+=1;
	}
	if (postbyte & 0x02)
	{
		cpuMemWrite8(pCPU,p6309->A_REG,--p6309->U_REG);
		p6309->cpu.CycleCounter+=1;
	}
	if (postbyte & 0x01)
	{
		cpuMemWrite8(pCPU,getcc(pCPU),--p6309->U_REG);
		p6309->cpu.CycleCounter+=1;
	}
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
	break;

case PULU_M: //37
	postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
	if (postbyte & 0x01)
	{
		setcc(pCPU,cpuMemRead8(pCPU,p6309->U_REG++));
		p6309->cpu.CycleCounter+=1;
	}
	if (postbyte & 0x02)
	{
		p6309->A_REG=cpuMemRead8(pCPU,p6309->U_REG++);
		p6309->cpu.CycleCounter+=1;
	}
	if (postbyte & 0x04)
	{
		p6309->B_REG=cpuMemRead8(pCPU,p6309->U_REG++);
		p6309->cpu.CycleCounter+=1;
	}
	if (postbyte & 0x08)
	{
		p6309->dp.B.msb=cpuMemRead8(pCPU,p6309->U_REG++);
		p6309->cpu.CycleCounter+=1;
	}
	if (postbyte & 0x10)
	{
		p6309->x.B.msb=cpuMemRead8(pCPU, p6309->U_REG++);
		p6309->x.B.lsb=cpuMemRead8(pCPU, p6309->U_REG++);
		p6309->cpu.CycleCounter+=2;
	}
	if (postbyte & 0x20)
	{
		p6309->y.B.msb=cpuMemRead8(pCPU, p6309->U_REG++);
		p6309->y.B.lsb=cpuMemRead8(pCPU, p6309->U_REG++);
		p6309->cpu.CycleCounter+=2;
	}
	if (postbyte & 0x40)
	{
		p6309->s.B.msb=cpuMemRead8(pCPU, p6309->U_REG++);
		p6309->s.B.lsb=cpuMemRead8(pCPU, p6309->U_REG++);
		p6309->cpu.CycleCounter+=2;
	}
	if (postbyte & 0x80)
	{
		p6309->pc.B.msb=cpuMemRead8(pCPU, p6309->U_REG++);
		p6309->pc.B.lsb=cpuMemRead8(pCPU, p6309->U_REG++);
		p6309->cpu.CycleCounter+=2;
	}
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
	break;

case RTS_I: //39
	p6309->pc.B.msb=cpuMemRead8(pCPU,p6309->S_REG++);
	p6309->pc.B.lsb=cpuMemRead8(pCPU,p6309->S_REG++);
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M51];
	break;

case ABX_I: //3A
	p6309->X_REG=p6309->X_REG+p6309->B_REG;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M31];
	break;

case RTI_I: //3B
	setcc(pCPU,cpuMemRead8(pCPU,p6309->S_REG++));
	p6309->cpu.CycleCounter+=6;
	p6309->InInterrupt=0;
	if (p6309->cc[E])
	{
		p6309->A_REG=cpuMemRead8(pCPU,p6309->S_REG++);
		p6309->B_REG=cpuMemRead8(pCPU,p6309->S_REG++);
		if (p6309->md[NATIVE6309])
		{
			(p6309->E_REG)=cpuMemRead8(pCPU,p6309->S_REG++);
			(p6309->F_REG)=cpuMemRead8(pCPU,p6309->S_REG++);
			p6309->cpu.CycleCounter+=2;
		}
		p6309->dp.B.msb=cpuMemRead8(pCPU,p6309->S_REG++);
		p6309->x.B.msb=cpuMemRead8(pCPU,p6309->S_REG++);
		p6309->x.B.lsb=cpuMemRead8(pCPU,p6309->S_REG++);
		p6309->y.B.msb=cpuMemRead8(pCPU,p6309->S_REG++);
		p6309->y.B.lsb=cpuMemRead8(pCPU,p6309->S_REG++);
		p6309->u.B.msb=cpuMemRead8(pCPU,p6309->S_REG++);
		p6309->u.B.lsb=cpuMemRead8(pCPU,p6309->S_REG++);
		p6309->cpu.CycleCounter+=9;
	}
	p6309->pc.B.msb=cpuMemRead8(pCPU,p6309->S_REG++);
	p6309->pc.B.lsb=cpuMemRead8(pCPU,p6309->S_REG++);
	break;

case CWAI_I: //3C
	postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
	p6309->ccbits=getcc(pCPU);
	p6309->ccbits = p6309->ccbits & postbyte;
	setcc(pCPU,p6309->ccbits);
	p6309->cpu.CycleCounter=CycleFor;
	p6309->SyncWaiting=1;
	break;

case MUL_I: //3D
	p6309->D_REG = p6309->A_REG * p6309->B_REG;
	p6309->cc[C] = p6309->B_REG >0x7F;
	p6309->cc[Z] = ZTEST(p6309->D_REG);
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M1110];
	break;

case RESET:	//Undocumented
	cpuHD6309Reset(pCPU);
	break;

case SWI1_I: //3F
	p6309->cc[E]=1;
	cpuMemWrite8(pCPU, p6309->pc.B.lsb,--p6309->S_REG);
	cpuMemWrite8(pCPU, p6309->pc.B.msb,--p6309->S_REG);
	cpuMemWrite8(pCPU, p6309->u.B.lsb,--p6309->S_REG);
	cpuMemWrite8(pCPU, p6309->u.B.msb,--p6309->S_REG);
	cpuMemWrite8(pCPU, p6309->y.B.lsb,--p6309->S_REG);
	cpuMemWrite8(pCPU, p6309->y.B.msb,--p6309->S_REG);
	cpuMemWrite8(pCPU, p6309->x.B.lsb,--p6309->S_REG);
	cpuMemWrite8(pCPU, p6309->x.B.msb,--p6309->S_REG);
	cpuMemWrite8(pCPU, p6309->dp.B.msb,--p6309->S_REG);
	if (p6309->md[NATIVE6309])
	{
		cpuMemWrite8(pCPU,(p6309->F_REG),--p6309->S_REG);
		cpuMemWrite8(pCPU,(p6309->E_REG),--p6309->S_REG);
		p6309->cpu.CycleCounter+=2;
	}
	cpuMemWrite8(pCPU,p6309->B_REG,--p6309->S_REG);
	cpuMemWrite8(pCPU,p6309->A_REG,--p6309->S_REG);
	cpuMemWrite8(pCPU,getcc(pCPU),--p6309->S_REG);
	p6309->PC_REG=cpuMemRead16(&p6309->cpu,VSWI);
	p6309->cpu.CycleCounter+=19;
	p6309->cc[I]=1;
	p6309->cc[F]=1;
	break;

case NEGA_I: //40
	temp8= 0-p6309->A_REG;
	p6309->cc[C] = temp8>0;
	p6309->cc[V] = p6309->A_REG==0x80; //cc[C] ^ ((p6309->A_REG^temp8)>>7);
	p6309->cc[N] = NTEST8(temp8);
	p6309->cc[Z] = ZTEST(temp8);
	p6309->A_REG= temp8;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M21];
	break;

case COMA_I: //43
	p6309->A_REG= 0xFF- p6309->A_REG;
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cc[C] = 1;
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M21];
	break;

case LSRA_I: //44
	p6309->cc[C] = p6309->A_REG & 1;
	p6309->A_REG= p6309->A_REG>>1;
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cc[N] = 0;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M21];
	break;

case RORA_I: //46
	postbyte=p6309->cc[C]<<7;
	p6309->cc[C] = p6309->A_REG & 1;
	p6309->A_REG= (p6309->A_REG>>1) | postbyte;
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M21];
	break;

case ASRA_I: //47
	p6309->cc[C] = p6309->A_REG & 1;
	p6309->A_REG= (p6309->A_REG & 0x80) | (p6309->A_REG >> 1);
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M21];
	break;

case ASLA_I: //48
	p6309->cc[C] = p6309->A_REG > 0x7F;
	p6309->cc[V] =  p6309->cc[C] ^((p6309->A_REG & 0x40)>>6);
	p6309->A_REG= p6309->A_REG<<1;
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M21];
	break;

case ROLA_I: //49
	postbyte=p6309->cc[C];
	p6309->cc[C] = p6309->A_REG > 0x7F;
	p6309->cc[V] = p6309->cc[C] ^ ((p6309->A_REG & 0x40)>>6);
	p6309->A_REG= (p6309->A_REG<<1) | postbyte;
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M21];
	break;

case DECA_I: //4A
	p6309->A_REG--;
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cc[V] = p6309->A_REG==0x7F;
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M21];
	break;

case INCA_I: //4C
	p6309->A_REG++;
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cc[V] = p6309->A_REG==0x80;
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M21];
	break;

case TSTA_I: //4D
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M21];
	break;

case CLRA_I: //4F
	p6309->A_REG= 0;
	p6309->cc[C] =0;
	p6309->cc[V] = 0;
	p6309->cc[N] =0;
	p6309->cc[Z] =1;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M21];
	break;

case NEGB_I: //50
	temp8= 0-p6309->B_REG;
	p6309->cc[C] = temp8>0;
	p6309->cc[V] = p6309->B_REG == 0x80;	
	p6309->cc[N] = NTEST8(temp8);
	p6309->cc[Z] = ZTEST(temp8);
	p6309->B_REG= temp8;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M21];
	break;

case COMB_I: //53
	p6309->B_REG= 0xFF- p6309->B_REG;
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cc[C] = 1;
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M21];
	break;

case LSRB_I: //54
	p6309->cc[C] = p6309->B_REG & 1;
	p6309->B_REG= p6309->B_REG>>1;
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cc[N] = 0;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M21];
	break;

case RORB_I: //56
	postbyte=p6309->cc[C]<<7;
	p6309->cc[C] = p6309->B_REG & 1;
	p6309->B_REG= (p6309->B_REG>>1) | postbyte;
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M21];
	break;

case ASRB_I: //57
	p6309->cc[C] = p6309->B_REG & 1;
	p6309->B_REG= (p6309->B_REG & 0x80) | (p6309->B_REG >> 1);
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M21];
	break;

case ASLB_I: //58
	p6309->cc[C] = p6309->B_REG > 0x7F;
	p6309->cc[V] =  p6309->cc[C] ^((p6309->B_REG & 0x40)>>6);
	p6309->B_REG= p6309->B_REG<<1;
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M21];
	break;

case ROLB_I: //59
	postbyte=p6309->cc[C];
	p6309->cc[C] = p6309->B_REG > 0x7F;
	p6309->cc[V] = p6309->cc[C] ^ ((p6309->B_REG & 0x40)>>6);
	p6309->B_REG= (p6309->B_REG<<1) | postbyte;
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M21];
	break;

case DECB_I: //5A
	p6309->B_REG--;
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cc[V] = p6309->B_REG==0x7F;
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M21];
	break;

case INCB_I: //5C
	p6309->B_REG++;
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cc[V] = p6309->B_REG==0x80; 
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M21];
	break;

case TSTB_I: //5D
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M21];
	break;

case CLRB_I: //5F
	p6309->B_REG=0;
	p6309->cc[C] =0;
	p6309->cc[N] =0;
	p6309->cc[V] = 0;
	p6309->cc[Z] =1;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M21];
	break;

case NEG_X: //60
	temp16=INDADDRESS(pCPU,p6309->PC_REG++);	
	postbyte=cpuMemRead8(pCPU,temp16);
	temp8= 0-postbyte;
	p6309->cc[C] = temp8>0;
	p6309->cc[V] = postbyte == 0x80;
	p6309->cc[N] = NTEST8(temp8);
	p6309->cc[Z] = ZTEST(temp8);
	cpuMemWrite8(pCPU,temp8,temp16);
	p6309->cpu.CycleCounter+=6;
	break;

case OIM_X: //61 6309 DONE
	postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
	temp16=INDADDRESS(pCPU,p6309->PC_REG++);
	postbyte |= cpuMemRead8(pCPU,temp16);
	cpuMemWrite8(pCPU,postbyte,temp16);
	p6309->cc[N] = NTEST8(postbyte);
	p6309->cc[Z] = ZTEST(postbyte);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=6;
	break;

case AIM_X: //62 6309 Phase 2
	postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
	temp16=INDADDRESS(pCPU,p6309->PC_REG++);
	postbyte &= cpuMemRead8(pCPU,temp16);
	cpuMemWrite8(pCPU,postbyte,temp16);
	p6309->cc[N] = NTEST8(postbyte);
	p6309->cc[Z] = ZTEST(postbyte);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=6;
	break;

case COM_X: //63
	temp16=INDADDRESS(pCPU,p6309->PC_REG++);
	temp8=cpuMemRead8(pCPU,temp16);
	temp8= 0xFF-temp8;
	p6309->cc[Z] = ZTEST(temp8);
	p6309->cc[N] = NTEST8(temp8);
	p6309->cc[V] = 0;
	p6309->cc[C] = 1;
	cpuMemWrite8(pCPU,temp8,temp16);
	p6309->cpu.CycleCounter+=6;
	break;

case LSR_X: //64
	temp16=INDADDRESS(pCPU,p6309->PC_REG++);
	temp8=cpuMemRead8(pCPU,temp16);
	p6309->cc[C] = temp8 & 1;
	temp8= temp8 >>1;
	p6309->cc[Z] = ZTEST(temp8);
	p6309->cc[N] = 0;
	cpuMemWrite8(pCPU,temp8,temp16);
	p6309->cpu.CycleCounter+=6;
	break;

case EIM_X: //65 6309 Untested TESTED NITRO
	postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
	temp16=INDADDRESS(pCPU,p6309->PC_REG++);
	postbyte ^= cpuMemRead8(pCPU,temp16);
	cpuMemWrite8(pCPU,postbyte,temp16);
	p6309->cc[N] = NTEST8(postbyte);
	p6309->cc[Z] = ZTEST(postbyte);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=7;
	break;

case ROR_X: //66
	temp16=INDADDRESS(pCPU,p6309->PC_REG++);
	temp8=cpuMemRead8(pCPU,temp16);
	postbyte=p6309->cc[C]<<7;
	p6309->cc[C] = (temp8 & 1);
	temp8= (temp8>>1) | postbyte;
	p6309->cc[Z] = ZTEST(temp8);
	p6309->cc[N] = NTEST8(temp8);
	cpuMemWrite8(pCPU,temp8,temp16);
	p6309->cpu.CycleCounter+=6;
	break;

case ASR_X: //67
	temp16=INDADDRESS(pCPU,p6309->PC_REG++);
	temp8=cpuMemRead8(pCPU,temp16);
	p6309->cc[C] = temp8 & 1;
	temp8= (temp8 & 0x80) | (temp8 >>1);
	p6309->cc[Z] = ZTEST(temp8);
	p6309->cc[N] = NTEST8(temp8);
	cpuMemWrite8(pCPU,temp8,temp16);
	p6309->cpu.CycleCounter+=6;
	break;

case ASL_X: //68 
	temp16=INDADDRESS(pCPU,p6309->PC_REG++);
	temp8= cpuMemRead8(pCPU,temp16);
	p6309->cc[C] = temp8 > 0x7F;
	p6309->cc[V] = p6309->cc[C] ^ ((temp8 & 0x40)>>6);
	temp8= temp8<<1;
	p6309->cc[N] = NTEST8(temp8);
	p6309->cc[Z] = ZTEST(temp8);	
	cpuMemWrite8(pCPU,temp8,temp16);
	p6309->cpu.CycleCounter+=6;
	break;

case ROL_X: //69
	temp16=INDADDRESS(pCPU,p6309->PC_REG++);
	temp8=cpuMemRead8(pCPU,temp16);
	postbyte=p6309->cc[C];
	p6309->cc[C] = temp8 > 0x7F;
	p6309->cc[V] = ( p6309->cc[C] ^ ((temp8 & 0x40)>>6));
	temp8= ((temp8<<1) | postbyte);
	p6309->cc[Z] = ZTEST(temp8);
	p6309->cc[N] = NTEST8(temp8);
	cpuMemWrite8(pCPU,temp8,temp16);
	p6309->cpu.CycleCounter+=6;
	break;

case DEC_X: //6A
	temp16=INDADDRESS(pCPU,p6309->PC_REG++);
	temp8=cpuMemRead8(pCPU,temp16);
	temp8--;
	p6309->cc[Z] = ZTEST(temp8);
	p6309->cc[N] = NTEST8(temp8);
	p6309->cc[V] = (temp8==0x7F);
	cpuMemWrite8(pCPU,temp8,temp16);
	p6309->cpu.CycleCounter+=6;
	break;

case TIM_X: //6B 6309
	postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
	temp8=cpuMemRead8(pCPU,INDADDRESS(pCPU,p6309->PC_REG++));
	postbyte&=temp8;
	p6309->cc[N] = NTEST8(postbyte);
	p6309->cc[Z] = ZTEST(postbyte);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=7;
	break;

case INC_X: //6C
	temp16=INDADDRESS(pCPU,p6309->PC_REG++);
	temp8=cpuMemRead8(pCPU,temp16);
	temp8++;
	p6309->cc[V] = (temp8 == 0x80);
	p6309->cc[N] = NTEST8(temp8);
	p6309->cc[Z] = ZTEST(temp8);
	cpuMemWrite8(pCPU,temp8,temp16);
	p6309->cpu.CycleCounter+=6;
	break;

case TST_X: //6D
	temp8=cpuMemRead8(pCPU,INDADDRESS(pCPU,p6309->PC_REG++));
	p6309->cc[Z] = ZTEST(temp8);
	p6309->cc[N] = NTEST8(temp8);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M65];
	break;

case JMP_X: //6E
	p6309->PC_REG=INDADDRESS(pCPU,p6309->PC_REG++);
	p6309->cpu.CycleCounter+=3;
	break;

case CLR_X: //6F
	cpuMemWrite8(pCPU,0,INDADDRESS(pCPU,p6309->PC_REG++));
	p6309->cc[C] = 0;
	p6309->cc[N] = 0;
	p6309->cc[V] = 0;
	p6309->cc[Z] = 1; 
	p6309->cpu.CycleCounter+=6;
	break;

case NEG_E: //70
	temp16=IMMADDRESS(p6309,p6309->PC_REG);
	postbyte=cpuMemRead8(pCPU,temp16);
	temp8=0-postbyte;
	p6309->cc[C] = temp8>0;
	p6309->cc[V] = postbyte == 0x80;
	p6309->cc[N] = NTEST8(temp8);
	p6309->cc[Z] = ZTEST(temp8);
	cpuMemWrite8(pCPU,temp8,temp16);
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
	break;

case OIM_E: //71 6309 Phase 2
	postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
	temp16=IMMADDRESS(p6309,p6309->PC_REG);
	postbyte|= cpuMemRead8(pCPU,temp16);
	cpuMemWrite8(pCPU,postbyte,temp16);
	p6309->cc[N] = NTEST8(postbyte);
	p6309->cc[Z] = ZTEST(postbyte);
	p6309->cc[V] = 0;
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=7;
	break;

case AIM_E: //72 6309 Untested CHECK NITRO
	postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
	temp16=IMMADDRESS(p6309,p6309->PC_REG);
	postbyte&= cpuMemRead8(pCPU,temp16);
	cpuMemWrite8(pCPU,postbyte,temp16);
	p6309->cc[N] = NTEST8(postbyte);
	p6309->cc[Z] = ZTEST(postbyte);
	p6309->cc[V] = 0;
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=7;
	break;

case COM_E: //73
	temp16=IMMADDRESS(p6309,p6309->PC_REG);
	temp8=cpuMemRead8(pCPU,temp16);
	temp8=0xFF-temp8;
	p6309->cc[Z] = ZTEST(temp8);
	p6309->cc[N] = NTEST8(temp8);
	p6309->cc[C] = 1;
	p6309->cc[V] = 0;
	cpuMemWrite8(pCPU,temp8,temp16);
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
	break;

case LSR_E:  //74
	temp16=IMMADDRESS(p6309,p6309->PC_REG);
	temp8=cpuMemRead8(pCPU,temp16);
	p6309->cc[C] = temp8 & 1;
	temp8= temp8>>1;
	p6309->cc[Z] = ZTEST(temp8);
	p6309->cc[N] = 0;
	cpuMemWrite8(pCPU,temp8,temp16);
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
	break;

case EIM_E: //75 6309 Untested CHECK NITRO
	postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
	temp16=IMMADDRESS(p6309,p6309->PC_REG);
	postbyte^= cpuMemRead8(pCPU,temp16);
	cpuMemWrite8(pCPU,postbyte,temp16);
	p6309->cc[N] = NTEST8(postbyte);
	p6309->cc[Z] = ZTEST(postbyte);
	p6309->cc[V] = 0;
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=7;
	break;

case ROR_E: //76
	temp16=IMMADDRESS(p6309,p6309->PC_REG);
	temp8=cpuMemRead8(pCPU,temp16);
	postbyte=p6309->cc[C]<<7;
	p6309->cc[C] = temp8 & 1;
	temp8= (temp8>>1) | postbyte;
	p6309->cc[Z] = ZTEST(temp8);
	p6309->cc[N] = NTEST8(temp8);
	cpuMemWrite8(pCPU,temp8,temp16);
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
	break;

case ASR_E: //77
	temp16=IMMADDRESS(p6309,p6309->PC_REG);
	temp8=cpuMemRead8(pCPU,temp16);
	p6309->cc[C] = temp8 & 1;
	temp8= (temp8 & 0x80) | (temp8 >>1);
	p6309->cc[Z] = ZTEST(temp8);
	p6309->cc[N] = NTEST8(temp8);
	cpuMemWrite8(pCPU,temp8,temp16);
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
	break;

case ASL_E: //78 6309
	temp16=IMMADDRESS(p6309,p6309->PC_REG);
	temp8= cpuMemRead8(pCPU,temp16);
	p6309->cc[C] = temp8 > 0x7F;
	p6309->cc[V] = p6309->cc[C] ^ ((temp8 & 0x40)>>6);
	temp8= temp8<<1;
	p6309->cc[N] = NTEST8(temp8);
	p6309->cc[Z] = ZTEST(temp8);
	cpuMemWrite8(pCPU,temp8,temp16);	
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
	break;

case ROL_E: //79
	temp16=IMMADDRESS(p6309,p6309->PC_REG);
	temp8=cpuMemRead8(pCPU,temp16);
	postbyte=p6309->cc[C];
	p6309->cc[C] = temp8 > 0x7F;
	p6309->cc[V] = p6309->cc[C] ^  ((temp8 & 0x40)>>6);
	temp8 = ((temp8<<1) | postbyte);
	p6309->cc[Z] = ZTEST(temp8);
	p6309->cc[N] = NTEST8(temp8);
	cpuMemWrite8(pCPU,temp8,temp16);
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
	break;

case DEC_E: //7A
	temp16=IMMADDRESS(p6309,p6309->PC_REG);
	temp8=cpuMemRead8(pCPU,temp16);
	temp8--;
	p6309->cc[Z] = ZTEST(temp8);
	p6309->cc[N] = NTEST8(temp8);
	p6309->cc[V] = temp8==0x7F;
	cpuMemWrite8(pCPU,temp8,temp16);
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
	break;

case TIM_E: //7B 6309 NITRO 
	postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
	temp16=IMMADDRESS(p6309,p6309->PC_REG);
	postbyte&=cpuMemRead8(pCPU,temp16);
	p6309->cc[N] = NTEST8(postbyte);
	p6309->cc[Z] = ZTEST(postbyte);
	p6309->cc[V] = 0;
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=7;
	break;

case INC_E: //7C
	temp16=IMMADDRESS(p6309,p6309->PC_REG);
	temp8=cpuMemRead8(pCPU,temp16);
	temp8++;
	p6309->cc[Z] = ZTEST(temp8);
	p6309->cc[V] = temp8==0x80;
	p6309->cc[N] = NTEST8(temp8);
	cpuMemWrite8(pCPU,temp8,temp16);
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
	break;

case TST_E: //7D
	temp8=cpuMemRead8(pCPU,IMMADDRESS(p6309,p6309->PC_REG));
	p6309->cc[Z] = ZTEST(temp8);
	p6309->cc[N] = NTEST8(temp8);
	p6309->cc[V] = 0;
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M75];
	break;

case JMP_E: //7E
	p6309->PC_REG=IMMADDRESS(p6309,p6309->PC_REG);
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M43];
	break;

case CLR_E: //7F
	cpuMemWrite8(pCPU,0,IMMADDRESS(p6309,p6309->PC_REG));
	p6309->cc[C] = 0;
	p6309->cc[N] = 0;
	p6309->cc[V] = 0;
	p6309->cc[Z] = 1;
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
	break;

case SUBA_M: //80
	postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
	temp16 = p6309->A_REG - postbyte;
	p6309->cc[C] = (temp16 & 0x100)>>8; 
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->A_REG);
	p6309->A_REG = (unsigned char)temp16;
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cpu.CycleCounter+=2;
	break;

case CMPA_M: //81
	postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
	temp8= p6309->A_REG-postbyte;
	p6309->cc[C] = temp8 > p6309->A_REG;
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp8,p6309->A_REG);
	p6309->cc[N] = NTEST8(temp8);
	p6309->cc[Z] = ZTEST(temp8);
	p6309->cpu.CycleCounter+=2;
	break;

case SBCA_M:  //82
	postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
	temp16=p6309->A_REG-postbyte-p6309->cc[C];
	p6309->cc[C] = (temp16 & 0x100)>>8;
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->A_REG);
	p6309->A_REG = (unsigned char)temp16;
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cpu.CycleCounter+=2;
	break;

case SUBD_M: //83
	temp16=IMMADDRESS(p6309,p6309->PC_REG);
	temp32=p6309->D_REG-temp16;
	p6309->cc[C] = (temp32 & 0x10000)>>16;
	p6309->cc[V] = OVERFLOW16(p6309->cc[C],temp32,temp16,p6309->D_REG);
	p6309->D_REG = temp32;
	p6309->cc[Z] = ZTEST(p6309->D_REG);
	p6309->cc[N] = NTEST16(p6309->D_REG);
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M43];
	break;

case ANDA_M: //84
	p6309->A_REG = p6309->A_REG & cpuMemRead8(pCPU,p6309->PC_REG++);
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=2;
	break;

case BITA_M: //85
	temp8 = p6309->A_REG & cpuMemRead8(pCPU,p6309->PC_REG++);
	p6309->cc[N] = NTEST8(temp8);
	p6309->cc[Z] = ZTEST(temp8);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=2;
	break;

case LDA_M: //86
	p6309->A_REG = cpuMemRead8(pCPU,p6309->PC_REG++);
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=2;
	break;

case EORA_M: //88
	p6309->A_REG = p6309->A_REG ^ cpuMemRead8(pCPU,p6309->PC_REG++);
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=2;
	break;

case ADCA_M: //89
	postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
	temp16= p6309->A_REG + postbyte + p6309->cc[C];
	p6309->cc[C] = (temp16 & 0x100)>>8;
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->A_REG);
	p6309->cc[H] = ((p6309->A_REG ^ temp16 ^ postbyte) & 0x10)>>4;
	p6309->A_REG= (unsigned char)temp16;
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cpu.CycleCounter+=2;
	break;

case ORA_M: //8A
	p6309->A_REG = p6309->A_REG | cpuMemRead8(pCPU,p6309->PC_REG++);
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=2;
	break;

case ADDA_M: //8B
	postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
	temp16=p6309->A_REG+postbyte;
	p6309->cc[C] =(temp16 & 0x100)>>8;
	p6309->cc[H] = ((p6309->A_REG ^ postbyte ^ temp16) & 0x10)>>4;
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->A_REG);
	p6309->A_REG= (unsigned char)temp16;
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cpu.CycleCounter+=2;
	break;

case CMPX_M: //8C
	postword=IMMADDRESS(p6309,p6309->PC_REG);
	temp16 = p6309->X_REG-postword;
	p6309->cc[C] = temp16 > p6309->X_REG;
	p6309->cc[V] = OVERFLOW16(p6309->cc[C],postword,temp16,p6309->X_REG);
	p6309->cc[N] = NTEST16(temp16);
	p6309->cc[Z] = ZTEST(temp16);
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M43];
	break;

case BSR_R: //8D
	*spostbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
	p6309->S_REG--;
	cpuMemWrite8(pCPU,p6309->pc.B.lsb,p6309->S_REG--);
	cpuMemWrite8(pCPU,p6309->pc.B.msb,p6309->S_REG);
	p6309->PC_REG+=*spostbyte;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
	break;

case LDX_M: //8E
	p6309->X_REG = IMMADDRESS(p6309,p6309->PC_REG);
	p6309->cc[Z] = ZTEST(p6309->X_REG);
	p6309->cc[N] = NTEST16(p6309->X_REG);
	p6309->cc[V] = 0;
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=3;
	break;

case SUBA_D: //90
	postbyte=cpuMemRead8(pCPU, DPADDRESS(p6309,p6309->PC_REG++));
	temp16 = p6309->A_REG - postbyte;
	p6309->cc[C] = (temp16 & 0x100)>>8; 
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->A_REG);
	p6309->A_REG = (unsigned char)temp16;
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M43];
	break;	

case CMPA_D: //91
	postbyte=cpuMemRead8(pCPU,DPADDRESS(p6309,p6309->PC_REG++));
	temp8 = p6309->A_REG-postbyte;
	p6309->cc[C] = temp8 > p6309->A_REG;
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp8,p6309->A_REG);
	p6309->cc[N] = NTEST8(temp8);
	p6309->cc[Z] = ZTEST(temp8);
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M43];
	break;

case SBCA_D: //92
	postbyte=cpuMemRead8(pCPU,DPADDRESS(p6309,p6309->PC_REG++));
	temp16=p6309->A_REG-postbyte-p6309->cc[C];
	p6309->cc[C] = (temp16 & 0x100)>>8;
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->A_REG);
	p6309->A_REG = (unsigned char)temp16;
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M43];
	break;

case SUBD_D: //93
	temp16= cpuMemRead16(&p6309->cpu,DPADDRESS(p6309,p6309->PC_REG++));
	temp32= p6309->D_REG-temp16;
	p6309->cc[C] = (temp32 & 0x10000)>>16;
	p6309->cc[V] = OVERFLOW16(p6309->cc[C],temp32,temp16,p6309->D_REG);
	p6309->D_REG = temp32;
	p6309->cc[Z] = ZTEST(p6309->D_REG);
	p6309->cc[N] = NTEST16(p6309->D_REG);
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M64];
	break;

case ANDA_D: //94
	p6309->A_REG = p6309->A_REG & cpuMemRead8(pCPU,DPADDRESS(p6309,p6309->PC_REG++));
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M43];
	break;

case BITA_D: //95
	temp8 = p6309->A_REG & cpuMemRead8(pCPU,DPADDRESS(p6309,p6309->PC_REG++));
	p6309->cc[N] = NTEST8(temp8);
	p6309->cc[Z] = ZTEST(temp8);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M43];
	break;

case LDA_D: //96
	p6309->A_REG = cpuMemRead8(pCPU,DPADDRESS(p6309,p6309->PC_REG++));
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M43];
	break;

case STA_D: //97
	cpuMemWrite8(pCPU, p6309->A_REG,DPADDRESS(p6309,p6309->PC_REG++));
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M43];
	break;

case EORA_D: //98
	p6309->A_REG= p6309->A_REG ^ cpuMemRead8(pCPU,DPADDRESS(p6309,p6309->PC_REG++));
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M43];
	break;

case ADCA_D: //99
	postbyte=cpuMemRead8(pCPU,DPADDRESS(p6309,p6309->PC_REG++));
	temp16= p6309->A_REG + postbyte + p6309->cc[C];
	p6309->cc[C] = (temp16 & 0x100)>>8;
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->A_REG);
	p6309->cc[H] = ((p6309->A_REG ^ temp16 ^ postbyte) & 0x10)>>4;
	p6309->A_REG= (unsigned char)temp16;
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M43];
	break;

case ORA_D: //9A
	p6309->A_REG = p6309->A_REG | cpuMemRead8(pCPU,DPADDRESS(p6309,p6309->PC_REG++));
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M43];
	break;

case ADDA_D: //9B
	postbyte=cpuMemRead8(pCPU,DPADDRESS(p6309,p6309->PC_REG++));
	temp16=p6309->A_REG+postbyte;
	p6309->cc[C] =(temp16 & 0x100)>>8;
	p6309->cc[H] = ((p6309->A_REG ^ postbyte ^ temp16) & 0x10)>>4;
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->A_REG);
	p6309->A_REG= (unsigned char)temp16;
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M43];
	break;

case CMPX_D: //9C
	postword=cpuMemRead16(&p6309->cpu,DPADDRESS(p6309,p6309->PC_REG++));
	temp16= p6309->X_REG - postword ;
	p6309->cc[C] = temp16 > p6309->X_REG;
	p6309->cc[V] = OVERFLOW16(p6309->cc[C],postword,temp16,p6309->X_REG);
	p6309->cc[N] = NTEST16(temp16);
	p6309->cc[Z] = ZTEST(temp16);
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M64];
	break;

case BSR_D: //9D
	temp16 = DPADDRESS(p6309,p6309->PC_REG++);
	p6309->S_REG--;
	cpuMemWrite8(pCPU,p6309->pc.B.lsb,p6309->S_REG--);
	cpuMemWrite8(pCPU,p6309->pc.B.msb,p6309->S_REG);
	p6309->PC_REG=temp16;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
	break;

case LDX_D: //9E
	p6309->X_REG=cpuMemRead16(&p6309->cpu,DPADDRESS(p6309,p6309->PC_REG++));
	p6309->cc[Z] = ZTEST(p6309->X_REG);	
	p6309->cc[N] = NTEST16(p6309->X_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
	break;

case STX_D: //9F
	cpuMemWrite16(&p6309->cpu,p6309->X_REG,DPADDRESS(p6309,p6309->PC_REG++));
	p6309->cc[Z] = ZTEST(p6309->X_REG);
	p6309->cc[N] = NTEST16(p6309->X_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
	break;

case SUBA_X: //A0
	postbyte=cpuMemRead8(pCPU,INDADDRESS(pCPU,p6309->PC_REG++));
	temp16 = p6309->A_REG - postbyte;
	p6309->cc[C] =(temp16 & 0x100)>>8; 
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->A_REG);
	p6309->A_REG= (unsigned char)temp16;
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cpu.CycleCounter+=4;
	break;	

case CMPA_X: //A1
	postbyte=cpuMemRead8(pCPU,INDADDRESS(pCPU,p6309->PC_REG++));
	temp8= p6309->A_REG-postbyte;
	p6309->cc[C] = temp8 > p6309->A_REG;
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp8,p6309->A_REG);
	p6309->cc[N] = NTEST8(temp8);
	p6309->cc[Z] = ZTEST(temp8);
	p6309->cpu.CycleCounter+=4;
	break;

case SBCA_X: //A2
	postbyte=cpuMemRead8(pCPU,INDADDRESS(pCPU,p6309->PC_REG++));
	temp16=p6309->A_REG-postbyte-p6309->cc[C];
	p6309->cc[C] = (temp16 & 0x100)>>8;
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->A_REG);
	p6309->A_REG = (unsigned char)temp16;
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cpu.CycleCounter+=4;
	break;

case SUBD_X: //A3
	temp16= cpuMemRead16(&p6309->cpu,INDADDRESS(pCPU,p6309->PC_REG++));
	temp32= p6309->D_REG-temp16;
	p6309->cc[C] = (temp32 & 0x10000)>>16;
	p6309->cc[V] = OVERFLOW16(p6309->cc[C],temp32,temp16,p6309->D_REG);
	p6309->D_REG = temp32;
	p6309->cc[Z] = ZTEST(p6309->D_REG);
	p6309->cc[N] = NTEST16(p6309->D_REG);
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M65];
	break;

case ANDA_X: //A4
	p6309->A_REG = p6309->A_REG & cpuMemRead8(pCPU,INDADDRESS(pCPU,p6309->PC_REG++));
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=4;
	break;

case BITA_X:  //A5
	temp8 = p6309->A_REG & cpuMemRead8(pCPU,INDADDRESS(pCPU,p6309->PC_REG++));
	p6309->cc[N] = NTEST8(temp8);
	p6309->cc[Z] = ZTEST(temp8);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=4;
	break;

case LDA_X: //A6
	p6309->A_REG = cpuMemRead8(pCPU,INDADDRESS(pCPU,p6309->PC_REG++));
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=4;
	break;

case STA_X: //A7
	cpuMemWrite8(pCPU,p6309->A_REG,INDADDRESS(pCPU,p6309->PC_REG++));
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=4;
	break;

case EORA_X: //A8
	p6309->A_REG= p6309->A_REG ^ cpuMemRead8(pCPU,INDADDRESS(pCPU,p6309->PC_REG++));
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=4;
	break;

case ADCA_X: //A9	
	postbyte=cpuMemRead8(pCPU,INDADDRESS(pCPU,p6309->PC_REG++));
	temp16= p6309->A_REG + postbyte + p6309->cc[C];
	p6309->cc[C] = (temp16 & 0x100)>>8;
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->A_REG);
	p6309->cc[H] = ((p6309->A_REG ^ temp16 ^ postbyte) & 0x10)>>4;
	p6309->A_REG= (unsigned char)temp16;
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cpu.CycleCounter+=4;
	break;

case ORA_X: //AA
	p6309->A_REG= p6309->A_REG | cpuMemRead8(pCPU,INDADDRESS(pCPU,p6309->PC_REG++));
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=4;
	break;

case ADDA_X: //AB
	postbyte=cpuMemRead8(pCPU,INDADDRESS(pCPU,p6309->PC_REG++));
	temp16= p6309->A_REG+postbyte;
	p6309->cc[C] = (temp16 & 0x100)>>8;
	p6309->cc[H] = ((p6309->A_REG ^ postbyte ^ temp16) & 0x10)>>4;
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->A_REG);
	p6309->A_REG = (unsigned char)temp16;
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cpu.CycleCounter+=4;
	break;

case CMPX_X: //AC
	postword=cpuMemRead16(&p6309->cpu,INDADDRESS(pCPU,p6309->PC_REG++));
	temp16= p6309->X_REG - postword ;
	p6309->cc[C] = temp16 > p6309->X_REG;
	p6309->cc[V] = OVERFLOW16(p6309->cc[C],postword,temp16,p6309->X_REG);
	p6309->cc[N] = NTEST16(temp16);
	p6309->cc[Z] = ZTEST(temp16);
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M65];
	break;

case BSR_X: //AD
	temp16=INDADDRESS(pCPU,p6309->PC_REG++);
	p6309->S_REG--;
	cpuMemWrite8(pCPU,p6309->pc.B.lsb,p6309->S_REG--);
	cpuMemWrite8(pCPU,p6309->pc.B.msb,p6309->S_REG);
	p6309->PC_REG=temp16;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
	break;

case LDX_X: //AE
	p6309->X_REG=cpuMemRead16(&p6309->cpu,INDADDRESS(pCPU,p6309->PC_REG++));
	p6309->cc[Z] = ZTEST(p6309->X_REG);
	p6309->cc[N] = NTEST16(p6309->X_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=5;
	break;

case STX_X: //AF
	cpuMemWrite16(&p6309->cpu,p6309->X_REG,INDADDRESS(pCPU,p6309->PC_REG++));
	p6309->cc[Z] = ZTEST(p6309->X_REG);
	p6309->cc[N] = NTEST16(p6309->X_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=5;
	break;

case SUBA_E: //B0
	postbyte=cpuMemRead8(pCPU,IMMADDRESS(p6309,p6309->PC_REG));
	temp16 = p6309->A_REG - postbyte;
	p6309->cc[C] = (temp16 & 0x100)>>8; 
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->A_REG);
	p6309->A_REG= (unsigned char)temp16;
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
	break;

case CMPA_E: //B1
	postbyte=cpuMemRead8(pCPU,IMMADDRESS(p6309,p6309->PC_REG));
	temp8= p6309->A_REG-postbyte;
	p6309->cc[C] = temp8 > p6309->A_REG;
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp8,p6309->A_REG);
	p6309->cc[N] = NTEST8(temp8);
	p6309->cc[Z] = ZTEST(temp8);
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
	break;

case SBCA_E: //B2
	postbyte=cpuMemRead8(pCPU,IMMADDRESS(p6309,p6309->PC_REG));
	temp16=p6309->A_REG-postbyte-p6309->cc[C];
	p6309->cc[C] = (temp16 & 0x100)>>8;
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->A_REG);
	p6309->A_REG = (unsigned char)temp16;
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
	break;

case SUBD_E: //B3
	temp16=cpuMemRead16(&p6309->cpu,IMMADDRESS(p6309,p6309->PC_REG));
	temp32=p6309->D_REG-temp16;
	p6309->cc[C] = (temp32 & 0x10000)>>16;
	p6309->cc[V] = OVERFLOW16(p6309->cc[C],temp32,temp16,p6309->D_REG);
	p6309->D_REG= temp32;
	p6309->cc[Z] = ZTEST(p6309->D_REG);
	p6309->cc[N] = NTEST16(p6309->D_REG);
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M75];
	break;

case ANDA_E: //B4
	postbyte=cpuMemRead8(pCPU,IMMADDRESS(p6309,p6309->PC_REG));
	p6309->A_REG = p6309->A_REG & postbyte;
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cc[V] = 0;
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
	break;

case BITA_E: //B5
	temp8 = p6309->A_REG & cpuMemRead8(pCPU,IMMADDRESS(p6309,p6309->PC_REG));
	p6309->cc[N] = NTEST8(temp8);
	p6309->cc[Z] = ZTEST(temp8);
	p6309->cc[V] = 0;
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
	break;

case LDA_E: //B6
	p6309->A_REG= cpuMemRead8(pCPU,IMMADDRESS(p6309,p6309->PC_REG));
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cc[V] = 0;
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
	break;

case STA_E: //B7
	cpuMemWrite8(pCPU,p6309->A_REG,IMMADDRESS(p6309,p6309->PC_REG));
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cc[V] = 0;
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
	break;

case EORA_E:  //B8
	p6309->A_REG = p6309->A_REG ^ cpuMemRead8(pCPU,IMMADDRESS(p6309,p6309->PC_REG));
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cc[V] = 0;
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
	break;

case ADCA_E: //B9
	postbyte=cpuMemRead8(pCPU,IMMADDRESS(p6309,p6309->PC_REG));
	temp16= p6309->A_REG + postbyte + p6309->cc[C];
	p6309->cc[C] = (temp16 & 0x100)>>8;
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->A_REG);
	p6309->cc[H] = ((p6309->A_REG ^ temp16 ^ postbyte) & 0x10)>>4;
	p6309->A_REG= (unsigned char)temp16;
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
	break;

case ORA_E: //BA
	p6309->A_REG = p6309->A_REG | cpuMemRead8(pCPU,IMMADDRESS(p6309,p6309->PC_REG));
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->cc[V] = 0;
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
	break;

case ADDA_E: //BB
	postbyte=cpuMemRead8(pCPU,IMMADDRESS(p6309,p6309->PC_REG));
	temp16=p6309->A_REG+postbyte;
	p6309->cc[C] = (temp16 & 0x100)>>8;
	p6309->cc[H] = ((p6309->A_REG ^ postbyte ^ temp16) & 0x10)>>4;
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->A_REG);
	p6309->A_REG = (unsigned char)temp16;
	p6309->cc[N] = NTEST8(p6309->A_REG);
	p6309->cc[Z] = ZTEST(p6309->A_REG);
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
	break;

case CMPX_E: //BC
	postword=cpuMemRead16(&p6309->cpu,IMMADDRESS(p6309,p6309->PC_REG));
	temp16 = p6309->X_REG-postword;
	p6309->cc[C] = temp16 > p6309->X_REG;
	p6309->cc[V] = OVERFLOW16(p6309->cc[C],postword,temp16,p6309->X_REG);
	p6309->cc[N] = NTEST16(temp16);
	p6309->cc[Z] = ZTEST(temp16);
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M75];
	break;

case BSR_E: //BD
	postword=IMMADDRESS(p6309,p6309->PC_REG);
	p6309->PC_REG+=2;
	p6309->S_REG--;
	cpuMemWrite8(pCPU,p6309->pc.B.lsb,p6309->S_REG--);
	cpuMemWrite8(pCPU,p6309->pc.B.msb,p6309->S_REG);
	p6309->PC_REG=postword;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M87];
	break;

case LDX_E: //BE
	p6309->X_REG=cpuMemRead16(&p6309->cpu,IMMADDRESS(p6309,p6309->PC_REG));
	p6309->cc[Z] = ZTEST(p6309->X_REG);
	p6309->cc[N] = NTEST16(p6309->X_REG);
	p6309->cc[V] = 0;
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M65];
	break;

case STX_E: //BF
	cpuMemWrite16(&p6309->cpu,p6309->X_REG,IMMADDRESS(p6309,p6309->PC_REG));
	p6309->cc[Z] = ZTEST(p6309->X_REG);
	p6309->cc[N] = NTEST16(p6309->X_REG);
	p6309->cc[V] = 0;
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M65];
	break;

case SUBB_M: //C0
	postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
	temp16 = p6309->B_REG - postbyte;
	p6309->cc[C] = (temp16 & 0x100)>>8; 
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->B_REG);
	p6309->B_REG= (unsigned char)temp16;
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cpu.CycleCounter+=2;
	break;

case CMPB_M: //C1
	postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
	temp8= p6309->B_REG-postbyte;
	p6309->cc[C] = temp8 > p6309->B_REG;
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp8,p6309->B_REG);
	p6309->cc[N] = NTEST8(temp8);
	p6309->cc[Z] = ZTEST(temp8);
	p6309->cpu.CycleCounter+=2;
	break;

case SBCB_M: //C3
	postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
	temp16=p6309->B_REG-postbyte-p6309->cc[C];
	p6309->cc[C] = (temp16 & 0x100)>>8;
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->B_REG);
	p6309->B_REG = (unsigned char)temp16;
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cpu.CycleCounter+=2;
	break;

case ADDD_M: //C3
	temp16=IMMADDRESS(p6309,p6309->PC_REG);
	temp32= p6309->D_REG+ temp16;
	p6309->cc[C] = (temp32 & 0x10000)>>16;
	p6309->cc[V] = OVERFLOW16(p6309->cc[C],temp32,temp16,p6309->D_REG);
	p6309->D_REG= temp32;
	p6309->cc[Z] = ZTEST(p6309->D_REG);
	p6309->cc[N] = NTEST16(p6309->D_REG);
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M43];
	break;

case ANDB_M: //C4 LOOK
	p6309->B_REG = p6309->B_REG & cpuMemRead8(pCPU,p6309->PC_REG++);
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=2;
	break;

case BITB_M: //C5
	temp8 = p6309->B_REG & cpuMemRead8(pCPU,p6309->PC_REG++);
	p6309->cc[N] = NTEST8(temp8);
	p6309->cc[Z] = ZTEST(temp8);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=2;
	break;

case LDB_M: //C6
	p6309->B_REG=cpuMemRead8(pCPU,p6309->PC_REG++);
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=2;
	break;

case EORB_M: //C8
	p6309->B_REG = p6309->B_REG ^ cpuMemRead8(pCPU,p6309->PC_REG++);
	p6309->cc[N] =NTEST8(p6309->B_REG);
	p6309->cc[Z] =ZTEST(p6309->B_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=2;
	break;

case ADCB_M: //C9
	postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
	temp16= p6309->B_REG + postbyte + p6309->cc[C];
	p6309->cc[C] = (temp16 & 0x100)>>8;
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->B_REG);
	p6309->cc[H] = ((p6309->B_REG ^ temp16 ^ postbyte) & 0x10)>>4;
	p6309->B_REG= (unsigned char)temp16;
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cpu.CycleCounter+=2;
	break;

case ORB_M: //CA
	p6309->B_REG= p6309->B_REG | cpuMemRead8(pCPU,p6309->PC_REG++);
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=2;
	break;

case ADDB_M: //CB
	postbyte=cpuMemRead8(pCPU,p6309->PC_REG++);
	temp16=p6309->B_REG+postbyte;
	p6309->cc[C] =(temp16 & 0x100)>>8;
	p6309->cc[H] = ((p6309->B_REG ^ postbyte ^ temp16) & 0x10)>>4;
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->B_REG);
	p6309->B_REG= (unsigned char)temp16;
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cpu.CycleCounter+=2;
	break;

case LDD_M: //CC
	p6309->D_REG=IMMADDRESS(p6309,p6309->PC_REG);
	p6309->cc[Z] = ZTEST(p6309->D_REG);
	p6309->cc[N] = NTEST16(p6309->D_REG);
	p6309->cc[V] = 0;
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=3;
	break;

case LDQ_M: //CD 6309 WORK
	p6309->Q_REG = cpuMemRead32(pCPU,p6309->PC_REG);
	p6309->PC_REG+=4;
	p6309->cc[Z] = ZTEST(p6309->Q_REG);
	p6309->cc[N] = NTEST32(p6309->Q_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=5;
	break;

case LDU_M: //CE
	p6309->U_REG = IMMADDRESS(p6309,p6309->PC_REG);
	p6309->cc[Z] = ZTEST(p6309->U_REG);
	p6309->cc[N] = NTEST16(p6309->U_REG);
	p6309->cc[V] = 0;
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=3;
	break;

case SUBB_D: //D0
	postbyte=cpuMemRead8(pCPU, DPADDRESS(p6309,p6309->PC_REG++));
	temp16 = p6309->B_REG - postbyte;
	p6309->cc[C] =(temp16 & 0x100)>>8; 
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->B_REG);
	p6309->B_REG = (unsigned char)temp16;
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M43];
	break;

case CMPB_D: //D1
	postbyte=cpuMemRead8(pCPU,DPADDRESS(p6309,p6309->PC_REG++));
	temp8= p6309->B_REG-postbyte;
	p6309->cc[C] = temp8 > p6309->B_REG;
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp8,p6309->B_REG);
	p6309->cc[N] = NTEST8(temp8);
	p6309->cc[Z] = ZTEST(temp8);
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M43];
	break;

case SBCB_D: //D2
	postbyte=cpuMemRead8(pCPU,DPADDRESS(p6309,p6309->PC_REG++));
	temp16=p6309->B_REG-postbyte-p6309->cc[C];
	p6309->cc[C] = (temp16 & 0x100)>>8;
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->B_REG);
	p6309->B_REG = (unsigned char)temp16;
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M43];
	break;

case ADDD_D: //D3
	temp16=cpuMemRead16(&p6309->cpu, DPADDRESS(p6309,p6309->PC_REG++));
	temp32= p6309->D_REG+ temp16;
	p6309->cc[C] =(temp32 & 0x10000)>>16;
	p6309->cc[V] = OVERFLOW16(p6309->cc[C],temp32,temp16,p6309->D_REG);
	p6309->D_REG= temp32;
	p6309->cc[Z] = ZTEST(p6309->D_REG);
	p6309->cc[N] = NTEST16(p6309->D_REG);
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M64];
	break;

case ANDB_D: //D4 
	p6309->B_REG = p6309->B_REG & cpuMemRead8(pCPU,DPADDRESS(p6309,p6309->PC_REG++));
	p6309->cc[N] =NTEST8(p6309->B_REG);
	p6309->cc[Z] =ZTEST(p6309->B_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M43];
	break;

case BITB_D: //D5
	temp8 = p6309->B_REG & cpuMemRead8(pCPU,DPADDRESS(p6309,p6309->PC_REG++));
	p6309->cc[N] = NTEST8(temp8);
	p6309->cc[Z] = ZTEST(temp8);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M43];
	break;

case LDB_D: //D6
	p6309->B_REG= cpuMemRead8(pCPU, DPADDRESS(p6309,p6309->PC_REG++));
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M43];
	break;

case	STB_D: //D7
	cpuMemWrite8(pCPU, p6309->B_REG,DPADDRESS(p6309,p6309->PC_REG++));
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M43];
	break;

case EORB_D: //D8	
	p6309->B_REG = p6309->B_REG ^ cpuMemRead8(pCPU,DPADDRESS(p6309,p6309->PC_REG++));
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M43];
	break;

case ADCB_D: //D9
	postbyte=cpuMemRead8(pCPU,DPADDRESS(p6309,p6309->PC_REG++));
	temp16= p6309->B_REG + postbyte + p6309->cc[C];
	p6309->cc[C] = (temp16 & 0x100)>>8;
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->B_REG);
	p6309->cc[H] = ((p6309->B_REG ^ temp16 ^ postbyte) & 0x10)>>4;
	p6309->B_REG = (unsigned char)temp16;
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M43];
	break;

case ORB_D: //DA
	p6309->B_REG = p6309->B_REG | cpuMemRead8(pCPU,DPADDRESS(p6309,p6309->PC_REG++));
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M43];
	break;

case ADDB_D: //DB
	postbyte=cpuMemRead8(pCPU,DPADDRESS(p6309,p6309->PC_REG++));
	temp16= p6309->B_REG+postbyte;
	p6309->cc[C] = (temp16 & 0x100)>>8;
	p6309->cc[H] = ((p6309->B_REG ^ postbyte ^ temp16) & 0x10)>>4;
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->B_REG);
	p6309->B_REG = (unsigned char)temp16;
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M43];
	break;

case LDD_D: //DC
	p6309->D_REG = cpuMemRead16(&p6309->cpu,DPADDRESS(p6309,p6309->PC_REG++));
	p6309->cc[Z] = ZTEST(p6309->D_REG);
	p6309->cc[N] = NTEST16(p6309->D_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
	break;

case STD_D: //DD
	cpuMemWrite16(&p6309->cpu,p6309->D_REG,DPADDRESS(p6309,p6309->PC_REG++));
	p6309->cc[Z] = ZTEST(p6309->D_REG);
	p6309->cc[N] = NTEST16(p6309->D_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
	break;

case LDU_D: //DE
	p6309->U_REG = cpuMemRead16(&p6309->cpu,DPADDRESS(p6309,p6309->PC_REG++));
	p6309->cc[Z] = ZTEST(p6309->U_REG);
	p6309->cc[N] = NTEST16(p6309->U_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
	break;

case STU_D: //DF
	cpuMemWrite16(&p6309->cpu,p6309->U_REG,DPADDRESS(p6309,p6309->PC_REG++));
	p6309->cc[Z] = ZTEST(p6309->U_REG);
	p6309->cc[N] = NTEST16(p6309->U_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
	break;

case SUBB_X: //E0
	postbyte=cpuMemRead8(pCPU,INDADDRESS(pCPU,p6309->PC_REG++));
	temp16 = p6309->B_REG - postbyte;
	p6309->cc[C] = (temp16 & 0x100)>>8; 
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->B_REG);
	p6309->B_REG = (unsigned char)temp16;
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cpu.CycleCounter+=4;
	break;

case CMPB_X: //E1
	postbyte=cpuMemRead8(pCPU,INDADDRESS(pCPU,p6309->PC_REG++));
	temp8 = p6309->B_REG-postbyte;
	p6309->cc[C] = temp8 > p6309->B_REG;
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp8,p6309->B_REG);
	p6309->cc[N] = NTEST8(temp8);
	p6309->cc[Z] = ZTEST(temp8);
	p6309->cpu.CycleCounter+=4;
	break;

case SBCB_X: //E2
	postbyte=cpuMemRead8(pCPU,INDADDRESS(pCPU,p6309->PC_REG++));
	temp16=p6309->B_REG-postbyte-p6309->cc[C];
	p6309->cc[C] = (temp16 & 0x100)>>8;
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->B_REG);
	p6309->B_REG = (unsigned char)temp16;
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cpu.CycleCounter+=4;
	break;

case ADDD_X: //E3 
	temp16=cpuMemRead16(&p6309->cpu,INDADDRESS(pCPU,p6309->PC_REG++));
	temp32= p6309->D_REG+ temp16;
	p6309->cc[C] =(temp32 & 0x10000)>>16;
	p6309->cc[V] = OVERFLOW16(p6309->cc[C],temp32,temp16,p6309->D_REG);
	p6309->D_REG= temp32;
	p6309->cc[Z] = ZTEST(p6309->D_REG);
	p6309->cc[N] = NTEST16(p6309->D_REG);
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M65];
	break;

case ANDB_X: //E4
	p6309->B_REG = p6309->B_REG & cpuMemRead8(pCPU,INDADDRESS(pCPU,p6309->PC_REG++));
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=4;
	break;

case BITB_X: //E5 
	temp8 = p6309->B_REG & cpuMemRead8(pCPU,INDADDRESS(pCPU,p6309->PC_REG++));
	p6309->cc[N] = NTEST8(temp8);
	p6309->cc[Z] = ZTEST(temp8);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=4;
	break;

case LDB_X: //E6
	p6309->B_REG=cpuMemRead8(pCPU,INDADDRESS(pCPU,p6309->PC_REG++));
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=4;
	break;

case STB_X: //E7
	cpuMemWrite8(pCPU,p6309->B_REG,CalculateEA(pCPU,cpuMemRead8(pCPU,p6309->PC_REG++)));
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=4;
	break;

case EORB_X: //E8
	p6309->B_REG= p6309->B_REG ^ cpuMemRead8(pCPU,INDADDRESS(pCPU,p6309->PC_REG++));
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=4;
	break;

case ADCB_X: //E9
	postbyte=cpuMemRead8(pCPU,INDADDRESS(pCPU,p6309->PC_REG++));
	temp16= p6309->B_REG + postbyte + p6309->cc[C];
	p6309->cc[C] = (temp16 & 0x100)>>8;
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->B_REG);
	p6309->cc[H] = ((p6309->B_REG ^ temp16 ^ postbyte) & 0x10)>>4;
	p6309->B_REG= (unsigned char)temp16;
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cpu.CycleCounter+=4;
	break;

case ORB_X: //EA 
	p6309->B_REG = p6309->B_REG | cpuMemRead8(pCPU,INDADDRESS(pCPU,p6309->PC_REG++));
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=4;

	break;

case ADDB_X: //EB
	postbyte=cpuMemRead8(pCPU,INDADDRESS(pCPU,p6309->PC_REG++));
	temp16=p6309->B_REG+postbyte;
	p6309->cc[C] =(temp16 & 0x100)>>8;
	p6309->cc[H] = ((p6309->B_REG ^ postbyte ^ temp16) & 0x10)>>4;
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->B_REG);
	p6309->B_REG = (unsigned char)temp16;
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cpu.CycleCounter+=4;
	break;

case LDD_X: //EC
	p6309->D_REG=cpuMemRead16(&p6309->cpu,INDADDRESS(pCPU,p6309->PC_REG++));
	p6309->cc[Z] = ZTEST(p6309->D_REG);
	p6309->cc[N] = NTEST16(p6309->D_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=5;
	break;

case STD_X: //ED
	cpuMemWrite16(&p6309->cpu,p6309->D_REG,INDADDRESS(pCPU,p6309->PC_REG++));
	p6309->cc[Z] = ZTEST(p6309->D_REG);
	p6309->cc[N] = NTEST16(p6309->D_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=5;
	break;

case LDU_X: //EE
	p6309->U_REG=cpuMemRead16(&p6309->cpu,INDADDRESS(pCPU,p6309->PC_REG++));
	p6309->cc[Z] =ZTEST(p6309->U_REG);
	p6309->cc[N] =NTEST16(p6309->U_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=5;
	break;

case STU_X: //EF
	cpuMemWrite16(&p6309->cpu,p6309->U_REG,INDADDRESS(pCPU,p6309->PC_REG++));
	p6309->cc[Z] = ZTEST(p6309->U_REG);
	p6309->cc[N] = NTEST16(p6309->U_REG);
	p6309->cc[V] = 0;
	p6309->cpu.CycleCounter+=5;
	break;

case SUBB_E: //F0
	postbyte=cpuMemRead8(pCPU,IMMADDRESS(p6309,p6309->PC_REG));
	temp16 = p6309->B_REG - postbyte;
	p6309->cc[C] = (temp16 & 0x100)>>8; 
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->B_REG);
	p6309->B_REG= (unsigned char)temp16;
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
	break;

case CMPB_E: //F1
	postbyte=cpuMemRead8(pCPU,IMMADDRESS(p6309,p6309->PC_REG));
	temp8= p6309->B_REG-postbyte;
	p6309->cc[C] = temp8 > p6309->B_REG;
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp8,p6309->B_REG);
	p6309->cc[N] = NTEST8(temp8);
	p6309->cc[Z] = ZTEST(temp8);
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
	break;

case SBCB_E: //F2
	postbyte=cpuMemRead8(pCPU,IMMADDRESS(p6309,p6309->PC_REG));
	temp16=p6309->B_REG-postbyte-p6309->cc[C];
	p6309->cc[C] = (temp16 & 0x100)>>8;
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->B_REG);
	p6309->B_REG = (unsigned char)temp16;
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
	break;

case ADDD_E: //F3
	temp16=cpuMemRead16(&p6309->cpu,IMMADDRESS(p6309,p6309->PC_REG));
	temp32= p6309->D_REG+ temp16;
	p6309->cc[C] =(temp32 & 0x10000)>>16;
	p6309->cc[V] = OVERFLOW16(p6309->cc[C],temp32,temp16,p6309->D_REG);
	p6309->D_REG = temp32;
	p6309->cc[Z] = ZTEST(p6309->D_REG);
	p6309->cc[N] = NTEST16(p6309->D_REG);
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M76];
	break;

case ANDB_E:  //F4
	p6309->B_REG = p6309->B_REG & cpuMemRead8(pCPU,IMMADDRESS(p6309,p6309->PC_REG));
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cc[V] = 0;
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
	break;

case BITB_E: //F5
	temp8 = p6309->B_REG & cpuMemRead8(pCPU,IMMADDRESS(p6309,p6309->PC_REG));
	p6309->cc[N] = NTEST8(temp8);
	p6309->cc[Z] = ZTEST(temp8);
	p6309->cc[V] = 0;
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
	break;

case LDB_E: //F6
	p6309->B_REG=cpuMemRead8(pCPU,IMMADDRESS(p6309,p6309->PC_REG));
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cc[V] = 0;
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
	break;

case STB_E: //F7 
	cpuMemWrite8(pCPU,p6309->B_REG,IMMADDRESS(p6309,p6309->PC_REG));
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cc[V] = 0;
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
	break;

case EORB_E: //F8
	p6309->B_REG = p6309->B_REG ^ cpuMemRead8(pCPU,IMMADDRESS(p6309,p6309->PC_REG));
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cc[V] = 0;
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
	break;

case ADCB_E: //F9
	postbyte=cpuMemRead8(pCPU,IMMADDRESS(p6309,p6309->PC_REG));
	temp16= p6309->B_REG + postbyte + p6309->cc[C];
	p6309->cc[C] = (temp16 & 0x100)>>8;
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->B_REG);
	p6309->cc[H] = ((p6309->B_REG ^ temp16 ^ postbyte) & 0x10)>>4;
	p6309->B_REG= (unsigned char)temp16;
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
	break;

case ORB_E: //FA
	p6309->B_REG = p6309->B_REG | cpuMemRead8(pCPU,IMMADDRESS(p6309,p6309->PC_REG));
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cc[Z] = ZTEST(p6309->B_REG);
	p6309->cc[V] = 0;
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
	break;

case ADDB_E: //FB
	postbyte=cpuMemRead8(pCPU,IMMADDRESS(p6309,p6309->PC_REG));
	temp16=p6309->B_REG+postbyte;
	p6309->cc[C] =(temp16 & 0x100)>>8;
	p6309->cc[H] = ((p6309->B_REG ^ postbyte ^ temp16) & 0x10)>>4;
	p6309->cc[V] = OVERFLOW8(p6309->cc[C],postbyte,temp16,p6309->B_REG);
	p6309->B_REG= (unsigned char)temp16;
	p6309->cc[N] = NTEST8(p6309->B_REG);
	p6309->cc[Z] =ZTEST(p6309->B_REG);
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M54];
	break;

case LDD_E: //FC
	p6309->D_REG=cpuMemRead16(&p6309->cpu,IMMADDRESS(p6309,p6309->PC_REG));
	p6309->cc[Z] = ZTEST(p6309->D_REG);
	p6309->cc[N] = NTEST16(p6309->D_REG);
	p6309->cc[V] = 0;
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M65];
	break;

case STD_E: //FD
	cpuMemWrite16(&p6309->cpu,p6309->D_REG,IMMADDRESS(p6309,p6309->PC_REG));
	p6309->cc[Z] = ZTEST(p6309->D_REG);
	p6309->cc[N] = NTEST16(p6309->D_REG);
	p6309->cc[V] = 0;
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M65];
	break;

case LDU_E: //FE
	p6309->U_REG= cpuMemRead16(&p6309->cpu,IMMADDRESS(p6309,p6309->PC_REG));
	p6309->cc[Z] = ZTEST(p6309->U_REG);
	p6309->cc[N] = NTEST16(p6309->U_REG);
	p6309->cc[V] = 0;
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M65];
	break;

case STU_E: //FF
	cpuMemWrite16(&p6309->cpu,p6309->U_REG,IMMADDRESS(p6309,p6309->PC_REG));
	p6309->cc[Z] = ZTEST(p6309->U_REG);
	p6309->cc[N] = NTEST16(p6309->U_REG);
	p6309->cc[V] = 0;
	p6309->PC_REG+=2;
	p6309->cpu.CycleCounter+=p6309->InsCycles[p6309->md[NATIVE6309]][M65];
	break;

default:
	InvalidInsHandler(pCPU);
	break;
	}//End Switch
}//End While

return(CycleFor-p6309->cpu.CycleCounter);

}

/**************************************************************************/

void cpuHD6309AssertInterrupt(cpu_t * pCPU, irqtype_e Interrupt, int waiter) // 4 nmi 2 firq 1 irq
{
	cpu6309_t *	p6309 = (cpu6309_t *)pCPU;
	
	assert(p6309 != NULL);
	
	p6309->SyncWaiting=0;
	p6309->PendingInterrupts = p6309->PendingInterrupts | Interrupt;
	p6309->IRQWaiter = waiter;
}

/**************************************************************************/

void cpuHD6309DeAssertInterrupt(cpu_t * pCPU, irqtype_e Interrupt) // 4 nmi 2 firq 1 irq
{
	cpu6309_t *	p6309 = (cpu6309_t *)pCPU;
	
	assert(p6309 != NULL);
	
	p6309->PendingInterrupts = p6309->PendingInterrupts & ~Interrupt;
	p6309->InInterrupt=0;
}

/**************************************************************************/

void cpuHD6309ForcePC(cpu_t * pCPU, int NewPC)
{
	cpu6309_t *	p6309 = (cpu6309_t *)pCPU;
	
	assert(p6309 != NULL);
	
	p6309->PC_REG=NewPC;
	p6309->PendingInterrupts=0;
	p6309->SyncWaiting=0;
}

/********************************************************************************/

#pragma mark -
#pragma mark --- emulator device callbacks ---

/********************************************************************/

result_t hd6309EmuDevDestroy(emudevice_t * pEmuDevice)
{
    cpu6309_t *	pHD6309;
    
    pHD6309 = (cpu6309_t *)pEmuDevice;
    
    ASSERT_HD6309(pHD6309);
    
    if ( pHD6309 != NULL )
    {
        emuDevRemoveChild(pHD6309->cpu.device.pParent,&pHD6309->cpu.device);
        cpuDestroy(&pHD6309->cpu);
    }
    
    return XERROR_NONE;
}

/********************************************************************************/

result_t hd6309EmuDevConfSave(emudevice_t * pEmuDevice, config_t * config)
{
    result_t	errResult	= XERROR_INVALID_PARAMETER;
    cpu6309_t *	pHD6309;
    
    pHD6309 = (cpu6309_t *)pEmuDevice;
    
    ASSERT_HD6309(pHD6309);
    
    if ( pHD6309 != NULL )
    {
        // do nothing for now
        
        errResult = XERROR_NONE;
    }
    
    return errResult;
}

/********************************************************************************/

result_t hd6309EmuDevConfLoad(emudevice_t * pEmuDevice, config_t * config)
{
    result_t	errResult	= XERROR_INVALID_PARAMETER;
    cpu6309_t *	pHD6309;
    
    pHD6309 = (cpu6309_t *)pEmuDevice;
    
    ASSERT_HD6309(pHD6309);
    
    if ( pHD6309 != NULL )
    {
        // do nothing for now
        
        errResult = XERROR_NONE;
    }
    
    return errResult;
}

/********************************************************************************/

#pragma mark -
#pragma mark --- exported plug-in functions ---

/********************************************************************************/

XAPI cpu_t * vcccpuModuleCreate()
{
    cpu6309_t *		p6309;
    
    p6309 = calloc(1,sizeof(cpu6309_t));
    assert(p6309 != NULL);
    if ( p6309 != NULL )
    {
        p6309->cpu.device.id		= EMU_DEVICE_ID;
        p6309->cpu.device.idModule	= EMU_CPU_ID;
        p6309->cpu.device.idDevice	= VCC_HD6309_ID;
        strncpy(p6309->cpu.device.Name,"HD6309",sizeof(p6309->cpu.device.Name)-1);
        
        p6309->cpu.device.pfnDestroy	= hd6309EmuDevDestroy;
        p6309->cpu.device.pfnSave		= hd6309EmuDevConfSave;
        p6309->cpu.device.pfnLoad		= hd6309EmuDevConfLoad;
        
        p6309->cpu.cpuInit				= cpuHD6309Init;
        p6309->cpu.cpuExec				= cpuHD6309Exec;
        p6309->cpu.cpuReset				= cpuHD6309Reset;
        p6309->cpu.cpuAssertInterrupt	= cpuHD6309AssertInterrupt;
        p6309->cpu.cpuDeAssertInterrupt	= cpuHD6309DeAssertInterrupt;
        p6309->cpu.cpuForcePC			= cpuHD6309ForcePC;
        p6309->cpu.cpuDestructor		= NULL;
        
        cpuInit(&p6309->cpu);
        
        // init vars here once moved into CPU struct
        
        return &p6309->cpu;
    }
    
    return NULL;
}

int performDivQ(cpu_t * pCPU,short divisor)
{
    cpu6309_t * p6309 = (cpu6309_t *)pCPU;
    int divOkay = 1;    //true
    if(divisor != 0)
    {
        int stemp32 = p6309->Q_REG;
        int stempW = stemp32 / divisor;
        if(stempW > 0xFFFF || stempW < SHRT_MIN)
        {
            //overflow, quotient is larger then can be stored in 16 bits
            //registers remain untouched, set flags accordingly
            p6309->cc[V] = 1;
            p6309->cc[N] = 0;
            p6309->cc[Z] = 0;
            p6309->cc[C] = 0;
        }
        else
        {
            p6309->W_REG = stempW;
            p6309->D_REG = stemp32 % divisor;
            p6309->cc[N] = p6309->W_REG >> 15;    //set to bit 15
            p6309->cc[Z] = ZTEST(p6309->W_REG);
            p6309->cc[C] = p6309->W_REG & 1;
            
            //check for overflow, quotient can't be represented by signed 16 bit value
            p6309->cc[V] = stempW > SHRT_MAX;
            if(p6309->cc[V])
            {
                p6309->cc[N] = 1;
            }
        }
    }
    else
    {
        divOkay = 0;
        DivbyZero(pCPU);
    }
    return divOkay;
}

/**************************************************************************/


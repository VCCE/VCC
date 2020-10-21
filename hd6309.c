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

		Additional 6309 modifications by Walter ZAMBOTTI 2019
*/

#include <stdio.h>
#include <stdlib.h>
#include "hd6309.h"
#include "hd6309defs.h"
#include "tcc1014mmu.h"
#include "logger.h"

#if defined(_WIN64)
#define MSABI 
#else
#define MSABI __attribute__((ms_abi))
#endif

//Global variables for CPU Emulation-----------------------
#define NTEST8(r) r>0x7F;
#define NTEST16(r) r>0x7FFF;
#define NTEST32(r) r>0x7FFFFFFF;
#define OVERFLOW8(c,a,b,r) c ^ (((a^b^r)>>7) &1);
#define OVERFLOW16(c,a,b,r) c ^ (((a^b^r)>>15)&1);
#define ZTEST(r) !r;

#define DPADDRESS(r) (dp.Reg |MemRead8(r))
#define IMMADDRESS(r) MemRead16(r)
#define INDADDRESS(r) CalculateEA(MemRead8(r))

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
#define M42 22
#define M53 23

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
static char RegName[16][10]={"D","X","Y","U","S","PC","W","V","A","B","CC","DP","ZERO","ZERO","E","F"};

static wideregister q;
static cpuregister pc, x, y, u, s, dp, v, z;
static unsigned char InsCycles[2][25];
static unsigned char cc[8];
static unsigned int md[8];
static unsigned char *ureg8[8]; 
static unsigned char ccbits,mdbits;
static unsigned short *xfreg16[8];
static int CycleCounter=0;
static unsigned int SyncWaiting=0;
unsigned short temp16;
static signed short stemp16;
static signed char stemp8;
static unsigned int  temp32;
static int stemp32;
static unsigned char temp8; 
static unsigned char PendingInterupts=0;
static unsigned char IRQWaiter=0;
static unsigned char Source=0,Dest=0;
static unsigned char postbyte=0;
static short unsigned postword=0;
static signed char *spostbyte=(signed char *)&postbyte;
static signed short *spostword=(signed short *)&postword;
static char InInterupt=0;
static int gCycleFor;

static unsigned char NatEmuCycles65 = 6;
static unsigned char NatEmuCycles64 = 6;
static unsigned char NatEmuCycles32 = 3;
static unsigned char NatEmuCycles21 = 2;
static unsigned char NatEmuCycles54 = 5;
static unsigned char NatEmuCycles97 = 9;
static unsigned char NatEmuCycles85 = 8;
static unsigned char NatEmuCycles51 = 5;
static unsigned char NatEmuCycles31 = 3;
static unsigned char NatEmuCycles1110 = 11;
static unsigned char NatEmuCycles76 = 7;
static unsigned char NatEmuCycles75 = 7;
static unsigned char NatEmuCycles43 = 4;
static unsigned char NatEmuCycles87 = 8;
static unsigned char NatEmuCycles86 = 8;
static unsigned char NatEmuCycles98 = 9;
static unsigned char NatEmuCycles2726 = 27;
static unsigned char NatEmuCycles3635 = 36;
static unsigned char NatEmuCycles3029 = 30;
static unsigned char NatEmuCycles2827 = 28;
static unsigned char NatEmuCycles3726 = 37;
static unsigned char NatEmuCycles3130 = 31;
static unsigned char NatEmuCycles42 = 4;
static unsigned char NatEmuCycles53 = 5;

static unsigned char *NatEmuCycles[] =
{
	&NatEmuCycles65,
	&NatEmuCycles64,
	&NatEmuCycles32,
	&NatEmuCycles21,
	&NatEmuCycles54,
	&NatEmuCycles97,
	&NatEmuCycles85,
	&NatEmuCycles51,
	&NatEmuCycles31,
	&NatEmuCycles1110,
	&NatEmuCycles76,
	&NatEmuCycles75,
	&NatEmuCycles43,
	&NatEmuCycles87,
	&NatEmuCycles86,
	&NatEmuCycles98,
	&NatEmuCycles2726,
	&NatEmuCycles3635,
	&NatEmuCycles3029,
	&NatEmuCycles2827,
	&NatEmuCycles3726,
	&NatEmuCycles3130,
	&NatEmuCycles42,
	&NatEmuCycles53
};

//END Global variables for CPU Emulation-------------------

//Fuction Prototypes---------------------------------------
static unsigned short CalculateEA(unsigned char);
void InvalidInsHandler(void);
void DivbyZero(void);
void ErrorVector(void);
void setcc (unsigned char);
unsigned char getcc(void);
void setmd (unsigned char);
unsigned char getmd(void);
static void cpu_firq(void);
static void cpu_irq(void);
static void cpu_nmi(void);
unsigned char GetSorceReg(unsigned char);
void Page_2(void);
void Page_3(void);
void MemWrite32(unsigned int, unsigned short);
unsigned int MemRead32(unsigned short);
// void MemWrite8(unsigned char, unsigned short);
// void MemWrite16(unsigned short, unsigned short);
// unsigned char MemRead8(unsigned short);
// unsigned short MemRead16(unsigned short);
// extern void SetNatEmuStat(unsigned char);

//unsigned char GetDestReg(unsigned char);
//END Fuction Prototypes-----------------------------------

void HD6309Reset(void)
{
	char index;
	for(index=0;index<=6;index++)		//Set all register to 0 except V
		*xfreg16[index] = 0;
	for(index=0;index<=7;index++)
		*ureg8[index]=0;
	for(index=0;index<=7;index++)
		cc[index]=0;
	for(index=0;index<=7;index++)
		md[index]=0;
	mdbits=getmd();
	dp.Reg=0;
	cc[I]=1;
	cc[F]=1;
	SyncWaiting=0;
	PC_REG=MemRead16(VRESET);	//PC gets its reset vector
	SetMapType(0);	//shouldn't be here
	return;
}

void HD6309Init(void)
{	//Call this first or RESET will core!
	// reg pointers for TFR and EXG and LEA ops
	xfreg16[0] = &D_REG;
	xfreg16[1] = &X_REG;
	xfreg16[2] = &Y_REG;
	xfreg16[3] = &U_REG;
	xfreg16[4] = &S_REG;
	xfreg16[5] = &PC_REG;
	xfreg16[6] = &W_REG;
	xfreg16[7] = &V_REG;

	ureg8[0]=(unsigned char*)&A_REG;		
	ureg8[1]=(unsigned char*)&B_REG;		
	ureg8[2]=(unsigned char*)&ccbits;
	ureg8[3]=(unsigned char*)&dp.B.msb;
	ureg8[4]=(unsigned char*)&z.B.msb;
	ureg8[5]=(unsigned char*)&z.B.lsb;
	ureg8[6]=(unsigned char*)&E_REG;
	ureg8[7]=(unsigned char*)&F_REG;

	//This handles the disparity between 6309 and 6809 Instruction timing
	InsCycles[0][M65]=6;	//6-5
	InsCycles[1][M65]=5;
	InsCycles[0][M64]=6;	//6-4
	InsCycles[1][M64]=4;
	InsCycles[0][M32]=3;	//3-2
	InsCycles[1][M32]=2;
	InsCycles[0][M21]=2;	//2-1
	InsCycles[1][M21]=1;
	InsCycles[0][M54]=5;	//5-4
	InsCycles[1][M54]=4;
	InsCycles[0][M97]=9;	//9-7
	InsCycles[1][M97]=7;
	InsCycles[0][M85]=8;	//8-5
	InsCycles[1][M85]=5;
	InsCycles[0][M51]=5;	//5-1
	InsCycles[1][M51]=1;
	InsCycles[0][M31]=3;	//3-1
	InsCycles[1][M31]=1;
	InsCycles[0][M1110]=11;	//11-10
	InsCycles[1][M1110]=10;
	InsCycles[0][M76]=7;	//7-6
	InsCycles[1][M76]=6;
	InsCycles[0][M75]=7;	//7-5
	InsCycles[1][M75]=5;
	InsCycles[0][M43]=4;	//4-3
	InsCycles[1][M43]=3;
	InsCycles[0][M87]=8;	//8-7
	InsCycles[1][M87]=7;
	InsCycles[0][M86]=8;	//8-6
	InsCycles[1][M86]=6;
	InsCycles[0][M98]=9;	//9-8
	InsCycles[1][M98]=8;
	InsCycles[0][M2726]=27;	//27-26
	InsCycles[1][M2726]=26;
	InsCycles[0][M3635]=36;	//36-25
	InsCycles[1][M3635]=35;	
	InsCycles[0][M3029]=30;	//30-29
	InsCycles[1][M3029]=29;	
	InsCycles[0][M2827]=28;	//28-27
	InsCycles[1][M2827]=27;	
	InsCycles[0][M3726]=37;	//37-26
	InsCycles[1][M3726]=26;		
	InsCycles[0][M3130]=31;	//31-30
	InsCycles[1][M3130]=30;		
	InsCycles[0][M42]=4;	//4-2
	InsCycles[1][M42]=2;		
	InsCycles[0][M53]=5;	//5-3
	InsCycles[1][M53]=3;		
	//SetNatEmuStat(1);
	cc[I]=1;
	cc[F]=1;
	return;
}


void Neg_D(void)
{ //0
	temp16 = DPADDRESS(PC_REG++);
	postbyte = MemRead8(temp16);
	temp8 = 0 - postbyte;
	cc[C] = temp8 > 0;
	cc[V] = (postbyte == 0x80);
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	MemWrite8(temp8, temp16);
	CycleCounter += NatEmuCycles65;
}

void Oim_D(void)
{//1 6309
	postbyte=MemRead8(PC_REG++);
	temp16 = DPADDRESS(PC_REG++);
	postbyte|= MemRead8(temp16);
	MemWrite8(postbyte,temp16);
	cc[N] = NTEST8(postbyte);
	cc[Z] = ZTEST(postbyte);
	cc[V] = 0;
	CycleCounter+=6;
}

void Aim_D(void)
{//2 Phase 2 6309
	postbyte=MemRead8(PC_REG++);
	temp16 = DPADDRESS(PC_REG++);
	postbyte&= MemRead8(temp16);
	MemWrite8(postbyte,temp16);
	cc[N] = NTEST8(postbyte);
	cc[Z] = ZTEST(postbyte);
	cc[V] = 0;
	CycleCounter+=6;
}

void Com_D(void)
{ //03
	temp16 = DPADDRESS(PC_REG++);
	temp8=MemRead8(temp16);
	temp8=0xFF-temp8;
	cc[Z] = ZTEST(temp8);
	cc[N] = NTEST8(temp8); 
	cc[C] = 1;
	cc[V] = 0;
	MemWrite8(temp8,temp16);
	CycleCounter+=NatEmuCycles65;
}

void Lsr_D(void)
{ //04 S2
	temp16 = DPADDRESS(PC_REG++);
	temp8 = MemRead8(temp16);
	cc[C] = temp8 & 1;
	temp8 = temp8 >>1;
	cc[Z] = ZTEST(temp8);
	cc[N] = 0;
	MemWrite8(temp8,temp16);
	CycleCounter+=NatEmuCycles65;
}

void Eim_D(void)
{ //05 6309 Untested
	postbyte=MemRead8(PC_REG++);
	temp16 = DPADDRESS(PC_REG++);
	postbyte^= MemRead8(temp16);
	MemWrite8(postbyte,temp16);
	cc[N] = NTEST8(postbyte);
	cc[Z] = ZTEST(postbyte);
	cc[V] = 0;
	CycleCounter+=6;
}

void Ror_D(void)
{ //06 S2
	temp16 = DPADDRESS(PC_REG++);
	temp8=MemRead8(temp16);
	postbyte= cc[C]<<7;
	cc[C] = temp8 & 1;
	temp8 = (temp8 >> 1)| postbyte;
	cc[Z] = ZTEST(temp8);
	cc[N] = NTEST8(temp8);
	MemWrite8(temp8,temp16);
	CycleCounter+=NatEmuCycles65;
}

void Asr_D(void)
{ //7
	temp16 = DPADDRESS(PC_REG++);
	temp8=MemRead8(temp16);
	cc[C] = temp8 & 1;
	temp8 = (temp8 & 0x80) | (temp8 >>1);
	cc[Z] = ZTEST(temp8);
	cc[N] = NTEST8(temp8);
	MemWrite8(temp8,temp16);
	CycleCounter+=NatEmuCycles65;
}

void Asl_D(void)
{ //8 
	temp16 = DPADDRESS(PC_REG++);
	temp8=MemRead8(temp16);
	cc[C] = (temp8 & 0x80) >>7;
	cc[V] = cc[C] ^ ((temp8 & 0x40) >> 6);
	temp8 = temp8 <<1;
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	MemWrite8(temp8,temp16);
	CycleCounter+=NatEmuCycles65;
}

void Rol_D(void)
{	//9
	temp16 = DPADDRESS(PC_REG++);
	temp8 = MemRead8(temp16);
	postbyte=cc[C];
	cc[C] =(temp8 & 0x80)>>7;
	cc[V] = cc[C] ^ ((temp8 & 0x40) >>6);
	temp8 = (temp8<<1) | postbyte;
	cc[Z] = ZTEST(temp8);
	cc[N] = NTEST8(temp8);
	MemWrite8(temp8,temp16);
	CycleCounter+=NatEmuCycles65;
}

void Dec_D(void)
{ //A
	temp16 = DPADDRESS(PC_REG++);
	temp8 = MemRead8(temp16)-1;
	cc[Z] = ZTEST(temp8);
	cc[N] = NTEST8(temp8);
	cc[V] = temp8==0x7F;
	MemWrite8(temp8,temp16);
	CycleCounter+=NatEmuCycles65;
}

void Tim_D(void)
{	//B 6309 Untested wcreate
	postbyte=MemRead8(PC_REG++);
	temp8=MemRead8(DPADDRESS(PC_REG++));
	postbyte&=temp8;
	cc[N] = NTEST8(postbyte);
	cc[Z] = ZTEST(postbyte);
	cc[V] = 0;
	CycleCounter+=6;
}

void Inc_D(void)
{ //C
	temp16=(DPADDRESS(PC_REG++));
	temp8 = MemRead8(temp16)+1;
	cc[Z] = ZTEST(temp8);
	cc[V] = temp8==0x80;
	cc[N] = NTEST8(temp8);
	MemWrite8(temp8,temp16);
	CycleCounter+=NatEmuCycles65;
}

void Tst_D(void)
{ //D
	temp8 = MemRead8(DPADDRESS(PC_REG++));
	cc[Z] = ZTEST(temp8);
	cc[N] = NTEST8(temp8);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles64;
}

void Jmp_D(void)
{	//E
	PC_REG= ((dp.Reg |MemRead8(PC_REG)));
	CycleCounter+=NatEmuCycles32;
}

void Clr_D(void)
{	//F
	MemWrite8(0,DPADDRESS(PC_REG++));
	cc[Z] = 1;
	cc[N] = 0;
	cc[V] = 0;
	cc[C] = 0;
	CycleCounter+=NatEmuCycles65;
}

void LBeq_R(void)
{ //1027
	if (cc[Z])
	{
		*spostword=IMMADDRESS(PC_REG);
		PC_REG+=*spostword;
		CycleCounter+=1;
	}
	PC_REG+=2;
	CycleCounter+=5;
}

void LBrn_R(void)
{ //1021
	PC_REG+=2;
	CycleCounter+=5;
}

void LBhi_R(void)
{ //1022
	if  (!(cc[C] | cc[Z]))
	{
		*spostword=IMMADDRESS(PC_REG);
		PC_REG+=*spostword;
		CycleCounter+=1;
	}
	PC_REG+=2;
	CycleCounter+=5;
}

void LBls_R(void)
{ //1023
	if (cc[C] | cc[Z])
	{
		*spostword=IMMADDRESS(PC_REG);
		PC_REG+=*spostword;
		CycleCounter+=1;
	}
	PC_REG+=2;
	CycleCounter+=5;
}

void LBhs_R(void)
{ //1024
	if (!cc[C])
	{
		*spostword=IMMADDRESS(PC_REG);
		PC_REG+=*spostword;
		CycleCounter+=1;
	}
	PC_REG+=2;
	CycleCounter+=6;
}

void LBcs_R(void)
{ //1025
	if (cc[C])
	{
		*spostword=IMMADDRESS(PC_REG);
		PC_REG+=*spostword;
		CycleCounter+=1;
	}
	PC_REG+=2;
	CycleCounter+=5;
}

void LBne_R(void)
{ //1026
	if (!cc[Z])
	{
		*spostword=IMMADDRESS(PC_REG);
		PC_REG+=*spostword;
		CycleCounter+=1;
	}
	PC_REG+=2;
	CycleCounter+=5;
}

void LBvc_R(void)
{ //1028
	if ( !cc[V])
	{
		*spostword=IMMADDRESS(PC_REG);
		PC_REG+=*spostword;
		CycleCounter+=1;
	}
	PC_REG+=2;
	CycleCounter+=5;
}

void LBvs_R(void)
{ //1029
	if ( cc[V])
	{
		*spostword=IMMADDRESS(PC_REG);
		PC_REG+=*spostword;
		CycleCounter+=1;
	}
	PC_REG+=2;
	CycleCounter+=5;
}

void LBpl_R(void)
{ //102A
if (!cc[N])
	{
		*spostword=IMMADDRESS(PC_REG);
		PC_REG+=*spostword;
		CycleCounter+=1;
	}
	PC_REG+=2;
	CycleCounter+=5;
}

void LBmi_R(void)
{ //102B
if ( cc[N])
	{
		*spostword=IMMADDRESS(PC_REG);
		PC_REG+=*spostword;
		CycleCounter+=1;
	}
	PC_REG+=2;
	CycleCounter+=5;
}

void LBge_R(void)
{ //102C
	if (! (cc[N] ^ cc[V]))
	{
		*spostword=IMMADDRESS(PC_REG);
		PC_REG+=*spostword;
		CycleCounter+=1;
	}
	PC_REG+=2;
	CycleCounter+=5;
}

void LBlt_R(void)
{ //102D
	if ( cc[V] ^ cc[N])
	{
		*spostword=IMMADDRESS(PC_REG);
		PC_REG+=*spostword;
		CycleCounter+=1;
	}
	PC_REG+=2;
	CycleCounter+=5;
}

void LBgt_R(void)
{ //102E
	if ( !( cc[Z] | (cc[N]^cc[V] ) ))
	{
		*spostword=IMMADDRESS(PC_REG);
		PC_REG+=*spostword;
		CycleCounter+=1;
	}
	PC_REG+=2;
	CycleCounter+=5;
}

void LBle_R(void)
{	//102F
	if ( cc[Z] | (cc[N]^cc[V]) )
	{
		*spostword=IMMADDRESS(PC_REG);
		PC_REG+=*spostword;
		CycleCounter+=1;
	}
	PC_REG+=2;
	CycleCounter+=5;
}

void Addr(void)
{ //1030 6309 - WallyZ 2019
	unsigned char dest8, source8;
	unsigned short dest16, source16;
	temp8 = MemRead8(PC_REG++);
	Source = temp8 >> 4;
	Dest = temp8 & 15;

	if (Dest > 7) // 8 bit dest
	{
		Dest &= 7;
		if (Dest == 2) dest8 = getcc();
		else dest8 = *ureg8[Dest];

		if (Source > 7) // 8 bit source
		{
			Source &= 7;
			if (Source == 2)
				source8 = getcc();
			else 
				source8 = *ureg8[Source];
		}
		else // 16 bit source - demote to 8 bit
		{
			Source &= 7;
			source8 = (unsigned char)*xfreg16[Source];
		}

		temp16 = source8 + dest8;
		switch (Dest)
		{
			case 2: 				setcc((unsigned char)temp16); break;
			case 4: case 5: break; // never assign to zero reg
			default: 				*ureg8[Dest] = (unsigned char)temp16; break;
		}
		cc[C] = (temp16 & 0x100) >> 8;
		cc[V] = OVERFLOW8(cc[C], source8, dest8, temp16);
		cc[N] = NTEST8(*ureg8[Dest]);
		cc[Z] = ZTEST(*ureg8[Dest]);
	}
	else // 16 bit dest
	{
		dest16 = *xfreg16[Dest];

		if (Source < 8) // 16 bit source
		{
			source16 = *xfreg16[Source];
		}
		else // 8 bit source - promote to 16 bit
		{
			Source &= 7;
			switch (Source)
			{
			case 0: case 1: source16 = D_REG; break; // A & B Reg
			case 2:	        source16 = (unsigned short)getcc(); break; // CC
			case 3:	        source16 = (unsigned short)dp.Reg; break; // DP
			case 4: case 5: source16 = 0; break; // Zero Reg
			case 6: case 7: source16 = W_REG; break; // E & F Reg
			}
		}

		temp32 = source16 + dest16;
		*xfreg16[Dest] = (unsigned short)temp32;
		cc[C] = (temp32 & 0x10000) >> 16;
		cc[V] = OVERFLOW16(cc[C], source16, dest16, temp32);
		cc[N] = NTEST16(*xfreg16[Dest]);
		cc[Z] = ZTEST(*xfreg16[Dest]);
	}
	CycleCounter += 4;
}

void Adcr(void)
{ //1031 6309 - WallyZ 2019
	unsigned char dest8, source8;
	unsigned short dest16, source16;
	temp8 = MemRead8(PC_REG++);
	Source = temp8 >> 4;
	Dest = temp8 & 15;

	if (Dest > 7) // 8 bit dest
	{
		Dest &= 7;
		if (Dest == 2) dest8 = getcc();
		else dest8 = *ureg8[Dest];

		if (Source > 7) // 8 bit source
		{
			Source &= 7;
			if (Source == 2) source8 = getcc();
			else source8 = *ureg8[Source];
		}
		else // 16 bit source - demote to 8 bit
		{
			Source &= 7;
			source8 = (unsigned char)*xfreg16[Source];
		}

		temp16 = source8 + dest8 + cc[C];
		switch (Dest)
		{
			case 2: 				setcc((unsigned char)temp16); break;
			case 4: case 5: break; // never assign to zero reg
			default: 				*ureg8[Dest] = (unsigned char)temp16; break;
		}
		cc[C] = (temp16 & 0x100) >> 8;
		cc[V] = OVERFLOW8(cc[C], source8, dest8, temp16);
		cc[N] = NTEST8(*ureg8[Dest]);
		cc[Z] = ZTEST(*ureg8[Dest]);
	}
	else // 16 bit dest
	{
		dest16 = *xfreg16[Dest];

		if (Source < 8) // 16 bit source
		{
			source16 = *xfreg16[Source];
		}
		else // 8 bit source - promote to 16 bit
		{
			Source &= 7;
			switch (Source)
			{
			case 0: case 1: source16 = D_REG; break; // A & B Reg
			case 2:	        source16 = (unsigned short)getcc(); break; // CC
			case 3:	        source16 = (unsigned short)dp.Reg; break; // DP
			case 4: case 5: source16 = 0; break; // Zero Reg
			case 6: case 7: source16 = W_REG; break; // E & F Reg
			}
		}

		temp32 = source16 + dest16 + cc[C];
		*xfreg16[Dest] = (unsigned short)temp32;
		cc[C] = (temp32 & 0x10000) >> 16;
		cc[V] = OVERFLOW16(cc[C], source16, dest16, temp32);
		cc[N] = NTEST16(*xfreg16[Dest]);		cc[Z] = ZTEST(*xfreg16[Dest]);
	}
	CycleCounter += 4;
}

void Subr(void)
{ //1032 6309 - WallyZ 2019
	unsigned char dest8, source8;
	unsigned short dest16, source16;
	temp8 = MemRead8(PC_REG++);
	Source = temp8 >> 4;
	Dest = temp8 & 15;

	if (Dest > 7) // 8 bit dest
	{
		Dest &= 7;
		if (Dest == 2) dest8 = getcc();
		else dest8 = *ureg8[Dest];

		if (Source > 7) // 8 bit source
		{
			Source &= 7;
			if (Source == 2) source8 = getcc();
			else source8 = *ureg8[Source];
		}
		else // 16 bit source - demote to 8 bit
		{
			Source &= 7;
			source8 = (unsigned char)*xfreg16[Source];
		}

		temp16 = dest8 - source8;
		switch (Dest)
		{
			case 2: 				setcc((unsigned char)temp16); break;
			case 4: case 5: break; // never assign to zero reg
			default: 				*ureg8[Dest] = (unsigned char)temp16; break;
		}
		cc[C] = (temp16 & 0x100) >> 8;
		cc[V] = cc[C] ^ ((dest8 ^ *ureg8[Dest] ^ source8) >> 7);
		cc[N] = *ureg8[Dest] >> 7;
		cc[Z] = ZTEST(*ureg8[Dest]);
	}
	else // 16 bit dest
	{
		dest16 = *xfreg16[Dest];

		if (Source < 8) // 16 bit source
		{
			source16 = *xfreg16[Source];
		}
		else // 8 bit source - promote to 16 bit
		{
			Source &= 7;
			switch (Source)
			{
			case 0: case 1: source16 = D_REG; break; // A & B Reg
			case 2:	        source16 = (unsigned short)getcc(); break; // CC
			case 3:	        source16 = (unsigned short)dp.Reg; break; // DP
			case 4: case 5: source16 = 0; break; // Zero Reg
			case 6: case 7: source16 = W_REG; break; // E & F Reg
			}
		}

		temp32 = dest16 - source16;
		cc[C] = (temp32 & 0x10000) >> 16;
		cc[V] = !!((dest16 ^ source16 ^ temp32 ^ (temp32 >> 1)) & 0x8000);
		*xfreg16[Dest] = (unsigned short)temp32;
		cc[N] = (temp32 & 0x8000) >> 15;
		cc[Z] = ZTEST(temp32);
	}
	CycleCounter += 4;
}

void Sbcr(void)
{ //1033 6309 - WallyZ 2019
	unsigned char dest8, source8;
	unsigned short dest16, source16;
	temp8 = MemRead8(PC_REG++);
	Source = temp8 >> 4;
	Dest = temp8 & 15;

	if (Dest > 7) // 8 bit dest
	{
		Dest &= 7;
		if (Dest == 2) dest8 = getcc();
		else dest8 = *ureg8[Dest];

		if (Source > 7) // 8 bit source
		{
			Source &= 7;
			if (Source == 2) source8 = getcc();
			else source8 = *ureg8[Source];
		}
		else // 16 bit source - demote to 8 bit
		{
			Source &= 7;
			source8 = (unsigned char)*xfreg16[Source];
		}

		temp16 = dest8 - source8 - cc[C];
		switch (Dest)
		{
			case 2: 				setcc((unsigned char)temp16); break;
			case 4: case 5: break; // never assign to zero reg
			default: 				*ureg8[Dest] = (unsigned char)temp16; break;
		}
		cc[C] = (temp16 & 0x100) >> 8;
		cc[V] = cc[C] ^ ((dest8 ^ *ureg8[Dest] ^ source8) >> 7);
		cc[N] = *ureg8[Dest] >> 7;
		cc[Z] = ZTEST(*ureg8[Dest]);
	}
	else // 16 bit dest
	{
		dest16 = *xfreg16[Dest];

		if (Source < 8) // 16 bit source
		{
			source16 = *xfreg16[Source];
		}
		else // 8 bit source - promote to 16 bit
		{
			Source &= 7;
			switch (Source)
			{
			case 0: case 1: source16 = D_REG; break; // A & B Reg
			case 2:	        source16 = (unsigned short)getcc(); break; // CC
			case 3:	        source16 = (unsigned short)dp.Reg; break; // DP
			case 4: case 5: source16 = 0; break; // Zero Reg
			case 6: case 7: source16 = W_REG; break; // E & F Reg
			}
		}

		temp32 = dest16 - source16 - cc[C];
		cc[C] = (temp32 & 0x10000) >> 16;
		cc[V] = !!((dest16 ^ source16 ^ temp32 ^ (temp32 >> 1)) & 0x8000);
		*xfreg16[Dest] = (unsigned short)temp32;
		cc[N] = (temp32 & 0x8000) >> 15;
		cc[Z] = ZTEST(temp32);
	}
	CycleCounter += 4;
}

void Andr(void)
{ //1034 6309 - WallyZ 2019
	unsigned char dest8, source8;
	unsigned short dest16, source16;
	temp8 = MemRead8(PC_REG++);
	Source = temp8 >> 4;
	Dest = temp8 & 15;

	if (Dest > 7) // 8 bit dest
	{
		Dest &= 7;
		if (Dest == 2) dest8 = getcc();
		else dest8 = *ureg8[Dest];

		if (Source > 7) // 8 bit source
		{
			Source &= 7;
			if (Source == 2) source8 = getcc();
			else source8 = *ureg8[Source];
		}
		else // 16 bit source - demote to 8 bit
		{
			Source &= 7;
			source8 = (unsigned char)*xfreg16[Source];
		}

		temp8 = dest8 & source8;
		switch (Dest)
		{
			case 2: 				setcc((unsigned char)temp8); break;
			case 4: case 5: break; // never assign to zero reg
			default: 				*ureg8[Dest] = (unsigned char)temp8; break;
		}
		cc[N] = temp8 >> 7;
		cc[Z] = ZTEST(temp8);
	}
	else // 16 bit dest
	{
		dest16 = *xfreg16[Dest];

		if (Source < 8) // 16 bit source
		{
			source16 = *xfreg16[Source];
		}
		else // 8 bit source - promote to 16 bit
		{
			Source &= 7;
			switch (Source)
			{
			case 0: case 1: source16 = D_REG; break; // A & B Reg
			case 2:	        source16 = (unsigned short)getcc(); break; // CC
			case 3:	        source16 = (unsigned short)dp.Reg; break; // DP
			case 4: case 5: source16 = 0; break; // Zero Reg
			case 6: case 7: source16 = W_REG; break; // E & F Reg
			}
		}

		temp16 = dest16 & source16;
		*xfreg16[Dest] = temp16;
		cc[N] = temp16 >> 15;
		cc[Z] = ZTEST(temp16);
	}
	cc[V] = 0;
	CycleCounter += 4;
}

void Orr(void)
{ //1035 6309 - WallyZ 2019
	unsigned char dest8, source8;
	unsigned short dest16, source16;
	temp8 = MemRead8(PC_REG++);
	Source = temp8 >> 4;
	Dest = temp8 & 15;

	if (Dest > 7) // 8 bit dest
	{
		Dest &= 7;
		if (Dest == 2) dest8 = getcc();
		else dest8 = *ureg8[Dest];

		if (Source > 7) // 8 bit source
		{
			Source &= 7;
			if (Source == 2) source8 = getcc();
			else source8 = *ureg8[Source];
		}
		else // 16 bit source - demote to 8 bit
		{
			Source &= 7;
			source8 = (unsigned char)*xfreg16[Source];
		}

		temp8 = dest8 | source8;
		switch (Dest)
		{
			case 2: 				setcc((unsigned char)temp8); break;
			case 4: case 5: break; // never assign to zero reg
			default: 				*ureg8[Dest] = (unsigned char)temp8; break;
		}
		cc[N] = temp8 >> 7;
		cc[Z] = ZTEST(temp8);
	}
	else // 16 bit dest
	{
		dest16 = *xfreg16[Dest];

		if (Source < 8) // 16 bit source
		{
			source16 = *xfreg16[Source];
		}
		else // 8 bit source - promote to 16 bit
		{
			Source &= 7;
			switch (Source)
			{
			case 0: case 1: source16 = D_REG; break; // A & B Reg
			case 2:	        source16 = (unsigned short)getcc(); break; // CC
			case 3:	        source16 = (unsigned short)dp.Reg; break; // DP
			case 4: case 5: source16 = 0; break; // Zero Reg
			case 6: case 7: source16 = W_REG; break; // E & F Reg
			}
		}

		temp16 = dest16 | source16;
		*xfreg16[Dest] = temp16;
		cc[N] = temp16 >> 15;
		cc[Z] = ZTEST(temp16);
	}
	cc[V] = 0;
	CycleCounter += 4;
}

void Eorr(void)
{ //1036 6309 - WallyZ 2019
	unsigned char dest8, source8;
	unsigned short dest16, source16;
	temp8 = MemRead8(PC_REG++);
	Source = temp8 >> 4;
	Dest = temp8 & 15;

	if (Dest > 7) // 8 bit dest
	{
		Dest &= 7;
		if (Dest == 2) dest8 = getcc();
		else dest8 = *ureg8[Dest];

		if (Source > 7) // 8 bit source
		{
			Source &= 7;
			if (Source == 2) source8 = getcc();
			else source8 = *ureg8[Source];
		}
		else // 16 bit source - demote to 8 bit
		{
			Source &= 7;
			source8 = (unsigned char)*xfreg16[Source];
		}

		temp8 = dest8 ^ source8;
		switch (Dest)
		{
			case 2: 				setcc((unsigned char)temp8); break;
			case 4: case 5: break; // never assign to zero reg
			default: 				*ureg8[Dest] = (unsigned char)temp8; break;
		}
		cc[N] = temp8 >> 7;
		cc[Z] = ZTEST(temp8);
	}
	else // 16 bit dest
	{
		dest16 = *xfreg16[Dest];

		if (Source < 8) // 16 bit source
		{
			source16 = *xfreg16[Source];
		}
		else // 8 bit source - promote to 16 bit
		{
			Source &= 7;
			switch (Source)
			{
			case 0: case 1: source16 = D_REG; break; // A & B Reg
			case 2:	        source16 = (unsigned short)getcc(); break; // CC
			case 3:	        source16 = (unsigned short)dp.Reg; break; // DP
			case 4: case 5: source16 = 0; break; // Zero Reg
			case 6: case 7: source16 = W_REG; break; // E & F Reg
			}
		}

		temp16 = dest16 ^ source16;
		*xfreg16[Dest] = temp16;
		cc[N] = temp16 >> 15;
		cc[Z] = ZTEST(temp16);
	}
	cc[V] = 0;
	CycleCounter += 4;
}

void Cmpr(void)
{ //1037 6309 - WallyZ 2019
	unsigned char dest8, source8;
	unsigned short dest16, source16;
	temp8 = MemRead8(PC_REG++);
	Source = temp8 >> 4;
	Dest = temp8 & 15;

	if (Dest > 7) // 8 bit dest
	{
		Dest &= 7;
		if (Dest == 2) dest8 = getcc();
		else dest8 = *ureg8[Dest];

		if (Source > 7) // 8 bit source
		{
			Source &= 7;
			if (Source == 2) source8 = getcc();
			else source8 = *ureg8[Source];
		}
		else // 16 bit source - demote to 8 bit
		{
			Source &= 7;
			source8 = (unsigned char)*xfreg16[Source];
		}

		temp16 = dest8 - source8;
		temp8 = (unsigned char)temp16;
		cc[C] = (temp16 & 0x100) >> 8;
		cc[V] = cc[C] ^ ((dest8 ^ temp8 ^ source8) >> 7);
		cc[N] = temp8 >> 7;
		cc[Z] = ZTEST(temp8);
	}
	else // 16 bit dest
	{
		dest16 = *xfreg16[Dest];

		if (Source < 8) // 16 bit source
		{
			source16 = *xfreg16[Source];
		}
		else // 8 bit source - promote to 16 bit
		{
			Source &= 7;
			switch (Source)
			{
			case 0: case 1: source16 = D_REG; break; // A & B Reg
			case 2:	        source16 = (unsigned short)getcc(); break; // CC
			case 3:	        source16 = (unsigned short)dp.Reg; break; // DP
			case 4: case 5: source16 = 0; break; // Zero Reg
			case 6: case 7: source16 = W_REG; break; // E & F Reg
			}
		}

		temp32 = dest16 - source16;
		cc[C] = (temp32 & 0x10000) >> 16;
		cc[V] = !!((dest16 ^ source16 ^ temp32 ^ (temp32 >> 1)) & 0x8000);
		cc[N] = (temp32 & 0x8000) >> 15;
		cc[Z] = ZTEST(temp32);
	}
	CycleCounter += 4;
}

void Pshsw(void)
{ //1038 DONE 6309
	MemWrite8((F_REG),--S_REG);
	MemWrite8((E_REG),--S_REG);
	CycleCounter+=6;
}

void Pulsw(void)
{	//1039 6309 Untested wcreate
	E_REG=MemRead8( S_REG++);
	F_REG=MemRead8( S_REG++);
	CycleCounter+=6;
}

void Pshuw(void)
{ //103A 6309 Untested
	MemWrite8((F_REG),--U_REG);
	MemWrite8((E_REG),--U_REG);
	CycleCounter+=6;
}

void Puluw(void)
{ //103B 6309 Untested
	E_REG=MemRead8( U_REG++);
	F_REG=MemRead8( U_REG++);
	CycleCounter+=6;
}

void Swi2_I(void)
{ //103F
	cc[E]=1;
	MemWrite8( pc.B.lsb,--S_REG);
	MemWrite8( pc.B.msb,--S_REG);
	MemWrite8( u.B.lsb,--S_REG);
	MemWrite8( u.B.msb,--S_REG);
	MemWrite8( y.B.lsb,--S_REG);
	MemWrite8( y.B.msb,--S_REG);
	MemWrite8( x.B.lsb,--S_REG);
	MemWrite8( x.B.msb,--S_REG);
	MemWrite8( dp.B.msb,--S_REG);
	if (md[NATIVE6309])
	{
		MemWrite8((F_REG),--S_REG);
		MemWrite8((E_REG),--S_REG);
		CycleCounter+=2;
	}
	MemWrite8(B_REG,--S_REG);
	MemWrite8(A_REG,--S_REG);
	MemWrite8(getcc(),--S_REG);
	PC_REG=MemRead16(VSWI2);
	CycleCounter+=20;
}

void Negd_I(void)
{ //1040 Phase 5 6309
	D_REG = 0-D_REG;
	cc[C] = temp16>0;
	cc[V] = D_REG==0x8000;
	cc[N] = NTEST16(D_REG);
	cc[Z] = ZTEST(D_REG);
	CycleCounter+=NatEmuCycles32;
}

void Comd_I(void)
{ //1043 6309
	D_REG = 0xFFFF- D_REG;
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	cc[C] = 1;
	cc[V] = 0;
	CycleCounter+=NatEmuCycles32;
}

void Lsrd_I(void)
{ //1044 6309
	cc[C] = D_REG & 1;
	D_REG = D_REG>>1;
	cc[Z] = ZTEST(D_REG);
	cc[N] = 0;
	CycleCounter+=NatEmuCycles32;
}

void Rord_I(void)
{ //1046 6309 Untested
	postword=cc[C]<<15;
	cc[C] = D_REG & 1;
	D_REG = (D_REG>>1) | postword;
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	CycleCounter+=NatEmuCycles32;
}

void Asrd_I(void)
{ //1047 6309 Untested TESTED NITRO MULTIVUE
	cc[C] = D_REG & 1;
	D_REG = (D_REG & 0x8000) | (D_REG >> 1);
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	CycleCounter+=NatEmuCycles32;
}

void Asld_I(void)
{ //1048 6309
	cc[C] = D_REG >>15;
	cc[V] =  cc[C] ^((D_REG & 0x4000)>>14);
	D_REG = D_REG<<1;
	cc[N] = NTEST16(D_REG);
	cc[Z] = ZTEST(D_REG);
	CycleCounter+=NatEmuCycles32;
}

void Rold_I(void)
{ //1049 6309 Untested
	postword=cc[C];
	cc[C] = D_REG >>15;
	cc[V] = cc[C] ^ ((D_REG & 0x4000)>>14);
	D_REG= (D_REG<<1) | postword;
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	CycleCounter+=NatEmuCycles32;
}

void Decd_I(void)
{ //104A 6309
	D_REG--;
	cc[Z] = ZTEST(D_REG);
	cc[V] = D_REG==0x7FFF;
	cc[N] = NTEST16(D_REG);
	CycleCounter+=NatEmuCycles32;
}

void Incd_I(void)
{ //104C 6309
	D_REG++;
	cc[Z] = ZTEST(D_REG);
	cc[V] = D_REG==0x8000;
	cc[N] = NTEST16(D_REG);
	CycleCounter+=NatEmuCycles32;
}

void Tstd_I(void)
{ //104D 6309
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles32;
}

void Clrd_I(void)
{ //104F 6309
	D_REG= 0;
	cc[C] = 0;
	cc[V] = 0;
	cc[N] = 0;
	cc[Z] = 1;
	CycleCounter+=NatEmuCycles32;
}

void Comw_I(void)
{ //1053 6309 Untested
	W_REG= 0xFFFF- W_REG;
	cc[Z] = ZTEST(W_REG);
	cc[N] = NTEST16(W_REG);
	cc[C] = 1;
	cc[V] = 0;
	CycleCounter+=NatEmuCycles32;
}

void Lsrw_I(void)
{ //1054 6309 Untested
	cc[C] = W_REG & 1;
	W_REG= W_REG>>1;
	cc[Z] = ZTEST(W_REG);
	cc[N] = 0;
	CycleCounter+=NatEmuCycles32;
}

void Rorw_I(void)
{ //1056 6309 Untested
	postword=cc[C]<<15;
	cc[C] = W_REG & 1;
	W_REG= (W_REG>>1) | postword;
	cc[Z] = ZTEST(W_REG);
	cc[N] = NTEST16(W_REG);
	CycleCounter+=NatEmuCycles32;
}

void Rolw_I(void)
{ //1059 6309
	postword=cc[C];
	cc[C] = W_REG >>15;
	cc[V] = cc[C] ^ ((W_REG & 0x4000)>>14);
	W_REG= ( W_REG<<1) | postword;
	cc[Z] = ZTEST(W_REG);
	cc[N] = NTEST16(W_REG);
	CycleCounter+=NatEmuCycles32;
}

void Decw_I(void)
{ //105A 6309
	W_REG--;
	cc[Z] = ZTEST(W_REG);
	cc[V] = W_REG==0x7FFF;
	cc[N] = NTEST16(W_REG);
	CycleCounter+=NatEmuCycles32;
}

void Incw_I(void)
{ //105C 6309
	W_REG++;
	cc[Z] = ZTEST(W_REG);
	cc[V] = W_REG==0x8000;
	cc[N] = NTEST16(W_REG);
	CycleCounter+=NatEmuCycles32;
}

void Tstw_I(void)
{ //105D Untested 6309 wcreate
	cc[Z] = ZTEST(W_REG);
	cc[N] = NTEST16(W_REG);
	cc[V] = 0;	
	CycleCounter+=NatEmuCycles32;
}

void Clrw_I(void)
{ //105F 6309
	W_REG = 0;
	cc[C] = 0;
	cc[V] = 0;
	cc[N] = 0;
	cc[Z] = 1;
	CycleCounter+=NatEmuCycles32;
}

void Subw_M(void)
{ //1080 6309 CHECK
	postword=IMMADDRESS(PC_REG);
	temp16= W_REG-postword;
	cc[C] = temp16 > W_REG;
	cc[V] = OVERFLOW16(cc[C],temp16,W_REG,postword);
	cc[N] = NTEST16(temp16);
	cc[Z] = ZTEST(temp16);
	W_REG= temp16;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}

void Cmpw_M(void)
{ //1081 6309 CHECK
	postword=IMMADDRESS(PC_REG);
	temp16= W_REG-postword;
	cc[C] = temp16 > W_REG;
	cc[V] = OVERFLOW16(cc[C],temp16,W_REG,postword);
	cc[N] = NTEST16(temp16);
	cc[Z] = ZTEST(temp16);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}

void Sbcd_M(void)
{ //1082 6309
	postword=IMMADDRESS(PC_REG);
	temp32=D_REG-postword-cc[C];
	cc[C] = (temp32 & 0x10000)>>16;
	cc[V] = OVERFLOW16(cc[C],temp32,D_REG,postword);
	D_REG= temp32;
	cc[N] = NTEST16(D_REG);
	cc[Z] = ZTEST(D_REG);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}

void Cmpd_M(void)
{ //1083
	postword=IMMADDRESS(PC_REG);
	temp16 = D_REG-postword;
	cc[C] = temp16 > D_REG;
	cc[V] = OVERFLOW16(cc[C],postword,temp16,D_REG);
	cc[N] = NTEST16(temp16);
	cc[Z] = ZTEST(temp16);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}

void Andd_M(void)
{ //1084 6309
	D_REG &= IMMADDRESS(PC_REG);
	cc[N] = NTEST16(D_REG);
	cc[Z] = ZTEST(D_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}

void Bitd_M(void)
{ //1085 6309 Untested
	temp16= D_REG & IMMADDRESS(PC_REG);
	cc[N] = NTEST16(temp16);
	cc[Z] = ZTEST(temp16);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}

void Ldw_M(void)
{ //1086 6309
	W_REG=IMMADDRESS(PC_REG);
	cc[Z] = ZTEST(W_REG);
	cc[N] = NTEST16(W_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}

void Eord_M(void)
{ //1088 6309 Untested
	D_REG ^= IMMADDRESS(PC_REG);
	cc[N] = NTEST16(D_REG);
	cc[Z] = ZTEST(D_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}

void Adcd_M(void)
{ //1089 6309
	postword=IMMADDRESS(PC_REG);
	temp32= D_REG + postword + cc[C];
	cc[C] = (temp32 & 0x10000)>>16;
	cc[V] = OVERFLOW16(cc[C],postword,temp32,D_REG);
	cc[H] = ((D_REG ^ temp32 ^ postword) & 0x100)>>8;
	D_REG = temp32;
	cc[N] = NTEST16(D_REG);
	cc[Z] = ZTEST(D_REG);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}

void Ord_M(void)
{ //108A 6309 Untested
	D_REG |= IMMADDRESS(PC_REG);
	cc[N] = NTEST16(D_REG);
	cc[Z] = ZTEST(D_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}

void Addw_M(void)
{ //108B Phase 5 6309
	temp16=IMMADDRESS(PC_REG);
	temp32= W_REG+ temp16;
	cc[C] = (temp32 & 0x10000)>>16;
	cc[V] = OVERFLOW16(cc[C],temp32,temp16,W_REG);
	W_REG = temp32;
	cc[Z] = ZTEST(W_REG);
	cc[N] = NTEST16(W_REG);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}

void Cmpy_M(void)
{ //108C
	postword=IMMADDRESS(PC_REG);
	temp16 = Y_REG-postword;
	cc[C] = temp16 > Y_REG;
	cc[V] = OVERFLOW16(cc[C],postword,temp16,Y_REG);
	cc[N] = NTEST16(temp16);
	cc[Z] = ZTEST(temp16);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}
	
void Ldy_M(void)
{ //108E
	Y_REG = IMMADDRESS(PC_REG);
	cc[Z] = ZTEST(Y_REG);
	cc[N] = NTEST16(Y_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}

void Subw_D(void)
{ //1090 Untested 6309
	temp16= MemRead16(DPADDRESS(PC_REG++));
	temp32= W_REG-temp16;
	cc[C] = (temp32 & 0x10000)>>16;
	cc[V] = OVERFLOW16(cc[C],temp32,temp16,W_REG);
	W_REG = temp32;
	cc[Z] = ZTEST(W_REG);
	cc[N] = NTEST16(W_REG);
	CycleCounter+=NatEmuCycles75;
}

void Cmpw_D(void)
{ //1091 6309 Untested
	postword=MemRead16(DPADDRESS(PC_REG++));
	temp16= W_REG - postword ;
	cc[C] = temp16 > W_REG;
	cc[V] = OVERFLOW16(cc[C],postword,temp16,W_REG); 
	cc[N] = NTEST16(temp16);
	cc[Z] = ZTEST(temp16);
	CycleCounter+=NatEmuCycles75;
}

void Sbcd_D(void)
{ //1092 6309
	postword= MemRead16(DPADDRESS(PC_REG++));
	temp32=D_REG-postword-cc[C];
	cc[C] = (temp32 & 0x10000)>>16;
	cc[V] = OVERFLOW16(cc[C],temp32,D_REG,postword);
	D_REG= temp32;
	cc[N] = NTEST16(D_REG);
	cc[Z] = ZTEST(D_REG);
	CycleCounter+=NatEmuCycles75;
}

void Cmpd_D(void)
{ //1093
	postword=MemRead16(DPADDRESS(PC_REG++));
	temp16= D_REG - postword ;
	cc[C] = temp16 > D_REG;
	cc[V] = OVERFLOW16(cc[C],postword,temp16,D_REG); 
	cc[N] = NTEST16(temp16);
	cc[Z] = ZTEST(temp16);
	CycleCounter+=NatEmuCycles75;
}

void Andd_D(void)
{ //1094 6309 Untested
	postword=MemRead16(DPADDRESS(PC_REG++));
	D_REG&=postword;
	cc[N] = NTEST16(D_REG);
	cc[Z] = ZTEST(D_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles75;
}

void Bitd_D(void)
{ //1095 6309 Untested
	temp16= D_REG & MemRead16(DPADDRESS(PC_REG++));
	cc[N] = NTEST16(temp16);
	cc[Z] = ZTEST(temp16);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles75;
}

void Ldw_D(void)
{ //1096 6309
	W_REG = MemRead16(DPADDRESS(PC_REG++));
	cc[Z] = ZTEST(W_REG);
	cc[N] = NTEST16(W_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles65;
}

void Stw_D(void)
{ //1097 6309
	MemWrite16(W_REG,DPADDRESS(PC_REG++));
	cc[Z] = ZTEST(W_REG);
	cc[N] = NTEST16(W_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles65;
}

void Eord_D(void)
{ //1098 6309 Untested
	D_REG^=MemRead16(DPADDRESS(PC_REG++));
	cc[N] = NTEST16(D_REG);
	cc[Z] = ZTEST(D_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles75;
}

void Adcd_D(void)
{ //1099 6309
	postword=MemRead16(DPADDRESS(PC_REG++));
	temp32= D_REG + postword + cc[C];
	cc[C] = (temp32 & 0x10000)>>16;
	cc[V] = OVERFLOW16(cc[C],postword,temp32,D_REG);
	cc[H] = ((D_REG ^ temp32 ^ postword) & 0x100)>>8;
	D_REG = temp32;
	cc[N] = NTEST16(D_REG);
	cc[Z] = ZTEST(D_REG);
	CycleCounter+=NatEmuCycles75;
}

void Ord_D(void)
{ //109A 6309 Untested
	D_REG|=MemRead16(DPADDRESS(PC_REG++));
	cc[N] = NTEST16(D_REG);
	cc[Z] = ZTEST(D_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles75;
}

void Addw_D(void)
{ //109B 6309
	temp16=MemRead16( DPADDRESS(PC_REG++));
	temp32= W_REG+ temp16;
	cc[C] =(temp32 & 0x10000)>>16;
	cc[V] = OVERFLOW16(cc[C],temp32,temp16,W_REG);
	W_REG = temp32;
	cc[Z] = ZTEST(W_REG);
	cc[N] = NTEST16(W_REG);
	CycleCounter+=NatEmuCycles75;
}

void Cmpy_D(void)
{	//109C
	postword=MemRead16(DPADDRESS(PC_REG++));
	temp16= Y_REG - postword ;
	cc[C] = temp16 > Y_REG;
	cc[V] = OVERFLOW16(cc[C],postword,temp16,Y_REG);
	cc[N] = NTEST16(temp16);
	cc[Z] = ZTEST(temp16);
	CycleCounter+=NatEmuCycles75;
}

void Ldy_D(void)
{ //109E
	Y_REG=MemRead16(DPADDRESS(PC_REG++));
	cc[Z] = ZTEST(Y_REG);	
	cc[N] = NTEST16(Y_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles65;
}
	
void Sty_D(void)
{ //109F
	MemWrite16(Y_REG,DPADDRESS(PC_REG++));
	cc[Z] = ZTEST(Y_REG);
	cc[N] = NTEST16(Y_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles65;
}

void Subw_X(void)
{ //10A0 6309 MODDED
	temp16=MemRead16(INDADDRESS(PC_REG++));
	temp32=W_REG-temp16;
	cc[C] = (temp32 & 0x10000)>>16;
	cc[V] = OVERFLOW16(cc[C],temp32,temp16,W_REG);
	W_REG= temp32;
	cc[Z] = ZTEST(W_REG);
	cc[N] = NTEST16(W_REG);
	CycleCounter+=NatEmuCycles76;
}

void Cmpw_X(void)
{ //10A1 6309
	postword=MemRead16(INDADDRESS(PC_REG++));
	temp16= W_REG - postword ;
	cc[C] = temp16 > W_REG;
	cc[V] = OVERFLOW16(cc[C],postword,temp16,W_REG);
	cc[N] = NTEST16(temp16);
	cc[Z] = ZTEST(temp16);
	CycleCounter+=NatEmuCycles76;
}

void Sbcd_X(void)
{ //10A2 6309
	postword=MemRead16(INDADDRESS(PC_REG++));
	temp32=D_REG-postword-cc[C];
	cc[C] = (temp32 & 0x10000)>>16;
	cc[V] = OVERFLOW16(cc[C],postword,temp32,D_REG);
	D_REG= temp32;
	cc[N] = NTEST16(D_REG);
	cc[Z] = ZTEST(D_REG);
	CycleCounter+=NatEmuCycles76;
}

void Cmpd_X(void)
{ //10A3
	postword=MemRead16(INDADDRESS(PC_REG++));
	temp16= D_REG - postword ;
	cc[C] = temp16 > D_REG;
	cc[V] = OVERFLOW16(cc[C],postword,temp16,D_REG);
	cc[N] = NTEST16(temp16);
	cc[Z] = ZTEST(temp16);
	CycleCounter+=NatEmuCycles76;
}

void Andd_X(void)
{ //10A4 6309
	D_REG&=MemRead16(INDADDRESS(PC_REG++));
	cc[N] = NTEST16(D_REG);
	cc[Z] = ZTEST(D_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles76;
}

void Bitd_X(void)
{ //10A5 6309 Untested
	temp16= D_REG & MemRead16(INDADDRESS(PC_REG++));
	cc[N] = NTEST16(temp16);
	cc[Z] = ZTEST(temp16);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles76;
}

void Ldw_X(void)
{ //10A6 6309
	W_REG=MemRead16(INDADDRESS(PC_REG++));
	cc[Z] = ZTEST(W_REG);
	cc[N] = NTEST16(W_REG);
	cc[V] = 0;
	CycleCounter+=6;
}

void Stw_X(void)
{ //10A7 6309
	MemWrite16(W_REG,INDADDRESS(PC_REG++));
	cc[Z] = ZTEST(W_REG);
	cc[N] = NTEST16(W_REG);
	cc[V] = 0;
	CycleCounter+=6;
}

void Eord_X(void)
{ //10A8 6309 Untested TESTED NITRO 
	D_REG ^= MemRead16(INDADDRESS(PC_REG++));
	cc[N] = NTEST16(D_REG);
	cc[Z] = ZTEST(D_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles76;
}

void Adcd_X(void)
{ //10A9 6309 untested
	postword=MemRead16(INDADDRESS(PC_REG++));
	temp32 = D_REG + postword + cc[C];
	cc[C] = (temp32 & 0x10000)>>16;
	cc[V] = OVERFLOW16(cc[C],postword,temp32,D_REG);
	cc[H] = (((D_REG ^ temp32 ^ postword) & 0x100)>>8);
	D_REG = temp32;
	cc[N] = NTEST16(D_REG);
	cc[Z] = ZTEST(D_REG);
	CycleCounter+=NatEmuCycles76;
}

void Ord_X(void)
{ //10AA 6309 Untested wcreate
	D_REG |= MemRead16(INDADDRESS(PC_REG++));
	cc[N] = NTEST16(D_REG);
	cc[Z] = ZTEST(D_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles76;
}

void Addw_X(void)
{ //10AB 6309 Untested TESTED NITRO CHECK no Half carry?
	temp16=MemRead16(INDADDRESS(PC_REG++));
	temp32= W_REG+ temp16;
	cc[C] =(temp32 & 0x10000)>>16;
	cc[V] = OVERFLOW16(cc[C],temp32,temp16,W_REG);
	W_REG= temp32;
	cc[Z] = ZTEST(W_REG);
	cc[N] = NTEST16(W_REG);
	CycleCounter+=NatEmuCycles76;
}

void Cmpy_X(void)
{ //10AC
	postword=MemRead16(INDADDRESS(PC_REG++));
	temp16= Y_REG - postword ;
	cc[C] = temp16 > Y_REG;
	cc[V] = OVERFLOW16(cc[C],postword,temp16,Y_REG);
	cc[N] = NTEST16(temp16);
	cc[Z] = ZTEST(temp16);
	CycleCounter+=NatEmuCycles76;
}

void Ldy_X(void)
{ //10AE
	Y_REG=MemRead16(INDADDRESS(PC_REG++));
	cc[Z] = ZTEST(Y_REG);
	cc[N] = NTEST16(Y_REG);
	cc[V] = 0;
	CycleCounter+=6;
	}

void Sty_X(void)
{ //10AF
	MemWrite16(Y_REG,INDADDRESS(PC_REG++));
	cc[Z] = ZTEST(Y_REG);
	cc[N] = NTEST16(Y_REG);
	cc[V] = 0;
	CycleCounter+=6;
}

void Subw_E(void)
{ //10B0 6309 Untested
	temp16=MemRead16(IMMADDRESS(PC_REG));
	temp32=W_REG-temp16;
	cc[C] = (temp32 & 0x10000)>>16;
	cc[V] = OVERFLOW16(cc[C],temp32,temp16,W_REG);
	W_REG= temp32;
	cc[Z] = ZTEST(W_REG);
	cc[N] = NTEST16(W_REG);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles86;
}

void Cmpw_E(void)
{ //10B1 6309 Untested
	postword=MemRead16(IMMADDRESS(PC_REG));
	temp16 = W_REG-postword;
	cc[C] = temp16 > W_REG;
	cc[V] = OVERFLOW16(cc[C],postword,temp16,W_REG);
	cc[N] = NTEST16(temp16);
	cc[Z] = ZTEST(temp16);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles86;
}

void Sbcd_E(void)
{ //10B2 6309 Untested
	temp16=MemRead16(IMMADDRESS(PC_REG));
	temp32=D_REG-temp16-cc[C];
	cc[C] = (temp32 & 0x10000)>>16;
	cc[V] = OVERFLOW16(cc[C],temp32,temp16,D_REG);
	D_REG= temp32;
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles86;
}

void Cmpd_E(void)
{ //10B3
	postword=MemRead16(IMMADDRESS(PC_REG));
	temp16 = D_REG-postword;
	cc[C] = temp16 > D_REG;
	cc[V] = OVERFLOW16(cc[C],postword,temp16,D_REG);
	cc[N] = NTEST16(temp16);
	cc[Z] = ZTEST(temp16);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles86;
}

void Andd_E(void)
{ //10B4 6309 Untested
	D_REG &= MemRead16(IMMADDRESS(PC_REG));
	cc[N] = NTEST16(D_REG);
	cc[Z] = ZTEST(D_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles86;
}

void Bitd_E(void)
{ //10B5 6309 Untested CHECK NITRO
	temp16= D_REG & MemRead16(IMMADDRESS(PC_REG));
	cc[N] = NTEST16(temp16);
	cc[Z] = ZTEST(temp16);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles86;
}

void Ldw_E(void)
{ //10B6 6309
	W_REG=MemRead16(IMMADDRESS(PC_REG));
	cc[Z] = ZTEST(W_REG);
	cc[N] = NTEST16(W_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles76;
}

void Stw_E(void)
{ //10B7 6309
	MemWrite16(W_REG,IMMADDRESS(PC_REG));
	cc[Z] = ZTEST(W_REG);
	cc[N] = NTEST16(W_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles76;
}

void Eord_E(void)
{ //10B8 6309 Untested
	D_REG ^= MemRead16(IMMADDRESS(PC_REG));
	cc[N] = NTEST16(D_REG);
	cc[Z] = ZTEST(D_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles86;
}

void Adcd_E(void)
{ //10B9 6309 untested
	postword = MemRead16(IMMADDRESS(PC_REG));
	temp32 = D_REG + postword + cc[C];
	cc[C] = (temp32 & 0x10000)>>16;
	cc[V] = OVERFLOW16(cc[C],postword,temp32,D_REG);
	cc[H] = (((D_REG ^ temp32 ^ postword) & 0x100)>>8);
	D_REG = temp32;
	cc[N] = NTEST16(D_REG);
	cc[Z] = ZTEST(D_REG);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles86;
}

void Ord_E(void)
{ //10BA 6309 Untested
	D_REG |= MemRead16(IMMADDRESS(PC_REG));
	cc[N] = NTEST16(D_REG);
	cc[Z] = ZTEST(D_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles86;
}

void Addw_E(void)
{ //10BB 6309 Untested
	temp16=MemRead16(IMMADDRESS(PC_REG));
	temp32= W_REG+ temp16;
	cc[C] =(temp32 & 0x10000)>>16;
	cc[V] = OVERFLOW16(cc[C],temp32,temp16,W_REG);
	W_REG= temp32;
	cc[Z] = ZTEST(W_REG);
	cc[N] = NTEST16(W_REG);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles86;
}

void Cmpy_E(void)
{ //10BC
	postword=MemRead16(IMMADDRESS(PC_REG));
	temp16 = Y_REG-postword;
	cc[C] = temp16 > Y_REG;
	cc[V] = OVERFLOW16(cc[C],postword,temp16,Y_REG);
	cc[N] = NTEST16(temp16);
	cc[Z] = ZTEST(temp16);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles86;
}

void Ldy_E(void)
{ //10BE
	Y_REG=MemRead16(IMMADDRESS(PC_REG));
	cc[Z] = ZTEST(Y_REG);
	cc[N] = NTEST16(Y_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles76;
}

void Sty_E(void)
{ //10BF
	MemWrite16(Y_REG,IMMADDRESS(PC_REG));
	cc[Z] = ZTEST(Y_REG);
	cc[N] = NTEST16(Y_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles76;
}

void Lds_I(void)
{  //10CE
	S_REG=IMMADDRESS(PC_REG);
	cc[Z] = ZTEST(S_REG);
	cc[N] = NTEST16(S_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=4;
}

void Ldq_D(void)
{ //10DC 6309
	Q_REG=MemRead32(DPADDRESS(PC_REG++));
	cc[Z] = ZTEST(Q_REG);
	cc[N] = NTEST32(Q_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles87;
}

void Stq_D(void)
{ //10DD 6309
	MemWrite32(Q_REG,DPADDRESS(PC_REG++));
	cc[Z] = ZTEST(Q_REG);
	cc[N] = NTEST32(Q_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles87;
}

void Lds_D(void)
{ //10DE
	S_REG=MemRead16(DPADDRESS(PC_REG++));
	cc[Z] = ZTEST(S_REG);
	cc[N] = NTEST16(S_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles65;
}

void Sts_D(void)
{ //10DF 6309
	MemWrite16(S_REG,DPADDRESS(PC_REG++));
	cc[Z] = ZTEST(S_REG);
	cc[N] = NTEST16(S_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles65;
}

void Ldq_X(void)
{ //10EC 6309
	Q_REG=MemRead32(INDADDRESS(PC_REG++));
	cc[Z] = ZTEST(Q_REG);
	cc[N] = NTEST32(Q_REG);
	cc[V] = 0;
	CycleCounter+=8;
}

void Stq_X(void)
{ //10ED 6309 DONE
	MemWrite32(Q_REG,INDADDRESS(PC_REG++));
	cc[Z] = ZTEST(Q_REG);
	cc[N] = NTEST32(Q_REG);
	cc[V] = 0;
	CycleCounter+=8;
}


void Lds_X(void)
{ //10EE
	S_REG=MemRead16(INDADDRESS(PC_REG++));
	cc[Z] = ZTEST(S_REG);
	cc[N] = NTEST16(S_REG);
	cc[V] = 0;
	CycleCounter+=6;
}

void Sts_X(void)
{ //10EF 6309
	MemWrite16(S_REG,INDADDRESS(PC_REG++));
	cc[Z] = ZTEST(S_REG);
	cc[N] = NTEST16(S_REG);
	cc[V] = 0;
	CycleCounter+=6;
}

void Ldq_E(void)
{ //10FC 6309
	Q_REG=MemRead32(IMMADDRESS(PC_REG));
	cc[Z] = ZTEST(Q_REG);
	cc[N] = NTEST32(Q_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles98;
}

void Stq_E(void)
{ //10FD 6309
	MemWrite32(Q_REG,IMMADDRESS(PC_REG));
	cc[Z] = ZTEST(Q_REG);
	cc[N] = NTEST32(Q_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles98;
}

void Lds_E(void)
{ //10FE
	S_REG=MemRead16(IMMADDRESS(PC_REG));
	cc[Z] = ZTEST(S_REG);
	cc[N] = NTEST16(S_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles76;
}

void Sts_E(void)
{ //10FF 6309
	MemWrite16(S_REG,IMMADDRESS(PC_REG));
	cc[Z] = ZTEST(S_REG);
	cc[N] = NTEST16(S_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles76;
}

void Band(void)
{ //1130 6309 untested
	postbyte = MemRead8(PC_REG++);
	temp8 = MemRead8(DPADDRESS(PC_REG++));
	Source = (postbyte >> 3) & 7;
	Dest = (postbyte) & 7;
	postbyte >>= 6;
	
	if (postbyte == 3)
	{
		InvalidInsHandler();
		return;
	}

	if ((temp8 & (1 << Source)) == 0)
	{
    switch (postbyte)
    {
    case 0 : // A Reg
    case 1 : // B Reg
      *ureg8[postbyte] &= ~(1 << Dest);
      break;
    case 2 : // CC Reg
      setcc(getcc() & ~(1 << Dest));
      break;
    }
	}
	// Else nothing changes
	CycleCounter+=NatEmuCycles76;
}

void Biand(void)
{ //1131 6309
	postbyte = MemRead8(PC_REG++);
	temp8 = MemRead8(DPADDRESS(PC_REG++));
	Source = (postbyte >> 3) & 7;
	Dest = (postbyte) & 7;
	postbyte >>= 6;

	if (postbyte == 3)
	{
		InvalidInsHandler();
		return;
	}

	if ((temp8 & (1 << Source)) != 0)
	{
    switch (postbyte)
    {
    case 0: // A Reg
    case 1: // B Reg
      *ureg8[postbyte] &= ~(1 << Dest);
      break;
    case 2: // CC Reg
      setcc(getcc() & ~(1 << Dest));
      break;
    }
  }
	// Else do nothing
	CycleCounter+=NatEmuCycles76;
}

void Bor(void)
{ //1132 6309
	postbyte = MemRead8(PC_REG++);
	temp8 = MemRead8(DPADDRESS(PC_REG++));
	Source = (postbyte >> 3) & 7;
	Dest = (postbyte) & 7;
	postbyte >>= 6;

	if (postbyte == 3)
	{
		InvalidInsHandler();
		return;
	}

	if ((temp8 & (1 << Source)) != 0)
	{
    switch (postbyte)
    {
    case 0: // A Reg
    case 1: // B Reg
      *ureg8[postbyte] |= (1 << Dest);
      break;
    case 2: // CC Reg
      setcc(getcc() | (1 << Dest));
      break;
    }
	}
	// Else do nothing
	CycleCounter+=NatEmuCycles76;
}

void Bior(void)
{ //1133 6309
	postbyte = MemRead8(PC_REG++);
	temp8 = MemRead8(DPADDRESS(PC_REG++));
	Source = (postbyte >> 3) & 7;
	Dest = (postbyte) & 7;
	postbyte >>= 6;

	if (postbyte == 3)
	{
		InvalidInsHandler();
		return;
	}

	if ((temp8 & (1 << Source)) == 0)
	{
    switch (postbyte)
    {
    case 0: // A Reg
    case 1: // B Reg
      *ureg8[postbyte] |= (1 << Dest);
      break;
    case 2: // CC Reg
      setcc(getcc() | (1 << Dest));
      break;
    }
  }
	// Else do nothing
	CycleCounter+=NatEmuCycles76;
}

void Beor(void)
{ //1134 6309
	postbyte = MemRead8(PC_REG++);
	temp8 = MemRead8(DPADDRESS(PC_REG++));
	Source = (postbyte >> 3) & 7;
	Dest = (postbyte) & 7;
	postbyte >>= 6;

	if (postbyte == 3)
	{
		InvalidInsHandler();
		return;
	}

	if ((temp8 & (1 << Source)) != 0)
	{
    switch (postbyte)
    {
    case 0: // A Reg
    case 1: // B Reg
      *ureg8[postbyte] ^= (1 << Dest);
      break;
    case 2: // CC Reg
      setcc(getcc() ^ (1 << Dest));
      break;
    }
	}
	CycleCounter+=NatEmuCycles76;
}

void Bieor(void)
{ //1135 6309
	postbyte = MemRead8(PC_REG++);
	temp8 = MemRead8(DPADDRESS(PC_REG++));
	Source = (postbyte >> 3) & 7;
	Dest = (postbyte) & 7;
	postbyte >>= 6;

	if (postbyte == 3)
	{
		InvalidInsHandler();
		return;
	}

	if ((temp8 & (1 << Source)) == 0)
	{
    switch (postbyte)
    {
    case 0: // A Reg
    case 1: // B Reg
      *ureg8[postbyte] ^= (1 << Dest);
      break;
    case 2: // CC Reg
      setcc(getcc() ^ (1 << Dest));
      break;
    }
  }
	CycleCounter+=NatEmuCycles76;
}

void Ldbt(void)
{ //1136 6309
	postbyte = MemRead8(PC_REG++);
	temp8 = MemRead8(DPADDRESS(PC_REG++));
	Source = (postbyte >> 3) & 7;
	Dest = (postbyte) & 7;
	postbyte >>= 6;

	if (postbyte == 3)
	{
		InvalidInsHandler();
		return;
	}

	if ((temp8 & (1 << Source)) != 0)
	{
    switch (postbyte)
    {
    case 0: // A Reg
    case 1: // B Reg
      *ureg8[postbyte] |= (1 << Dest);
      break;
    case 2: // CC Reg
      setcc(getcc() | (1 << Dest));
      break;
    }
  }
	else
	{
    switch (postbyte)
    {
    case 0: // A Reg
    case 1: // B Reg
      *ureg8[postbyte] &= ~(1 << Dest);
      break;
    case 2: // CC Reg
      setcc(getcc() & ~(1 << Dest));
      break;
    }
	}
	CycleCounter+=NatEmuCycles76;
}

void Stbt(void)
{ //1137 6309
	postbyte = MemRead8(PC_REG++);
	temp16 = DPADDRESS(PC_REG++);
	temp8 = MemRead8(temp16);
	Source = (postbyte >> 3) & 7;
	Dest = (postbyte) & 7;
	postbyte >>= 6;

	if (postbyte == 3)
	{
		InvalidInsHandler();
		return;
	}

  switch (postbyte)
  {
  case 0: // A Reg
  case 1: // B Reg
    postbyte = *ureg8[postbyte];
    break;
  case 2: // CC Reg
    postbyte = getcc();
    break;
  }

	if ((postbyte & (1 << Source)) != 0)
	{
		temp8 |= (1 << Dest);
	}
	else
	{
		temp8 &= ~(1 << Dest);
	}
	MemWrite8(temp8, temp16);
	CycleCounter+=NatEmuCycles87;
}

void Tfm1(void)
{ //1138 TFM R+,R+ 6309
	if (W_REG == 0)
	{
    CycleCounter += 6;
    PC_REG++;
		return;
  }

	postbyte=MemRead8(PC_REG);
	Source=postbyte>>4;
	Dest=postbyte&15;

	if (Source > 4 || Dest > 4)
	{
		InvalidInsHandler();
		return;
	}

  temp8 = MemRead8(*xfreg16[Source]);
  MemWrite8(temp8, *xfreg16[Dest]);
  (*xfreg16[Dest])++;
  (*xfreg16[Source])++;
  W_REG--;
	CycleCounter += 3;
	PC_REG -= 2;
}

void Tfm2(void)
{ //1139 TFM R-,R- Phase 3 6309
	if (W_REG == 0)
	{
		CycleCounter+=6;
		PC_REG++;
		return;
	}			

	postbyte=MemRead8(PC_REG);
	Source=postbyte>>4;
	Dest=postbyte&15;

	if (Source > 4 || Dest > 4)
	{
		InvalidInsHandler();
		return;
	}

  temp8 = MemRead8(*xfreg16[Source]);
  MemWrite8(temp8, *xfreg16[Dest]);
  (*xfreg16[Dest])--;
  (*xfreg16[Source])--;
  W_REG--;
	CycleCounter+=3;
	PC_REG-=2;
}

void Tfm3(void)
{ //113A 6309 TFM R+,R 6309
	if (W_REG == 0)
	{
		CycleCounter+=6;
		PC_REG++;
		return;
	}			

	postbyte = MemRead8(PC_REG);
	Source = postbyte >> 4;
	Dest = postbyte & 15;

	if (Source > 4 || Dest > 4)
	{
		InvalidInsHandler();
		return;
	}

  temp8 = MemRead8(*xfreg16[Source]);
  MemWrite8(temp8, *xfreg16[Dest]);
  (*xfreg16[Source])++;
  W_REG--;
	PC_REG -= 2; //Hit the same instruction on the next loop if not done copying
	CycleCounter += 3;
}

void Tfm4(void)
{ //113B TFM R,R+ 6309 
	if (W_REG == 0)
	{
		CycleCounter+=6;
		PC_REG++;
		return;
	}			

	postbyte=MemRead8(PC_REG);
	Source=postbyte>>4;
	Dest=postbyte&15;

	if (Source > 4 || Dest > 4)
	{
		InvalidInsHandler();
		return;
	}

  temp8 = MemRead8(*xfreg16[Source]);
  MemWrite8(temp8, *xfreg16[Dest]);
  (*xfreg16[Dest])++;
  W_REG--;
	PC_REG-=2; //Hit the same instruction on the next loop if not done copying
	CycleCounter+=3;
}

void Bitmd_M(void)
{ //113C  6309
	postbyte = MemRead8(PC_REG++) & 0xC0;
	temp8 = getmd() & postbyte;
	cc[Z] = ZTEST(temp8);
	if (temp8 & 0x80) md[7] = 0;
	if (temp8 & 0x40) md[6] = 0;
	CycleCounter+=4;
}

void Ldmd_M(void)
{ //113D DONE 6309
	mdbits= MemRead8(PC_REG++)&0x03;
	setmd(mdbits);
	CycleCounter+=5;
}

void Swi3_I(void)
{ //113F
	cc[E]=1;
	MemWrite8( pc.B.lsb,--S_REG);
	MemWrite8( pc.B.msb,--S_REG);
	MemWrite8( u.B.lsb,--S_REG);
	MemWrite8( u.B.msb,--S_REG);
	MemWrite8( y.B.lsb,--S_REG);
	MemWrite8( y.B.msb,--S_REG);
	MemWrite8( x.B.lsb,--S_REG);
	MemWrite8( x.B.msb,--S_REG);
	MemWrite8( dp.B.msb,--S_REG);
	if (md[NATIVE6309])
	{
		MemWrite8((F_REG),--S_REG);
		MemWrite8((E_REG),--S_REG);
		CycleCounter+=2;
	}
	MemWrite8(B_REG,--S_REG);
	MemWrite8(A_REG,--S_REG);
	MemWrite8(getcc(),--S_REG);
	PC_REG=MemRead16(VSWI3);
	CycleCounter+=20;
}

void Come_I(void)
{ //1143 6309 Untested
	E_REG = 0xFF- E_REG;
	cc[Z] = ZTEST(E_REG);
	cc[N] = NTEST8(E_REG);
	cc[C] = 1;
	cc[V] = 0;
	CycleCounter+=NatEmuCycles32;
}

void Dece_I(void)
{ //114A 6309
	E_REG--;
	cc[Z] = ZTEST(E_REG);
	cc[N] = NTEST8(E_REG);
	cc[V] = E_REG==0x7F;
	CycleCounter+=NatEmuCycles32;
}

void Ince_I(void)
{ //114C 6309
	E_REG++;
	cc[Z] = ZTEST(E_REG);
	cc[N] = NTEST8(E_REG);
	cc[V] = E_REG==0x80;
	CycleCounter+=NatEmuCycles32;
}

void Tste_I(void)
{ //114D 6309 Untested TESTED NITRO
	cc[Z] = ZTEST(E_REG);
	cc[N] = NTEST8(E_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles32;
}

void Clre_I(void)
{ //114F 6309
	E_REG= 0;
	cc[C] = 0;
	cc[V] = 0;
	cc[N] = 0;
	cc[Z] = 1;
	CycleCounter+=NatEmuCycles32;
}

void Comf_I(void)
{ //1153 6309 Untested
	F_REG= 0xFF- F_REG;
	cc[Z] = ZTEST(F_REG);
	cc[N] = NTEST8(F_REG);
	cc[C] = 1;
	cc[V] = 0;
	CycleCounter+=NatEmuCycles32;
}

void Decf_I(void)
{ //115A 6309
	F_REG--;
	cc[Z] = ZTEST(F_REG);
	cc[N] = NTEST8(F_REG);
	cc[V] = F_REG==0x7F;
	CycleCounter+=NatEmuCycles21;
}

void Incf_I(void)
{ //115C 6309 Untested
	F_REG++;
	cc[Z] = ZTEST(F_REG);
	cc[N] = NTEST8(F_REG);
	cc[V] = F_REG==0x80;
	CycleCounter+=NatEmuCycles32;
}

void Tstf_I(void)
{ //115D 6309
	cc[Z] = ZTEST(F_REG);
	cc[N] = NTEST8(F_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles32;
}

void Clrf_I(void)
{ //115F 6309 Untested wcreate
	F_REG= 0;
	cc[C] = 0;
	cc[V] = 0;
	cc[N] = 0;
	cc[Z] = 1;
	CycleCounter+=NatEmuCycles32;
}

void Sube_M(void)
{ //1180 6309 Untested
	postbyte=MemRead8(PC_REG++);
	temp16 = E_REG - postbyte;
	cc[C] =(temp16 & 0x100)>>8; 
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,E_REG);
	E_REG= (unsigned char)temp16;
	cc[Z] = ZTEST(E_REG);
	cc[N] = NTEST8(E_REG);
	CycleCounter+=3;
}

void Cmpe_M(void)
{ //1181 6309
	postbyte=MemRead8(PC_REG++);
	temp8= E_REG-postbyte;
	cc[C] = temp8 > E_REG;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp8,E_REG);
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	CycleCounter+=3;
}

void Cmpu_M(void)
{ //1183
	postword=IMMADDRESS(PC_REG);
	temp16 = U_REG-postword;
	cc[C] = temp16 > U_REG;
	cc[V] = OVERFLOW16(cc[C],postword,temp16,U_REG);
	cc[N] = NTEST16(temp16);
	cc[Z] = ZTEST(temp16);
	PC_REG+=2;	
	CycleCounter+=NatEmuCycles54;
}

void Lde_M(void)
{ //1186 6309
	E_REG= MemRead8(PC_REG++);
	cc[Z] = ZTEST(E_REG);
	cc[N] = NTEST8(E_REG);
	cc[V] = 0;
	CycleCounter+=3;
}

void Adde_M(void)
{ //118B 6309
	postbyte=MemRead8(PC_REG++);
	temp16=E_REG+postbyte;
	cc[C] =(temp16 & 0x100)>>8;
	cc[H] = ((E_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc[V] =OVERFLOW8(cc[C],postbyte,temp16,E_REG);
	E_REG= (unsigned char)temp16;
	cc[N] = NTEST8(E_REG);
	cc[Z] = ZTEST(E_REG);
	CycleCounter+=3;
}

void Cmps_M(void)
{ //118C
	postword=IMMADDRESS(PC_REG);
	temp16 = S_REG-postword;
	cc[C] = temp16 > S_REG;
	cc[V] = OVERFLOW16(cc[C],postword,temp16,S_REG);
	cc[N] = NTEST16(temp16);
	cc[Z] = ZTEST(temp16);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}

void Divd_M(void)
{ //118D 6309
	postbyte = MemRead8(PC_REG++);

	if (postbyte == 0)
	{
		CycleCounter+=3;
		DivbyZero();
		return;			
	}

	postword = D_REG;
	stemp16 = (signed short)postword / (signed char)postbyte;

	if ((stemp16 > 255) || (stemp16 < -256)) //Abort
	{
		cc[V] = 1;
		cc[N] = 0;
		cc[Z] = 0;
		cc[C] = 0;
		CycleCounter+=17;
		return;
	}

	A_REG = (unsigned char)((signed short)postword % (signed char)postbyte);
	B_REG = stemp16;

	if ((stemp16 > 127) || (stemp16 < -128)) 
	{
		cc[V] = 1;
		cc[N] = 1;
	}
	else
	{
		cc[Z] = ZTEST(B_REG);
		cc[N] = NTEST8(B_REG);
		cc[V] = 0;
	}
	cc[C] = B_REG & 1;
	CycleCounter+=25;
}

void Divq_M(void)
{ //118E 6309
	postword = MemRead16(PC_REG);
	PC_REG+=2;

	if(postword == 0)
	{
		CycleCounter+=4;
		DivbyZero();
		return;			
	}

	temp32 = Q_REG;
	stemp32 = (signed int)temp32 / (signed short int)postword;

	if ((stemp32 > 65535) || (stemp32 < -65536)) //Abort
	{
		cc[V] = 1;
		cc[N] = 0;
		cc[Z] = 0;
		cc[C] = 0;
		CycleCounter+=34-21;
		return;
	}

	D_REG = (unsigned short)((signed int)temp32 % (signed short int)postword);
	W_REG = stemp32;
	if ((stemp16 > 32767) || (stemp16 < -32768)) 
	{
		cc[V] = 1;
		cc[N] = 1;
	}
	else
	{
		cc[Z] = ZTEST(W_REG);
		cc[N] = NTEST16(W_REG);
		cc[V] = 0;
	}
	cc[C] = B_REG & 1;
	CycleCounter+=34;
}

void Muld_M(void)
{ //118F Phase 5 6309
	Q_REG =  (signed short)D_REG * (signed short)IMMADDRESS(PC_REG);
	cc[C] = 0; 
	cc[Z] = ZTEST(Q_REG);
	cc[V] = 0;
	cc[N] = NTEST32(Q_REG);
	PC_REG+=2;
	CycleCounter+=28;
}

void Sube_D(void)
{ //1190 6309 Untested HERE
	postbyte=MemRead8( DPADDRESS(PC_REG++));
	temp16 = E_REG - postbyte;
	cc[C] =(temp16 & 0x100)>>8; 
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,E_REG);
	E_REG= (unsigned char)temp16;
	cc[Z] = ZTEST(E_REG);
	cc[N] = NTEST8(E_REG);
	CycleCounter+=NatEmuCycles54;
}

void Cmpe_D(void)
{ //1191 6309 Untested
	postbyte=MemRead8(DPADDRESS(PC_REG++));
	temp8= E_REG-postbyte;
	cc[C] = temp8 > E_REG;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp8,E_REG);
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	CycleCounter+=NatEmuCycles54;
}

void Cmpu_D(void)
{ //1193
	postword=MemRead16(DPADDRESS(PC_REG++));
	temp16= U_REG - postword ;
	cc[C] = temp16 > U_REG;
	cc[V] = OVERFLOW16(cc[C],postword,temp16,U_REG);
	cc[N] = NTEST16(temp16);
	cc[Z] = ZTEST(temp16);
	CycleCounter+=NatEmuCycles75;
	}

void Lde_D(void)
{ //1196 6309
	E_REG= MemRead8(DPADDRESS(PC_REG++));
	cc[Z] = ZTEST(E_REG);
	cc[N] = NTEST8(E_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles54;
}

void Ste_D(void)
{ //1197 Phase 5 6309
	MemWrite8( E_REG,DPADDRESS(PC_REG++));
	cc[Z] = ZTEST(E_REG);
	cc[N] = NTEST8(E_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles54;
}

void Adde_D(void)
{ //119B 6309 Untested
	postbyte=MemRead8(DPADDRESS(PC_REG++));
	temp16=E_REG+postbyte;
	cc[C] = (temp16 & 0x100)>>8;
	cc[H] = ((E_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,E_REG);
	E_REG= (unsigned char)temp16;
	cc[N] = NTEST8(E_REG);
	cc[Z] =ZTEST(E_REG);
	CycleCounter+=NatEmuCycles54;
}

void Cmps_D(void)
{ //119C
	postword=MemRead16(DPADDRESS(PC_REG++));
	temp16= S_REG - postword ;
	cc[C] = temp16 > S_REG;
	cc[V] = OVERFLOW16(cc[C],postword,temp16,S_REG);
	cc[N] = NTEST16(temp16);
	cc[Z] = ZTEST(temp16);
	CycleCounter+=NatEmuCycles75;
}

void Divd_D(void)
{ //119D 6309 02292008
	postbyte = MemRead8(DPADDRESS(PC_REG++));

	if (postbyte == 0)
	{
		CycleCounter+=3;
		DivbyZero();
		return;			
	}

	postword = D_REG;
	stemp16 = (signed short)postword / (signed char)postbyte;

	if ((stemp16 > 255) || (stemp16 < -256)) //Abort
	{
		cc[V] = 1;
		cc[N] = 0;
		cc[Z] = 0;
		cc[C] = 0;
		CycleCounter+=19;
		return;
	}

	A_REG = (unsigned char)((signed short)postword % (signed char)postbyte);
	B_REG = stemp16;

	if ((stemp16 > 127) || (stemp16 < -128)) 
	{
		cc[V] = 1;
		cc[N] = 1;
	}
	else
	{
		cc[Z] = ZTEST(B_REG);
		cc[N] = NTEST8(B_REG);
		cc[V] = 0;
	}
	cc[C] = B_REG & 1;
	CycleCounter+=27;
}

void Divq_D(void)
{ //119E 6309
	postword = MemRead16(DPADDRESS(PC_REG++));

	if(postword == 0)
	{
		CycleCounter+=4;
		DivbyZero();
		return;			
	}

	temp32 = Q_REG;
	stemp32 = (signed int)temp32 / (signed short int)postword;

	if ((stemp32 > 65535) || (stemp32 < -65536)) //Abort
	{
		cc[V] = 1;
		cc[N] = 0;
		cc[Z] = 0;
		cc[C] = 0;
		CycleCounter+=NatEmuCycles3635-21;
		return;
	}

	D_REG = (unsigned short)((signed int)temp32 % (signed short int)postword);
	W_REG = stemp32;
	if ((stemp16 > 32767) || (stemp32 < -32768)) 
	{
		cc[V] = 1;
		cc[N] = 1;
	}
	else
	{
		cc[Z] = ZTEST(W_REG);
		cc[N] = NTEST16(W_REG);
		cc[V] = 0;
	}
	cc[C] = B_REG & 1;
  CycleCounter+=NatEmuCycles3635;
}

void Muld_D(void)
{ //119F 6309 02292008
	Q_REG = (signed short)D_REG * (signed short)MemRead16(DPADDRESS(PC_REG++));
	cc[C] = 0;
	cc[Z] = ZTEST(Q_REG);
	cc[V] = 0;
	cc[N] = NTEST32(Q_REG);
	CycleCounter+=NatEmuCycles3029;
}

void Sube_X(void)
{ //11A0 6309 Untested
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp16 = E_REG - postbyte;
	cc[C] =(temp16 & 0x100)>>8; 
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,E_REG);
	E_REG= (unsigned char)temp16;
	cc[Z] = ZTEST(E_REG);
	cc[N] = NTEST8(E_REG);
	CycleCounter+=5;
}

void Cmpe_X(void)
{ //11A1 6309
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp8= E_REG-postbyte;
	cc[C] = temp8 > E_REG;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp8,E_REG);
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	CycleCounter+=5;
}

void Cmpu_X(void)
{ //11A3
	postword=MemRead16(INDADDRESS(PC_REG++));
	temp16= U_REG - postword ;
	cc[C] = temp16 > U_REG;
	cc[V] = OVERFLOW16(cc[C],postword,temp16,U_REG);
	cc[N] = NTEST16(temp16);
	cc[Z] = ZTEST(temp16);
	CycleCounter+=NatEmuCycles76;
}

void Lde_X(void)
{ //11A6 6309
	E_REG= MemRead8(INDADDRESS(PC_REG++));
	cc[Z] = ZTEST(E_REG);
	cc[N] = NTEST8(E_REG);
	cc[V] = 0;
	CycleCounter+=5;
}

void Ste_X(void)
{ //11A7 6309
	MemWrite8(E_REG,INDADDRESS(PC_REG++));
	cc[Z] = ZTEST(E_REG);
	cc[N] = NTEST8(E_REG);
	cc[V] = 0;
	CycleCounter+=5;
}

void Adde_X(void)
{ //11AB 6309 Untested
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp16=E_REG+postbyte;
	cc[C] =(temp16 & 0x100)>>8;
	cc[H] = ((E_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc[V] =OVERFLOW8(cc[C],postbyte,temp16,E_REG);
	E_REG= (unsigned char)temp16;
	cc[N] = NTEST8(E_REG);
	cc[Z] = ZTEST(E_REG);
	CycleCounter+=5;
}

void Cmps_X(void)
{  //11AC
	postword=MemRead16(INDADDRESS(PC_REG++));
	temp16= S_REG - postword ;
	cc[C] = temp16 > S_REG;
	cc[V] = OVERFLOW16(cc[C],postword,temp16,S_REG);
	cc[N] = NTEST16(temp16);
	cc[Z] = ZTEST(temp16);
	CycleCounter+=NatEmuCycles76;
}

void Divd_X(void)
{ //11AD wcreate  6309
	postbyte = MemRead8(INDADDRESS(PC_REG++));

  if (postbyte == 0)
  {
    CycleCounter += 3;
    DivbyZero();
    return;
  }

  postword = D_REG;
  stemp16 = (signed short)postword / (signed char)postbyte;

  if ((stemp16 > 255) || (stemp16 < -256)) //Abort
  {
    cc[V] = 1;
    cc[N] = 0;
    cc[Z] = 0;
    cc[C] = 0;
    CycleCounter += 19;
    return;
  }

  A_REG = (unsigned char)((signed short)postword % (signed char)postbyte);
  B_REG = stemp16;

  if ((stemp16 > 127) || (stemp16 < -128))
  {
    cc[V] = 1;
    cc[N] = 1;
  }
  else
  {
    cc[Z] = ZTEST(B_REG);
    cc[N] = NTEST8(B_REG);
    cc[V] = 0;
  }
  cc[C] = B_REG & 1;
  CycleCounter += 27;
}

void Divq_X(void)
{ //11AE Phase 5 6309 CHECK
	postword = MemRead16(INDADDRESS(PC_REG++));

  if (postword == 0)
  {
    CycleCounter += 4;
    DivbyZero();
    return;
  }

  temp32 = Q_REG;
  stemp32 = (signed int)temp32 / (signed short int)postword;

  if ((stemp32 > 65535) || (stemp32 < -65536)) //Abort
  {
    cc[V] = 1;
    cc[N] = 0;
    cc[Z] = 0;
    cc[C] = 0;
    CycleCounter += NatEmuCycles3635 - 21;
    return;
  }

  D_REG = (unsigned short)((signed int)temp32 % (signed short int)postword);
  W_REG = stemp32;
  if ((stemp16 > 32767) || (stemp16 < -32768))
  {
    cc[V] = 1;
    cc[N] = 1;
  }
  else
  {
    cc[Z] = ZTEST(W_REG);
    cc[N] = NTEST16(W_REG);
    cc[V] = 0;
  }
  cc[C] = B_REG & 1;
  CycleCounter += NatEmuCycles3635;
}

void Muld_X(void)
{ //11AF 6309 CHECK
	Q_REG=  (signed short)D_REG * (signed short)MemRead16(INDADDRESS(PC_REG++));
	cc[C] = 0;
	cc[Z] = ZTEST(Q_REG);
	cc[V] = 0;
	cc[N] = NTEST32(Q_REG);
	CycleCounter+=30;
}

void Sube_E(void)
{ //11B0 6309 Untested
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp16 = E_REG - postbyte;
	cc[C] =(temp16 & 0x100)>>8; 
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,E_REG);
	E_REG= (unsigned char)temp16;
	cc[Z] = ZTEST(E_REG);
	cc[N] = NTEST8(E_REG);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles65;
}

void Cmpe_E(void)
{ //11B1 6309 Untested
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp8= E_REG-postbyte;
	cc[C] = temp8 > E_REG;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp8,E_REG);
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles65;
}

void Cmpu_E(void)
{ //11B3
	postword=MemRead16(IMMADDRESS(PC_REG));
	temp16 = U_REG-postword;
	cc[C] = temp16 > U_REG;
	cc[V] = OVERFLOW16(cc[C],postword,temp16,U_REG);
	cc[N] = NTEST16(temp16);
	cc[Z] = ZTEST(temp16);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles86;
}

void Lde_E(void)
{ //11B6 6309
	E_REG= MemRead8(IMMADDRESS(PC_REG));
	cc[Z] = ZTEST(E_REG);
	cc[N] = NTEST8(E_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles65;
}

void Ste_E(void)
{ //11B7 6309
	MemWrite8(E_REG,IMMADDRESS(PC_REG));
	cc[Z] = ZTEST(E_REG);
	cc[N] = NTEST8(E_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles65;
}

void Adde_E(void)
{ //11BB 6309 Untested
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp16=E_REG+postbyte;
	cc[C] = (temp16 & 0x100)>>8;
	cc[H] = ((E_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,E_REG);
	E_REG= (unsigned char)temp16;
	cc[N] = NTEST8(E_REG);
	cc[Z] = ZTEST(E_REG);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles65;
}

void Cmps_E(void)
{ //11BC
	postword=MemRead16(IMMADDRESS(PC_REG));
	temp16 = S_REG-postword;
	cc[C] = temp16 > S_REG;
	cc[V] = OVERFLOW16(cc[C],postword,temp16,S_REG);
	cc[N] = NTEST16(temp16);
	cc[Z] = ZTEST(temp16);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles86;
}

void Divd_E(void)
{ //11BD 6309 02292008 Untested
	postbyte = MemRead8(IMMADDRESS(PC_REG));
	PC_REG+=2;

  if (postbyte == 0)
  {
    CycleCounter += 3;
    DivbyZero();
    return;
  }

  postword = D_REG;
  stemp16 = (signed short)postword / (signed char)postbyte;

  if ((stemp16 > 255) || (stemp16 < -256)) //Abort
  {
    cc[V] = 1;
    cc[N] = 0;
    cc[Z] = 0;
    cc[C] = 0;
    CycleCounter += 17;
    return;
  }

  A_REG = (unsigned char)((signed short)postword % (signed char)postbyte);
  B_REG = stemp16;

  if ((stemp16 > 127) || (stemp16 < -128))
  {
    cc[V] = 1;
    cc[N] = 1;
  }
  else
  {
    cc[Z] = ZTEST(B_REG);
    cc[N] = NTEST8(B_REG);
    cc[V] = 0;
  }
  cc[C] = B_REG & 1;
  CycleCounter += 25;
}

void Divq_E(void)
{ //11BE Phase 5 6309 CHECK
	postword = MemRead16(IMMADDRESS(PC_REG));
	PC_REG+=2;

  if (postword == 0)
  {
    CycleCounter += 4;
    DivbyZero();
    return;
  }

  temp32 = Q_REG;
  stemp32 = (signed int)temp32 / (signed short int)postword;

  if ((stemp32 > 65535) || (stemp32 < -65536)) //Abort
  {
    cc[V] = 1;
    cc[N] = 0;
    cc[Z] = 0;
    cc[C] = 0;
    CycleCounter += NatEmuCycles3635 - 21;
    return;
  }

  D_REG = (unsigned short)((signed int)temp32 % (signed short int)postword);
  W_REG = stemp32;
  if ((stemp16 > 32767) || (stemp16 < -32768))
  {
    cc[V] = 1;
    cc[N] = 1;
  }
  else
  {
    cc[Z] = ZTEST(W_REG);
    cc[N] = NTEST16(W_REG);
    cc[V] = 0;
  }
  cc[C] = B_REG & 1;
  CycleCounter += NatEmuCycles3635;
}

void Muld_E(void)
{ //11BF 6309
	Q_REG=  (signed short)D_REG * (signed short)MemRead16(IMMADDRESS(PC_REG));
	PC_REG+=2;
	cc[C] = 0;
	cc[Z] = ZTEST(Q_REG);
	cc[V] = 0;
	cc[N] = NTEST32(Q_REG);
	CycleCounter+=NatEmuCycles3130;
}

void Subf_M(void)
{ //11C0 6309 Untested
	postbyte=MemRead8(PC_REG++);
	temp16 = F_REG - postbyte;
	cc[C] = (temp16 & 0x100)>>8; 
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,F_REG);
	F_REG= (unsigned char)temp16;
	cc[Z] = ZTEST(F_REG);
	cc[N] = NTEST8(F_REG);
	CycleCounter+=3;
}

void Cmpf_M(void)
{ //11C1 6309
	postbyte=MemRead8(PC_REG++);
	temp8= F_REG-postbyte;
	cc[C] = temp8 > F_REG;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp8,F_REG);
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	CycleCounter+=3;
}

void Ldf_M(void)
{ //11C6 6309
	F_REG= MemRead8(PC_REG++);
	cc[Z] = ZTEST(F_REG);
	cc[N] = NTEST8(F_REG);
	cc[V] = 0;
	CycleCounter+=3;
}

void Addf_M(void)
{ //11CB 6309 Untested
	postbyte=MemRead8(PC_REG++);
	temp16=F_REG+postbyte;
	cc[C] = (temp16 & 0x100)>>8;
	cc[H] = ((F_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,F_REG);
	F_REG= (unsigned char)temp16;
	cc[N] = NTEST8(F_REG);
	cc[Z] = ZTEST(F_REG);
	CycleCounter+=3;
}

void Subf_D(void)
{ //11D0 6309 Untested
	postbyte=MemRead8( DPADDRESS(PC_REG++));
	temp16 = F_REG - postbyte;
	cc[C] =(temp16 & 0x100)>>8; 
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,F_REG);
	F_REG= (unsigned char)temp16;
	cc[Z] = ZTEST(F_REG);
	cc[N] = NTEST8(F_REG);
	CycleCounter+=NatEmuCycles54;
}

void Cmpf_D(void)
{ //11D1 6309 Untested
	postbyte=MemRead8(DPADDRESS(PC_REG++));
	temp8= F_REG-postbyte;
	cc[C] = temp8 > F_REG;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp8,F_REG);
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	CycleCounter+=NatEmuCycles54;
}

void Ldf_D(void)
{ //11D6 6309 Untested wcreate
	F_REG= MemRead8(DPADDRESS(PC_REG++));
	cc[Z] = ZTEST(F_REG);
	cc[N] = NTEST8(F_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles54;
}

void Stf_D(void)
{ //11D7 Phase 5 6309
	MemWrite8(F_REG,DPADDRESS(PC_REG++));
	cc[Z] = ZTEST(F_REG);
	cc[N] = NTEST8(F_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles54;
}

void Addf_D(void)
{ //11DB 6309 Untested
	postbyte=MemRead8(DPADDRESS(PC_REG++));
	temp16=F_REG+postbyte;
	cc[C] =(temp16 & 0x100)>>8;
	cc[H] = ((F_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc[V] =OVERFLOW8(cc[C],postbyte,temp16,F_REG);
	F_REG= (unsigned char)temp16;
	cc[N] = NTEST8(F_REG);
	cc[Z] = ZTEST(F_REG);
	CycleCounter+=NatEmuCycles54;
}

void Subf_X(void)
{ //11E0 6309 Untested
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp16 = F_REG - postbyte;
	cc[C] =(temp16 & 0x100)>>8; 
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,F_REG);
	F_REG= (unsigned char)temp16;
	cc[Z] = ZTEST(F_REG);
	cc[N] = NTEST8(F_REG);
	CycleCounter+=5;
}

void Cmpf_X(void)
{ //11E1 6309 Untested
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp8= F_REG-postbyte;
	cc[C] = temp8 > F_REG;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp8,F_REG);
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	CycleCounter+=5;
}

void Ldf_X(void)
{ //11E6 6309
	F_REG=MemRead8(INDADDRESS(PC_REG++));
	cc[Z] = ZTEST(F_REG);
	cc[N] = NTEST8(F_REG);
	cc[V] = 0;
	CycleCounter+=5;
}

void Stf_X(void)
{ //11E7 Phase 5 6309
	MemWrite8(F_REG,INDADDRESS(PC_REG++));
	cc[Z] = ZTEST(F_REG);
	cc[N] = NTEST8(F_REG);
	cc[V] = 0;
	CycleCounter+=5;
}

void Addf_X(void)
{ //11EB 6309 Untested
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp16=F_REG+postbyte;
	cc[C] =(temp16 & 0x100)>>8;
	cc[H] = ((F_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,F_REG);
	F_REG= (unsigned char)temp16;
	cc[N] = NTEST8(F_REG);
	cc[Z] = ZTEST(F_REG);
	CycleCounter+=5;
}

void Subf_E(void)
{ //11F0 6309 Untested
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp16 = F_REG - postbyte;
	cc[C] = (temp16 & 0x100)>>8; 
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,F_REG);
	F_REG= (unsigned char)temp16;
	cc[Z] = ZTEST(F_REG);
	cc[N] = NTEST8(F_REG);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles65;
}

void Cmpf_E(void)
{ //11F1 6309 Untested
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp8= F_REG-postbyte;
	cc[C] = temp8 > F_REG;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp8,F_REG);
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles65;
}

void Ldf_E(void)
{ //11F6 6309
	F_REG= MemRead8(IMMADDRESS(PC_REG));
	cc[Z] = ZTEST(F_REG);
	cc[N] = NTEST8(F_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles65;
}

void Stf_E(void)
{ //11F7 Phase 5 6309
	MemWrite8(F_REG,IMMADDRESS(PC_REG));
	cc[Z] = ZTEST(F_REG);
	cc[N] = NTEST8(F_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles65;
}

void Addf_E(void)
{ //11FB 6309 Untested
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp16=F_REG+postbyte;
	cc[C] = (temp16 & 0x100)>>8;
	cc[H] = ((F_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,F_REG);
	F_REG= (unsigned char)temp16;
	cc[N] = NTEST8(F_REG);
	cc[Z] = ZTEST(F_REG);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles65;
}

void Nop_I(void)
{	//12
	CycleCounter+=NatEmuCycles21;
}

void Sync_I(void)
{ //13
	CycleCounter=gCycleFor;
	SyncWaiting=1;
}

void Sexw_I(void)
{ //14 6309 CHECK
	if (W_REG & 32768)
		D_REG=0xFFFF;
	else
		D_REG=0;
	cc[Z] = ZTEST(Q_REG);
	cc[N] = NTEST16(D_REG);
	CycleCounter+=4;
}

void Lbra_R(void)
{ //16
	*spostword=IMMADDRESS(PC_REG);
	PC_REG+=2;
	PC_REG+=*spostword;
	CycleCounter+=NatEmuCycles54;
}

void Lbsr_R(void)
{ //17
	*spostword=IMMADDRESS(PC_REG);
	PC_REG+=2;
	S_REG--;
	MemWrite8(pc.B.lsb,S_REG--);
	MemWrite8(pc.B.msb,S_REG);
	PC_REG+=*spostword;
	CycleCounter+=NatEmuCycles97;
}

void Daa_I(void)
{ //19
	static unsigned char msn, lsn;

	msn=(A_REG & 0xF0) ;
	lsn=(A_REG & 0xF);
	temp8=0;
	if ( cc[H] ||  (lsn >9) )
		temp8 |= 0x06;

	if ( (msn>0x80) && (lsn>9)) 
		temp8|=0x60;
	
	if ( (msn>0x90) || cc[C] )
		temp8|=0x60;

	temp16= A_REG+temp8;
	cc[C]|= ((temp16 & 0x100)>>8);
	A_REG= (unsigned char)temp16;
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	CycleCounter+=NatEmuCycles21;
}

void Orcc_M(void)
{ //1A
	postbyte=MemRead8(PC_REG++);
	temp8=getcc();
	temp8 = (temp8 | postbyte);
	setcc(temp8);
	CycleCounter+=NatEmuCycles32;
}

void Andcc_M(void)
{ //1C
	postbyte=MemRead8(PC_REG++);
	temp8=getcc();
	temp8 = (temp8 & postbyte);
	setcc(temp8);
	CycleCounter+=3;
}

void Sex_I(void)
{ //1D
	A_REG= 0-(B_REG>>7);
	cc[Z] = ZTEST(D_REG);
	cc[N] = D_REG >> 15;
	CycleCounter+=NatEmuCycles21;
}

void Exg_M(void)
{ //1E
	postbyte = MemRead8(PC_REG++);
	Source = postbyte >> 4;
	Dest = postbyte & 15;

	ccbits = getcc();
	if ((Source & 0x08) == (Dest & 0x08)) //Verify like size registers
	{
		if (Dest & 0x08) //8 bit EXG
		{
			Source &= 0x07;
			Dest &= 0x07;
			temp8 = (*ureg8[Source]);
			*ureg8[Source] = (*ureg8[Dest]);
			*ureg8[Dest] = temp8;
			O_REG = 0;
		}
		else // 16 bit EXG
		{
			Source &= 0x07;
			Dest &= 0x07;
			temp16 = (*xfreg16[Source]);
			(*xfreg16[Source]) = (*xfreg16[Dest]);
			(*xfreg16[Dest]) = temp16;
		}
	}
	else
	{
		if (Dest & 0x08) // Swap 16 to 8 bit exchange to be 8 to 16 bit exchange (for convenience)
		{
			temp8 = Dest; Dest = Source; Source = temp8;
		}

		Source &= 0x07;
		Dest &= 0x07;

		switch (Source)
		{
		case 0x04: // Z
		case 0x05: // Z
			(*xfreg16[Dest]) = 0; // Source is Zero reg. Just zero the Destination.
			break;
		case 0x00: // A
		case 0x03: // DP
		case 0x06: // E
			temp8 = *ureg8[Source];
			temp16 = (temp8 << 8) | temp8;
			(*ureg8[Source]) = (*xfreg16[Dest]) >> 8; // A, DP, E get high byte of 16 bit Dest
			(*xfreg16[Dest]) = temp16; // Place 8 bit source in both halves of 16 bit Dest
			break;
		case 0x01: // B
		case 0x02: // CC
		case 0x07: // F
			temp8 = *ureg8[Source];
			temp16 = (temp8 << 8) | temp8;
			(*ureg8[Source]) = (*xfreg16[Dest]) & 0xFF; // B, CC, F get low byte of 16 bit Dest
			(*xfreg16[Dest]) = temp16; // Place 8 bit source in both halves of 16 bit Dest
			break;
		}
	}
	setcc(ccbits);
	CycleCounter += NatEmuCycles85;
}

void Tfr_M(void)
{ //1F
	postbyte=MemRead8(PC_REG++);
	Source= postbyte>>4;
	Dest=postbyte & 15;

	if (Dest < 8)
	{
		if (Source < 8)
			*xfreg16[Dest] = *xfreg16[Source];
		else
			*xfreg16[Dest] = (*ureg8[Source & 7] << 8) | *ureg8[Source & 7];
	}
	else
	{
		ccbits = getcc();
		Dest &= 7;

		if (Source < 8)
			switch (Dest)
			{
			case 0:  // A
			case 3: // DP
			case 6: // E
				*ureg8[Dest] = *xfreg16[Source] >> 8;
				break;
			case 1:  // B
			case 2: // CC
			case 7: // F
				*ureg8[Dest] = *xfreg16[Source] & 0xFF;
				break;
			}
		else
			*ureg8[Dest] = *ureg8[Source & 7];

		O_REG = 0;
		setcc(ccbits);
	}
	
	CycleCounter+=NatEmuCycles64;
}

void Bra_R(void)
{ //20
	*spostbyte=MemRead8(PC_REG++);
	PC_REG+=*spostbyte;
	CycleCounter+=3;
}

void Brn_R(void)
{ //21
	CycleCounter+=3;
	PC_REG++;
}

void Bhi_R(void)
{ //22
	if  (!(cc[C] | cc[Z]))
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
}

void Bls_R(void)
{ //23
	if (cc[C] | cc[Z])
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
}

void Bhs_R(void)
{ //24
	if (!cc[C])
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
}

void Blo_R(void)
{ //25
	if (cc[C])
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
}

void Bne_R(void)
{ //26
	if (!cc[Z])
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
}

void Beq_R(void)
{ //27
	if (cc[Z])
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
}

void Bvc_R(void)
{ //28
	if (!cc[V])
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
}

void Bvs_R(void)
{ //29
	if ( cc[V])
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
}

void Bpl_R(void)
{ //2A
	if (!cc[N])
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
}

void Bmi_R(void)
{ //2B
	if ( cc[N])
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
}

void Bge_R(void)
{ //2C
	if (! (cc[N] ^ cc[V]))
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
}

void Blt_R(void)
{ //2D
	if ( cc[V] ^ cc[N])
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
}

void Bgt_R(void)
{ //2E
	if ( !( cc[Z] | (cc[N]^cc[V] ) ))
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
}

void Ble_R(void)
{ //2F
	if ( cc[Z] | (cc[N]^cc[V]) )
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
}

void Leax_X(void)
{ //30
	X_REG=INDADDRESS(PC_REG++);
	cc[Z] = ZTEST(X_REG);
	CycleCounter+=4;
}

void Leay_X(void)
{ //31
	Y_REG=INDADDRESS(PC_REG++);
	cc[Z] = ZTEST(Y_REG);
	CycleCounter+=4;
}

void Leas_X(void)
{ //32
	S_REG=INDADDRESS(PC_REG++);
	CycleCounter+=4;
}

void Leau_X(void)
{ //33
	U_REG=INDADDRESS(PC_REG++);
	CycleCounter+=4;
}

void Pshs_M(void)
{ //34
	postbyte=MemRead8(PC_REG++);
	if (postbyte & 0x80)
	{
		MemWrite8( pc.B.lsb,--S_REG);
		MemWrite8( pc.B.msb,--S_REG);
		CycleCounter+=2;
	}
	if (postbyte & 0x40)
	{
		MemWrite8( u.B.lsb,--S_REG);
		MemWrite8( u.B.msb,--S_REG);
		CycleCounter+=2;
	}
	if (postbyte & 0x20)
	{
		MemWrite8( y.B.lsb,--S_REG);
		MemWrite8( y.B.msb,--S_REG);
		CycleCounter+=2;
	}
	if (postbyte & 0x10)
	{
		MemWrite8( x.B.lsb,--S_REG);
		MemWrite8( x.B.msb,--S_REG);
		CycleCounter+=2;
	}
	if (postbyte & 0x08)
	{
		MemWrite8( dp.B.msb,--S_REG);
		CycleCounter+=1;
	}
	if (postbyte & 0x04)
	{
		MemWrite8(B_REG,--S_REG);
		CycleCounter+=1;
	}
	if (postbyte & 0x02)
	{
		MemWrite8(A_REG,--S_REG);
		CycleCounter+=1;
	}
	if (postbyte & 0x01)
	{
		MemWrite8(getcc(),--S_REG);
		CycleCounter+=1;
	}

	CycleCounter+=NatEmuCycles54;
}

void Puls_M(void)
{ //35
	postbyte=MemRead8(PC_REG++);
	if (postbyte & 0x01)
	{
		setcc(MemRead8(S_REG++));
		CycleCounter+=1;
	}
	if (postbyte & 0x02)
	{
		A_REG=MemRead8(S_REG++);
		CycleCounter+=1;
	}
	if (postbyte & 0x04)
	{
		B_REG=MemRead8(S_REG++);
		CycleCounter+=1;
	}
	if (postbyte & 0x08)
	{
		dp.B.msb=MemRead8(S_REG++);
		CycleCounter+=1;
	}
	if (postbyte & 0x10)
	{
		x.B.msb=MemRead8( S_REG++);
		x.B.lsb=MemRead8( S_REG++);
		CycleCounter+=2;
	}
	if (postbyte & 0x20)
	{
		y.B.msb=MemRead8( S_REG++);
		y.B.lsb=MemRead8( S_REG++);
		CycleCounter+=2;
	}
	if (postbyte & 0x40)
	{
		u.B.msb=MemRead8( S_REG++);
		u.B.lsb=MemRead8( S_REG++);
		CycleCounter+=2;
	}
	if (postbyte & 0x80)
	{
		pc.B.msb=MemRead8( S_REG++);
		pc.B.lsb=MemRead8( S_REG++);
		CycleCounter+=2;
	}
	CycleCounter+=NatEmuCycles54;
}

void Pshu_M(void)
{ //36
	postbyte=MemRead8(PC_REG++);
	if (postbyte & 0x80)
	{
		MemWrite8( pc.B.lsb,--U_REG);
		MemWrite8( pc.B.msb,--U_REG);
		CycleCounter+=2;
	}
	if (postbyte & 0x40)
	{
		MemWrite8( s.B.lsb,--U_REG);
		MemWrite8( s.B.msb,--U_REG);
		CycleCounter+=2;
	}
	if (postbyte & 0x20)
	{
		MemWrite8( y.B.lsb,--U_REG);
		MemWrite8( y.B.msb,--U_REG);
		CycleCounter+=2;
	}
	if (postbyte & 0x10)
	{
		MemWrite8( x.B.lsb,--U_REG);
		MemWrite8( x.B.msb,--U_REG);
		CycleCounter+=2;
	}
	if (postbyte & 0x08)
	{
		MemWrite8( dp.B.msb,--U_REG);
		CycleCounter+=1;
	}
	if (postbyte & 0x04)
	{
		MemWrite8(B_REG,--U_REG);
		CycleCounter+=1;
	}
	if (postbyte & 0x02)
	{
		MemWrite8(A_REG,--U_REG);
		CycleCounter+=1;
	}
	if (postbyte & 0x01)
	{
		MemWrite8(getcc(),--U_REG);
		CycleCounter+=1;
	}
	CycleCounter+=NatEmuCycles54;
}

void Pulu_M(void)
{ //37
	postbyte=MemRead8(PC_REG++);
	if (postbyte & 0x01)
	{
		setcc(MemRead8(U_REG++));
		CycleCounter+=1;
	}
	if (postbyte & 0x02)
	{
		A_REG=MemRead8(U_REG++);
		CycleCounter+=1;
	}
	if (postbyte & 0x04)
	{
		B_REG=MemRead8(U_REG++);
		CycleCounter+=1;
	}
	if (postbyte & 0x08)
	{
		dp.B.msb=MemRead8(U_REG++);
		CycleCounter+=1;
	}
	if (postbyte & 0x10)
	{
		x.B.msb=MemRead8( U_REG++);
		x.B.lsb=MemRead8( U_REG++);
		CycleCounter+=2;
	}
	if (postbyte & 0x20)
	{
		y.B.msb=MemRead8( U_REG++);
		y.B.lsb=MemRead8( U_REG++);
		CycleCounter+=2;
	}
	if (postbyte & 0x40)
	{
		s.B.msb=MemRead8( U_REG++);
		s.B.lsb=MemRead8( U_REG++);
		CycleCounter+=2;
	}
	if (postbyte & 0x80)
	{
		pc.B.msb=MemRead8( U_REG++);
		pc.B.lsb=MemRead8( U_REG++);
		CycleCounter+=2;
	}
	CycleCounter+=NatEmuCycles54;
}

void Rts_I(void)
{ //39
	pc.B.msb=MemRead8(S_REG++);
	pc.B.lsb=MemRead8(S_REG++);
	CycleCounter+=NatEmuCycles51;
}

void Abx_I(void)
{ //3A
	X_REG=X_REG+B_REG;
	CycleCounter+=NatEmuCycles31;
}

void Rti_I(void)
{ //3B
	setcc(MemRead8(S_REG++));
	CycleCounter+=6;
	InInterupt=0;
	if (cc[E])
	{
		A_REG=MemRead8(S_REG++);
		B_REG=MemRead8(S_REG++);
		if (md[NATIVE6309])
		{
			(E_REG)=MemRead8(S_REG++);
			(F_REG)=MemRead8(S_REG++);
			CycleCounter+=2;
		}
		dp.B.msb=MemRead8(S_REG++);
		x.B.msb=MemRead8(S_REG++);
		x.B.lsb=MemRead8(S_REG++);
		y.B.msb=MemRead8(S_REG++);
		y.B.lsb=MemRead8(S_REG++);
		u.B.msb=MemRead8(S_REG++);
		u.B.lsb=MemRead8(S_REG++);
		CycleCounter+=9;
	}
	pc.B.msb=MemRead8(S_REG++);
	pc.B.lsb=MemRead8(S_REG++);
}

void Cwai_I(void)
{ //3C
	postbyte=MemRead8(PC_REG++);
	ccbits=getcc();
	ccbits = ccbits & postbyte;
	setcc(ccbits);
	CycleCounter=gCycleFor;
	SyncWaiting=1;
}

void Mul_I(void)
{ //3D
	D_REG = A_REG * B_REG;
	cc[C] = B_REG >0x7F;
	cc[Z] = ZTEST(D_REG);
	CycleCounter+=NatEmuCycles1110;
}

void Reset(void) // 3E
{	//Undocumented
	HD6309Reset();
}

void Swi1_I(void)
{ //3F
	cc[E]=1;
	MemWrite8( pc.B.lsb,--S_REG);
	MemWrite8( pc.B.msb,--S_REG);
	MemWrite8( u.B.lsb,--S_REG);
	MemWrite8( u.B.msb,--S_REG);
	MemWrite8( y.B.lsb,--S_REG);
	MemWrite8( y.B.msb,--S_REG);
	MemWrite8( x.B.lsb,--S_REG);
	MemWrite8( x.B.msb,--S_REG);
	MemWrite8( dp.B.msb,--S_REG);
	if (md[NATIVE6309])
	{
		MemWrite8((F_REG),--S_REG);
		MemWrite8((E_REG),--S_REG);
		CycleCounter+=2;
	}
	MemWrite8(B_REG,--S_REG);
	MemWrite8(A_REG,--S_REG);
	MemWrite8(getcc(),--S_REG);
	PC_REG=MemRead16(VSWI);
	CycleCounter+=19;
	cc[I]=1;
	cc[F]=1;
}

void Nega_I(void)
{ //40
	temp8= 0-A_REG;
	cc[C] = temp8>0;
	cc[V] = A_REG==0x80; //cc[C] ^ ((A_REG^temp8)>>7);
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	A_REG= temp8;
	CycleCounter+=NatEmuCycles21;
}

void Coma_I(void)
{ //43
	A_REG= 0xFF- A_REG;
	cc[Z] = ZTEST(A_REG);
	cc[N] = NTEST8(A_REG);
	cc[C] = 1;
	cc[V] = 0;
	CycleCounter+=NatEmuCycles21;
}

void Lsra_I(void)
{ //44
	cc[C] = A_REG & 1;
	A_REG= A_REG>>1;
	cc[Z] = ZTEST(A_REG);
	cc[N] = 0;
	CycleCounter+=NatEmuCycles21;
}

void Rora_I(void)
{ //46
	postbyte=cc[C]<<7;
	cc[C] = A_REG & 1;
	A_REG= (A_REG>>1) | postbyte;
	cc[Z] = ZTEST(A_REG);
	cc[N] = NTEST8(A_REG);
	CycleCounter+=NatEmuCycles21;
}

void Asra_I(void)
{ //47
	cc[C] = A_REG & 1;
	A_REG= (A_REG & 0x80) | (A_REG >> 1);
	cc[Z] = ZTEST(A_REG);
	cc[N] = NTEST8(A_REG);
	CycleCounter+=NatEmuCycles21;
}

void Asla_I(void)
{ //48
	cc[C] = A_REG > 0x7F;
	cc[V] =  cc[C] ^((A_REG & 0x40)>>6);
	A_REG= A_REG<<1;
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	CycleCounter+=NatEmuCycles21;
}

void Rola_I(void)
{ //49
	postbyte=cc[C];
	cc[C] = A_REG > 0x7F;
	cc[V] = cc[C] ^ ((A_REG & 0x40)>>6);
	A_REG= (A_REG<<1) | postbyte;
	cc[Z] = ZTEST(A_REG);
	cc[N] = NTEST8(A_REG);
	CycleCounter+=NatEmuCycles21;
}

void Deca_I(void)
{ //4A
	A_REG--;
	cc[Z] = ZTEST(A_REG);
	cc[V] = A_REG==0x7F;
	cc[N] = NTEST8(A_REG);
	CycleCounter+=NatEmuCycles21;
}

void Inca_I(void)
{ //4C
	A_REG++;
	cc[Z] = ZTEST(A_REG);
	cc[V] = A_REG==0x80;
	cc[N] = NTEST8(A_REG);
	CycleCounter+=NatEmuCycles21;
}

void Tsta_I(void)
{ //4D
	cc[Z] = ZTEST(A_REG);
	cc[N] = NTEST8(A_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles21;
}

void Clra_I(void)
{ //4F
	A_REG= 0;
	cc[C] =0;
	cc[V] = 0;
	cc[N] =0;
	cc[Z] =1;
	CycleCounter+=NatEmuCycles21;
}

void Negb_I(void)
{ //50
	temp8= 0-B_REG;
	cc[C] = temp8>0;
	cc[V] = B_REG == 0x80;	
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	B_REG= temp8;
	CycleCounter+=NatEmuCycles21;
}

void Comb_I(void)
{ //53
	B_REG= 0xFF- B_REG;
	cc[Z] = ZTEST(B_REG);
	cc[N] = NTEST8(B_REG);
	cc[C] = 1;
	cc[V] = 0;
	CycleCounter+=NatEmuCycles21;
}

void Lsrb_I(void)
{ //54
	cc[C] = B_REG & 1;
	B_REG= B_REG>>1;
	cc[Z] = ZTEST(B_REG);
	cc[N] = 0;
	CycleCounter+=NatEmuCycles21;
}

void Rorb_I(void)
{ //56
	postbyte=cc[C]<<7;
	cc[C] = B_REG & 1;
	B_REG= (B_REG>>1) | postbyte;
	cc[Z] = ZTEST(B_REG);
	cc[N] = NTEST8(B_REG);
	CycleCounter+=NatEmuCycles21;
}

void Asrb_I(void)
{ //57
	cc[C] = B_REG & 1;
	B_REG= (B_REG & 0x80) | (B_REG >> 1);
	cc[Z] = ZTEST(B_REG);
	cc[N] = NTEST8(B_REG);
	CycleCounter+=NatEmuCycles21;
}

void Aslb_I(void)
{ //58
	cc[C] = B_REG > 0x7F;
	cc[V] =  cc[C] ^((B_REG & 0x40)>>6);
	B_REG= B_REG<<1;
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	CycleCounter+=NatEmuCycles21;
}

void Rolb_I(void)
{ //59
	postbyte=cc[C];
	cc[C] = B_REG > 0x7F;
	cc[V] = cc[C] ^ ((B_REG & 0x40)>>6);
	B_REG= (B_REG<<1) | postbyte;
	cc[Z] = ZTEST(B_REG);
	cc[N] = NTEST8(B_REG);
	CycleCounter+=NatEmuCycles21;
}

void Decb_I(void)
{ //5A
	B_REG--;
	cc[Z] = ZTEST(B_REG);
	cc[V] = B_REG==0x7F;
	cc[N] = NTEST8(B_REG);
	CycleCounter+=NatEmuCycles21;
}

void Incb_I(void)
{ //5C
	B_REG++;
	cc[Z] = ZTEST(B_REG);
	cc[V] = B_REG==0x80; 
	cc[N] = NTEST8(B_REG);
	CycleCounter+=NatEmuCycles21;
}

void Tstb_I(void)
{ //5D
	cc[Z] = ZTEST(B_REG);
	cc[N] = NTEST8(B_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles21;
}

void Clrb_I(void)
{ //5F
	B_REG=0;
	cc[C] =0;
	cc[N] =0;
	cc[V] = 0;
	cc[Z] =1;
	CycleCounter+=NatEmuCycles21;
}

void Neg_X(void)
{ //60
	temp16=INDADDRESS(PC_REG++);	
	postbyte=MemRead8(temp16);
	temp8= 0-postbyte;
	cc[C] = temp8>0;
	cc[V] = postbyte == 0x80;
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	MemWrite8(temp8,temp16);
	CycleCounter+=6;
}

void Oim_X(void)
{ //61 6309 DONE
	postbyte=MemRead8(PC_REG++);
	temp16=INDADDRESS(PC_REG++);
	postbyte |= MemRead8(temp16);
	MemWrite8(postbyte,temp16);
	cc[N] = NTEST8(postbyte);
	cc[Z] = ZTEST(postbyte);
	cc[V] = 0;
	CycleCounter+=6;
}

void Aim_X(void)
{ //62 6309 Phase 2
	postbyte=MemRead8(PC_REG++);
	temp16=INDADDRESS(PC_REG++);
	postbyte &= MemRead8(temp16);
	MemWrite8(postbyte,temp16);
	cc[N] = NTEST8(postbyte);
	cc[Z] = ZTEST(postbyte);
	cc[V] = 0;
	CycleCounter+=6;
}

void Com_X(void)
{ //63
	temp16=INDADDRESS(PC_REG++);
	temp8=MemRead8(temp16);
	temp8= 0xFF-temp8;
	cc[Z] = ZTEST(temp8);
	cc[N] = NTEST8(temp8);
	cc[V] = 0;
	cc[C] = 1;
	MemWrite8(temp8,temp16);
	CycleCounter+=6;
}

void Lsr_X(void)
{ //64
	temp16=INDADDRESS(PC_REG++);
	temp8=MemRead8(temp16);
	cc[C] = temp8 & 1;
	temp8= temp8 >>1;
	cc[Z] = ZTEST(temp8);
	cc[N] = 0;
	MemWrite8(temp8,temp16);
	CycleCounter+=6;
}

void Eim_X(void)
{ //65 6309 Untested TESTED NITRO
	postbyte=MemRead8(PC_REG++);
	temp16=INDADDRESS(PC_REG++);
	postbyte ^= MemRead8(temp16);
	MemWrite8(postbyte,temp16);
	cc[N] = NTEST8(postbyte);
	cc[Z] = ZTEST(postbyte);
	cc[V] = 0;
	CycleCounter+=7;
}

void Ror_X(void)
{ //66
	temp16=INDADDRESS(PC_REG++);
	temp8=MemRead8(temp16);
	postbyte=cc[C]<<7;
	cc[C] = (temp8 & 1);
	temp8= (temp8>>1) | postbyte;
	cc[Z] = ZTEST(temp8);
	cc[N] = NTEST8(temp8);
	MemWrite8(temp8,temp16);
	CycleCounter+=6;
}

void Asr_X(void)
{ //67
	temp16=INDADDRESS(PC_REG++);
	temp8=MemRead8(temp16);
	cc[C] = temp8 & 1;
	temp8= (temp8 & 0x80) | (temp8 >>1);
	cc[Z] = ZTEST(temp8);
	cc[N] = NTEST8(temp8);
	MemWrite8(temp8,temp16);
	CycleCounter+=6;
}

void Asl_X(void)
{ //68 
	temp16=INDADDRESS(PC_REG++);
	temp8= MemRead8(temp16);
	cc[C] = temp8 > 0x7F;
	cc[V] = cc[C] ^ ((temp8 & 0x40)>>6);
	temp8= temp8<<1;
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);	
	MemWrite8(temp8,temp16);
	CycleCounter+=6;
}

void Rol_X(void)
{ //69
	temp16=INDADDRESS(PC_REG++);
	temp8=MemRead8(temp16);
	postbyte=cc[C];
	cc[C] = temp8 > 0x7F;
	cc[V] = ( cc[C] ^ ((temp8 & 0x40)>>6));
	temp8= ((temp8<<1) | postbyte);
	cc[Z] = ZTEST(temp8);
	cc[N] = NTEST8(temp8);
	MemWrite8(temp8,temp16);
	CycleCounter+=6;
}

void Dec_X(void)
{ //6A
	temp16=INDADDRESS(PC_REG++);
	temp8=MemRead8(temp16);
	temp8--;
	cc[Z] = ZTEST(temp8);
	cc[N] = NTEST8(temp8);
	cc[V] = (temp8==0x7F);
	MemWrite8(temp8,temp16);
	CycleCounter+=6;
}

void Tim_X(void)
{ //6B 6309
	postbyte=MemRead8(PC_REG++);
	temp8=MemRead8(INDADDRESS(PC_REG++));
	postbyte&=temp8;
	cc[N] = NTEST8(postbyte);
	cc[Z] = ZTEST(postbyte);
	cc[V] = 0;
	CycleCounter+=7;
}

void Inc_X(void)
{ //6C
	temp16=INDADDRESS(PC_REG++);
	temp8=MemRead8(temp16);
	temp8++;
	cc[V] = (temp8 == 0x80);
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	MemWrite8(temp8,temp16);
	CycleCounter+=6;
}

void Tst_X(void)
{ //6D
	temp8=MemRead8(INDADDRESS(PC_REG++));
	cc[Z] = ZTEST(temp8);
	cc[N] = NTEST8(temp8);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles65;
}

void Jmp_X(void)
{ //6E
	PC_REG=INDADDRESS(PC_REG++);
	CycleCounter+=3;
}

void Clr_X(void)
{ //6F
	MemWrite8(0,INDADDRESS(PC_REG++));
	cc[C] = 0;
	cc[N] = 0;
	cc[V] = 0;
	cc[Z] = 1; 
	CycleCounter+=6;
}

void Neg_E(void)
{ //70
	temp16=IMMADDRESS(PC_REG);
	postbyte=MemRead8(temp16);
	temp8=0-postbyte;
	cc[C] = temp8>0;
	cc[V] = postbyte == 0x80;
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	MemWrite8(temp8,temp16);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles76;
}

void Oim_E(void)
{ //71 6309 Phase 2
	postbyte=MemRead8(PC_REG++);
	temp16=IMMADDRESS(PC_REG);
	postbyte|= MemRead8(temp16);
	MemWrite8(postbyte,temp16);
	cc[N] = NTEST8(postbyte);
	cc[Z] = ZTEST(postbyte);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=7;
}

void Aim_E(void)
{ //72 6309 Untested CHECK NITRO
	postbyte=MemRead8(PC_REG++);
	temp16=IMMADDRESS(PC_REG);
	postbyte&= MemRead8(temp16);
	MemWrite8(postbyte,temp16);
	cc[N] = NTEST8(postbyte);
	cc[Z] = ZTEST(postbyte);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=7;
}

void Com_E(void)
{ //73
	temp16=IMMADDRESS(PC_REG);
	temp8=MemRead8(temp16);
	temp8=0xFF-temp8;
	cc[Z] = ZTEST(temp8);
	cc[N] = NTEST8(temp8);
	cc[C] = 1;
	cc[V] = 0;
	MemWrite8(temp8,temp16);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles76;
}

void Lsr_E(void)
{  //74
	temp16=IMMADDRESS(PC_REG);
	temp8=MemRead8(temp16);
	cc[C] = temp8 & 1;
	temp8= temp8>>1;
	cc[Z] = ZTEST(temp8);
	cc[N] = 0;
	MemWrite8(temp8,temp16);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles76;
}

void Eim_E(void)
{ //75 6309 Untested CHECK NITRO
	postbyte=MemRead8(PC_REG++);
	temp16=IMMADDRESS(PC_REG);
	postbyte^= MemRead8(temp16);
	MemWrite8(postbyte,temp16);
	cc[N] = NTEST8(postbyte);
	cc[Z] = ZTEST(postbyte);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=7;
}

void Ror_E(void)
{ //76
	temp16=IMMADDRESS(PC_REG);
	temp8=MemRead8(temp16);
	postbyte=cc[C]<<7;
	cc[C] = temp8 & 1;
	temp8= (temp8>>1) | postbyte;
	cc[Z] = ZTEST(temp8);
	cc[N] = NTEST8(temp8);
	MemWrite8(temp8,temp16);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles76;
}

void Asr_E(void)
{ //77
	temp16=IMMADDRESS(PC_REG);
	temp8=MemRead8(temp16);
	cc[C] = temp8 & 1;
	temp8= (temp8 & 0x80) | (temp8 >>1);
	cc[Z] = ZTEST(temp8);
	cc[N] = NTEST8(temp8);
	MemWrite8(temp8,temp16);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles76;
}

void Asl_E(void)
{ //78 6309
	temp16=IMMADDRESS(PC_REG);
	temp8= MemRead8(temp16);
	cc[C] = temp8 > 0x7F;
	cc[V] = cc[C] ^ ((temp8 & 0x40)>>6);
	temp8= temp8<<1;
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	MemWrite8(temp8,temp16);	
	PC_REG+=2;
	CycleCounter+=NatEmuCycles76;
}

void Rol_E(void)
{ //79
	temp16=IMMADDRESS(PC_REG);
	temp8=MemRead8(temp16);
	postbyte=cc[C];
	cc[C] = temp8 > 0x7F;
	cc[V] = cc[C] ^  ((temp8 & 0x40)>>6);
	temp8 = ((temp8<<1) | postbyte);
	cc[Z] = ZTEST(temp8);
	cc[N] = NTEST8(temp8);
	MemWrite8(temp8,temp16);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles76;
}

void Dec_E(void)
{ //7A
	temp16=IMMADDRESS(PC_REG);
	temp8=MemRead8(temp16);
	temp8--;
	cc[Z] = ZTEST(temp8);
	cc[N] = NTEST8(temp8);
	cc[V] = temp8==0x7F;
	MemWrite8(temp8,temp16);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles76;
}

void Tim_E(void)
{ //7B 6309 NITRO 
	postbyte=MemRead8(PC_REG++);
	temp16=IMMADDRESS(PC_REG);
	postbyte&=MemRead8(temp16);
	cc[N] = NTEST8(postbyte);
	cc[Z] = ZTEST(postbyte);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=7;
}

void Inc_E(void)
{ //7C
	temp16=IMMADDRESS(PC_REG);
	temp8=MemRead8(temp16);
	temp8++;
	cc[Z] = ZTEST(temp8);
	cc[V] = temp8==0x80;
	cc[N] = NTEST8(temp8);
	MemWrite8(temp8,temp16);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles76;
}

void Tst_E(void)
{ //7D
	temp8=MemRead8(IMMADDRESS(PC_REG));
	cc[Z] = ZTEST(temp8);
	cc[N] = NTEST8(temp8);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles75;
}

void Jmp_E(void)
{ //7E
	PC_REG=IMMADDRESS(PC_REG);
	CycleCounter+=NatEmuCycles43;
}

void Clr_E(void)
{ //7F
	MemWrite8(0,IMMADDRESS(PC_REG));
	cc[C] = 0;
	cc[N] = 0;
	cc[V] = 0;
	cc[Z] = 1;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles76;
}

void Suba_M(void)
{ //80
	postbyte=MemRead8(PC_REG++);
	temp16 = A_REG - postbyte;
	cc[C] = (temp16 & 0x100)>>8; 
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,A_REG);
	A_REG = (unsigned char)temp16;
	cc[Z] = ZTEST(A_REG);
	cc[N] = NTEST8(A_REG);
	CycleCounter+=2;
}

void Cmpa_M(void)
{ //81
	postbyte=MemRead8(PC_REG++);
	temp8= A_REG-postbyte;
	cc[C] = temp8 > A_REG;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp8,A_REG);
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	CycleCounter+=2;
}

void Sbca_M(void)
{  //82
	postbyte=MemRead8(PC_REG++);
	temp16=A_REG-postbyte-cc[C];
	cc[C] = (temp16 & 0x100)>>8;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,A_REG);
	A_REG = (unsigned char)temp16;
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	CycleCounter+=2;
}

void Subd_M(void)
{ //83
	temp16=IMMADDRESS(PC_REG);
	temp32=D_REG-temp16;
	cc[C] = (temp32 & 0x10000)>>16;
	cc[V] = OVERFLOW16(cc[C],temp32,temp16,D_REG);
	D_REG = temp32;
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles43;
}

void Anda_M(void)
{ //84
	A_REG = A_REG & MemRead8(PC_REG++);
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	cc[V] = 0;
	CycleCounter+=2;
}

void Bita_M(void)
{ //85
	temp8 = A_REG & MemRead8(PC_REG++);
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	cc[V] = 0;
	CycleCounter+=2;
}

void Lda_M(void)
{ //86
	A_REG = MemRead8(PC_REG++);
	cc[Z] = ZTEST(A_REG);
	cc[N] = NTEST8(A_REG);
	cc[V] = 0;
	CycleCounter+=2;
}

void Eora_M(void)
{ //88
	A_REG = A_REG ^ MemRead8(PC_REG++);
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	cc[V] = 0;
	CycleCounter+=2;
}

void Adca_M(void)
{ //89
	postbyte=MemRead8(PC_REG++);
	temp16= A_REG + postbyte + cc[C];
	cc[C] = (temp16 & 0x100)>>8;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,A_REG);
	cc[H] = ((A_REG ^ temp16 ^ postbyte) & 0x10)>>4;
	A_REG= (unsigned char)temp16;
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	CycleCounter+=2;
}

void Ora_M(void)
{ //8A
	A_REG = A_REG | MemRead8(PC_REG++);
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	cc[V] = 0;
	CycleCounter+=2;
}

void Adda_M(void)
{ //8B
	postbyte=MemRead8(PC_REG++);
	temp16=A_REG+postbyte;
	cc[C] =(temp16 & 0x100)>>8;
	cc[H] = ((A_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,A_REG);
	A_REG= (unsigned char)temp16;
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	CycleCounter+=2;
}

void Cmpx_M(void)
{ //8C
	postword=IMMADDRESS(PC_REG);
	temp16 = X_REG-postword;
	cc[C] = temp16 > X_REG;
	cc[V] = OVERFLOW16(cc[C],postword,temp16,X_REG);
	cc[N] = NTEST16(temp16);
	cc[Z] = ZTEST(temp16);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles43;
}

void Bsr_R(void)
{ //8D
	*spostbyte=MemRead8(PC_REG++);
	S_REG--;
	MemWrite8(pc.B.lsb,S_REG--);
	MemWrite8(pc.B.msb,S_REG);
	PC_REG+=*spostbyte;
	CycleCounter+=NatEmuCycles76;
}

void Ldx_M(void)
{ //8E
	X_REG = IMMADDRESS(PC_REG);
	cc[Z] = ZTEST(X_REG);
	cc[N] = NTEST16(X_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=3;
}

void Suba_D(void)
{ //90
	postbyte=MemRead8( DPADDRESS(PC_REG++));
	temp16 = A_REG - postbyte;
	cc[C] = (temp16 & 0x100)>>8; 
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,A_REG);
	A_REG = (unsigned char)temp16;
	cc[Z] = ZTEST(A_REG);
	cc[N] = NTEST8(A_REG);
	CycleCounter+=NatEmuCycles43;
}	

void Cmpa_D(void)
{ //91
	postbyte=MemRead8(DPADDRESS(PC_REG++));
	temp8 = A_REG-postbyte;
	cc[C] = temp8 > A_REG;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp8,A_REG);
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	CycleCounter+=NatEmuCycles43;
}

void Scba_D(void)
{ //92
	postbyte=MemRead8(DPADDRESS(PC_REG++));
	temp16=A_REG-postbyte-cc[C];
	cc[C] = (temp16 & 0x100)>>8;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,A_REG);
	A_REG = (unsigned char)temp16;
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	CycleCounter+=NatEmuCycles43;
}

void Subd_D(void)
{ //93
	temp16= MemRead16(DPADDRESS(PC_REG++));
	temp32= D_REG-temp16;
	cc[C] = (temp32 & 0x10000)>>16;
	cc[V] = OVERFLOW16(cc[C],temp32,temp16,D_REG);
	D_REG = temp32;
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	CycleCounter+=NatEmuCycles64;
}

void Anda_D(void)
{ //94
	A_REG = A_REG & MemRead8(DPADDRESS(PC_REG++));
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles43;
}

void Bita_D(void)
{ //95
	temp8 = A_REG & MemRead8(DPADDRESS(PC_REG++));
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles43;
}

void Lda_D(void)
{ //96
	A_REG = MemRead8(DPADDRESS(PC_REG++));
	cc[Z] = ZTEST(A_REG);
	cc[N] = NTEST8(A_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles43;
}

void Sta_D(void)
{ //97
	MemWrite8( A_REG,DPADDRESS(PC_REG++));
	cc[Z] = ZTEST(A_REG);
	cc[N] = NTEST8(A_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles43;
}

void Eora_D(void)
{ //98
	A_REG= A_REG ^ MemRead8(DPADDRESS(PC_REG++));
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles43;
}

void Adca_D(void)
{ //99
	postbyte=MemRead8(DPADDRESS(PC_REG++));
	temp16= A_REG + postbyte + cc[C];
	cc[C] = (temp16 & 0x100)>>8;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,A_REG);
	cc[H] = ((A_REG ^ temp16 ^ postbyte) & 0x10)>>4;
	A_REG= (unsigned char)temp16;
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	CycleCounter+=NatEmuCycles43;
}

void Ora_D(void)
{ //9A
	A_REG = A_REG | MemRead8(DPADDRESS(PC_REG++));
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles43;
}

void Adda_D(void)
{ //9B
	postbyte=MemRead8(DPADDRESS(PC_REG++));
	temp16=A_REG+postbyte;
	cc[C] =(temp16 & 0x100)>>8;
	cc[H] = ((A_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,A_REG);
	A_REG= (unsigned char)temp16;
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	CycleCounter+=NatEmuCycles43;
}

void Cmpx_D(void)
{ //9C
	postword=MemRead16(DPADDRESS(PC_REG++));
	temp16= X_REG - postword ;
	cc[C] = temp16 > X_REG;
	cc[V] = OVERFLOW16(cc[C],postword,temp16,X_REG);
	cc[N] = NTEST16(temp16);
	cc[Z] = ZTEST(temp16);
	CycleCounter+=NatEmuCycles64;
}

void Jsr_D(void)
{ //9D
	temp16 = DPADDRESS(PC_REG++);
	S_REG--;
	MemWrite8(pc.B.lsb,S_REG--);
	MemWrite8(pc.B.msb,S_REG);
	PC_REG=temp16;
	CycleCounter+=NatEmuCycles76;
}

void Ldx_D(void)
{ //9E
	X_REG=MemRead16(DPADDRESS(PC_REG++));
	cc[Z] = ZTEST(X_REG);	
	cc[N] = NTEST16(X_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles54;
}

void Stx_D(void)
{ //9F
	MemWrite16(X_REG,DPADDRESS(PC_REG++));
	cc[Z] = ZTEST(X_REG);
	cc[N] = NTEST16(X_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles54;
}

void Suba_X(void)
{ //A0
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp16 = A_REG - postbyte;
	cc[C] =(temp16 & 0x100)>>8; 
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,A_REG);
	A_REG= (unsigned char)temp16;
	cc[Z] = ZTEST(A_REG);
	cc[N] = NTEST8(A_REG);
	CycleCounter+=4;
}	

void Cmpa_X(void)
{ //A1
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp8= A_REG-postbyte;
	cc[C] = temp8 > A_REG;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp8,A_REG);
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	CycleCounter+=4;
}

void Sbca_X(void)
{ //A2
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp16=A_REG-postbyte-cc[C];
	cc[C] = (temp16 & 0x100)>>8;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,A_REG);
	A_REG = (unsigned char)temp16;
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	CycleCounter+=4;
}

void Subd_X(void)
{ //A3
	temp16= MemRead16(INDADDRESS(PC_REG++));
	temp32= D_REG-temp16;
	cc[C] = (temp32 & 0x10000)>>16;
	cc[V] = OVERFLOW16(cc[C],temp32,temp16,D_REG);
	D_REG = temp32;
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	CycleCounter+=NatEmuCycles65;
}

void Anda_X(void)
{ //A4
	A_REG = A_REG & MemRead8(INDADDRESS(PC_REG++));
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	cc[V] = 0;
	CycleCounter+=4;
}

void Bita_X(void)
{  //A5
	temp8 = A_REG & MemRead8(INDADDRESS(PC_REG++));
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	cc[V] = 0;
	CycleCounter+=4;
}

void Lda_X(void)
{ //A6
	A_REG = MemRead8(INDADDRESS(PC_REG++));
	cc[Z] = ZTEST(A_REG);
	cc[N] = NTEST8(A_REG);
	cc[V] = 0;
	CycleCounter+=4;
}

void Sta_X(void)
{ //A7
	MemWrite8(A_REG,INDADDRESS(PC_REG++));
	cc[Z] = ZTEST(A_REG);
	cc[N] = NTEST8(A_REG);
	cc[V] = 0;
	CycleCounter+=4;
}

void Eora_X(void)
{ //A8
	A_REG= A_REG ^ MemRead8(INDADDRESS(PC_REG++));
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	cc[V] = 0;
	CycleCounter+=4;
}

void Adca_X(void)
{ //A9	
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp16= A_REG + postbyte + cc[C];
	cc[C] = (temp16 & 0x100)>>8;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,A_REG);
	cc[H] = ((A_REG ^ temp16 ^ postbyte) & 0x10)>>4;
	A_REG= (unsigned char)temp16;
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	CycleCounter+=4;
}

void Ora_X(void)
{ //AA
	A_REG= A_REG | MemRead8(INDADDRESS(PC_REG++));
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	cc[V] = 0;
	CycleCounter+=4;
}

void Adda_X(void)
{ //AB
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp16= A_REG+postbyte;
	cc[C] = (temp16 & 0x100)>>8;
	cc[H] = ((A_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,A_REG);
	A_REG = (unsigned char)temp16;
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	CycleCounter+=4;
}

void Cmpx_X(void)
{ //AC
	postword=MemRead16(INDADDRESS(PC_REG++));
	temp16= X_REG - postword ;
	cc[C] = temp16 > X_REG;
	cc[V] = OVERFLOW16(cc[C],postword,temp16,X_REG);
	cc[N] = NTEST16(temp16);
	cc[Z] = ZTEST(temp16);
	CycleCounter+=NatEmuCycles65;
}

void Jsr_X(void)
{ //AD
	temp16=INDADDRESS(PC_REG++);
	S_REG--;
	MemWrite8(pc.B.lsb,S_REG--);
	MemWrite8(pc.B.msb,S_REG);
	PC_REG=temp16;
	CycleCounter+=NatEmuCycles76;
}

void Ldx_X(void)
{ //AE
	X_REG=MemRead16(INDADDRESS(PC_REG++));
	cc[Z] = ZTEST(X_REG);
	cc[N] = NTEST16(X_REG);
	cc[V] = 0;
	CycleCounter+=5;
}

void Stx_X(void)
{ //AF
	MemWrite16(X_REG,INDADDRESS(PC_REG++));
	cc[Z] = ZTEST(X_REG);
	cc[N] = NTEST16(X_REG);
	cc[V] = 0;
	CycleCounter+=5;
}

void Suba_E(void)
{ //B0
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp16 = A_REG - postbyte;
	cc[C] = (temp16 & 0x100)>>8; 
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,A_REG);
	A_REG= (unsigned char)temp16;
	cc[Z] = ZTEST(A_REG);
	cc[N] = NTEST8(A_REG);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}

void Cmpa_E(void)
{ //B1
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp8= A_REG-postbyte;
	cc[C] = temp8 > A_REG;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp8,A_REG);
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}

void Sbca_E(void)
{ //B2
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp16=A_REG-postbyte-cc[C];
	cc[C] = (temp16 & 0x100)>>8;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,A_REG);
	A_REG = (unsigned char)temp16;
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}

void Subd_E(void)
{ //B3
	temp16=MemRead16(IMMADDRESS(PC_REG));
	temp32=D_REG-temp16;
	cc[C] = (temp32 & 0x10000)>>16;
	cc[V] = OVERFLOW16(cc[C],temp32,temp16,D_REG);
	D_REG= temp32;
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles75;
}

void Anda_E(void)
{ //B4
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	A_REG = A_REG & postbyte;
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}

void Bita_E(void)
{ //B5
	temp8 = A_REG & MemRead8(IMMADDRESS(PC_REG));
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}

void Lda_E(void)
{ //B6
	A_REG= MemRead8(IMMADDRESS(PC_REG));
	cc[Z] = ZTEST(A_REG);
	cc[N] = NTEST8(A_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}

void Sta_E(void)
{ //B7
	MemWrite8(A_REG,IMMADDRESS(PC_REG));
	cc[Z] = ZTEST(A_REG);
	cc[N] = NTEST8(A_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}

void Eora_E(void)
{  //B8
	A_REG = A_REG ^ MemRead8(IMMADDRESS(PC_REG));
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}

void Adca_E(void)
{ //B9
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp16= A_REG + postbyte + cc[C];
	cc[C] = (temp16 & 0x100)>>8;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,A_REG);
	cc[H] = ((A_REG ^ temp16 ^ postbyte) & 0x10)>>4;
	A_REG= (unsigned char)temp16;
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}

void Ora_E(void)
{ //BA
	A_REG = A_REG | MemRead8(IMMADDRESS(PC_REG));
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}

void Adda_E(void)
{ //BB
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp16=A_REG+postbyte;
	cc[C] = (temp16 & 0x100)>>8;
	cc[H] = ((A_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,A_REG);
	A_REG = (unsigned char)temp16;
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}

void Cmpx_E(void)
{ //BC
	postword=MemRead16(IMMADDRESS(PC_REG));
	temp16 = X_REG-postword;
	cc[C] = temp16 > X_REG;
	cc[V] = OVERFLOW16(cc[C],postword,temp16,X_REG);
	cc[N] = NTEST16(temp16);
	cc[Z] = ZTEST(temp16);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles75;
}

void Bsr_E(void)
{ //BD
	postword=IMMADDRESS(PC_REG);
	PC_REG+=2;
	S_REG--;
	MemWrite8(pc.B.lsb,S_REG--);
	MemWrite8(pc.B.msb,S_REG);
	PC_REG=postword;
	CycleCounter+=NatEmuCycles87;
}

void Ldx_E(void)
{ //BE
	X_REG=MemRead16(IMMADDRESS(PC_REG));
	cc[Z] = ZTEST(X_REG);
	cc[N] = NTEST16(X_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles65;
}

void Stx_E(void)
{ //BF
	MemWrite16(X_REG,IMMADDRESS(PC_REG));
	cc[Z] = ZTEST(X_REG);
	cc[N] = NTEST16(X_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles65;
}

void Subb_M(void)
{ //C0
	postbyte=MemRead8(PC_REG++);
	temp16 = B_REG - postbyte;
	cc[C] = (temp16 & 0x100)>>8; 
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,B_REG);
	B_REG= (unsigned char)temp16;
	cc[Z] = ZTEST(B_REG);
	cc[N] = NTEST8(B_REG);
	CycleCounter+=2;
}

void Cmpb_M(void)
{ //C1
	postbyte=MemRead8(PC_REG++);
	temp8= B_REG-postbyte;
	cc[C] = temp8 > B_REG;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp8,B_REG);
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	CycleCounter+=2;
}

void Sbcb_M(void)
{ //C2
	postbyte=MemRead8(PC_REG++);
	temp16=B_REG-postbyte-cc[C];
	cc[C] = (temp16 & 0x100)>>8;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,B_REG);
	B_REG = (unsigned char)temp16;
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	CycleCounter+=2;
}

void Addd_M(void)
{ //C3
	temp16=IMMADDRESS(PC_REG);
	temp32= D_REG+ temp16;
	cc[C] = (temp32 & 0x10000)>>16;
	cc[V] = OVERFLOW16(cc[C],temp32,temp16,D_REG);
	D_REG= temp32;
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles43;
}

void Andb_M(void)
{ //C4 LOOK
	B_REG = B_REG & MemRead8(PC_REG++);
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	cc[V] = 0;
	CycleCounter+=2;
}

void Bitb_M(void)
{ //C5
	temp8 = B_REG & MemRead8(PC_REG++);
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	cc[V] = 0;
	CycleCounter+=2;
}

void Ldb_M(void)
{ //C6
	B_REG=MemRead8(PC_REG++);
	cc[Z] = ZTEST(B_REG);
	cc[N] = NTEST8(B_REG);
	cc[V] = 0;
	CycleCounter+=2;
}

void Eorb_M(void)
{ //C8
	B_REG = B_REG ^ MemRead8(PC_REG++);
	cc[N] =NTEST8(B_REG);
	cc[Z] =ZTEST(B_REG);
	cc[V] = 0;
	CycleCounter+=2;
}

void Adcb_M(void)
{ //C9
	postbyte=MemRead8(PC_REG++);
	temp16= B_REG + postbyte + cc[C];
	cc[C] = (temp16 & 0x100)>>8;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,B_REG);
	cc[H] = ((B_REG ^ temp16 ^ postbyte) & 0x10)>>4;
	B_REG= (unsigned char)temp16;
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	CycleCounter+=2;
}

void Orb_M(void)
{ //CA
	B_REG= B_REG | MemRead8(PC_REG++);
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	cc[V] = 0;
	CycleCounter+=2;
}

void Addb_M(void)
{ //CB
	postbyte=MemRead8(PC_REG++);
	temp16=B_REG+postbyte;
	cc[C] =(temp16 & 0x100)>>8;
	cc[H] = ((B_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,B_REG);
	B_REG= (unsigned char)temp16;
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	CycleCounter+=2;
}

void Ldd_M(void)
{ //CC
	D_REG=IMMADDRESS(PC_REG);
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=3;
}

void Ldq_M(void)
{ //CD 6309 WORK
	Q_REG = MemRead32(PC_REG);
	PC_REG+=4;
	cc[Z] = ZTEST(Q_REG);
	cc[N] = NTEST32(Q_REG);
	cc[V] = 0;
	CycleCounter+=5;
}

void Ldu_M(void)
{ //CE
	U_REG = IMMADDRESS(PC_REG);
	cc[Z] = ZTEST(U_REG);
	cc[N] = NTEST16(U_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=3;
}

void Subb_D(void)
{ //D0
	postbyte=MemRead8( DPADDRESS(PC_REG++));
	temp16 = B_REG - postbyte;
	cc[C] =(temp16 & 0x100)>>8; 
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,B_REG);
	B_REG = (unsigned char)temp16;
	cc[Z] = ZTEST(B_REG);
	cc[N] = NTEST8(B_REG);
	CycleCounter+=NatEmuCycles43;
}

void Cmpb_D(void)
{ //D1
	postbyte=MemRead8(DPADDRESS(PC_REG++));
	temp8= B_REG-postbyte;
	cc[C] = temp8 > B_REG;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp8,B_REG);
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	CycleCounter+=NatEmuCycles43;
}

void Sbcb_D(void)
{ //D2
	postbyte=MemRead8(DPADDRESS(PC_REG++));
	temp16=B_REG-postbyte-cc[C];
	cc[C] = (temp16 & 0x100)>>8;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,B_REG);
	B_REG = (unsigned char)temp16;
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	CycleCounter+=NatEmuCycles43;
}

void Addd_D(void)
{ //D3
	temp16=MemRead16( DPADDRESS(PC_REG++));
	temp32= D_REG+ temp16;
	cc[C] =(temp32 & 0x10000)>>16;
	cc[V] = OVERFLOW16(cc[C],temp32,temp16,D_REG);
	D_REG= temp32;
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	CycleCounter+=NatEmuCycles64;
}

void Andb_D(void)
{ //D4 
	B_REG = B_REG & MemRead8(DPADDRESS(PC_REG++));
	cc[N] =NTEST8(B_REG);
	cc[Z] =ZTEST(B_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles43;
}

void Bitb_D(void)
{ //D5
	temp8 = B_REG & MemRead8(DPADDRESS(PC_REG++));
	cc[N] = NTEST8(temp8);
	cc[Z] =ZTEST(temp8);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles43;
}

void Ldb_D(void)
{ //D6
	B_REG= MemRead8( DPADDRESS(PC_REG++));
	cc[Z] = ZTEST(B_REG);
	cc[N] = NTEST8(B_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles43;
}

void Stb_D(void)
{ //D7
	MemWrite8( B_REG,DPADDRESS(PC_REG++));
	cc[Z] = ZTEST(B_REG);
	cc[N] = NTEST8(B_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles43;
}

void Eorb_D(void)
{ //D8	
	B_REG = B_REG ^ MemRead8(DPADDRESS(PC_REG++));
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles43;
}

void Adcb_D(void)
{ //D9
	postbyte=MemRead8(DPADDRESS(PC_REG++));
	temp16= B_REG + postbyte + cc[C];
	cc[C] = (temp16 & 0x100)>>8;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,B_REG);
	cc[H] = ((B_REG ^ temp16 ^ postbyte) & 0x10)>>4;
	B_REG = (unsigned char)temp16;
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	CycleCounter+=NatEmuCycles43;
}

void Orb_D(void)
{ //DA
	B_REG = B_REG | MemRead8(DPADDRESS(PC_REG++));
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles43;
}

void Addb_D(void)
{ //DB
	postbyte=MemRead8(DPADDRESS(PC_REG++));
	temp16= B_REG+postbyte;
	cc[C] = (temp16 & 0x100)>>8;
	cc[H] = ((B_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,B_REG);
	B_REG = (unsigned char)temp16;
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	CycleCounter+=NatEmuCycles43;
}

void Ldd_D(void)
{ //DC
	D_REG = MemRead16(DPADDRESS(PC_REG++));
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles54;
}

void Std_D(void)
{ //DD
	MemWrite16(D_REG,DPADDRESS(PC_REG++));
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles54;
}

void Ldu_D(void)
{ //DE
	U_REG = MemRead16(DPADDRESS(PC_REG++));
	cc[Z] = ZTEST(U_REG);
	cc[N] = NTEST16(U_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles54;
}

void Stu_D(void)
{ //DF
	MemWrite16(U_REG,DPADDRESS(PC_REG++));
	cc[Z] = ZTEST(U_REG);
	cc[N] = NTEST16(U_REG);
	cc[V] = 0;
	CycleCounter+=NatEmuCycles54;
}

void Subb_X(void)
{ //E0
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp16 = B_REG - postbyte;
	cc[C] = (temp16 & 0x100)>>8; 
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,B_REG);
	B_REG = (unsigned char)temp16;
	cc[Z] = ZTEST(B_REG);
	cc[N] = NTEST8(B_REG);
	CycleCounter+=4;
}

void Cmpb_X(void)
{ //E1
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp8 = B_REG-postbyte;
	cc[C] = temp8 > B_REG;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp8,B_REG);
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	CycleCounter+=4;
}

void Sbcb_X(void)
{ //E2
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp16=B_REG-postbyte-cc[C];
	cc[C] = (temp16 & 0x100)>>8;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,B_REG);
	B_REG = (unsigned char)temp16;
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	CycleCounter+=4;
}

void Addd_X(void)
{ //E3 
	temp16=MemRead16(INDADDRESS(PC_REG++));
	temp32= D_REG+ temp16;
	cc[C] =(temp32 & 0x10000)>>16;
	cc[V] = OVERFLOW16(cc[C],temp32,temp16,D_REG);
	D_REG= temp32;
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	CycleCounter+=NatEmuCycles65;
}

void Andb_X(void)
{ //E4
	B_REG = B_REG & MemRead8(INDADDRESS(PC_REG++));
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	cc[V] = 0;
	CycleCounter+=4;
}

void Bitb_X(void)
{ //E5 
	temp8 = B_REG & MemRead8(INDADDRESS(PC_REG++));
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	cc[V] = 0;
	CycleCounter+=4;
}

void Ldb_X(void)
{ //E6
	B_REG=MemRead8(INDADDRESS(PC_REG++));
	cc[Z] = ZTEST(B_REG);
	cc[N] = NTEST8(B_REG);
	cc[V] = 0;
	CycleCounter+=4;
}

void Stb_X(void)
{ //E7
	MemWrite8(B_REG,CalculateEA( MemRead8(PC_REG++)));
	cc[Z] = ZTEST(B_REG);
	cc[N] = NTEST8(B_REG);
	cc[V] = 0;
	CycleCounter+=4;
}

void Eorb_X(void)
{ //E8
	B_REG= B_REG ^ MemRead8(INDADDRESS(PC_REG++));
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	cc[V] = 0;
	CycleCounter+=4;
}

void Adcb_X(void)
{ //E9
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp16= B_REG + postbyte + cc[C];
	cc[C] = (temp16 & 0x100)>>8;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,B_REG);
	cc[H] = ((B_REG ^ temp16 ^ postbyte) & 0x10)>>4;
	B_REG= (unsigned char)temp16;
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	CycleCounter+=4;
}

void Orb_X(void)
{ //EA 
	B_REG = B_REG | MemRead8(INDADDRESS(PC_REG++));
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	cc[V] = 0;
	CycleCounter+=4;
}

void Addb_X(void)
{ //EB
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp16=B_REG+postbyte;
	cc[C] =(temp16 & 0x100)>>8;
	cc[H] = ((B_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,B_REG);
	B_REG = (unsigned char)temp16;
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	CycleCounter+=4;
}

void Ldd_X(void)
{ //EC
	D_REG=MemRead16(INDADDRESS(PC_REG++));
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	cc[V] = 0;
	CycleCounter+=5;
}

void Std_X(void)
{ //ED
	MemWrite16(D_REG,INDADDRESS(PC_REG++));
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	cc[V] = 0;
	CycleCounter+=5;
}

void Ldu_X(void)
{ //EE
	U_REG=MemRead16(INDADDRESS(PC_REG++));
	cc[Z] =ZTEST(U_REG);
	cc[N] =NTEST16(U_REG);
	cc[V] = 0;
	CycleCounter+=5;
}

void Stu_X(void)
{ //EF
	MemWrite16(U_REG,INDADDRESS(PC_REG++));
	cc[Z] = ZTEST(U_REG);
	cc[N] = NTEST16(U_REG);
	cc[V] = 0;
	CycleCounter+=5;
}

void Subb_E(void)
{ //F0
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp16 = B_REG - postbyte;
	cc[C] = (temp16 & 0x100)>>8; 
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,B_REG);
	B_REG= (unsigned char)temp16;
	cc[Z] = ZTEST(B_REG);
	cc[N] = NTEST8(B_REG);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}

void Cmpb_E(void)
{ //F1
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp8= B_REG-postbyte;
	cc[C] = temp8 > B_REG;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp8,B_REG);
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}

void Sbcb_E(void)
{ //F2
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp16=B_REG-postbyte-cc[C];
	cc[C] = (temp16 & 0x100)>>8;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,B_REG);
	B_REG = (unsigned char)temp16;
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}

void Addd_E(void)
{ //F3
	temp16=MemRead16(IMMADDRESS(PC_REG));
	temp32= D_REG+ temp16;
	cc[C] =(temp32 & 0x10000)>>16;
	cc[V] = OVERFLOW16(cc[C],temp32,temp16,D_REG);
	D_REG = temp32;
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles76;
}

void Andb_E(void)
{  //F4
	B_REG = B_REG & MemRead8(IMMADDRESS(PC_REG));
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}

void Bitb_E(void)
{ //F5
	temp8 = B_REG & MemRead8(IMMADDRESS(PC_REG));
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}

void Ldb_E(void)
{ //F6
	B_REG=MemRead8(IMMADDRESS(PC_REG));
	cc[Z] = ZTEST(B_REG);
	cc[N] = NTEST8(B_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}

void Stb_E(void)
{ //F7 
	MemWrite8(B_REG,IMMADDRESS(PC_REG));
	cc[Z] = ZTEST(B_REG);
	cc[N] = NTEST8(B_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}

void Eorb_E(void)
{ //F8
	B_REG = B_REG ^ MemRead8(IMMADDRESS(PC_REG));
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}

void Adcb_E(void)
{ //F9
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp16= B_REG + postbyte + cc[C];
	cc[C] = (temp16 & 0x100)>>8;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,B_REG);
	cc[H] = ((B_REG ^ temp16 ^ postbyte) & 0x10)>>4;
	B_REG= (unsigned char)temp16;
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}

void Orb_E(void)
{ //FA
	B_REG = B_REG | MemRead8(IMMADDRESS(PC_REG));
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}

void Addb_E(void)
{ //FB
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp16=B_REG+postbyte;
	cc[C] =(temp16 & 0x100)>>8;
	cc[H] = ((B_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,B_REG);
	B_REG= (unsigned char)temp16;
	cc[N] = NTEST8(B_REG);
	cc[Z] =ZTEST(B_REG);
	PC_REG+=2;
	CycleCounter+=NatEmuCycles54;
}

void Ldd_E(void)
{ //FC
	D_REG=MemRead16(IMMADDRESS(PC_REG));
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles65;
}

void Std_E(void)
{ //FD
	MemWrite16(D_REG,IMMADDRESS(PC_REG));
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles65;
}

void Ldu_E(void)
{ //FE
	U_REG= MemRead16(IMMADDRESS(PC_REG));
	cc[Z] = ZTEST(U_REG);
	cc[N] = NTEST16(U_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles65;
}

void Stu_E(void)
{ //FF
	MemWrite16(U_REG,IMMADDRESS(PC_REG));
	cc[Z] = ZTEST(U_REG);
	cc[N] = NTEST16(U_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=NatEmuCycles65;
}

void(*JmpVec1[256])(void) = {
	Neg_D,		// 00
	Oim_D,		// 01
	Aim_D,		// 02
	Com_D,		// 03
	Lsr_D,		// 04
	Eim_D,		// 05
	Ror_D,		// 06
	Asr_D,		// 07
	Asl_D,		// 08
	Rol_D,		// 09
	Dec_D,		// 0A
	Tim_D,		// 0B
	Inc_D,		// 0C
	Tst_D,		// 0D
	Jmp_D,		// 0E
	Clr_D,		// 0F
	Page_2,		// 10
	Page_3,		// 11
	Nop_I,		// 12
	Sync_I,		// 13
	Sexw_I,		// 14
	InvalidInsHandler,	// 15
	Lbra_R,		// 16
	Lbsr_R,		// 17
	InvalidInsHandler,	// 18
	Daa_I,		// 19
	Orcc_M,		// 1A
	InvalidInsHandler,	// 1B
	Andcc_M,	// 1C
	Sex_I,		// 1D
	Exg_M,		// 1E
	Tfr_M,		// 1F
	Bra_R,		// 20
	Brn_R,		// 21
	Bhi_R,		// 22
	Bls_R,		// 23
	Bhs_R,		// 24
	Blo_R,		// 25
	Bne_R,		// 26
	Beq_R,		// 27
	Bvc_R,		// 28
	Bvs_R,		// 29
	Bpl_R,		// 2A
	Bmi_R,		// 2B
	Bge_R,		// 2C
	Blt_R,		// 2D
	Bgt_R,		// 2E
	Ble_R,		// 2F
	Leax_X,		// 30
	Leay_X,		// 31
	Leas_X,		// 32
	Leau_X,		// 33
	Pshs_M,		// 34
	Puls_M,		// 35
	Pshu_M,		// 36
	Pulu_M,		// 37
	InvalidInsHandler,	// 38
	Rts_I,		// 39
	Abx_I,		// 3A
	Rti_I,		// 3B
	Cwai_I,		// 3C
	Mul_I,		// 3D
	Reset,		// 3E
	Swi1_I,		// 3F
	Nega_I,		// 40
	InvalidInsHandler,	// 41
	InvalidInsHandler,	// 42
	Coma_I,		// 43
	Lsra_I,		// 44
	InvalidInsHandler,	// 45
	Rora_I,		// 46
	Asra_I,		// 47
	Asla_I,		// 48
	Rola_I,		// 49
	Deca_I,		// 4A
	InvalidInsHandler,	// 4B
	Inca_I,		// 4C
	Tsta_I,		// 4D
	InvalidInsHandler,	// 4E
	Clra_I,		// 4F
	Negb_I,		// 50
	InvalidInsHandler,	// 51
	InvalidInsHandler,	// 52
	Comb_I,		// 53
	Lsrb_I,		// 54
	InvalidInsHandler,	// 55
	Rorb_I,		// 56
	Asrb_I,		// 57
	Aslb_I,		// 58
	Rolb_I,		// 59
	Decb_I,		// 5A
	InvalidInsHandler,	// 5B
	Incb_I,		// 5C
	Tstb_I,		// 5D
	InvalidInsHandler,	// 5E
	Clrb_I,		// 5F
	Neg_X,		// 60
	Oim_X,		// 61
	Aim_X,		// 62
	Com_X,		// 63
	Lsr_X,		// 64
	Eim_X,		// 65
	Ror_X,		// 66
	Asr_X,		// 67
	Asl_X,		// 68
	Rol_X,		// 69
	Dec_X,		// 6A
	Tim_X,		// 6B
	Inc_X,		// 6C
	Tst_X,		// 6D
	Jmp_X,		// 6E
	Clr_X,		// 6F
	Neg_E,		// 70
	Oim_E,		// 71
	Aim_E,		// 72
	Com_E,		// 73
	Lsr_E,		// 74
	Eim_E,		// 75
	Ror_E,		// 76
	Asr_E,		// 77
	Asl_E,		// 78
	Rol_E,		// 79
	Dec_E,		// 7A
	Tim_E,		// 7B
	Inc_E,		// 7C
	Tst_E,		// 7D
	Jmp_E,		// 7E
	Clr_E,		// 7F
	Suba_M,		// 80
	Cmpa_M,		// 81
	Sbca_M,		// 82
	Subd_M,		// 83
	Anda_M,		// 84
	Bita_M,		// 85
	Lda_M,		// 86
	InvalidInsHandler,	// 87
	Eora_M,		// 88
	Adca_M,		// 89
	Ora_M,		// 8A
	Adda_M,		// 8B
	Cmpx_M,		// 8C
	Bsr_R,		// 8D
	Ldx_M,		// 8E
	InvalidInsHandler,	// 8F
	Suba_D,		// 90
	Cmpa_D,		// 91
	Scba_D,		// 92
	Subd_D,		// 93
	Anda_D,		// 94
	Bita_D,		// 95
	Lda_D,		// 96
	Sta_D,		// 97
	Eora_D,		// 98
	Adca_D,		// 99
	Ora_D,		// 9A
	Adda_D,		// 9B
	Cmpx_D,		// 9C
	Jsr_D,		// 9D
	Ldx_D,		// 9E
	Stx_D,		// 9A
	Suba_X,		// A0
	Cmpa_X,		// A1
	Sbca_X,		// A2
	Subd_X,		// A3
	Anda_X,		// A4
	Bita_X,		// A5
	Lda_X,		// A6
	Sta_X,		// A7
	Eora_X,		// a8
	Adca_X,		// A9
	Ora_X,		// AA
	Adda_X,		// AB
	Cmpx_X,		// AC
	Jsr_X,		// AD
	Ldx_X,		// AE
	Stx_X,		// AF
	Suba_E,		// B0
	Cmpa_E,		// B1
	Sbca_E,		// B2
	Subd_E,		// B3
	Anda_E,		// B4
	Bita_E,		// B5
	Lda_E,		// B6
	Sta_E,		// B7
	Eora_E,		// B8
	Adca_E,		// B9
	Ora_E,		// BA
	Adda_E,		// BB
	Cmpx_E,		// BC
	Bsr_E,		// BD
	Ldx_E,		// BE
	Stx_E,		// BF
	Subb_M,		// C0
	Cmpb_M,		// C1
	Sbcb_M,		// C2
	Addd_M,		// C3
	Andb_M,		// C4
	Bitb_M,		// C5
	Ldb_M,		// C6
	InvalidInsHandler,		// C7
	Eorb_M,		// C8
	Adcb_M,		// C9
	Orb_M,		// CA
	Addb_M,		// CB
	Ldd_M,		// CC
	Ldq_M,		// CD
	Ldu_M,		// CE
	InvalidInsHandler,		// CF
	Subb_D,		// D0
	Cmpb_D,		// D1
	Sbcb_D,		// D2
	Addd_D,		// D3
	Andb_D,		// D4
	Bitb_D,		// D5
	Ldb_D,		// D6
	Stb_D,		// D7
	Eorb_D,		// D8
	Adcb_D,		// D9
	Orb_D,		// DA
	Addb_D,		// DB
	Ldd_D,		// DC
	Std_D,		// DD
	Ldu_D,		// DE
	Stu_D,		// DF
	Subb_X,		// E0
	Cmpb_X,		// E1
	Sbcb_X,		// E2
	Addd_X,		// E3
	Andb_X,		// E4
	Bitb_X,		// E5
	Ldb_X,		// E6
	Stb_X,		// E7
	Eorb_X,		// E8
	Adcb_X,		// E9
	Orb_X,		// EA
	Addb_X,		// EB
	Ldd_X,		// EC
	Std_X,		// ED
	Ldu_X,		// EE
	Stu_X,		// EF
	Subb_E,		// F0
	Cmpb_E,		// F1
	Sbcb_E,		// F2
	Addd_E,		// F3
	Andb_E,		// F4
	Bitb_E,		// F5
	Ldb_E,		// F6
	Stb_E,		// F7
	Eorb_E,		// F8
	Adcb_E,		// F9
	Orb_E,		// FA
	Addb_E,		// FB
	Ldd_E,		// FC
	Std_E,		// FD
	Ldu_E,		// FE
	Stu_E,		// FF
};

void(*JmpVec2[256])(void) = {
	InvalidInsHandler,		// 00
	InvalidInsHandler,		// 01
	InvalidInsHandler,		// 02
	InvalidInsHandler,		// 03
	InvalidInsHandler,		// 04
	InvalidInsHandler,		// 05
	InvalidInsHandler,		// 06
	InvalidInsHandler,		// 07
	InvalidInsHandler,		// 08
	InvalidInsHandler,		// 09
	InvalidInsHandler,		// 0A
	InvalidInsHandler,		// 0B
	InvalidInsHandler,		// 0C
	InvalidInsHandler,		// 0D
	InvalidInsHandler,		// 0E
	InvalidInsHandler,		// 0F
	InvalidInsHandler,		// 10
	InvalidInsHandler,		// 11
	InvalidInsHandler,		// 12
	InvalidInsHandler,		// 13
	InvalidInsHandler,		// 14
	InvalidInsHandler,		// 15
	InvalidInsHandler,		// 16
	InvalidInsHandler,		// 17
	InvalidInsHandler,		// 18
	InvalidInsHandler,		// 19
	InvalidInsHandler,		// 1A
	InvalidInsHandler,		// 1B
	InvalidInsHandler,		// 1C
	InvalidInsHandler,		// 1D
	InvalidInsHandler,		// 1E
	InvalidInsHandler,		// 1F
	InvalidInsHandler,		// 20
	LBrn_R,		// 21
	LBhi_R,		// 22
	LBls_R,		// 23
	LBhs_R,		// 24
	LBcs_R,		// 25
	LBne_R,		// 26
	LBeq_R,		// 27
	LBvc_R,		// 28
	LBvs_R,		// 29
	LBpl_R,		// 2A
	LBmi_R,		// 2B
	LBge_R,		// 2C
	LBlt_R,		// 2D
	LBgt_R,		// 2E
	LBle_R,		// 2F
	Addr,		// 30
	Adcr,		// 31
	Subr,		// 32
	Sbcr,		// 33
	Andr,		// 34
	Orr,		// 35
	Eorr,		// 36
	Cmpr,		// 37
	Pshsw,		// 38
	Pulsw,		// 39
	Pshuw,		// 3A
	Puluw,		// 3B
	InvalidInsHandler,		// 3C
	InvalidInsHandler,		// 3D
	InvalidInsHandler,		// 3E
	Swi2_I,		// 3F
	Negd_I,		// 40
	InvalidInsHandler,		// 41
	InvalidInsHandler,		// 42
	Comd_I,		// 43
	Lsrd_I,		// 44
	InvalidInsHandler,		// 45
	Rord_I,		// 46
	Asrd_I,		// 47
	Asld_I,		// 48
	Rold_I,		// 49
	Decd_I,		// 4A
	InvalidInsHandler,		// 4B
	Incd_I,		// 4C
	Tstd_I,		// 4D
	InvalidInsHandler,		// 4E
	Clrd_I,		// 4F
	InvalidInsHandler,		// 50
	InvalidInsHandler,		// 51
	InvalidInsHandler,		// 52
	Comw_I,		// 53
	Lsrw_I,		// 54
	InvalidInsHandler,		// 55
	Rorw_I,		// 56
	InvalidInsHandler,		// 57
	InvalidInsHandler,		// 58
	Rolw_I,		// 59
	Decw_I,		// 5A
	InvalidInsHandler,		// 5B
	Incw_I,		// 5C
	Tstw_I,		// 5D
	InvalidInsHandler,		// 5E
	Clrw_I,		// 5F
	InvalidInsHandler,		// 60
	InvalidInsHandler,		// 61
	InvalidInsHandler,		// 62
	InvalidInsHandler,		// 63
	InvalidInsHandler,		// 64
	InvalidInsHandler,		// 65
	InvalidInsHandler,		// 66
	InvalidInsHandler,		// 67
	InvalidInsHandler,		// 68
	InvalidInsHandler,		// 69
	InvalidInsHandler,		// 6A
	InvalidInsHandler,		// 6B
	InvalidInsHandler,		// 6C
	InvalidInsHandler,		// 6D
	InvalidInsHandler,		// 6E
	InvalidInsHandler,		// 6F
	InvalidInsHandler,		// 70
	InvalidInsHandler,		// 71
	InvalidInsHandler,		// 72
	InvalidInsHandler,		// 73
	InvalidInsHandler,		// 74
	InvalidInsHandler,		// 75
	InvalidInsHandler,		// 76
	InvalidInsHandler,		// 77
	InvalidInsHandler,		// 78
	InvalidInsHandler,		// 79
	InvalidInsHandler,		// 7A
	InvalidInsHandler,		// 7B
	InvalidInsHandler,		// 7C
	InvalidInsHandler,		// 7D
	InvalidInsHandler,		// 7E
	InvalidInsHandler,		// 7F
	Subw_M,		// 80
	Cmpw_M,		// 81
	Sbcd_M,		// 82
	Cmpd_M,		// 83
	Andd_M,		// 84
	Bitd_M,		// 85
	Ldw_M,		// 86
	InvalidInsHandler,		// 87
	Eord_M,		// 88
	Adcd_M,		// 89
	Ord_M,		// 8A
	Addw_M,		// 8B
	Cmpy_M,		// 8C
	InvalidInsHandler,		// 8D
	Ldy_M,		// 8E
	InvalidInsHandler,		// 8F
	Subw_D,		// 90
	Cmpw_D,		// 91
	Sbcd_D,		// 92
	Cmpd_D,		// 93
	Andd_D,		// 94
	Bitd_D,		// 95
	Ldw_D,		// 96
	Stw_D,		// 97
	Eord_D,		// 98
	Adcd_D,		// 99
	Ord_D,		// 9A
	Addw_D,		// 9B
	Cmpy_D,		// 9C
	InvalidInsHandler,		// 9D
	Ldy_D,		// 9E
	Sty_D,		// 9F
	Subw_X,		// A0
	Cmpw_X,		// A1
	Sbcd_X,		// A2
	Cmpd_X,		// A3
	Andd_X,		// A4
	Bitd_X,		// A5
	Ldw_X,		// A6
	Stw_X,		// A7
	Eord_X,		// A8
	Adcd_X,		// A9
	Ord_X,		// AA
	Addw_X,		// AB
	Cmpy_X,		// AC
	InvalidInsHandler,		// AD
	Ldy_X,		// AE
	Sty_X,		// AF
	Subw_E,		// B0
	Cmpw_E,		// B1
	Sbcd_E,		// B2
	Cmpd_E,		// B3
	Andd_E,		// B4
	Bitd_E,		// B5
	Ldw_E,		// B6
	Stw_E,		// B7
	Eord_E,		// B8
	Adcd_E,		// B9
	Ord_E,		// BA
	Addw_E,		// BB
	Cmpy_E,		// BC
	InvalidInsHandler,		// BD
	Ldy_E,		// BE
	Sty_E,		// BF
	InvalidInsHandler,		// C0
	InvalidInsHandler,		// C1
	InvalidInsHandler,		// C2
	InvalidInsHandler,		// C3
	InvalidInsHandler,		// C4
	InvalidInsHandler,		// C5
	InvalidInsHandler,		// C6
	InvalidInsHandler,		// C7
	InvalidInsHandler,		// C8
	InvalidInsHandler,		// C9
	InvalidInsHandler,		// CA
	InvalidInsHandler,		// CB
	InvalidInsHandler,		// CC
	InvalidInsHandler,		// CD
	Lds_I,		// CE
	InvalidInsHandler,		// CF
	InvalidInsHandler,		// D0
	InvalidInsHandler,		// D1
	InvalidInsHandler,		// D2
	InvalidInsHandler,		// D3
	InvalidInsHandler,		// D4
	InvalidInsHandler,		// D5
	InvalidInsHandler,		// D6
	InvalidInsHandler,		// D7
	InvalidInsHandler,		// D8
	InvalidInsHandler,		// D9
	InvalidInsHandler,		// DA
	InvalidInsHandler,		// DB
	Ldq_D,		// DC
	Stq_D,		// DD
	Lds_D,		// DE
	Sts_D,		// DF
	InvalidInsHandler,		// E0
	InvalidInsHandler,		// E1
	InvalidInsHandler,		// E2
	InvalidInsHandler,		// E3
	InvalidInsHandler,		// E4
	InvalidInsHandler,		// E5
	InvalidInsHandler,		// E6
	InvalidInsHandler,		// E7
	InvalidInsHandler,		// E8
	InvalidInsHandler,		// E9
	InvalidInsHandler,		// EA
	InvalidInsHandler,		// EB
	Ldq_X,		// EC
	Stq_X,		// ED
	Lds_X,		// EE
	Sts_X,		// EF
	InvalidInsHandler,		// F0
	InvalidInsHandler,		// F1
	InvalidInsHandler,		// F2
	InvalidInsHandler,		// F3
	InvalidInsHandler,		// F4
	InvalidInsHandler,		// F5
	InvalidInsHandler,		// F6
	InvalidInsHandler,		// F7
	InvalidInsHandler,		// F8
	InvalidInsHandler,		// F9
	InvalidInsHandler,		// FA
	InvalidInsHandler,		// FB
	Ldq_E,		// FC
	Stq_E,		// FD
	Lds_E,		// FE
	Sts_E,		// FF
};

void(*JmpVec3[256])(void) = {
	InvalidInsHandler,		// 00
	InvalidInsHandler,		// 01
	InvalidInsHandler,		// 02
	InvalidInsHandler,		// 03
	InvalidInsHandler,		// 04
	InvalidInsHandler,		// 05
	InvalidInsHandler,		// 06
	InvalidInsHandler,		// 07
	InvalidInsHandler,		// 08
	InvalidInsHandler,		// 09
	InvalidInsHandler,		// 0A
	InvalidInsHandler,		// 0B
	InvalidInsHandler,		// 0C
	InvalidInsHandler,		// 0D
	InvalidInsHandler,		// 0E
	InvalidInsHandler,		// 0F
	InvalidInsHandler,		// 10
	InvalidInsHandler,		// 11
	InvalidInsHandler,		// 12
	InvalidInsHandler,		// 13
	InvalidInsHandler,		// 14
	InvalidInsHandler,		// 15
	InvalidInsHandler,		// 16
	InvalidInsHandler,		// 17
	InvalidInsHandler,		// 18
	InvalidInsHandler,		// 19
	InvalidInsHandler,		// 1A
	InvalidInsHandler,		// 1B
	InvalidInsHandler,		// 1C
	InvalidInsHandler,		// 1D
	InvalidInsHandler,		// 1E
	InvalidInsHandler,		// 1F
	InvalidInsHandler,		// 20
	InvalidInsHandler,		// 21
	InvalidInsHandler,		// 22
	InvalidInsHandler,		// 23
	InvalidInsHandler,		// 24
	InvalidInsHandler,		// 25
	InvalidInsHandler,		// 26
	InvalidInsHandler,		// 27
	InvalidInsHandler,		// 28
	InvalidInsHandler,		// 29
	InvalidInsHandler,		// 2A
	InvalidInsHandler,		// 2B
	InvalidInsHandler,		// 2C
	InvalidInsHandler,		// 2D
	InvalidInsHandler,		// 2E
	InvalidInsHandler,		// 2F
	Band,		// 30
	Biand,		// 31
	Bor,		// 32
	Bior,		// 33
	Beor,		// 34
	Bieor,		// 35
	Ldbt,		// 36
	Stbt,		// 37
	Tfm1,		// 38
	Tfm2,		// 39
	Tfm3,		// 3A
	Tfm4,		// 3B
	Bitmd_M,	// 3C
	Ldmd_M,		// 3D
	InvalidInsHandler,		// 3E
	Swi3_I,		// 3F
	InvalidInsHandler,		// 40
	InvalidInsHandler,		// 41
	InvalidInsHandler,		// 42
	Come_I,		// 43
	InvalidInsHandler,		// 44
	InvalidInsHandler,		// 45
	InvalidInsHandler,		// 46
	InvalidInsHandler,		// 47
	InvalidInsHandler,		// 48
	InvalidInsHandler,		// 49
	Dece_I,		// 4A
	InvalidInsHandler,		// 4B
	Ince_I,		// 4C
	Tste_I,		// 4D
	InvalidInsHandler,		// 4E
	Clre_I,		// 4F
	InvalidInsHandler,		// 50
	InvalidInsHandler,		// 51
	InvalidInsHandler,		// 52
	Comf_I,		// 53
	InvalidInsHandler,		// 54
	InvalidInsHandler,		// 55
	InvalidInsHandler,		// 56
	InvalidInsHandler,		// 57
	InvalidInsHandler,		// 58
	InvalidInsHandler,		// 59
	Decf_I,		// 5A
	InvalidInsHandler,		// 5B
	Incf_I,		// 5C
	Tstf_I,		// 5D
	InvalidInsHandler,		// 5E
	Clrf_I,		// 5F
	InvalidInsHandler,		// 60
	InvalidInsHandler,		// 61
	InvalidInsHandler,		// 62
	InvalidInsHandler,		// 63
	InvalidInsHandler,		// 64
	InvalidInsHandler,		// 65
	InvalidInsHandler,		// 66
	InvalidInsHandler,		// 67
	InvalidInsHandler,		// 68
	InvalidInsHandler,		// 69
	InvalidInsHandler,		// 6A
	InvalidInsHandler,		// 6B
	InvalidInsHandler,		// 6C
	InvalidInsHandler,		// 6D
	InvalidInsHandler,		// 6E
	InvalidInsHandler,		// 6F
	InvalidInsHandler,		// 70
	InvalidInsHandler,		// 71
	InvalidInsHandler,		// 72
	InvalidInsHandler,		// 73
	InvalidInsHandler,		// 74
	InvalidInsHandler,		// 75
	InvalidInsHandler,		// 76
	InvalidInsHandler,		// 77
	InvalidInsHandler,		// 78
	InvalidInsHandler,		// 79
	InvalidInsHandler,		// 7A
	InvalidInsHandler,		// 7B
	InvalidInsHandler,		// 7C
	InvalidInsHandler,		// 7D
	InvalidInsHandler,		// 7E
	InvalidInsHandler,		// 7F
	Sube_M,		// 80
	Cmpe_M,		// 81
	InvalidInsHandler,		// 82
	Cmpu_M,		// 83
	InvalidInsHandler,		// 84
	InvalidInsHandler,		// 85
	Lde_M,		// 86
	InvalidInsHandler,		// 87
	InvalidInsHandler,		// 88
	InvalidInsHandler,		// 89
	InvalidInsHandler,		// 8A
	Adde_M,		// 8B
	Cmps_M,		// 8C
	Divd_M,		// 8D
	Divq_M,		// 8E
	Muld_M,		// 8F
	Sube_D,		// 90
	Cmpe_D,		// 91
	InvalidInsHandler,		// 92
	Cmpu_D,		// 93
	InvalidInsHandler,		// 94
	InvalidInsHandler,		// 95
	Lde_D,		// 96
	Ste_D,		// 97
	InvalidInsHandler,		// 98
	InvalidInsHandler,		// 99
	InvalidInsHandler,		// 9A
	Adde_D,		// 9B
	Cmps_D,		// 9C
	Divd_D,		// 9D
	Divq_D,		// 9E
	Muld_D,		// 9F
	Sube_X,		// A0
	Cmpe_X,		// A1
	InvalidInsHandler,		// A2
	Cmpu_X,		// A3
	InvalidInsHandler,		// A4
	InvalidInsHandler,		// A5
	Lde_X,		// A6
	Ste_X,		// A7
	InvalidInsHandler,		// A8
	InvalidInsHandler,		// A9
	InvalidInsHandler,		// AA
	Adde_X,		// AB
	Cmps_X,		// AC
	Divd_X,		// AD
	Divq_X,		// AE
	Muld_X,		// AF
	Sube_E,		// B0
	Cmpe_E,		// B1
	InvalidInsHandler,		// B2
	Cmpu_E,		// B3
	InvalidInsHandler,		// B4
	InvalidInsHandler,		// B5
	Lde_E,		// B6
	Ste_E,		// B7
	InvalidInsHandler,		// B8
	InvalidInsHandler,		// B9
	InvalidInsHandler,		// BA
	Adde_E,		// BB
	Cmps_E,		// BC
	Divd_E,		// BD
	Divq_E,		// BE
	Muld_E,		// BF
	Subf_M,		// C0
	Cmpf_M,		// C1
	InvalidInsHandler,		// C2
	InvalidInsHandler,		// C3
	InvalidInsHandler,		// C4
	InvalidInsHandler,		// C5
	Ldf_M,		// C6
	InvalidInsHandler,		// C7
	InvalidInsHandler,		// C8
	InvalidInsHandler,		// C9
	InvalidInsHandler,		// CA
	Addf_M,		// CB
	InvalidInsHandler,		// CC
	InvalidInsHandler,		// CD
	InvalidInsHandler,		// CE
	InvalidInsHandler,		// CF
	Subf_D,		// D0
	Cmpf_D,		// D1
	InvalidInsHandler,		// D2
	InvalidInsHandler,		// D3
	InvalidInsHandler,		// D4
	InvalidInsHandler,		// D5
	Ldf_D,		// D6
	Stf_D,		// D7
	InvalidInsHandler,		// D8
	InvalidInsHandler,		// D9
	InvalidInsHandler,		// DA
	Addf_D,		// DB
	InvalidInsHandler,		// DC
	InvalidInsHandler,		// DD
	InvalidInsHandler,		// DE
	InvalidInsHandler,		// DF
	Subf_X,		// E0
	Cmpf_X,		// E1
	InvalidInsHandler,		// E2
	InvalidInsHandler,		// E3
	InvalidInsHandler,		// E4
	InvalidInsHandler,		// E5
	Ldf_X,		// E6
	Stf_X,		// E7
	InvalidInsHandler,		// E8
	InvalidInsHandler,		// E9
	InvalidInsHandler,		// EA
	Addf_X,		// EB
	InvalidInsHandler,		// EC
	InvalidInsHandler,		// ED
	InvalidInsHandler,		// EE
	InvalidInsHandler,		// EF
	Subf_E,		// F0
	Cmpf_E,		// F1
	InvalidInsHandler,		// F2
	InvalidInsHandler,		// F3
	InvalidInsHandler,		// F4
	InvalidInsHandler,		// F5
	Ldf_E,		// F6
	Stf_E,		// F7
	InvalidInsHandler,		// F8
	InvalidInsHandler,		// F9
	InvalidInsHandler,		// FA
	Addf_E,		// FB
	InvalidInsHandler,		// FC
	InvalidInsHandler,		// FD
	InvalidInsHandler,		// FE
	InvalidInsHandler,		// FF
};

// static unsigned char op1, op2;
// static unsigned short opstack[16];
// static unsigned short addrstack[16];
// static short int stckidx = 0;

int HD6309Exec(int CycleFor)
{

	//static unsigned char opcode = 0;
	CycleCounter = 0;
	gCycleFor = CycleFor;
	while (CycleCounter < CycleFor) {

		if (PendingInterupts)
		{
			if (PendingInterupts & 4)
				cpu_nmi();

			if (PendingInterupts & 2)
				cpu_firq();

			if (PendingInterupts & 1)
			{
				if (IRQWaiter == 0)	// This is needed to fix a subtle timming problem
					cpu_irq();		// It allows the CPU to see $FF03 bit 7 high before
				else				// The IRQ is asserted.
					IRQWaiter -= 1;
			}
		}

		if (SyncWaiting == 1)	//Abort the run nothing happens asyncronously from the CPU
			return(0); // WDZ - Experimental SyncWaiting should still return used cycles (and not zero) by breaking from loop

		JmpVec1[MemRead8(PC_REG++)](); // Execute instruction pointed to by PC_REG
	}//End While

	return(CycleFor - CycleCounter);
}

void Page_2(void) //10
{
	JmpVec2[MemRead8(PC_REG++)](); // Execute instruction pointed to by PC_REG
}

void Page_3(void) //11
{
	JmpVec3[MemRead8(PC_REG++)](); // Execute instruction pointed to by PC_REG
}

void cpu_firq(void)
{
	
	if (!cc[F])
	{
		InInterupt=1; //Flag to indicate FIRQ has been asserted
		switch (md[FIRQMODE])
		{
		case 0:
			cc[E]=0; // Turn E flag off
			MemWrite8( pc.B.lsb,--S_REG);
			MemWrite8( pc.B.msb,--S_REG);
			MemWrite8(getcc(),--S_REG);
			cc[I]=1;
			cc[F]=1;
			PC_REG=MemRead16(VFIRQ);
		break;

		case 1:		//6309
			cc[E]=1;
			MemWrite8( pc.B.lsb,--S_REG);
			MemWrite8( pc.B.msb,--S_REG);
			MemWrite8( u.B.lsb,--S_REG);
			MemWrite8( u.B.msb,--S_REG);
			MemWrite8( y.B.lsb,--S_REG);
			MemWrite8( y.B.msb,--S_REG);
			MemWrite8( x.B.lsb,--S_REG);
			MemWrite8( x.B.msb,--S_REG);
			MemWrite8( dp.B.msb,--S_REG);
			if (md[NATIVE6309])
			{
				MemWrite8((F_REG),--S_REG);
				MemWrite8((E_REG),--S_REG);
			}
			MemWrite8(B_REG,--S_REG);
			MemWrite8(A_REG,--S_REG);
			MemWrite8(getcc(),--S_REG);
			cc[I]=1;
			cc[F]=1;
			PC_REG=MemRead16(VFIRQ);
		break;
		}
	}
	PendingInterupts=PendingInterupts & 253;
	return;
}

void cpu_irq(void)
{
	if (InInterupt==1) //If FIRQ is running postpone the IRQ
		return;			
	if ((!cc[I]) )
	{
		cc[E]=1;
		MemWrite8( pc.B.lsb,--S_REG);
		MemWrite8( pc.B.msb,--S_REG);
		MemWrite8( u.B.lsb,--S_REG);
		MemWrite8( u.B.msb,--S_REG);
		MemWrite8( y.B.lsb,--S_REG);
		MemWrite8( y.B.msb,--S_REG);
		MemWrite8( x.B.lsb,--S_REG);
		MemWrite8( x.B.msb,--S_REG);
		MemWrite8( dp.B.msb,--S_REG);
		if (md[NATIVE6309])
		{
			MemWrite8((F_REG),--S_REG);
			MemWrite8((E_REG),--S_REG);
		}
		MemWrite8(B_REG,--S_REG);
		MemWrite8(A_REG,--S_REG);
		MemWrite8(getcc(),--S_REG);
		PC_REG=MemRead16(VIRQ);
		cc[I]=1; 
	} //Fi I test
	PendingInterupts=PendingInterupts & 254;
	return;
}

void cpu_nmi(void)
{
	cc[E]=1;
	MemWrite8( pc.B.lsb,--S_REG);
	MemWrite8( pc.B.msb,--S_REG);
	MemWrite8( u.B.lsb,--S_REG);
	MemWrite8( u.B.msb,--S_REG);
	MemWrite8( y.B.lsb,--S_REG);
	MemWrite8( y.B.msb,--S_REG);
	MemWrite8( x.B.lsb,--S_REG);
	MemWrite8( x.B.msb,--S_REG);
	MemWrite8( dp.B.msb,--S_REG);
	if (md[NATIVE6309])
	{
		MemWrite8((F_REG),--S_REG);
		MemWrite8((E_REG),--S_REG);
	}
	MemWrite8(B_REG,--S_REG);
	MemWrite8(A_REG,--S_REG);
	MemWrite8(getcc(),--S_REG);
	cc[I]=1;
	cc[F]=1;
	PC_REG=MemRead16(VNMI);
	PendingInterupts=PendingInterupts & 251;
	return;
}

static unsigned short CalculateEA(unsigned char postbyte)
{
	static unsigned short int ea = 0;
	static signed char byte = 0;
	static unsigned char Register;

	Register = ((postbyte >> 5) & 3) + 1;

	if (postbyte & 0x80)
	{
		switch (postbyte & 0x1F)
		{
		case 0: // Post inc by 1
			ea = (*xfreg16[Register]);
			(*xfreg16[Register])++;
			CycleCounter += NatEmuCycles21;
			break;

		case 1: // post in by 2
			ea = (*xfreg16[Register]);
			(*xfreg16[Register]) += 2;
			CycleCounter += NatEmuCycles32;
			break;

		case 2: // pre dec by 1
			(*xfreg16[Register]) -= 1;
			ea = (*xfreg16[Register]);
			CycleCounter += NatEmuCycles21;
			break;

		case 3: // pre dec by 2
			(*xfreg16[Register]) -= 2;
			ea = (*xfreg16[Register]);
			CycleCounter += NatEmuCycles32;
			break;

		case 4: // no offset
			ea = (*xfreg16[Register]);
			break;

		case 5: // B reg offset
			ea = (*xfreg16[Register]) + ((signed char)B_REG);
			CycleCounter += 1;
			break;

		case 6: // A reg offset
			ea = (*xfreg16[Register]) + ((signed char)A_REG);
			CycleCounter += 1;
			break;

		case 7: // E reg offset 
			ea = (*xfreg16[Register]) + ((signed char)E_REG);
			CycleCounter += 1;
			break;

		case 8: // 8 bit offset
			ea = (*xfreg16[Register]) + (signed char)MemRead8(PC_REG++);
			CycleCounter += 1;
			break;

		case 9: // 16 bit offset
			ea = (*xfreg16[Register]) + IMMADDRESS(PC_REG);
			CycleCounter += NatEmuCycles43;
			PC_REG += 2;
			break;

		case 10: // F reg offset
			ea = (*xfreg16[Register]) + ((signed char)F_REG);
			CycleCounter += 1;
			break;

		case 11: // D reg offset 
			ea = (*xfreg16[Register]) + D_REG; //Changed to unsigned 03/14/2005 NG Was signed
			CycleCounter += NatEmuCycles42;
			break;

		case 12: // 8 bit PC relative
			ea = (signed short)PC_REG + (signed char)MemRead8(PC_REG) + 1;
			CycleCounter += 1;
			PC_REG++;
			break;

		case 13: // 16 bit PC relative
			ea = PC_REG + IMMADDRESS(PC_REG) + 2;
			CycleCounter += NatEmuCycles53;
			PC_REG += 2;
			break;

		case 14: // W reg offset
			ea = (*xfreg16[Register]) + W_REG;
			CycleCounter += 4;
			break;

		case 15: // W reg
			byte = (postbyte >> 5) & 3;
			switch (byte)
			{
			case 0: // No offset from W reg
				ea = W_REG;
				break;
			case 1: // 16 bit offset from W reg
				ea = W_REG + IMMADDRESS(PC_REG);
				PC_REG += 2;
				CycleCounter += 2;
				break;
			case 2: // Post inc by 2 from W reg
				ea = W_REG;
				W_REG += 2;
				CycleCounter += 1;
				break;
			case 3: // Pre dec by 2 from W reg
				W_REG -= 2;
				ea = W_REG;
				CycleCounter += 1;
				break;
			}
			break;

		case 16: // W reg
			byte = (postbyte >> 5) & 3;
			switch (byte)
			{
			case 0: // Indirect no offset from W reg
				ea = MemRead16(W_REG);
				CycleCounter += 3;
				break;
			case 1: // Indirect 16 bit offset from W reg
				ea = MemRead16(W_REG + IMMADDRESS(PC_REG));
				PC_REG += 2;
				CycleCounter += 5;
				break;
			case 2: // Indirect post inc by 2 from W reg
				ea = MemRead16(W_REG);
				W_REG += 2;
				CycleCounter += 4;
				break;
			case 3: // Indirect pre dec by 2 from W reg
				W_REG -= 2;
				ea = MemRead16(W_REG);
				CycleCounter += 4;
				break;
			}
			break;

		case 17: // Indirect post inc by 2 
			ea = (*xfreg16[Register]);
			(*xfreg16[Register]) += 2;
			ea = MemRead16(ea);
			CycleCounter += 6;
			break;

		case 18: // possibly illegal instruction
			CycleCounter += 6;
			break;

		case 19: // Indirect pre dec by 2
			(*xfreg16[Register]) -= 2;
			ea = (*xfreg16[Register]);
			ea = MemRead16(ea);
			CycleCounter += 6;
			break;

		case 20: // Indirect no offset 
			ea = (*xfreg16[Register]);
			ea = MemRead16(ea);
			CycleCounter += 3;
			break;

		case 21: // Indirect B reg offset
			ea = (*xfreg16[Register]) + ((signed char)B_REG);
			ea = MemRead16(ea);
			CycleCounter += 4;
			break;

		case 22: // indirect A reg offset
			ea = (*xfreg16[Register]) + ((signed char)A_REG);
			ea = MemRead16(ea);
			CycleCounter += 4;
			break;

		case 23: // indirect E reg offset
			ea = (*xfreg16[Register]) + ((signed char)E_REG);
			ea = MemRead16(ea);
			CycleCounter += 4;
			break;

		case 24: // indirect 8 bit offset
			ea = (*xfreg16[Register]) + (signed char)MemRead8(PC_REG++);
			ea = MemRead16(ea);
			CycleCounter += 4;
			break;

		case 25: // indirect 16 bit offset
			ea = (*xfreg16[Register]) + IMMADDRESS(PC_REG);
			ea = MemRead16(ea);
			CycleCounter += 7;
			PC_REG += 2;
			break;

		case 26: // indirect F reg offset
			ea = (*xfreg16[Register]) + ((signed char)F_REG);
			ea = MemRead16(ea);
			CycleCounter += 4;
			break;

		case 27: // indirect D reg offset
			ea = (*xfreg16[Register]) + D_REG;
			ea = MemRead16(ea);
			CycleCounter += 7;
			break;

		case 28: // indirect 8 bit PC relative
			ea = (signed short)PC_REG + (signed char)MemRead8(PC_REG) + 1;
			ea = MemRead16(ea);
			CycleCounter += 4;
			PC_REG++;
			break;

		case 29: //indirect 16 bit PC relative
			ea = PC_REG + IMMADDRESS(PC_REG) + 2;
			ea = MemRead16(ea);
			CycleCounter += 8;
			PC_REG += 2;
			break;

		case 30: // indirect W reg offset
			ea = (*xfreg16[Register]) + W_REG;
			ea = MemRead16(ea);
			CycleCounter += 7;
			break;

		case 31: // extended indirect
			ea = IMMADDRESS(PC_REG);
			ea = MemRead16(ea);
			CycleCounter += 8;
			PC_REG += 2;
			break;

		} //END Switch
	}
	else // 5 bit offset
	{
		byte = (postbyte & 31);
		byte = (byte << 3);
		byte = byte / 8;
		ea = *xfreg16[Register] + byte; //Was signed
		CycleCounter += 1;
	}
	return(ea);
}

void setcc (unsigned char bincc)
{
	unsigned char bit;
	ccbits = bincc;
	for (bit=0;bit<=7;bit++)
		cc[bit]=!!(bincc & (1<<bit));
	return;
}

unsigned char getcc(void)
{
	unsigned char bincc=0,bit=0;
	for (bit=0;bit<=7;bit++)
		if (cc[bit])
			bincc=bincc | (1<<bit);
		return(bincc);
}

void setmd (unsigned char binmd)
{
	unsigned char bit;
	for (bit=0;bit<=1;bit++)
		md[bit]=!!(binmd & (1<<bit));
	//if (md[NATIVE6309]) 
	//{
	//	SetNatEmuStat(2);
	//}
	//else 
	//{
	//	SetNatEmuStat(1);
	//}
	for(short i = 0 ; i < 24 ; i++)
	{
		*NatEmuCycles[i] = InsCycles[md[NATIVE6309]][i];
	}
	return;
}

unsigned char getmd(void)
{
	unsigned char binmd=0,bit=0;
	for (bit=6;bit<=7;bit++)
		if (md[bit])
			binmd=binmd | (1<<bit);
		return(binmd);
}
	
void HD6309AssertInterupt(unsigned char Interupt,unsigned char waiter)// 4 nmi 2 firq 1 irq
{
	SyncWaiting=0;
	PendingInterupts=PendingInterupts | (1<<(Interupt-1));
	IRQWaiter=waiter;
	return;
}

void HD6309DeAssertInterupt(unsigned char Interupt)// 4 nmi 2 firq 1 irq
{
	PendingInterupts=PendingInterupts & ~(1<<(Interupt-1));
	InInterupt=0;
	return;
}

void InvalidInsHandler(void)
{	
	// fprintf(stderr, "Illegal instruction %02x %02x \n", (int)op1, (int)op2);
	// extern void dumpMem(UINT16, UINT16);
	// dumpMem(PC_REG-8, (UINT16)16);
	// for(short i = 0 ; i > -16 ; i--)
	// {
	// 	fprintf(stderr, "(%04x %04x)", addrstack[stckidx+i], opstack[stckidx+i]);
	// }
	// fprintf(stderr, "\n");
	md[ILLEGAL]=1;
	mdbits=getmd();
	ErrorVector();
	return;
}

void DivbyZero(void)
{
	md[ZERODIV]=1;
	mdbits=getmd();
	ErrorVector();
	return;
}

void ErrorVector(void)
{
	cc[E]=1;
	MemWrite8( pc.B.lsb,--S_REG);
	MemWrite8( pc.B.msb,--S_REG);
	MemWrite8( u.B.lsb,--S_REG);
	MemWrite8( u.B.msb,--S_REG);
	MemWrite8( y.B.lsb,--S_REG);
	MemWrite8( y.B.msb,--S_REG);
	MemWrite8( x.B.lsb,--S_REG);
	MemWrite8( x.B.msb,--S_REG);
	MemWrite8( dp.B.msb,--S_REG);
	if (md[NATIVE6309])
	{
		MemWrite8((F_REG),--S_REG);
		MemWrite8((E_REG),--S_REG);
		CycleCounter+=2;
	}
	MemWrite8(B_REG,--S_REG);
	MemWrite8(A_REG,--S_REG);
	MemWrite8(getcc(),--S_REG);
	PC_REG=MemRead16(VTRAP);
	CycleCounter+=(12 + NatEmuCycles54);	//One for each byte +overhead? Guessing from PSHS
	return;
}

unsigned char GetSorceReg(unsigned char Tmp)
{
	unsigned char Source=(Tmp>>4);
	unsigned char Dest= Tmp & 15;
	unsigned char Translate[]={0,0};
	if ( (Source & 8) == (Dest & 8) ) //like size registers
		return(Source );
return(0);
}

void HD6309ForcePC(unsigned short NewPC)
{
	PC_REG=NewPC;
	PendingInterupts=0;
	SyncWaiting=0;
	return;
}

/*****************************************************************
* 32 bit memory handling routines                                *
*****************************************************************/

unsigned int MemRead32(unsigned short addr)
{
	return (MemRead16(addr) << 16 | MemRead16(addr + 2));
}

void MemWrite32(unsigned int data, unsigned short addr)
{
	MemWrite16(data >> 16, addr);
	MemWrite16(data & 0xFFFF, addr + 2);
	return;
}


// unsigned short GetPC(void)
// {
// 	return(PC_REG);
// }



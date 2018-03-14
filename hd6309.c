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

#include "windows.h"
#include "stdio.h"
#include "hd6309.h"
#include "hd6309defs.h"
#include "tcc1014mmu.h"
#include "logger.h"


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
static cpuregister pc,x,y,u,s,dp,v,z;
static unsigned char InsCycles[2][25];
static unsigned int cc[8];
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
static int stempW;
static unsigned char temp8; 
static unsigned char PendingInterupts=0;
static unsigned char IRQWaiter=0;
static unsigned char Source=0,Dest=0;
static unsigned char postbyte=0;
static short unsigned postword=0;
static signed char *spostbyte=(signed char *)&postbyte;
static signed short *spostword=(signed short *)&postword;
static char InInterupt=0;
//END Global variables for CPU Emulation-------------------

//Fuction Prototypes---------------------------------------
static unsigned short CalculateEA(unsigned char);
void InvalidInsHandler(void);
void DivbyZero(void);
void ErrorVector(void);
static void setcc (unsigned char);
static unsigned char getcc(void);
static void setmd (unsigned char);
static unsigned char getmd(void);
static void cpu_firq(void);
static void cpu_irq(void);
static void cpu_nmi(void);
unsigned int MemRead32(unsigned short);
void MemWrite32(unsigned int,unsigned short);
unsigned char GetSorceReg(unsigned char);
int performDivQ(short divisor);

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
	xfreg16[0]=&D_REG;
	xfreg16[1]=&X_REG;
	xfreg16[2]=&Y_REG;
	xfreg16[3]=&U_REG;
	xfreg16[4]=&S_REG;
	xfreg16[5]=&PC_REG;
	xfreg16[6]=&W_REG;
	xfreg16[7]=&V_REG;

	ureg8[0]=(unsigned char*)&A_REG;		
	ureg8[1]=(unsigned char*)&B_REG;		
	ureg8[2]=(unsigned char*)&ccbits;
	ureg8[3]=(unsigned char*)&dp.B.msb;
	ureg8[4]=(unsigned char*)&O_REG;
	ureg8[5]=(unsigned char*)&O_REG;
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
	cc[I]=1;
	cc[F]=1;
	return;
}

int HD6309Exec( int CycleFor)
{
static unsigned char opcode=0;
static unsigned char msn,lsn;
CycleCounter=0; 
while (CycleCounter<CycleFor) {

	if (PendingInterupts)
	{
		if (PendingInterupts & 4)
			cpu_nmi();

		if (PendingInterupts & 2)
			cpu_firq();

		if (PendingInterupts & 1)
		{
			if (IRQWaiter==0)	// This is needed to fix a subtle timming problem
				cpu_irq();		// It allows the CPU to see $FF03 bit 7 high before
			else				// The IRQ is asserted.
				IRQWaiter-=1;
		}
	}

	if (SyncWaiting==1)	//Abort the run nothing happens asyncronously from the CPU
		return(0);

switch (MemRead8(PC_REG++)){

case NEG_D: //0
	temp16 = DPADDRESS(PC_REG++);
	postbyte=MemRead8(temp16);
	temp8=0-postbyte;
	cc[C] =temp8>0;
	cc[V] = (postbyte==0x80);
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	MemWrite8(temp8,temp16);
	CycleCounter+=InsCycles[md[NATIVE6309]][M65];
	break;

case OIM_D://1 6309
	postbyte=MemRead8(PC_REG++);
	temp16 = DPADDRESS(PC_REG++);
	postbyte|= MemRead8(temp16);
	MemWrite8(postbyte,temp16);
	cc[N] = NTEST8(postbyte);
	cc[Z] = ZTEST(postbyte);
	cc[V] = 0;
	CycleCounter+=6;
	break;

case AIM_D://2 Phase 2 6309
	postbyte=MemRead8(PC_REG++);
	temp16 = DPADDRESS(PC_REG++);
	postbyte&= MemRead8(temp16);
	MemWrite8(postbyte,temp16);
	cc[N] = NTEST8(postbyte);
	cc[Z] = ZTEST(postbyte);
	cc[V] = 0;
	CycleCounter+=6;
	break;

case COM_D:
	temp16 = DPADDRESS(PC_REG++);
	temp8=MemRead8(temp16);
	temp8=0xFF-temp8;
	cc[Z] = ZTEST(temp8);
	cc[N] = NTEST8(temp8); 
	cc[C] = 1;
	cc[V] = 0;
	MemWrite8(temp8,temp16);
	CycleCounter+=InsCycles[md[NATIVE6309]][M65];
	break;

case LSR_D: //S2
	temp16 = DPADDRESS(PC_REG++);
	temp8 = MemRead8(temp16);
	cc[C] = temp8 & 1;
	temp8 = temp8 >>1;
	cc[Z] = ZTEST(temp8);
	cc[N] = 0;
	MemWrite8(temp8,temp16);
	CycleCounter+=InsCycles[md[NATIVE6309]][M65];
	break;

case EIM_D: //6309 Untested
	postbyte=MemRead8(PC_REG++);
	temp16 = DPADDRESS(PC_REG++);
	postbyte^= MemRead8(temp16);
	MemWrite8(postbyte,temp16);
	cc[N] = NTEST8(postbyte);
	cc[Z] = ZTEST(postbyte);
	cc[V] = 0;
	CycleCounter+=6;
	break;

case ROR_D: //S2
	temp16 = DPADDRESS(PC_REG++);
	temp8=MemRead8(temp16);
	postbyte= cc[C]<<7;
	cc[C] = temp8 & 1;
	temp8 = (temp8 >> 1)| postbyte;
	cc[Z] = ZTEST(temp8);
	cc[N] = NTEST8(temp8);
	MemWrite8(temp8,temp16);
	CycleCounter+=InsCycles[md[NATIVE6309]][M65];
	break;

case ASR_D: //7
	temp16 = DPADDRESS(PC_REG++);
	temp8=MemRead8(temp16);
	cc[C] = temp8 & 1;
	temp8 = (temp8 & 0x80) | (temp8 >>1);
	cc[Z] = ZTEST(temp8);
	cc[N] = NTEST8(temp8);
	MemWrite8(temp8,temp16);
	CycleCounter+=InsCycles[md[NATIVE6309]][M65];
	break;

case ASL_D: //8 
	temp16 = DPADDRESS(PC_REG++);
	temp8=MemRead8(temp16);
	cc[C] = (temp8 & 0x80) >>7;
	cc[V] = cc[C] ^ ((temp8 & 0x40) >> 6);
	temp8 = temp8 <<1;
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	MemWrite8(temp8,temp16);
	CycleCounter+=InsCycles[md[NATIVE6309]][M65];
	break;

case ROL_D:	//9
	temp16 = DPADDRESS(PC_REG++);
	temp8 = MemRead8(temp16);
	postbyte=cc[C];
	cc[C] =(temp8 & 0x80)>>7;
	cc[V] = cc[C] ^ ((temp8 & 0x40) >>6);
	temp8 = (temp8<<1) | postbyte;
	cc[Z] = ZTEST(temp8);
	cc[N] = NTEST8(temp8);
	MemWrite8(temp8,temp16);
	CycleCounter+=InsCycles[md[NATIVE6309]][M65];
	break;

case DEC_D: //A
	temp16 = DPADDRESS(PC_REG++);
	temp8 = MemRead8(temp16)-1;
	cc[Z] = ZTEST(temp8);
	cc[N] = NTEST8(temp8);
	cc[V] = temp8==0x7F;
	MemWrite8(temp8,temp16);
	CycleCounter+=InsCycles[md[NATIVE6309]][M65];
	break;

case TIM_D:	//B 6309 Untested wcreate
	postbyte=MemRead8(PC_REG++);
	temp8=MemRead8(DPADDRESS(PC_REG++));
	postbyte&=temp8;
	cc[N] = NTEST8(postbyte);
	cc[Z] = ZTEST(postbyte);
	cc[V] = 0;
	CycleCounter+=6;
	break;

case INC_D: //C
	temp16=(DPADDRESS(PC_REG++));
	temp8 = MemRead8(temp16)+1;
	cc[Z] = ZTEST(temp8);
	cc[V] = temp8==0x80;
	cc[N] = NTEST8(temp8);
	MemWrite8(temp8,temp16);
	CycleCounter+=InsCycles[md[NATIVE6309]][M65];
	break;

case TST_D: //D
	temp8 = MemRead8(DPADDRESS(PC_REG++));
	cc[Z] = ZTEST(temp8);
	cc[N] = NTEST8(temp8);
	cc[V] = 0;
	CycleCounter+=InsCycles[md[NATIVE6309]][M64];
	break;

case JMP_D:	//E
	PC_REG= ((dp.Reg |MemRead8(PC_REG)));
	CycleCounter+=InsCycles[md[NATIVE6309]][M32];
	break;

case CLR_D:	//F
	MemWrite8(0,DPADDRESS(PC_REG++));
	cc[Z] = 1;
	cc[N] = 0;
	cc[V] = 0;
	cc[C] = 0;
	CycleCounter+=InsCycles[md[NATIVE6309]][M65];
	break;

case Page2:
		switch (MemRead8(PC_REG++)) 
		{

		case LBEQ_R: //1027
			if (cc[Z])
			{
				*spostword=IMMADDRESS(PC_REG);
				PC_REG+=*spostword;
				CycleCounter+=1;
			}
			PC_REG+=2;
			CycleCounter+=5;
			break;

		case LBRN_R: //1021
			PC_REG+=2;
			CycleCounter+=5;
			break;

		case LBHI_R: //1022
			if  (!(cc[C] | cc[Z]))
			{
				*spostword=IMMADDRESS(PC_REG);
				PC_REG+=*spostword;
				CycleCounter+=1;
			}
			PC_REG+=2;
			CycleCounter+=5;
			break;

		case LBLS_R: //1023
			if (cc[C] | cc[Z])
			{
				*spostword=IMMADDRESS(PC_REG);
				PC_REG+=*spostword;
				CycleCounter+=1;
			}
			PC_REG+=2;
			CycleCounter+=5;
			break;

		case LBHS_R: //1024
			if (!cc[C])
			{
				*spostword=IMMADDRESS(PC_REG);
				PC_REG+=*spostword;
				CycleCounter+=1;
			}
			PC_REG+=2;
			CycleCounter+=6;
			break;

		case LBCS_R: //1025
			if (cc[C])
			{
				*spostword=IMMADDRESS(PC_REG);
				PC_REG+=*spostword;
				CycleCounter+=1;
			}
			PC_REG+=2;
			CycleCounter+=5;
			break;

		case LBNE_R: //1026
			if (!cc[Z])
			{
				*spostword=IMMADDRESS(PC_REG);
				PC_REG+=*spostword;
				CycleCounter+=1;
			}
			PC_REG+=2;
			CycleCounter+=5;
			break;

		case LBVC_R: //1028
			if ( !cc[V])
			{
				*spostword=IMMADDRESS(PC_REG);
				PC_REG+=*spostword;
				CycleCounter+=1;
			}
			PC_REG+=2;
			CycleCounter+=5;
			break;

		case LBVS_R: //1029
			if ( cc[V])
			{
				*spostword=IMMADDRESS(PC_REG);
				PC_REG+=*spostword;
				CycleCounter+=1;
			}
			PC_REG+=2;
			CycleCounter+=5;
			break;

		case LBPL_R: //102A
		if (!cc[N])
			{
				*spostword=IMMADDRESS(PC_REG);
				PC_REG+=*spostword;
				CycleCounter+=1;
			}
			PC_REG+=2;
			CycleCounter+=5;
			break;

		case LBMI_R: //102B
		if ( cc[N])
			{
				*spostword=IMMADDRESS(PC_REG);
				PC_REG+=*spostword;
				CycleCounter+=1;
			}
			PC_REG+=2;
			CycleCounter+=5;
			break;

		case LBGE_R: //102C
			if (! (cc[N] ^ cc[V]))
			{
				*spostword=IMMADDRESS(PC_REG);
				PC_REG+=*spostword;
				CycleCounter+=1;
			}
			PC_REG+=2;
			CycleCounter+=5;
			break;

		case LBLT_R: //102D
			if ( cc[V] ^ cc[N])
			{
				*spostword=IMMADDRESS(PC_REG);
				PC_REG+=*spostword;
				CycleCounter+=1;
			}
			PC_REG+=2;
			CycleCounter+=5;
			break;

		case LBGT_R: //102E
			if ( !( cc[Z] | (cc[N]^cc[V] ) ))
			{
				*spostword=IMMADDRESS(PC_REG);
				PC_REG+=*spostword;
				CycleCounter+=1;
			}
			PC_REG+=2;
			CycleCounter+=5;
			break;

		case LBLE_R:	//102F
			if ( cc[Z] | (cc[N]^cc[V]) )
			{
				*spostword=IMMADDRESS(PC_REG);
				PC_REG+=*spostword;
				CycleCounter+=1;
			}
			PC_REG+=2;
			CycleCounter+=5;
			break;

		case ADDR: //1030 6309 CC? NITRO 8 bit code
			temp8=MemRead8(PC_REG++);
			Source= temp8>>4;
			Dest=temp8 & 15;

			if ( (Source>7) & (Dest>7) )
			{
				temp16= *ureg8[Source & 7] + *ureg8[Dest & 7];
				cc[C] = (temp16 & 0x100)>>8;
				cc[V] = OVERFLOW8(cc[C],*ureg8[Source & 7],*ureg8[Dest & 7],temp16);
				*ureg8[Dest & 7]=(temp16 & 0xFF);
				cc[N] = NTEST8(*ureg8[Dest & 7]);	
				cc[Z] = ZTEST(*ureg8[Dest & 7]);
			}
			else
			{
				temp32= *xfreg16[Source] + *xfreg16[Dest];
				cc[C] = (temp32 & 0x10000)>>16;
				cc[V] = OVERFLOW16(cc[C],*xfreg16[Source],*xfreg16[Dest],temp32);
				*xfreg16[Dest]=(temp32 & 0xFFFF);
				cc[N] = NTEST16(*xfreg16[Dest]);
				cc[Z] = ZTEST(*xfreg16[Dest]);
			}
			cc[H] =0;
			CycleCounter+=4;
		break;

		case ADCR: //1031 6309
			WriteLog("Hitting UNEMULATED INS ADCR",TOCONS);
			CycleCounter+=4;
		break;

		case SUBR: //1032 6309
			temp8=MemRead8(PC_REG++);
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
						postword=*xfreg16[Source];

					temp32=  *xfreg16[Dest] - postword;
					cc[C] =(temp32 & 0x10000)>>16;
					cc[V] =!!(((*xfreg16[Dest])^postword^temp32^(temp32>>1)) & 0x8000);
					cc[N] =(temp32 & 0x8000)>>15;	
					cc[Z] = !temp32;
					*xfreg16[Dest]=temp32;
				break;

				case 8:
				case 9:
				case 10:
				case 11:
				case 14:
				case 15:
					if (Source>=8)
					{
						temp8= *ureg8[Dest&7] - *ureg8[Source&7];
						cc[C] = temp8 > *ureg8[Dest&7];
						cc[V] = cc[C] ^ ( ((*ureg8[Dest&7])^temp8^(*ureg8[Source&7]))>>7);
						cc[N] = temp8>>7;
						cc[Z] = !temp8;
						*ureg8[Dest&7]=temp8;
					}
				break;

			}
			CycleCounter+=4;
		break;

		case SBCR: //1033 6309
			WriteLog("Hitting UNEMULATED INS SBCR",TOCONS);
			CycleCounter+=4;
		break;

		case ANDR: //1034 6309 Untested wcreate
			temp8=MemRead8(PC_REG++);
			Source=temp8>>4;
			Dest=temp8 & 15;

			if ( (Source >=8) & (Dest >=8) )
			{
				(*ureg8[(Dest & 7)])&=(*ureg8[(Source & 7)]);
				cc[N] = (*ureg8[(Dest & 7)]) >>7;
				cc[Z] = !(*ureg8[(Dest & 7)]);
			}
			else
			{
				(*xfreg16[Dest])&=(*xfreg16[Source]);
				cc[N] = (*xfreg16[Dest]) >>15;
				cc[Z] = !(*xfreg16[Dest]);
			}
			cc[V] = 0;
			CycleCounter+=4;
		break;

		case ORR: //1035 6309
			temp8=MemRead8(PC_REG++);
			Source=temp8>>4;
			Dest=temp8 & 15;

			if ( (Source >=8) & (Dest >=8) )
			{
				(*ureg8[(Dest & 7)])|=(*ureg8[(Source & 7)]);
				cc[N] = (*ureg8[(Dest & 7)]) >>7;
				cc[Z] = !(*ureg8[(Dest & 7)]);
			}
			else
			{
				(*xfreg16[Dest])|=(*xfreg16[Source]);
				cc[N] = (*xfreg16[Dest]) >>15;
				cc[Z] = !(*xfreg16[Dest]);
			}
			cc[V] = 0;
			CycleCounter+=4;
		break;

		case EORR: //1036 6309
			temp8=MemRead8(PC_REG++);
			Source=temp8>>4;
			Dest=temp8 & 15;

			if ( (Source >=8) & (Dest >=8) )
			{
				(*ureg8[(Dest & 7)])^=(*ureg8[(Source & 7)]);
				cc[N] = (*ureg8[(Dest & 7)]) >>7;
				cc[Z] = !(*ureg8[(Dest & 7)]);
			}
			else
			{
				(*xfreg16[Dest])^=(*xfreg16[Source]);
				cc[N] = (*xfreg16[Dest]) >>15;
				cc[Z] = !(*xfreg16[Dest]);
			}
			cc[V] = 0;
			CycleCounter+=4;
		break;

		case CMPR: //1037 6309
			temp8=MemRead8(PC_REG++);
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
						postword=*xfreg16[Source];

					temp16=   (*xfreg16[Dest]) - postword;
					cc[C] = temp16 > *xfreg16[Dest];
					cc[V] = cc[C]^(( (*xfreg16[Dest])^temp16^postword)>>15);
					cc[N] = (temp16 >> 15);
					cc[Z] = !temp16;
				break;

				case 8:
				case 9:
				case 10:
				case 11:
				case 14:
				case 15:
					if (Source>=8)
					{
						temp8= *ureg8[Dest&7] - *ureg8[Source&7];
						cc[C] = temp8 > *ureg8[Dest&7];
						cc[V] = cc[C] ^ ( ((*ureg8[Dest&7])^temp8^(*ureg8[Source&7]))>>7);
						cc[N] = temp8>>7;
						cc[Z] = !temp8;
					}
				break;

			}
			CycleCounter+=4;
		break;

		case PSHSW: //1038 DONE 6309
			MemWrite8((F_REG),--S_REG);
			MemWrite8((E_REG),--S_REG);
			CycleCounter+=6;
		break;

		case PULSW:	//1039 6309 Untested wcreate
			E_REG=MemRead8( S_REG++);
			F_REG=MemRead8( S_REG++);
			CycleCounter+=6;
		break;

		case PSHUW: //103A 6309 Untested
			MemWrite8((F_REG),--U_REG);
			MemWrite8((E_REG),--U_REG);
			CycleCounter+=6;
		break;

		case PULUW: //103B 6309 Untested
			E_REG=MemRead8( U_REG++);
			F_REG=MemRead8( U_REG++);
			CycleCounter+=6;
		break;

		case SWI2_I: //103F
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
		break;

		case NEGD_I: //1040 Phase 5 6309
			temp16= 0-D_REG;
			cc[C] = temp16>0;
			cc[V] = D_REG==0x8000;
			cc[N] = NTEST16(temp16);
			cc[Z] = ZTEST(temp16);
			D_REG= temp16;
			CycleCounter+=InsCycles[md[NATIVE6309]][M32];
		break;

		case COMD_I: //1043 6309
			D_REG = 0xFFFF- D_REG;
			cc[Z] = ZTEST(D_REG);
			cc[N] = NTEST16(D_REG);
			cc[C] = 1;
			cc[V] = 0;
			CycleCounter+=InsCycles[md[NATIVE6309]][M32];
		break;

		case LSRD_I: //1044 6309
			cc[C] = D_REG & 1;
			D_REG = D_REG>>1;
			cc[Z] = ZTEST(D_REG);
			cc[N] = 0;
			CycleCounter+=InsCycles[md[NATIVE6309]][M32];
		break;

		case RORD_I: //1046 6309 Untested
			postword=cc[C]<<15;
			cc[C] = D_REG & 1;
			D_REG = (D_REG>>1) | postword;
			cc[Z] = ZTEST(D_REG);
			cc[N] = NTEST16(D_REG);
			CycleCounter+=InsCycles[md[NATIVE6309]][M32];
		break;

		case ASRD_I: //1047 6309 Untested TESTED NITRO MULTIVUE
			cc[C] = D_REG & 1;
			D_REG = (D_REG & 0x8000) | (D_REG >> 1);
			cc[Z] = ZTEST(D_REG);
			cc[N] = NTEST16(D_REG);
			CycleCounter+=InsCycles[md[NATIVE6309]][M32];
		break;

		case ASLD_I: //1048 6309
			cc[C] = D_REG >>15;
			cc[V] =  cc[C] ^((D_REG & 0x4000)>>14);
			D_REG = D_REG<<1;
			cc[N] = NTEST16(D_REG);
			cc[Z] = ZTEST(D_REG);
			CycleCounter+=InsCycles[md[NATIVE6309]][M32];
		break;

		case ROLD_I: //1049 6309 Untested
			postword=cc[C];
			cc[C] = D_REG >>15;
			cc[V] = cc[C] ^ ((D_REG & 0x4000)>>14);
			D_REG= (D_REG<<1) | postword;
			cc[Z] = ZTEST(D_REG);
			cc[N] = NTEST16(D_REG);
			CycleCounter+=InsCycles[md[NATIVE6309]][M32];
		break;

		case DECD_I: //104A 6309
			D_REG--;
			cc[Z] = ZTEST(D_REG);
			cc[V] = D_REG==0x7FFF;
			cc[N] = NTEST16(D_REG);
			CycleCounter+=InsCycles[md[NATIVE6309]][M32];
		break;

		case INCD_I: //104C 6309
			D_REG++;
			cc[Z] = ZTEST(D_REG);
			cc[V] = D_REG==0x8000;
			cc[N] = NTEST16(D_REG);
			CycleCounter+=InsCycles[md[NATIVE6309]][M32];
		break;

		case TSTD_I: //104D 6309
			cc[Z] = ZTEST(D_REG);
			cc[N] = NTEST16(D_REG);
			cc[V] = 0;
			CycleCounter+=InsCycles[md[NATIVE6309]][M32];
		break;

		case CLRD_I: //104F 6309
			D_REG= 0;
			cc[C] = 0;
			cc[V] = 0;
			cc[N] = 0;
			cc[Z] = 1;
			CycleCounter+=InsCycles[md[NATIVE6309]][M32];
		break;

		case COMW_I: //1053 6309 Untested
			W_REG= 0xFFFF- W_REG;
			cc[Z] = ZTEST(W_REG);
			cc[N] = NTEST16(W_REG);
			cc[C] = 1;
			cc[V] = 0;
			CycleCounter+=InsCycles[md[NATIVE6309]][M32];
			break;

		case LSRW_I: //1054 6309 Untested
			cc[C] = W_REG & 1;
			W_REG= W_REG>>1;
			cc[Z] = ZTEST(W_REG);
			cc[N] = 0;
			CycleCounter+=InsCycles[md[NATIVE6309]][M32];
		break;

		case RORW_I: //1056 6309 Untested
			postword=cc[C]<<15;
			cc[C] = W_REG & 1;
			W_REG= (W_REG>>1) | postword;
			cc[Z] = ZTEST(W_REG);
			cc[N] = NTEST16(W_REG);
			CycleCounter+=InsCycles[md[NATIVE6309]][M32];
		break;

		case ROLW_I: //1059 6309
			postword=cc[C];
			cc[C] = W_REG >>15;
			cc[V] = cc[C] ^ ((W_REG & 0x4000)>>14);
			W_REG= ( W_REG<<1) | postword;
			cc[Z] = ZTEST(W_REG);
			cc[N] = NTEST16(W_REG);
			CycleCounter+=InsCycles[md[NATIVE6309]][M32];
		break;

		case DECW_I: //105A 6309
			W_REG--;
			cc[Z] = ZTEST(W_REG);
			cc[V] = W_REG==0x7FFF;
			cc[N] = NTEST16(W_REG);
			CycleCounter+=InsCycles[md[NATIVE6309]][M32];
		break;

		case INCW_I: //105C 6309
			W_REG++;
			cc[Z] = ZTEST(W_REG);
			cc[V] = W_REG==0x8000;
			cc[N] = NTEST16(W_REG);
			CycleCounter+=InsCycles[md[NATIVE6309]][M32];
		break;

		case TSTW_I: //105D Untested 6309 wcreate
			cc[Z] = ZTEST(W_REG);
			cc[N] = NTEST16(W_REG);
			cc[V] = 0;	
			CycleCounter+=InsCycles[md[NATIVE6309]][M32];
		break;

		case CLRW_I: //105F 6309
			W_REG = 0;
			cc[C] = 0;
			cc[V] = 0;
			cc[N] = 0;
			cc[Z] = 1;
			CycleCounter+=InsCycles[md[NATIVE6309]][M32];
		break;

		case SUBW_M: //1080 6309 CHECK
			postword=IMMADDRESS(PC_REG);
			temp16= W_REG-postword;
			cc[C] = temp16 > W_REG;
			cc[V] = OVERFLOW16(cc[C],temp16,W_REG,postword);
			cc[N] = NTEST16(temp16);
			cc[Z] = ZTEST(temp16);
			W_REG= temp16;
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M54];
		break;

		case CMPW_M: //1081 6309 CHECK
			postword=IMMADDRESS(PC_REG);
			temp16= W_REG-postword;
			cc[C] = temp16 > W_REG;
			cc[V] = OVERFLOW16(cc[C],temp16,W_REG,postword);
			cc[N] = NTEST16(temp16);
			cc[Z] = ZTEST(temp16);
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M54];
		break;

		case SBCD_M: //1082 P6309
			postword=IMMADDRESS(PC_REG);
			temp32=D_REG-postword-cc[C];
			cc[C] = (temp32 & 0x10000)>>16;
			cc[V] = OVERFLOW16(cc[C],temp32,D_REG,postword);
			D_REG= temp32;
			cc[N] = NTEST16(D_REG);
			cc[Z] = ZTEST(D_REG);
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M54];
		break;

		case CMPD_M: //1083
			postword=IMMADDRESS(PC_REG);
			temp16 = D_REG-postword;
			cc[C] = temp16 > D_REG;
			cc[V] = OVERFLOW16(cc[C],postword,temp16,D_REG);
			cc[N] = NTEST16(temp16);
			cc[Z] = ZTEST(temp16);
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M54];
			break;

		case ANDD_M: //1084 6309
			D_REG &= IMMADDRESS(PC_REG);
			cc[N] = NTEST16(D_REG);
			cc[Z] = ZTEST(D_REG);
			cc[V] = 0;
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M54];
		break;

		case BITD_M: //1085 6309 Untested
			temp16= D_REG & IMMADDRESS(PC_REG);
			cc[N] = NTEST16(temp16);
			cc[Z] = ZTEST(temp16);
			cc[V] = 0;
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M54];
		break;

		case LDW_M: //1086 6309
			W_REG=IMMADDRESS(PC_REG);
			cc[Z] = ZTEST(W_REG);
			cc[N] = NTEST16(W_REG);
			cc[V] = 0;
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M54];
		break;

		case EORD_M: //1088 6309 Untested
			D_REG ^= IMMADDRESS(PC_REG);
			cc[N] = NTEST16(D_REG);
			cc[Z] = ZTEST(D_REG);
			cc[V] = 0;
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M54];
		break;

		case ADCD_M: //1089 6309
			postword=IMMADDRESS(PC_REG);
			temp32= D_REG + postword + cc[C];
			cc[C] = (temp32 & 0x10000)>>16;
			cc[V] = OVERFLOW16(cc[C],postword,temp32,D_REG);
			cc[H] = ((D_REG ^ temp32 ^ postword) & 0x100)>>8;
			D_REG = temp32;
			cc[N] = NTEST16(D_REG);
			cc[Z] = ZTEST(D_REG);
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M54];
		break;

		case ORD_M: //108A 6309 Untested
			D_REG |= IMMADDRESS(PC_REG);
			cc[N] = NTEST16(D_REG);
			cc[Z] = ZTEST(D_REG);
			cc[V] = 0;
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M54];
		break;

		case ADDW_M: //108B Phase 5 6309
			temp16=IMMADDRESS(PC_REG);
			temp32= W_REG+ temp16;
			cc[C] = (temp32 & 0x10000)>>16;
			cc[V] = OVERFLOW16(cc[C],temp32,temp16,W_REG);
			W_REG = temp32;
			cc[Z] = ZTEST(W_REG);
			cc[N] = NTEST16(W_REG);
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M54];
		break;

		case CMPY_M: //108C
			postword=IMMADDRESS(PC_REG);
			temp16 = Y_REG-postword;
			cc[C] = temp16 > Y_REG;
			cc[V] = OVERFLOW16(cc[C],postword,temp16,Y_REG);
			cc[N] = NTEST16(temp16);
			cc[Z] = ZTEST(temp16);
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M54];
			break;
	
		case LDY_M: //108E
			Y_REG = IMMADDRESS(PC_REG);
			cc[Z] = ZTEST(Y_REG);
			cc[N] = NTEST16(Y_REG);
			cc[V] = 0;
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M54];
			break;

		case SUBW_D: //1090 Untested 6309
			temp16= MemRead16(DPADDRESS(PC_REG++));
			temp32= W_REG-temp16;
			cc[C] = (temp32 & 0x10000)>>16;
			cc[V] = OVERFLOW16(cc[C],temp32,temp16,W_REG);
			W_REG = temp32;
			cc[Z] = ZTEST(W_REG);
			cc[N] = NTEST16(W_REG);
			CycleCounter+=InsCycles[md[NATIVE6309]][M75];
		break;

		case CMPW_D: //1091 6309 Untested
			postword=MemRead16(DPADDRESS(PC_REG++));
			temp16= W_REG - postword ;
			cc[C] = temp16 > W_REG;
			cc[V] = OVERFLOW16(cc[C],postword,temp16,W_REG); 
			cc[N] = NTEST16(temp16);
			cc[Z] = ZTEST(temp16);
			CycleCounter+=InsCycles[md[NATIVE6309]][M75];
		break;

		case SBCD_D: //1092 6309
			WriteLog("Hitting UNEMULATED INS SBCD_D",TOCONS);
			CycleCounter+=InsCycles[md[NATIVE6309]][M75];
		break;

		case CMPD_D: //1093
			postword=MemRead16(DPADDRESS(PC_REG++));
			temp16= D_REG - postword ;
			cc[C] = temp16 > D_REG;
			cc[V] = OVERFLOW16(cc[C],postword,temp16,D_REG); 
			cc[N] = NTEST16(temp16);
			cc[Z] = ZTEST(temp16);
			CycleCounter+=InsCycles[md[NATIVE6309]][M75];
			break;

		case ANDD_D: //1094 6309 Untested
			postword=MemRead16(DPADDRESS(PC_REG++));
			D_REG&=postword;
			cc[N] = NTEST16(D_REG);
			cc[Z] = ZTEST(D_REG);
			cc[V] = 0;
			CycleCounter+=InsCycles[md[NATIVE6309]][M75];
		break;

		case BITD_D: //1095 6309 Untested
			temp16= D_REG & MemRead16(DPADDRESS(PC_REG++));
			cc[N] = NTEST16(temp16);
			cc[Z] = ZTEST(temp16);
			cc[V] = 0;
			CycleCounter+=InsCycles[md[NATIVE6309]][M75];
		break;

		case LDW_D: //1096 6309
			W_REG = MemRead16(DPADDRESS(PC_REG++));
			cc[Z] = ZTEST(W_REG);
			cc[N] = NTEST16(W_REG);
			cc[V] = 0;
			CycleCounter+=InsCycles[md[NATIVE6309]][M65];
		break;

		case STW_D: //1097 6309
			MemWrite16(W_REG,DPADDRESS(PC_REG++));
			cc[Z] = ZTEST(W_REG);
			cc[N] = NTEST16(W_REG);
			cc[V] = 0;
			CycleCounter+=InsCycles[md[NATIVE6309]][M65];
		break;

		case EORD_D: //1098 6309 Untested
			D_REG^=MemRead16(DPADDRESS(PC_REG++));
			cc[N] = NTEST16(D_REG);
			cc[Z] = ZTEST(D_REG);
			cc[V] = 0;
			CycleCounter+=InsCycles[md[NATIVE6309]][M75];
		break;

		case ADCD_D: //1099 6309
			WriteLog("Hitting UNEMULATED INS ADCD_D",TOCONS);
			CycleCounter+=InsCycles[md[NATIVE6309]][M75];
		
		break;

		case ORD_D: //109A 6309 Untested
			D_REG|=MemRead16(DPADDRESS(PC_REG++));
			cc[N] = NTEST16(D_REG);
			cc[Z] = ZTEST(D_REG);
			cc[V] = 0;
			CycleCounter+=InsCycles[md[NATIVE6309]][M75];
		break;

		case ADDW_D: //109B 6309
			temp16=MemRead16( DPADDRESS(PC_REG++));
			temp32= W_REG+ temp16;
			cc[C] =(temp32 & 0x10000)>>16;
			cc[V] = OVERFLOW16(cc[C],temp32,temp16,W_REG);
			W_REG = temp32;
			cc[Z] = ZTEST(W_REG);
			cc[N] = NTEST16(W_REG);
			CycleCounter+=InsCycles[md[NATIVE6309]][M75];
		break;

		case CMPY_D:	//109C
			postword=MemRead16(DPADDRESS(PC_REG++));
			temp16= Y_REG - postword ;
			cc[C] = temp16 > Y_REG;
			cc[V] = OVERFLOW16(cc[C],postword,temp16,Y_REG);
			cc[N] = NTEST16(temp16);
			cc[Z] = ZTEST(temp16);
			CycleCounter+=InsCycles[md[NATIVE6309]][M75];
			break;

		case LDY_D: //109E
			Y_REG=MemRead16(DPADDRESS(PC_REG++));
			cc[Z] = ZTEST(Y_REG);	
			cc[N] = NTEST16(Y_REG);
			cc[V] = 0;
			CycleCounter+=InsCycles[md[NATIVE6309]][M65];
			break;
	
		case STY_D: //109F
			MemWrite16(Y_REG,DPADDRESS(PC_REG++));
			cc[Z] = ZTEST(Y_REG);
			cc[N] = NTEST16(Y_REG);
			cc[V] = 0;
			CycleCounter+=InsCycles[md[NATIVE6309]][M65];
		break;

		case SUBW_X: //10A0 6309 MODDED
			temp16=MemRead16(INDADDRESS(PC_REG++));
			temp32=W_REG-temp16;
			cc[C] = (temp32 & 0x10000)>>16;
			cc[V] = OVERFLOW16(cc[C],temp32,temp16,W_REG);
			W_REG= temp32;
			cc[Z] = ZTEST(W_REG);
			cc[N] = NTEST16(W_REG);
			CycleCounter+=InsCycles[md[NATIVE6309]][M76];
		break;

		case CMPW_X: //10A1 6309
			postword=MemRead16(INDADDRESS(PC_REG++));
			temp16= W_REG - postword ;
			cc[C] = temp16 > W_REG;
			cc[V] = OVERFLOW16(cc[C],postword,temp16,W_REG);
			cc[N] = NTEST16(temp16);
			cc[Z] = ZTEST(temp16);
			CycleCounter+=InsCycles[md[NATIVE6309]][M76];
		break;

		case SBCD_X: //10A2 6309
			postword=MemRead16(INDADDRESS(PC_REG++));
			temp32=D_REG-postword-cc[C];
			cc[C] = (temp32 & 0x10000)>>16;
			cc[V] = OVERFLOW16(cc[C],postword,temp32,D_REG);
			D_REG= temp32;
			cc[N] = NTEST16(D_REG);
			cc[Z] = ZTEST(D_REG);
			CycleCounter+=InsCycles[md[NATIVE6309]][M76];
		break;

		case CMPD_X: //10A3
			postword=MemRead16(INDADDRESS(PC_REG++));
			temp16= D_REG - postword ;
			cc[C] = temp16 > D_REG;
			cc[V] = OVERFLOW16(cc[C],postword,temp16,D_REG);
			cc[N] = NTEST16(temp16);
			cc[Z] = ZTEST(temp16);
			CycleCounter+=InsCycles[md[NATIVE6309]][M76];
		break;

		case ANDD_X: //10A4 6309
			D_REG&=MemRead16(INDADDRESS(PC_REG++));
			cc[N] = NTEST16(D_REG);
			cc[Z] = ZTEST(D_REG);
			cc[V] = 0;
			CycleCounter+=InsCycles[md[NATIVE6309]][M76];
		break;

		case BITD_X: //10A5 6309 Untested
			temp16= D_REG & MemRead16(INDADDRESS(PC_REG++));
			cc[N] = NTEST16(temp16);
			cc[Z] = ZTEST(temp16);
			cc[V] = 0;
			CycleCounter+=InsCycles[md[NATIVE6309]][M76];
		break;

		case LDW_X: //10A6 6309
			W_REG=MemRead16(INDADDRESS(PC_REG++));
			cc[Z] = ZTEST(W_REG);
			cc[N] = NTEST16(W_REG);
			cc[V] = 0;
			CycleCounter+=6;
		break;

		case STW_X: //10A7 6309
			MemWrite16(W_REG,INDADDRESS(PC_REG++));
			cc[Z] = ZTEST(W_REG);
			cc[N] = NTEST16(W_REG);
			cc[V] = 0;
			CycleCounter+=6;
		break;

		case EORD_X: //10A8 6309 Untested TESTED NITRO 
			D_REG ^= MemRead16(INDADDRESS(PC_REG++));
			cc[N] = NTEST16(D_REG);
			cc[Z] = ZTEST(D_REG);
			cc[V] = 0;
			CycleCounter+=InsCycles[md[NATIVE6309]][M76];
		break;

		case ADCD_X: //10A9 6309
			postword=MemRead16(INDADDRESS(PC_REG++));
			temp32 = D_REG + postword + cc[C];
			cc[C] = (temp32 & 0x10000)>>16;
			cc[V] = OVERFLOW16(cc[C],postword,temp32,D_REG);
			cc[H] = (((D_REG ^ temp32 ^ postword) & 0x100)>>8);
			D_REG = temp32;
			cc[N] = NTEST16(D_REG);
			cc[Z] = ZTEST(D_REG);
			CycleCounter+=InsCycles[md[NATIVE6309]][M76];
		break;

		case ORD_X: //10AA 6309 Untested wcreate
			D_REG |= MemRead16(INDADDRESS(PC_REG++));
			cc[N] = NTEST16(D_REG);
			cc[Z] = ZTEST(D_REG);
			cc[V] = 0;
			CycleCounter+=InsCycles[md[NATIVE6309]][M76];
		break;

		case ADDW_X: //10AB 6309 Untested TESTED NITRO CHECK no Half carry?
			temp16=MemRead16(INDADDRESS(PC_REG++));
			temp32= W_REG+ temp16;
			cc[C] =(temp32 & 0x10000)>>16;
			cc[V] = OVERFLOW16(cc[C],temp32,temp16,W_REG);
			W_REG= temp32;
			cc[Z] = ZTEST(W_REG);
			cc[N] = NTEST16(W_REG);
			CycleCounter+=InsCycles[md[NATIVE6309]][M76];
		break;

		case CMPY_X: //10AC
			postword=MemRead16(INDADDRESS(PC_REG++));
			temp16= Y_REG - postword ;
			cc[C] = temp16 > Y_REG;
			cc[V] = OVERFLOW16(cc[C],postword,temp16,Y_REG);
			cc[N] = NTEST16(temp16);
			cc[Z] = ZTEST(temp16);
			CycleCounter+=InsCycles[md[NATIVE6309]][M76];
			break;

		case LDY_X: //10AE
			Y_REG=MemRead16(INDADDRESS(PC_REG++));
			cc[Z] = ZTEST(Y_REG);
			cc[N] = NTEST16(Y_REG);
			cc[V] = 0;
			CycleCounter+=6;
			break;

		case STY_X: //10AF
			MemWrite16(Y_REG,INDADDRESS(PC_REG++));
			cc[Z] = ZTEST(Y_REG);
			cc[N] = NTEST16(Y_REG);
			cc[V] = 0;
			CycleCounter+=6;
			break;

		case SUBW_E: //10B0 6309 Untested
			temp16=MemRead16(IMMADDRESS(PC_REG));
			temp32=W_REG-temp16;
			cc[C] = (temp32 & 0x10000)>>16;
			cc[V] = OVERFLOW16(cc[C],temp32,temp16,W_REG);
			W_REG= temp32;
			cc[Z] = ZTEST(W_REG);
			cc[N] = NTEST16(W_REG);
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M86];
		break;

		case CMPW_E: //10B1 6309 Untested
			postword=MemRead16(IMMADDRESS(PC_REG));
			temp16 = W_REG-postword;
			cc[C] = temp16 > W_REG;
			cc[V] = OVERFLOW16(cc[C],postword,temp16,W_REG);
			cc[N] = NTEST16(temp16);
			cc[Z] = ZTEST(temp16);
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M86];
		break;

		case SBCD_E: //10B2 6309
			WriteLog("Hitting UNEMULATED INS SBCD_E",TOCONS);
			CycleCounter+=InsCycles[md[NATIVE6309]][M86];
		break;

		case CMPD_E: //10B3
			postword=MemRead16(IMMADDRESS(PC_REG));
			temp16 = D_REG-postword;
			cc[C] = temp16 > D_REG;
			cc[V] = OVERFLOW16(cc[C],postword,temp16,D_REG);
			cc[N] = NTEST16(temp16);
			cc[Z] = ZTEST(temp16);
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M86];
			break;

		case ANDD_E: //10B4 6309 Untested
			D_REG &= MemRead16(IMMADDRESS(PC_REG));
			cc[N] = NTEST16(D_REG);
			cc[Z] = ZTEST(D_REG);
			cc[V] = 0;
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M86];
		break;

		case BITD_E: //10B5 6309 Untested CHECK NITRO
			temp16= D_REG & MemRead16(IMMADDRESS(PC_REG));
			cc[N] = NTEST16(temp16);
			cc[Z] = ZTEST(temp16);
			cc[V] = 0;
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M86];
		break;

		case LDW_E: //10B6 6309
			W_REG=MemRead16(IMMADDRESS(PC_REG));
			cc[Z] = ZTEST(W_REG);
			cc[N] = NTEST16(W_REG);
			cc[V] = 0;
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M76];
		break;

		case STW_E: //10B7 6309
			MemWrite16(W_REG,IMMADDRESS(PC_REG));
			cc[Z] = ZTEST(W_REG);
			cc[N] = NTEST16(W_REG);
			cc[V] = 0;
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M76];
		break;

		case EORD_E: //10B8 6309 Untested
			D_REG ^= MemRead16(IMMADDRESS(PC_REG));
			cc[N] = NTEST16(D_REG);
			cc[Z] = ZTEST(D_REG);
			cc[V] = 0;
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M86];
		break;

		case ADCD_E: //10B9 6309
			WriteLog("Hitting UNEMULATED INS ADCD_E",TOCONS);
			CycleCounter+=InsCycles[md[NATIVE6309]][M86];
		break;

		case ORD_E: //10BA 6309 Untested
			D_REG |= MemRead16(IMMADDRESS(PC_REG));
			cc[N] = NTEST16(D_REG);
			cc[Z] = ZTEST(D_REG);
			cc[V] = 0;
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M86];
		break;

		case ADDW_E: //10BB 6309 Untested
			temp16=MemRead16(IMMADDRESS(PC_REG));
			temp32= W_REG+ temp16;
			cc[C] =(temp32 & 0x10000)>>16;
			cc[V] = OVERFLOW16(cc[C],temp32,temp16,W_REG);
			W_REG= temp32;
			cc[Z] = ZTEST(W_REG);
			cc[N] = NTEST16(W_REG);
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M86];
		break;

		case CMPY_E: //10BC
			postword=MemRead16(IMMADDRESS(PC_REG));
			temp16 = Y_REG-postword;
			cc[C] = temp16 > Y_REG;
			cc[V] = OVERFLOW16(cc[C],postword,temp16,Y_REG);
			cc[N] = NTEST16(temp16);
			cc[Z] = ZTEST(temp16);
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M86];
			break;

		case LDY_E: //10BE
			Y_REG=MemRead16(IMMADDRESS(PC_REG));
			cc[Z] = ZTEST(Y_REG);
			cc[N] = NTEST16(Y_REG);
			cc[V] = 0;
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M76];
			break;

		case STY_E: //10BF
			MemWrite16(Y_REG,IMMADDRESS(PC_REG));
			cc[Z] = ZTEST(Y_REG);
			cc[N] = NTEST16(Y_REG);
			cc[V] = 0;
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M76];
			break;

		case LDS_I:  //10CE
			S_REG=IMMADDRESS(PC_REG);
			cc[Z] = ZTEST(S_REG);
			cc[N] = NTEST16(S_REG);
			cc[V] = 0;
			PC_REG+=2;
			CycleCounter+=4;
			break;

		case LDQ_D: //10DC 6309
			Q_REG=MemRead32(DPADDRESS(PC_REG++));
			cc[Z] = ZTEST(Q_REG);
			cc[N] = NTEST32(Q_REG);
			cc[V] = 0;
			CycleCounter+=InsCycles[md[NATIVE6309]][M87];
		break;

		case STQ_D: //10DD 6309
			MemWrite32(Q_REG,DPADDRESS(PC_REG++));
			cc[Z] = ZTEST(Q_REG);
			cc[N] = NTEST32(Q_REG);
			cc[V] = 0;
			CycleCounter+=InsCycles[md[NATIVE6309]][M87];
		break;

		case LDS_D: //10DE
			S_REG=MemRead16(DPADDRESS(PC_REG++));
			cc[Z] = ZTEST(S_REG);
			cc[N] = NTEST16(S_REG);
			cc[V] = 0;
			CycleCounter+=InsCycles[md[NATIVE6309]][M65];
			break;

		case STS_D: //10DF 6309
			MemWrite16(S_REG,DPADDRESS(PC_REG++));
			cc[Z] = ZTEST(S_REG);
			cc[N] = NTEST16(S_REG);
			cc[V] = 0;
			CycleCounter+=InsCycles[md[NATIVE6309]][M65];
			break;

		case LDQ_X: //10EC 6309
			Q_REG=MemRead32(INDADDRESS(PC_REG++));
			cc[Z] = ZTEST(Q_REG);
			cc[N] = NTEST32(Q_REG);
			cc[V] = 0;
			CycleCounter+=8;
			break;

		case STQ_X: //10ED 6309 DONE
			MemWrite32(Q_REG,INDADDRESS(PC_REG++));
			cc[Z] = ZTEST(Q_REG);
			cc[N] = NTEST32(Q_REG);
			cc[V] = 0;
			CycleCounter+=8;
			break;


		case LDS_X: //10EE
			S_REG=MemRead16(INDADDRESS(PC_REG++));
			cc[Z] = ZTEST(S_REG);
			cc[N] = NTEST16(S_REG);
			cc[V] = 0;
			CycleCounter+=6;
			break;

		case STS_X: //10EF 6309
			MemWrite16(S_REG,INDADDRESS(PC_REG++));
			cc[Z] = ZTEST(S_REG);
			cc[N] = NTEST16(S_REG);
			cc[V] = 0;
			CycleCounter+=6;
			break;

		case LDQ_E: //10FC 6309
			Q_REG=MemRead32(IMMADDRESS(PC_REG));
			cc[Z] = ZTEST(Q_REG);
			cc[N] = NTEST32(Q_REG);
			cc[V] = 0;
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M98];
		break;

		case STQ_E: //10FD 6309
			MemWrite32(Q_REG,IMMADDRESS(PC_REG));
			cc[Z] = ZTEST(Q_REG);
			cc[N] = NTEST32(Q_REG);
			cc[V] = 0;
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M98];
		break;

		case LDS_E: //10FE
			S_REG=MemRead16(IMMADDRESS(PC_REG));
			cc[Z] = ZTEST(S_REG);
			cc[N] = NTEST16(S_REG);
			cc[V] = 0;
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M76];
			break;

		case STS_E: //10FF 6309
			MemWrite16(S_REG,IMMADDRESS(PC_REG));
			cc[Z] = ZTEST(S_REG);
			cc[N] = NTEST16(S_REG);
			cc[V] = 0;
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M76];
			break;

		default:
			InvalidInsHandler();
			break;
	} //Page 2 switch END
	break;
case	Page3:

		switch (MemRead8(PC_REG++))
		{
		case BAND: //1130 6309
			WriteLog("Hitting UNEMULATED INS BAND",TOCONS);
			CycleCounter+=InsCycles[md[NATIVE6309]][M76];
		break;

		case BIAND: //1131 6309
			WriteLog("Hitting UNEMULATED INS BIAND",TOCONS);
			CycleCounter+=InsCycles[md[NATIVE6309]][M76];
		break;

		case BOR: //1132 6309
			WriteLog("Hitting UNEMULATED INS BOR",TOCONS);
			CycleCounter+=InsCycles[md[NATIVE6309]][M76];
		break;

		case BIOR: //1133 6309
			WriteLog("Hitting UNEMULATED INS BIOR",TOCONS);
			CycleCounter+=InsCycles[md[NATIVE6309]][M76];
		break;

		case BEOR: //1134 6309
			WriteLog("Hitting UNEMULATED INS BEOR",TOCONS);
			CycleCounter+=InsCycles[md[NATIVE6309]][M76];
		break;

		case BIEOR: //1135 6309
			WriteLog("Hitting UNEMULATED INS BIEOR",TOCONS);
			CycleCounter+=InsCycles[md[NATIVE6309]][M76];
		break;

		case LDBT: //1136 6309
			WriteLog("Hitting UNEMULATED INS LDBT",TOCONS);
			CycleCounter+=InsCycles[md[NATIVE6309]][M76];
		break;

		case STBT: //1137 6309
			WriteLog("Hitting UNEMULATED INS STBT",TOCONS);
			CycleCounter+=InsCycles[md[NATIVE6309]][M87];
		break;

		case TFM1: //1138 TFM R+,R+ 6309
			postbyte=MemRead8(PC_REG);
			Source=postbyte>>4;
			Dest=postbyte&15;
			if ((W_REG)>0)
			{
				temp8=MemRead8(*xfreg16[Source]);
				MemWrite8(temp8,*xfreg16[Dest]);
				(*xfreg16[Dest])++;
				(*xfreg16[Source])++;
				W_REG--;
				CycleCounter+=3;
				PC_REG-=2;
			}
			else
			{
				CycleCounter+=6;
				PC_REG++;
			}
		break;

		case TFM2: //1139 TFM R-,R- Phase 3
			postbyte=MemRead8(PC_REG);
			Source=postbyte>>4;
			Dest=postbyte&15;

			if (W_REG>0)
			{
				temp8=MemRead8(*xfreg16[Source]);
				MemWrite8(temp8,*xfreg16[Dest]);
				(*xfreg16[Dest])--;
				(*xfreg16[Source])--;
				W_REG--;
				CycleCounter+=3;
				PC_REG-=2;
			}
			else
			{
				CycleCounter+=6;
				PC_REG++;
			}			
		break;

		case TFM3: //113A 6309
			WriteLog("Hitting TFM3",TOCONS);
			CycleCounter+=6;
		break;

		case TFM4: //113B TFM R,R+ 6309 
			postbyte=MemRead8(PC_REG);
			Source=postbyte>>4;
			Dest=postbyte&15;

			if (W_REG>0)
			{
				temp8=MemRead8(*xfreg16[Source]);
				MemWrite8(temp8,*xfreg16[Dest]);
				(*xfreg16[Dest])++;
				W_REG--;
				PC_REG-=2; //Hit the same instruction on the next loop if not done copying
				CycleCounter+=3;
			}
			else
			{
				CycleCounter+=6;
				PC_REG++;
			}
		break;

		case BITMD_M: //113C  6309
			WriteLog("Hitting bitmd_m",TOCONS);
			CycleCounter+=4;
		break;

		case LDMD_M: //113D DONE 6309
			mdbits= MemRead8(PC_REG++);
			setmd(mdbits);
			CycleCounter+=5;
		break;

		case SWI3_I: //113F
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
			break;

		case COME_I: //1143 6309 Untested
			E_REG = 0xFF- E_REG;
			cc[Z] = ZTEST(E_REG);
			cc[N] = NTEST8(E_REG);
			cc[C] = 1;
			cc[V] = 0;
			CycleCounter+=InsCycles[md[NATIVE6309]][M32];
		break;

		case DECE_I: //114A 6309
			E_REG--;
			cc[Z] = ZTEST(E_REG);
			cc[N] = NTEST8(E_REG);
			cc[V] = E_REG==0x7F;
			CycleCounter+=InsCycles[md[NATIVE6309]][M32];
		break;

		case INCE_I: //114C 6309
			E_REG++;
			cc[Z] = ZTEST(E_REG);
			cc[N] = NTEST8(E_REG);
			cc[V] = E_REG==0x80;
			CycleCounter+=InsCycles[md[NATIVE6309]][M32];
		break;

		case TSTE_I: //114D 6309 Untested TESTED NITRO
			cc[Z] = ZTEST(E_REG);
			cc[N] = NTEST8(E_REG);
			cc[V] = 0;
			CycleCounter+=InsCycles[md[NATIVE6309]][M32];
		break;

		case CLRE_I: //114F 6309
			E_REG= 0;
			cc[C] = 0;
			cc[V] = 0;
			cc[N] = 0;
			cc[Z] = 1;
			CycleCounter+=InsCycles[md[NATIVE6309]][M32];
		break;

		case COMF_I: //1153 6309 Untested
			F_REG= 0xFF- F_REG;
			cc[Z] = ZTEST(F_REG);
			cc[N] = NTEST8(F_REG);
			cc[C] = 1;
			cc[V] = 0;
			CycleCounter+=InsCycles[md[NATIVE6309]][M32];
		break;

		case DECF_I: //115A 6309
			F_REG--;
			cc[Z] = ZTEST(F_REG);
			cc[N] = NTEST8(F_REG);
			cc[V] = F_REG==0x7F;
			CycleCounter+=InsCycles[md[NATIVE6309]][M21];
		break;

		case INCF_I: //115C 6309 Untested
			F_REG++;
			cc[Z] = ZTEST(F_REG);
			cc[N] = NTEST8(F_REG);
			cc[V] = F_REG==0x80;
			CycleCounter+=InsCycles[md[NATIVE6309]][M32];
		break;

		case TSTF_I: //115D 6309
			cc[Z] = ZTEST(F_REG);
			cc[N] = NTEST8(F_REG);
			cc[V] = 0;
			CycleCounter+=InsCycles[md[NATIVE6309]][M32];
		break;

		case CLRF_I: //115F 6309 Untested wcreate
			F_REG= 0;
			cc[C] = 0;
			cc[V] = 0;
			cc[N] = 0;
			cc[Z] = 1;
			CycleCounter+=InsCycles[md[NATIVE6309]][M32];
		break;

		case SUBE_M: //1180 6309 Untested
			postbyte=MemRead8(PC_REG++);
			temp16 = E_REG - postbyte;
			cc[C] =(temp16 & 0x100)>>8; 
			cc[V] = OVERFLOW8(cc[C],postbyte,temp16,E_REG);
			E_REG= (unsigned char)temp16;
			cc[Z] = ZTEST(E_REG);
			cc[N] = NTEST8(E_REG);
			CycleCounter+=3;
		break;

		case CMPE_M: //1181 6309
			postbyte=MemRead8(PC_REG++);
			temp8= E_REG-postbyte;
			cc[C] = temp8 > E_REG;
			cc[V] = OVERFLOW8(cc[C],postbyte,temp8,E_REG);
			cc[N] = NTEST8(temp8);
			cc[Z] = ZTEST(temp8);
			CycleCounter+=3;
		break;

		case CMPU_M: //1183
			postword=IMMADDRESS(PC_REG);
			temp16 = U_REG-postword;
			cc[C] = temp16 > U_REG;
			cc[V] = OVERFLOW16(cc[C],postword,temp16,U_REG);
			cc[N] = NTEST16(temp16);
			cc[Z] = ZTEST(temp16);
			PC_REG+=2;	
			CycleCounter+=InsCycles[md[NATIVE6309]][M54];
		break;

		case LDE_M: //1186 6309
			E_REG= MemRead8(PC_REG++);
			cc[Z] = ZTEST(E_REG);
			cc[N] = NTEST8(E_REG);
			cc[V] = 0;
			CycleCounter+=3;
		break;

		case ADDE_M: //118B 6309
			postbyte=MemRead8(PC_REG++);
			temp16=E_REG+postbyte;
			cc[C] =(temp16 & 0x100)>>8;
			cc[H] = ((E_REG ^ postbyte ^ temp16) & 0x10)>>4;
			cc[V] =OVERFLOW8(cc[C],postbyte,temp16,E_REG);
			E_REG= (unsigned char)temp16;
			cc[N] = NTEST8(E_REG);
			cc[Z] = ZTEST(E_REG);
			CycleCounter+=3;
		break;

		case CMPS_M: //118C
			postword=IMMADDRESS(PC_REG);
			temp16 = S_REG-postword;
			cc[C] = temp16 > S_REG;
			cc[V] = OVERFLOW16(cc[C],postword,temp16,S_REG);
			cc[N] = NTEST16(temp16);
			cc[Z] = ZTEST(temp16);
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M54];
			break;

		case DIVD_M: //118D 6309 NITRO
			*spostbyte=(signed char)MemRead8(PC_REG++);
			if (*spostbyte)
			{	
				*spostword=D_REG;
				stemp16= (signed short)D_REG / *spostbyte;
				A_REG = (signed short)D_REG % *spostbyte;
				B_REG=(unsigned char)stemp16;

				cc[Z] = ZTEST(B_REG);
				cc[N] = NTEST16(D_REG);
				cc[C] = B_REG & 1;
				cc[V] =(stemp16 >127) | (stemp16 <-128);

				if ( (stemp16 > 255) | (stemp16 < -256) ) //Abort
				{
					D_REG=abs (*spostword);
					cc[N] = NTEST16(D_REG);
					cc[Z] = ZTEST(D_REG);
				}
				CycleCounter+=25;
			}
			else
			{
				CycleCounter+=17;
				DivbyZero();				
			}
		break;

		case DIVQ_M: //118E 6309 
			if (performDivQ((short)IMMADDRESS(PC_REG)))
			{
				CycleCounter += 36;
				PC_REG += 2;	//PC_REG+2 + 2 more for a total of 4
			}
			else
			{
				CycleCounter += 17;
			}
			break;

		case MULD_M: //118F Phase 5 6309
			Q_REG=  D_REG * IMMADDRESS(PC_REG);
			cc[C] = 0; 
			cc[Z] = ZTEST(D_REG);
			cc[V] = 0;
			cc[N] = NTEST16(D_REG);
			PC_REG+=2;
			CycleCounter+=28;
		break;

		case SUBE_D: //1190 6309 Untested HERE
			postbyte=MemRead8( DPADDRESS(PC_REG++));
			temp16 = E_REG - postbyte;
			cc[C] =(temp16 & 0x100)>>8; 
			cc[V] = OVERFLOW8(cc[C],postbyte,temp16,E_REG);
			E_REG= (unsigned char)temp16;
			cc[Z] = ZTEST(E_REG);
			cc[N] = NTEST8(E_REG);
			CycleCounter+=InsCycles[md[NATIVE6309]][M54];
		break;

		case CMPE_D: //1191 6309 Untested
			postbyte=MemRead8(DPADDRESS(PC_REG++));
			temp8= E_REG-postbyte;
			cc[C] = temp8 > E_REG;
			cc[V] = OVERFLOW8(cc[C],postbyte,temp8,E_REG);
			cc[N] = NTEST8(temp8);
			cc[Z] = ZTEST(temp8);
			CycleCounter+=InsCycles[md[NATIVE6309]][M54];
		break;

		case CMPU_D: //1193
			postword=MemRead16(DPADDRESS(PC_REG++));
			temp16= U_REG - postword ;
			cc[C] = temp16 > U_REG;
			cc[V] = OVERFLOW16(cc[C],postword,temp16,U_REG);
			cc[N] = NTEST16(temp16);
			cc[Z] = ZTEST(temp16);
			CycleCounter+=InsCycles[md[NATIVE6309]][M75];
			break;

		case LDE_D: //1196 6309
			E_REG= MemRead8(DPADDRESS(PC_REG++));
			cc[Z] = ZTEST(E_REG);
			cc[N] = NTEST8(E_REG);
			cc[V] = 0;
			CycleCounter+=InsCycles[md[NATIVE6309]][M54];
		break;

		case STE_D: //1197 Phase 5 6309
			MemWrite8( E_REG,DPADDRESS(PC_REG++));
			cc[Z] = ZTEST(E_REG);
			cc[N] = NTEST8(E_REG);
			cc[V] = 0;
			CycleCounter+=InsCycles[md[NATIVE6309]][M54];
		break;

		case ADDE_D: //119B 6309 Untested
			postbyte=MemRead8(DPADDRESS(PC_REG++));
			temp16=E_REG+postbyte;
			cc[C] = (temp16 & 0x100)>>8;
			cc[H] = ((E_REG ^ postbyte ^ temp16) & 0x10)>>4;
			cc[V] = OVERFLOW8(cc[C],postbyte,temp16,E_REG);
			E_REG= (unsigned char)temp16;
			cc[N] = NTEST8(E_REG);
			cc[Z] =ZTEST(E_REG);
			CycleCounter+=InsCycles[md[NATIVE6309]][M54];
		break;

		case CMPS_D: //119C
			postword=MemRead16(DPADDRESS(PC_REG++));
			temp16= S_REG - postword ;
			cc[C] = temp16 > S_REG;
			cc[V] = OVERFLOW16(cc[C],postword,temp16,S_REG);
			cc[N] = NTEST16(temp16);
			cc[Z] = ZTEST(temp16);
			CycleCounter+=InsCycles[md[NATIVE6309]][M75];
			break;

		case DIVD_D: //119D 6309 02292008
			*spostbyte=(signed char)MemRead8(DPADDRESS(PC_REG++));
			if (*spostbyte)
			{	
				*spostword=D_REG;
				stemp16= (signed short)D_REG / *spostbyte;
				A_REG = (signed short)D_REG % *spostbyte;
				B_REG=(unsigned char)stemp16;

				cc[Z] = ZTEST(B_REG);
				cc[N] = NTEST16(D_REG);
				cc[C] = B_REG & 1;
				cc[V] =(stemp16 >127) | (stemp16 <-128);

				if ( (stemp16 > 255) | (stemp16 < -256) ) //Abort
				{
					D_REG=abs (*spostword);
					cc[N] = NTEST16(D_REG);
					cc[Z] = ZTEST(D_REG);
				}
				CycleCounter+=27;
			}
			else
			{
				CycleCounter+=19;
				DivbyZero();				
			}
			CycleCounter+=InsCycles[md[NATIVE6309]][M2726];
		break;

		case DIVQ_D: //119E 6309
			if (performDivQ(MemRead16(DPADDRESS(PC_REG))))
			{
				CycleCounter += 35;
				PC_REG += 1;
			}
			else
			{
				CycleCounter += 17;
			}
		break;

		case MULD_D: //119F 6309 02292008
			Q_REG=  D_REG * MemRead16(DPADDRESS(PC_REG++));
			cc[C] = 0;
			cc[Z] = ZTEST(D_REG);
			cc[V] = 0;
			cc[N] = NTEST16(D_REG);
			CycleCounter+=InsCycles[md[NATIVE6309]][M3029];
		break;

		case SUBE_X: //11A0 6309 Untested
			postbyte=MemRead8(INDADDRESS(PC_REG++));
			temp16 = E_REG - postbyte;
			cc[C] =(temp16 & 0x100)>>8; 
			cc[V] = OVERFLOW8(cc[C],postbyte,temp16,E_REG);
			E_REG= (unsigned char)temp16;
			cc[Z] = ZTEST(E_REG);
			cc[N] = NTEST8(E_REG);
			CycleCounter+=5;
		break;

		case CMPE_X: //11A1 6309
			postbyte=MemRead8(INDADDRESS(PC_REG++));
			temp8= E_REG-postbyte;
			cc[C] = temp8 > E_REG;
			cc[V] = OVERFLOW8(cc[C],postbyte,temp8,E_REG);
			cc[N] = NTEST8(temp8);
			cc[Z] = ZTEST(temp8);
			CycleCounter+=5;
		break;

		case CMPU_X: //11A3
			postword=MemRead16(INDADDRESS(PC_REG++));
			temp16= U_REG - postword ;
			cc[C] = temp16 > U_REG;
			cc[V] = OVERFLOW16(cc[C],postword,temp16,U_REG);
			cc[N] = NTEST16(temp16);
			cc[Z] = ZTEST(temp16);
			CycleCounter+=InsCycles[md[NATIVE6309]][M76];
			break;

		case LDE_X: //11A6 6309
			E_REG= MemRead8(INDADDRESS(PC_REG++));
			cc[Z] = ZTEST(E_REG);
			cc[N] = NTEST8(E_REG);
			cc[V] = 0;
			CycleCounter+=5;
		break;

		case STE_X: //11A7 6309
			MemWrite8(E_REG,INDADDRESS(PC_REG++));
			cc[Z] = ZTEST(E_REG);
			cc[N] = NTEST8(E_REG);
			cc[V] = 0;
			CycleCounter+=5;
		break;

		case ADDE_X: //11AB 6309 Untested
			postbyte=MemRead8(INDADDRESS(PC_REG++));
			temp16=E_REG+postbyte;
			cc[C] =(temp16 & 0x100)>>8;
			cc[H] = ((E_REG ^ postbyte ^ temp16) & 0x10)>>4;
			cc[V] =OVERFLOW8(cc[C],postbyte,temp16,E_REG);
			E_REG= (unsigned char)temp16;
			cc[N] = NTEST8(E_REG);
			cc[Z] = ZTEST(E_REG);
			CycleCounter+=5;
		break;

		case CMPS_X:  //11AC
			postword=MemRead16(INDADDRESS(PC_REG++));
			temp16= S_REG - postword ;
			cc[C] = temp16 > S_REG;
			cc[V] = OVERFLOW16(cc[C],postword,temp16,S_REG);
			cc[N] = NTEST16(temp16);
			cc[Z] = ZTEST(temp16);
			CycleCounter+=InsCycles[md[NATIVE6309]][M76];
			break;

		case DIVD_X: //11AD wcreate  6309
			*spostbyte=(signed char)MemRead8(INDADDRESS(PC_REG++));
			if (*spostbyte)
			{	
				*spostword=D_REG;
				stemp16= (signed short)D_REG / *spostbyte;
				A_REG = (signed short)D_REG % *spostbyte;
				B_REG=(unsigned char)stemp16;

				cc[Z] = ZTEST(B_REG);
				cc[N] = NTEST16(D_REG);	//cc[N] = NTEST8(B_REG);
				cc[C] = B_REG & 1;
				cc[V] =(stemp16 >127) | (stemp16 <-128);

				if ( (stemp16 > 255) | (stemp16 < -256) ) //Abort
				{
					D_REG=abs (*spostword);
					cc[N] = NTEST16(D_REG);
					cc[Z] = ZTEST(D_REG);
				}
				CycleCounter+=27;
			}
			else
			{
				CycleCounter+=19;
				DivbyZero();				
			}
		break;

		case DIVQ_X: //11AE 6309
			if (performDivQ(MemRead16(INDADDRESS(PC_REG++))))
			{
				CycleCounter += 36;
			}
			else
			{
				CycleCounter += 17;
			}
			break;

		case MULD_X: //11AF 6309 CHECK
			Q_REG=  D_REG * MemRead16(INDADDRESS(PC_REG++));
			cc[C] = 0;
			cc[Z] = ZTEST(D_REG);
			cc[V] = 0;
			cc[N] = NTEST16(D_REG);
			CycleCounter+=30;
		break;

		case SUBE_E: //11B0 6309 Untested
			postbyte=MemRead8(IMMADDRESS(PC_REG));
			temp16 = E_REG - postbyte;
			cc[C] =(temp16 & 0x100)>>8; 
			cc[V] = OVERFLOW8(cc[C],postbyte,temp16,E_REG);
			E_REG= (unsigned char)temp16;
			cc[Z] = ZTEST(E_REG);
			cc[N] = NTEST8(E_REG);
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M65];
		break;

		case CMPE_E: //11B1 6309 Untested
			postbyte=MemRead8(IMMADDRESS(PC_REG));
			temp8= E_REG-postbyte;
			cc[C] = temp8 > E_REG;
			cc[V] = OVERFLOW8(cc[C],postbyte,temp8,E_REG);
			cc[N] = NTEST8(temp8);
			cc[Z] = ZTEST(temp8);
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M65];
		break;

		case CMPU_E: //11B3
			postword=MemRead16(IMMADDRESS(PC_REG));
			temp16 = U_REG-postword;
			cc[C] = temp16 > U_REG;
			cc[V] = OVERFLOW16(cc[C],postword,temp16,U_REG);
			cc[N] = NTEST16(temp16);
			cc[Z] = ZTEST(temp16);
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M86];
			break;

		case LDE_E: //11B6 6309
			E_REG= MemRead8(IMMADDRESS(PC_REG));
			cc[Z] = ZTEST(E_REG);
			cc[N] = NTEST8(E_REG);
			cc[V] = 0;
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M65];
		break;

		case STE_E: //11B7 6309
			MemWrite8(E_REG,IMMADDRESS(PC_REG));
			cc[Z] = ZTEST(E_REG);
			cc[N] = NTEST8(E_REG);
			cc[V] = 0;
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M65];
		break;

		case ADDE_E: //11BB 6309 Untested
			postbyte=MemRead8(IMMADDRESS(PC_REG));
			temp16=E_REG+postbyte;
			cc[C] = (temp16 & 0x100)>>8;
			cc[H] = ((E_REG ^ postbyte ^ temp16) & 0x10)>>4;
			cc[V] = OVERFLOW8(cc[C],postbyte,temp16,E_REG);
			E_REG= (unsigned char)temp16;
			cc[N] = NTEST8(E_REG);
			cc[Z] = ZTEST(E_REG);
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M65];
		break;

		case CMPS_E: //11BC
			postword=MemRead16(IMMADDRESS(PC_REG));
			temp16 = S_REG-postword;
			cc[C] = temp16 > S_REG;
			cc[V] = OVERFLOW16(cc[C],postword,temp16,S_REG);
			cc[N] = NTEST16(temp16);
			cc[Z] = ZTEST(temp16);
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M86];
			break;

		case DIVD_E: //11BD 6309 02292008 Untested
			*spostbyte=(signed char)MemRead8(IMMADDRESS(PC_REG));
			if (*spostbyte)
			{	
				*spostword=D_REG;
				stemp16= (signed short)D_REG / *spostbyte;
				A_REG = (signed short)D_REG % *spostbyte;
				B_REG=(unsigned char)stemp16;

				cc[Z] = ZTEST(B_REG);
				cc[N] = NTEST16(D_REG);
				cc[C] = B_REG & 1;
				cc[V] =(stemp16 >127) | (stemp16 <-128);

				if ( (stemp16 > 255) | (stemp16 < -256) ) //Abort
				{
					D_REG=abs (*spostword);
					cc[N] = NTEST16(D_REG);
					cc[Z] = ZTEST(D_REG);
				}
				CycleCounter+=25;
			}
			else
			{
				CycleCounter+=17;
				DivbyZero();				
			}
			CycleCounter+=InsCycles[md[NATIVE6309]][M2827];
		break;

		case DIVQ_E: //11BE Phase 5 6309 CHECK
			temp16 = IMMADDRESS(PC_REG);
			if (performDivQ(MemRead16(temp16)))
			{
				CycleCounter += 37;
				PC_REG += 2;
			}
			else
			{
				CycleCounter += 17;
			}
		break;

		case MULD_E: //11BF 6309
			Q_REG=  D_REG * MemRead16(IMMADDRESS(PC_REG));
			cc[C] = 0;
			cc[Z] = ZTEST(D_REG);
			cc[V] = 0;
			cc[N] = NTEST16(D_REG);
			CycleCounter+=InsCycles[md[NATIVE6309]][M3130];
		break;

		case SUBF_M: //11C0 6309 Untested
			postbyte=MemRead8(PC_REG++);
			temp16 = F_REG - postbyte;
			cc[C] = (temp16 & 0x100)>>8; 
			cc[V] = OVERFLOW8(cc[C],postbyte,temp16,F_REG);
			F_REG= (unsigned char)temp16;
			cc[Z] = ZTEST(F_REG);
			cc[N] = NTEST8(F_REG);
			CycleCounter+=3;
		break;

		case CMPF_M: //11C1 6309
			postbyte=MemRead8(PC_REG++);
			temp8= F_REG-postbyte;
			cc[C] = temp8 > F_REG;
			cc[V] = OVERFLOW8(cc[C],postbyte,temp8,F_REG);
			cc[N] = NTEST8(temp8);
			cc[Z] = ZTEST(temp8);
			CycleCounter+=3;
		break;

		case LDF_M: //11C6 6309
			F_REG= MemRead8(PC_REG++);
			cc[Z] = ZTEST(F_REG);
			cc[N] = NTEST8(F_REG);
			cc[V] = 0;
			CycleCounter+=3;
		break;

		case ADDF_M: //11CB 6309 Untested
			postbyte=MemRead8(PC_REG++);
			temp16=F_REG+postbyte;
			cc[C] = (temp16 & 0x100)>>8;
			cc[H] = ((F_REG ^ postbyte ^ temp16) & 0x10)>>4;
			cc[V] = OVERFLOW8(cc[C],postbyte,temp16,F_REG);
			F_REG= (unsigned char)temp16;
			cc[N] = NTEST8(F_REG);
			cc[Z] = ZTEST(F_REG);
			CycleCounter+=3;
		break;

		case SUBF_D: //11D0 6309 Untested
			postbyte=MemRead8( DPADDRESS(PC_REG++));
			temp16 = F_REG - postbyte;
			cc[C] =(temp16 & 0x100)>>8; 
			cc[V] = OVERFLOW8(cc[C],postbyte,temp16,F_REG);
			F_REG= (unsigned char)temp16;
			cc[Z] = ZTEST(F_REG);
			cc[N] = NTEST8(F_REG);
			CycleCounter+=InsCycles[md[NATIVE6309]][M54];
		break;

		case CMPF_D: //11D1 6309 Untested
			postbyte=MemRead8(DPADDRESS(PC_REG++));
			temp8= F_REG-postbyte;
			cc[C] = temp8 > F_REG;
			cc[V] = OVERFLOW8(cc[C],postbyte,temp8,F_REG);
			cc[N] = NTEST8(temp8);
			cc[Z] = ZTEST(temp8);
			CycleCounter+=InsCycles[md[NATIVE6309]][M54];
		break;

		case LDF_D: //11D6 6309 Untested wcreate
			F_REG= MemRead8(DPADDRESS(PC_REG++));
			cc[Z] = ZTEST(F_REG);
			cc[N] = NTEST8(F_REG);
			cc[V] = 0;
			CycleCounter+=InsCycles[md[NATIVE6309]][M54];
		break;

		case STF_D: //11D7 Phase 5 6309
			MemWrite8(F_REG,DPADDRESS(PC_REG++));
			cc[Z] = ZTEST(F_REG);
			cc[N] = NTEST8(F_REG);
			cc[V] = 0;
			CycleCounter+=InsCycles[md[NATIVE6309]][M54];
		break;

		case ADDF_D: //11D8 6309 Untested
			postbyte=MemRead8(DPADDRESS(PC_REG++));
			temp16=F_REG+postbyte;
			cc[C] =(temp16 & 0x100)>>8;
			cc[H] = ((F_REG ^ postbyte ^ temp16) & 0x10)>>4;
			cc[V] =OVERFLOW8(cc[C],postbyte,temp16,F_REG);
			F_REG= (unsigned char)temp16;
			cc[N] = NTEST8(F_REG);
			cc[Z] = ZTEST(F_REG);
			CycleCounter+=InsCycles[md[NATIVE6309]][M54];
		break;

		case SUBF_X: //11E0 6309 Untested
			postbyte=MemRead8(INDADDRESS(PC_REG++));
			temp16 = F_REG - postbyte;
			cc[C] =(temp16 & 0x100)>>8; 
			cc[V] = OVERFLOW8(cc[C],postbyte,temp16,F_REG);
			F_REG= (unsigned char)temp16;
			cc[Z] = ZTEST(F_REG);
			cc[N] = NTEST8(F_REG);
			CycleCounter+=5;
		break;

		case CMPF_X: //11E1 6309 Untested
			postbyte=MemRead8(INDADDRESS(PC_REG++));
			temp8= F_REG-postbyte;
			cc[C] = temp8 > F_REG;
			cc[V] = OVERFLOW8(cc[C],postbyte,temp8,F_REG);
			cc[N] = NTEST8(temp8);
			cc[Z] = ZTEST(temp8);
			CycleCounter+=5;
		break;

		case LDF_X: //11E6 6309
			F_REG=MemRead8(INDADDRESS(PC_REG++));
			cc[Z] = ZTEST(F_REG);
			cc[N] = NTEST8(F_REG);
			cc[V] = 0;
			CycleCounter+=5;
		break;

		case STF_X: //11E7 Phase 5 6309
			MemWrite8(F_REG,INDADDRESS(PC_REG++));
			cc[Z] = ZTEST(F_REG);
			cc[N] = NTEST8(F_REG);
			cc[V] = 0;
			CycleCounter+=5;
		break;

		case ADDF_X: //11EB 6309 Untested
			postbyte=MemRead8(INDADDRESS(PC_REG++));
			temp16=F_REG+postbyte;
			cc[C] =(temp16 & 0x100)>>8;
			cc[H] = ((F_REG ^ postbyte ^ temp16) & 0x10)>>4;
			cc[V] = OVERFLOW8(cc[C],postbyte,temp16,F_REG);
			F_REG= (unsigned char)temp16;
			cc[N] = NTEST8(F_REG);
			cc[Z] = ZTEST(F_REG);
			CycleCounter+=5;
		break;

		case SUBF_E: //11F0 6309 Untested
			postbyte=MemRead8(IMMADDRESS(PC_REG));
			temp16 = F_REG - postbyte;
			cc[C] = (temp16 & 0x100)>>8; 
			cc[V] = OVERFLOW8(cc[C],postbyte,temp16,F_REG);
			F_REG= (unsigned char)temp16;
			cc[Z] = ZTEST(F_REG);
			cc[N] = NTEST8(F_REG);
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M65];
		break;

		case CMPF_E: //11F1 6309 Untested
			postbyte=MemRead8(IMMADDRESS(PC_REG));
			temp8= F_REG-postbyte;
			cc[C] = temp8 > F_REG;
			cc[V] = OVERFLOW8(cc[C],postbyte,temp8,F_REG);
			cc[N] = NTEST8(temp8);
			cc[Z] = ZTEST(temp8);
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M65];
		break;

		case LDF_E: //11F6 6309
			F_REG= MemRead8(IMMADDRESS(PC_REG));
			cc[Z] = ZTEST(F_REG);
			cc[N] = NTEST8(F_REG);
			cc[V] = 0;
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M65];
		break;

		case STF_E: //11F7 Phase 5 6309
			MemWrite8(F_REG,IMMADDRESS(PC_REG));
			cc[Z] = ZTEST(F_REG);
			cc[N] = NTEST8(F_REG);
			cc[V] = 0;
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M65];
		break;

		case ADDF_E: //11FB 6309 Untested
			postbyte=MemRead8(IMMADDRESS(PC_REG));
			temp16=F_REG+postbyte;
			cc[C] = (temp16 & 0x100)>>8;
			cc[H] = ((F_REG ^ postbyte ^ temp16) & 0x10)>>4;
			cc[V] = OVERFLOW8(cc[C],postbyte,temp16,F_REG);
			F_REG= (unsigned char)temp16;
			cc[N] = NTEST8(F_REG);
			cc[Z] = ZTEST(F_REG);
			PC_REG+=2;
			CycleCounter+=InsCycles[md[NATIVE6309]][M65];
		break;

		default:
			InvalidInsHandler();
		break;

	} //Page 3 switch END
	break;

case NOP_I:	//12
	CycleCounter+=InsCycles[md[NATIVE6309]][M21];
	break;

case SYNC_I: //13
	CycleCounter=CycleFor;
	SyncWaiting=1;
	break;

case SEXW_I: //14 6309 CHECK
	if (W_REG & 32768)
		D_REG=0xFFFF;
	else
		D_REG=0;
	cc[Z] = ZTEST(Q_REG);
	cc[N] = NTEST16(D_REG);
	CycleCounter+=4;
	break;

case LBRA_R: //16
	*spostword=IMMADDRESS(PC_REG);
	PC_REG+=2;
	PC_REG+=*spostword;
	CycleCounter+=InsCycles[md[NATIVE6309]][M54];
	break;

case	LBSR_R: //17
	*spostword=IMMADDRESS(PC_REG);
	PC_REG+=2;
	S_REG--;
	MemWrite8(pc.B.lsb,S_REG--);
	MemWrite8(pc.B.msb,S_REG);
	PC_REG+=*spostword;
	CycleCounter+=InsCycles[md[NATIVE6309]][M97];
	break;

case DAA_I: //19

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
	CycleCounter+=InsCycles[md[NATIVE6309]][M21];
	break;

case ORCC_M: //1A
	postbyte=MemRead8(PC_REG++);
	temp8=getcc();
	temp8 = (temp8 | postbyte);
	setcc(temp8);
	CycleCounter+=InsCycles[md[NATIVE6309]][M32];
	break;

case ANDCC_M: //1C
	postbyte=MemRead8(PC_REG++);
	temp8=getcc();
	temp8 = (temp8 & postbyte);
	setcc(temp8);
	CycleCounter+=3;
	break;

case SEX_I: //1D
	A_REG= 0-(B_REG>>7);
	cc[Z] = ZTEST(D_REG);
	cc[N] = D_REG >> 15;
	CycleCounter+=InsCycles[md[NATIVE6309]][M21];
	break;

case EXG_M: //1E
	postbyte=MemRead8(PC_REG++);
	Source= postbyte>>4;
	Dest=postbyte & 15;

	ccbits=getcc();
	if ( ((postbyte & 0x80)>>4)==(postbyte & 0x08)) //Verify like size registers
	{
		if (postbyte & 0x08) //8 bit EXG
		{
			temp8= (*ureg8[((postbyte & 0x70) >> 4)]); //
			(*ureg8[((postbyte & 0x70) >> 4)]) = (*ureg8[postbyte & 0x07]);
			(*ureg8[postbyte & 0x07])=temp8;
		}
		else // 16 bit EXG
		{
			temp16=(*xfreg16[((postbyte & 0x70) >> 4)]);
			(*xfreg16[((postbyte & 0x70) >> 4)])=(*xfreg16[postbyte & 0x07]);
			(*xfreg16[postbyte & 0x07])=temp16;
		}
	}
	setcc(ccbits);
	CycleCounter+=InsCycles[md[NATIVE6309]][M85];
	break;

case TFR_M: //1F
	postbyte=MemRead8(PC_REG++);
	Source= postbyte>>4;
	Dest=postbyte & 15;

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
			*xfreg16[Dest]=0xFFFF;
			if ( (Source==12) | (Source==13) )
				*xfreg16[Dest]=0;
			else
				if (Source<=7)
					*xfreg16[Dest]=*xfreg16[Source];
		break;

		case 8:
		case 9:
		case 10:
		case 11:
		case 14:
		case 15:
			ccbits=getcc();
			*ureg8[Dest&7]=0xFF;
			if ( (Source==12) | (Source==13) )
				*ureg8[Dest&7]=0;
			else
				if (Source>7)
					*ureg8[Dest&7]=*ureg8[Source&7];
			setcc(ccbits);
		break;
	}
	CycleCounter+=InsCycles[md[NATIVE6309]][M64];
	break;

case BRA_R: //20
	*spostbyte=MemRead8(PC_REG++);
	PC_REG+=*spostbyte;
	CycleCounter+=3;
	break;

case BRN_R: //21
	CycleCounter+=3;
	PC_REG++;
	break;

case BHI_R: //22
	if  (!(cc[C] | cc[Z]))
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
	break;

case BLS_R: //23
	if (cc[C] | cc[Z])
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
	break;

case BHS_R: //24
	if (!cc[C])
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
	break;

case BLO_R: //25
	if (cc[C])
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
	break;

case BNE_R: //26
	if (!cc[Z])
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
	break;

case BEQ_R: //27
	if (cc[Z])
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
	break;

case BVC_R: //28
	if (!cc[V])
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
	break;

case BVS_R: //29
	if ( cc[V])
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
	break;

case BPL_R: //2A
	if (!cc[N])
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
	break;

case BMI_R: //2B
	if ( cc[N])
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
	break;

case BGE_R: //2C
	if (! (cc[N] ^ cc[V]))
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
	break;

case BLT_R: //2D
	if ( cc[V] ^ cc[N])
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
	break;

case BGT_R: //2E
	if ( !( cc[Z] | (cc[N]^cc[V] ) ))
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
	break;

case BLE_R: //2F
	if ( cc[Z] | (cc[N]^cc[V]) )
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
	break;

case LEAX_X: //30
	X_REG=INDADDRESS(PC_REG++);
	cc[Z] = ZTEST(X_REG);
	CycleCounter+=4;
	break;

case LEAY_X: //31
	Y_REG=INDADDRESS(PC_REG++);
	cc[Z] = ZTEST(Y_REG);
	CycleCounter+=4;
	break;

case LEAS_X: //32
	S_REG=INDADDRESS(PC_REG++);
	CycleCounter+=4;
	break;

case LEAU_X: //33
	U_REG=INDADDRESS(PC_REG++);
	CycleCounter+=4;
	break;

case PSHS_M: //34
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

	CycleCounter+=InsCycles[md[NATIVE6309]][M54];
	break;

case PULS_M: //35
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
	CycleCounter+=InsCycles[md[NATIVE6309]][M54];
	break;

case PSHU_M: //36
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
	CycleCounter+=InsCycles[md[NATIVE6309]][M54];
	break;

case PULU_M: //37
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
	CycleCounter+=InsCycles[md[NATIVE6309]][M54];
	break;

case RTS_I: //39
	pc.B.msb=MemRead8(S_REG++);
	pc.B.lsb=MemRead8(S_REG++);
	CycleCounter+=InsCycles[md[NATIVE6309]][M51];
	break;

case ABX_I: //3A
	X_REG=X_REG+B_REG;
	CycleCounter+=InsCycles[md[NATIVE6309]][M31];
	break;

case RTI_I: //3B
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
	break;

case CWAI_I: //3C
	postbyte=MemRead8(PC_REG++);
	ccbits=getcc();
	ccbits = ccbits & postbyte;
	setcc(ccbits);
	CycleCounter=CycleFor;
	SyncWaiting=1;
	break;

case MUL_I: //3D
	D_REG = A_REG * B_REG;
	cc[C] = B_REG >0x7F;
	cc[Z] = ZTEST(D_REG);
	CycleCounter+=InsCycles[md[NATIVE6309]][M1110];
	break;

case RESET:	//Undocumented
	HD6309Reset();
	break;

case SWI1_I: //3F
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
	break;

case NEGA_I: //40
	temp8= 0-A_REG;
	cc[C] = temp8>0;
	cc[V] = A_REG==0x80; //cc[C] ^ ((A_REG^temp8)>>7);
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	A_REG= temp8;
	CycleCounter+=InsCycles[md[NATIVE6309]][M21];
	break;

case COMA_I: //43
	A_REG= 0xFF- A_REG;
	cc[Z] = ZTEST(A_REG);
	cc[N] = NTEST8(A_REG);
	cc[C] = 1;
	cc[V] = 0;
	CycleCounter+=InsCycles[md[NATIVE6309]][M21];
	break;

case LSRA_I: //44
	cc[C] = A_REG & 1;
	A_REG= A_REG>>1;
	cc[Z] = ZTEST(A_REG);
	cc[N] = 0;
	CycleCounter+=InsCycles[md[NATIVE6309]][M21];
	break;

case RORA_I: //46
	postbyte=cc[C]<<7;
	cc[C] = A_REG & 1;
	A_REG= (A_REG>>1) | postbyte;
	cc[Z] = ZTEST(A_REG);
	cc[N] = NTEST8(A_REG);
	CycleCounter+=InsCycles[md[NATIVE6309]][M21];
	break;

case ASRA_I: //47
	cc[C] = A_REG & 1;
	A_REG= (A_REG & 0x80) | (A_REG >> 1);
	cc[Z] = ZTEST(A_REG);
	cc[N] = NTEST8(A_REG);
	CycleCounter+=InsCycles[md[NATIVE6309]][M21];
	break;

case ASLA_I: //48
	cc[C] = A_REG > 0x7F;
	cc[V] =  cc[C] ^((A_REG & 0x40)>>6);
	A_REG= A_REG<<1;
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	CycleCounter+=InsCycles[md[NATIVE6309]][M21];
	break;

case ROLA_I: //49
	postbyte=cc[C];
	cc[C] = A_REG > 0x7F;
	cc[V] = cc[C] ^ ((A_REG & 0x40)>>6);
	A_REG= (A_REG<<1) | postbyte;
	cc[Z] = ZTEST(A_REG);
	cc[N] = NTEST8(A_REG);
	CycleCounter+=InsCycles[md[NATIVE6309]][M21];
	break;

case DECA_I: //4A
	A_REG--;
	cc[Z] = ZTEST(A_REG);
	cc[V] = A_REG==0x7F;
	cc[N] = NTEST8(A_REG);
	CycleCounter+=InsCycles[md[NATIVE6309]][M21];
	break;

case INCA_I: //4C
	A_REG++;
	cc[Z] = ZTEST(A_REG);
	cc[V] = A_REG==0x80;
	cc[N] = NTEST8(A_REG);
	CycleCounter+=InsCycles[md[NATIVE6309]][M21];
	break;

case TSTA_I: //4D
	cc[Z] = ZTEST(A_REG);
	cc[N] = NTEST8(A_REG);
	cc[V] = 0;
	CycleCounter+=InsCycles[md[NATIVE6309]][M21];
	break;

case CLRA_I: //4F
	A_REG= 0;
	cc[C] =0;
	cc[V] = 0;
	cc[N] =0;
	cc[Z] =1;
	CycleCounter+=InsCycles[md[NATIVE6309]][M21];
	break;

case NEGB_I: //50
	temp8= 0-B_REG;
	cc[C] = temp8>0;
	cc[V] = B_REG == 0x80;	
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	B_REG= temp8;
	CycleCounter+=InsCycles[md[NATIVE6309]][M21];
	break;

case COMB_I: //53
	B_REG= 0xFF- B_REG;
	cc[Z] = ZTEST(B_REG);
	cc[N] = NTEST8(B_REG);
	cc[C] = 1;
	cc[V] = 0;
	CycleCounter+=InsCycles[md[NATIVE6309]][M21];
	break;

case LSRB_I: //54
	cc[C] = B_REG & 1;
	B_REG= B_REG>>1;
	cc[Z] = ZTEST(B_REG);
	cc[N] = 0;
	CycleCounter+=InsCycles[md[NATIVE6309]][M21];
	break;

case RORB_I: //56
	postbyte=cc[C]<<7;
	cc[C] = B_REG & 1;
	B_REG= (B_REG>>1) | postbyte;
	cc[Z] = ZTEST(B_REG);
	cc[N] = NTEST8(B_REG);
	CycleCounter+=InsCycles[md[NATIVE6309]][M21];
	break;

case ASRB_I: //57
	cc[C] = B_REG & 1;
	B_REG= (B_REG & 0x80) | (B_REG >> 1);
	cc[Z] = ZTEST(B_REG);
	cc[N] = NTEST8(B_REG);
	CycleCounter+=InsCycles[md[NATIVE6309]][M21];
	break;

case ASLB_I: //58
	cc[C] = B_REG > 0x7F;
	cc[V] =  cc[C] ^((B_REG & 0x40)>>6);
	B_REG= B_REG<<1;
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	CycleCounter+=InsCycles[md[NATIVE6309]][M21];
	break;

case ROLB_I: //59
	postbyte=cc[C];
	cc[C] = B_REG > 0x7F;
	cc[V] = cc[C] ^ ((B_REG & 0x40)>>6);
	B_REG= (B_REG<<1) | postbyte;
	cc[Z] = ZTEST(B_REG);
	cc[N] = NTEST8(B_REG);
	CycleCounter+=InsCycles[md[NATIVE6309]][M21];
	break;

case DECB_I: //5A
	B_REG--;
	cc[Z] = ZTEST(B_REG);
	cc[V] = B_REG==0x7F;
	cc[N] = NTEST8(B_REG);
	CycleCounter+=InsCycles[md[NATIVE6309]][M21];
	break;

case INCB_I: //5C
	B_REG++;
	cc[Z] = ZTEST(B_REG);
	cc[V] = B_REG==0x80; 
	cc[N] = NTEST8(B_REG);
	CycleCounter+=InsCycles[md[NATIVE6309]][M21];
	break;

case TSTB_I: //5D
	cc[Z] = ZTEST(B_REG);
	cc[N] = NTEST8(B_REG);
	cc[V] = 0;
	CycleCounter+=InsCycles[md[NATIVE6309]][M21];
	break;

case CLRB_I: //5F
	B_REG=0;
	cc[C] =0;
	cc[N] =0;
	cc[V] = 0;
	cc[Z] =1;
	CycleCounter+=InsCycles[md[NATIVE6309]][M21];
	break;

case NEG_X: //60
	temp16=INDADDRESS(PC_REG++);	
	postbyte=MemRead8(temp16);
	temp8= 0-postbyte;
	cc[C] = temp8>0;
	cc[V] = postbyte == 0x80;
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	MemWrite8(temp8,temp16);
	CycleCounter+=6;
	break;

case OIM_X: //61 6309 DONE
	postbyte=MemRead8(PC_REG++);
	temp16=INDADDRESS(PC_REG++);
	postbyte |= MemRead8(temp16);
	MemWrite8(postbyte,temp16);
	cc[N] = NTEST8(postbyte);
	cc[Z] = ZTEST(postbyte);
	cc[V] = 0;
	CycleCounter+=6;
	break;

case AIM_X: //62 6309 Phase 2
	postbyte=MemRead8(PC_REG++);
	temp16=INDADDRESS(PC_REG++);
	postbyte &= MemRead8(temp16);
	MemWrite8(postbyte,temp16);
	cc[N] = NTEST8(postbyte);
	cc[Z] = ZTEST(postbyte);
	cc[V] = 0;
	CycleCounter+=6;
	break;

case COM_X: //63
	temp16=INDADDRESS(PC_REG++);
	temp8=MemRead8(temp16);
	temp8= 0xFF-temp8;
	cc[Z] = ZTEST(temp8);
	cc[N] = NTEST8(temp8);
	cc[V] = 0;
	cc[C] = 1;
	MemWrite8(temp8,temp16);
	CycleCounter+=6;
	break;

case LSR_X: //64
	temp16=INDADDRESS(PC_REG++);
	temp8=MemRead8(temp16);
	cc[C] = temp8 & 1;
	temp8= temp8 >>1;
	cc[Z] = ZTEST(temp8);
	cc[N] = 0;
	MemWrite8(temp8,temp16);
	CycleCounter+=6;
	break;

case EIM_X: //65 6309 Untested TESTED NITRO
	postbyte=MemRead8(PC_REG++);
	temp16=INDADDRESS(PC_REG++);
	postbyte ^= MemRead8(temp16);
	MemWrite8(postbyte,temp16);
	cc[N] = NTEST8(postbyte);
	cc[Z] = ZTEST(postbyte);
	cc[V] = 0;
	CycleCounter+=7;
	break;

case ROR_X: //66
	temp16=INDADDRESS(PC_REG++);
	temp8=MemRead8(temp16);
	postbyte=cc[C]<<7;
	cc[C] = (temp8 & 1);
	temp8= (temp8>>1) | postbyte;
	cc[Z] = ZTEST(temp8);
	cc[N] = NTEST8(temp8);
	MemWrite8(temp8,temp16);
	CycleCounter+=6;
	break;

case ASR_X: //67
	temp16=INDADDRESS(PC_REG++);
	temp8=MemRead8(temp16);
	cc[C] = temp8 & 1;
	temp8= (temp8 & 0x80) | (temp8 >>1);
	cc[Z] = ZTEST(temp8);
	cc[N] = NTEST8(temp8);
	MemWrite8(temp8,temp16);
	CycleCounter+=6;
	break;

case ASL_X: //68 
	temp16=INDADDRESS(PC_REG++);
	temp8= MemRead8(temp16);
	cc[C] = temp8 > 0x7F;
	cc[V] = cc[C] ^ ((temp8 & 0x40)>>6);
	temp8= temp8<<1;
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);	
	MemWrite8(temp8,temp16);
	CycleCounter+=6;
	break;

case ROL_X: //69
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
	break;

case DEC_X: //6A
	temp16=INDADDRESS(PC_REG++);
	temp8=MemRead8(temp16);
	temp8--;
	cc[Z] = ZTEST(temp8);
	cc[N] = NTEST8(temp8);
	cc[V] = (temp8==0x7F);
	MemWrite8(temp8,temp16);
	CycleCounter+=6;
	break;

case TIM_X: //6B 6309
	postbyte=MemRead8(PC_REG++);
	temp8=MemRead8(INDADDRESS(PC_REG++));
	postbyte&=temp8;
	cc[N] = NTEST8(postbyte);
	cc[Z] = ZTEST(postbyte);
	cc[V] = 0;
	CycleCounter+=7;
	break;

case INC_X: //6C
	temp16=INDADDRESS(PC_REG++);
	temp8=MemRead8(temp16);
	temp8++;
	cc[V] = (temp8 == 0x80);
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	MemWrite8(temp8,temp16);
	CycleCounter+=6;
	break;

case TST_X: //6D
	temp8=MemRead8(INDADDRESS(PC_REG++));
	cc[Z] = ZTEST(temp8);
	cc[N] = NTEST8(temp8);
	cc[V] = 0;
	CycleCounter+=InsCycles[md[NATIVE6309]][M65];
	break;

case JMP_X: //6E
	PC_REG=INDADDRESS(PC_REG++);
	CycleCounter+=3;
	break;

case CLR_X: //6F
	MemWrite8(0,INDADDRESS(PC_REG++));
	cc[C] = 0;
	cc[N] = 0;
	cc[V] = 0;
	cc[Z] = 1; 
	CycleCounter+=6;
	break;

case NEG_E: //70
	temp16=IMMADDRESS(PC_REG);
	postbyte=MemRead8(temp16);
	temp8=0-postbyte;
	cc[C] = temp8>0;
	cc[V] = postbyte == 0x80;
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	MemWrite8(temp8,temp16);
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M76];
	break;

case OIM_E: //71 6309 Phase 2
	postbyte=MemRead8(PC_REG++);
	temp16=IMMADDRESS(PC_REG);
	postbyte|= MemRead8(temp16);
	MemWrite8(postbyte,temp16);
	cc[N] = NTEST8(postbyte);
	cc[Z] = ZTEST(postbyte);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=7;
	break;

case AIM_E: //72 6309 Untested CHECK NITRO
	postbyte=MemRead8(PC_REG++);
	temp16=IMMADDRESS(PC_REG);
	postbyte&= MemRead8(temp16);
	MemWrite8(postbyte,temp16);
	cc[N] = NTEST8(postbyte);
	cc[Z] = ZTEST(postbyte);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=7;
	break;

case COM_E: //73
	temp16=IMMADDRESS(PC_REG);
	temp8=MemRead8(temp16);
	temp8=0xFF-temp8;
	cc[Z] = ZTEST(temp8);
	cc[N] = NTEST8(temp8);
	cc[C] = 1;
	cc[V] = 0;
	MemWrite8(temp8,temp16);
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M76];
	break;

case LSR_E:  //74
	temp16=IMMADDRESS(PC_REG);
	temp8=MemRead8(temp16);
	cc[C] = temp8 & 1;
	temp8= temp8>>1;
	cc[Z] = ZTEST(temp8);
	cc[N] = 0;
	MemWrite8(temp8,temp16);
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M76];
	break;

case EIM_E: //75 6309 Untested CHECK NITRO
	postbyte=MemRead8(PC_REG++);
	temp16=IMMADDRESS(PC_REG);
	postbyte^= MemRead8(temp16);
	MemWrite8(postbyte,temp16);
	cc[N] = NTEST8(postbyte);
	cc[Z] = ZTEST(postbyte);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=7;
	break;

case ROR_E: //76
	temp16=IMMADDRESS(PC_REG);
	temp8=MemRead8(temp16);
	postbyte=cc[C]<<7;
	cc[C] = temp8 & 1;
	temp8= (temp8>>1) | postbyte;
	cc[Z] = ZTEST(temp8);
	cc[N] = NTEST8(temp8);
	MemWrite8(temp8,temp16);
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M76];
	break;

case ASR_E: //77
	temp16=IMMADDRESS(PC_REG);
	temp8=MemRead8(temp16);
	cc[C] = temp8 & 1;
	temp8= (temp8 & 0x80) | (temp8 >>1);
	cc[Z] = ZTEST(temp8);
	cc[N] = NTEST8(temp8);
	MemWrite8(temp8,temp16);
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M76];
	break;

case ASL_E: //78 6309
	temp16=IMMADDRESS(PC_REG);
	temp8= MemRead8(temp16);
	cc[C] = temp8 > 0x7F;
	cc[V] = cc[C] ^ ((temp8 & 0x40)>>6);
	temp8= temp8<<1;
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	MemWrite8(temp8,temp16);	
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M76];
	break;

case ROL_E: //79
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
	CycleCounter+=InsCycles[md[NATIVE6309]][M76];
	break;

case DEC_E: //7A
	temp16=IMMADDRESS(PC_REG);
	temp8=MemRead8(temp16);
	temp8--;
	cc[Z] = ZTEST(temp8);
	cc[N] = NTEST8(temp8);
	cc[V] = temp8==0x7F;
	MemWrite8(temp8,temp16);
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M76];
	break;

case TIM_E: //7B 6309 NITRO 
	postbyte=MemRead8(PC_REG++);
	temp16=IMMADDRESS(PC_REG);
	postbyte&=MemRead8(temp16);
	cc[N] = NTEST8(postbyte);
	cc[Z] = ZTEST(postbyte);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=7;
	break;

case INC_E: //7C
	temp16=IMMADDRESS(PC_REG);
	temp8=MemRead8(temp16);
	temp8++;
	cc[Z] = ZTEST(temp8);
	cc[V] = temp8==0x80;
	cc[N] = NTEST8(temp8);
	MemWrite8(temp8,temp16);
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M76];
	break;

case TST_E: //7D
	temp8=MemRead8(IMMADDRESS(PC_REG));
	cc[Z] = ZTEST(temp8);
	cc[N] = NTEST8(temp8);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M75];
	break;

case JMP_E: //7E
	PC_REG=IMMADDRESS(PC_REG);
	CycleCounter+=InsCycles[md[NATIVE6309]][M43];
	break;

case CLR_E: //7F
	MemWrite8(0,IMMADDRESS(PC_REG));
	cc[C] = 0;
	cc[N] = 0;
	cc[V] = 0;
	cc[Z] = 1;
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M76];
	break;

case SUBA_M: //80
	postbyte=MemRead8(PC_REG++);
	temp16 = A_REG - postbyte;
	cc[C] = (temp16 & 0x100)>>8; 
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,A_REG);
	A_REG = (unsigned char)temp16;
	cc[Z] = ZTEST(A_REG);
	cc[N] = NTEST8(A_REG);
	CycleCounter+=2;
	break;

case CMPA_M: //81
	postbyte=MemRead8(PC_REG++);
	temp8= A_REG-postbyte;
	cc[C] = temp8 > A_REG;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp8,A_REG);
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	CycleCounter+=2;
	break;

case SBCA_M:  //82
	postbyte=MemRead8(PC_REG++);
	temp16=A_REG-postbyte-cc[C];
	cc[C] = (temp16 & 0x100)>>8;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,A_REG);
	A_REG = (unsigned char)temp16;
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	CycleCounter+=2;
	break;

case SUBD_M: //83
	temp16=IMMADDRESS(PC_REG);
	temp32=D_REG-temp16;
	cc[C] = (temp32 & 0x10000)>>16;
	cc[V] = OVERFLOW16(cc[C],temp32,temp16,D_REG);
	D_REG = temp32;
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M43];
	break;

case ANDA_M: //84
	A_REG = A_REG & MemRead8(PC_REG++);
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	cc[V] = 0;
	CycleCounter+=2;
	break;

case BITA_M: //85
	temp8 = A_REG & MemRead8(PC_REG++);
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	cc[V] = 0;
	CycleCounter+=2;
	break;

case LDA_M: //86
	A_REG = MemRead8(PC_REG++);
	cc[Z] = ZTEST(A_REG);
	cc[N] = NTEST8(A_REG);
	cc[V] = 0;
	CycleCounter+=2;
	break;

case EORA_M: //88
	A_REG = A_REG ^ MemRead8(PC_REG++);
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	cc[V] = 0;
	CycleCounter+=2;
	break;

case ADCA_M: //89
	postbyte=MemRead8(PC_REG++);
	temp16= A_REG + postbyte + cc[C];
	cc[C] = (temp16 & 0x100)>>8;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,A_REG);
	cc[H] = ((A_REG ^ temp16 ^ postbyte) & 0x10)>>4;
	A_REG= (unsigned char)temp16;
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	CycleCounter+=2;
	break;

case ORA_M: //8A
	A_REG = A_REG | MemRead8(PC_REG++);
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	cc[V] = 0;
	CycleCounter+=2;
	break;

case ADDA_M: //8B
	postbyte=MemRead8(PC_REG++);
	temp16=A_REG+postbyte;
	cc[C] =(temp16 & 0x100)>>8;
	cc[H] = ((A_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,A_REG);
	A_REG= (unsigned char)temp16;
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	CycleCounter+=2;
	break;

case CMPX_M: //8C
	postword=IMMADDRESS(PC_REG);
	temp16 = X_REG-postword;
	cc[C] = temp16 > X_REG;
	cc[V] = OVERFLOW16(cc[C],postword,temp16,X_REG);
	cc[N] = NTEST16(temp16);
	cc[Z] = ZTEST(temp16);
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M43];
	break;

case BSR_R: //8D
	*spostbyte=MemRead8(PC_REG++);
	S_REG--;
	MemWrite8(pc.B.lsb,S_REG--);
	MemWrite8(pc.B.msb,S_REG);
	PC_REG+=*spostbyte;
	CycleCounter+=InsCycles[md[NATIVE6309]][M76];
	break;

case LDX_M: //8E
	X_REG = IMMADDRESS(PC_REG);
	cc[Z] = ZTEST(X_REG);
	cc[N] = NTEST16(X_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=3;
	break;

case SUBA_D: //90
	postbyte=MemRead8( DPADDRESS(PC_REG++));
	temp16 = A_REG - postbyte;
	cc[C] = (temp16 & 0x100)>>8; 
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,A_REG);
	A_REG = (unsigned char)temp16;
	cc[Z] = ZTEST(A_REG);
	cc[N] = NTEST8(A_REG);
	CycleCounter+=InsCycles[md[NATIVE6309]][M43];
	break;	

case CMPA_D: //91
	postbyte=MemRead8(DPADDRESS(PC_REG++));
	temp8 = A_REG-postbyte;
	cc[C] = temp8 > A_REG;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp8,A_REG);
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	CycleCounter+=InsCycles[md[NATIVE6309]][M43];
	break;

case SBCA_D: //92
	postbyte=MemRead8(DPADDRESS(PC_REG++));
	temp16=A_REG-postbyte-cc[C];
	cc[C] = (temp16 & 0x100)>>8;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,A_REG);
	A_REG = (unsigned char)temp16;
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	CycleCounter+=InsCycles[md[NATIVE6309]][M43];
	break;

case SUBD_D: //93
	temp16= MemRead16(DPADDRESS(PC_REG++));
	temp32= D_REG-temp16;
	cc[C] = (temp32 & 0x10000)>>16;
	cc[V] = OVERFLOW16(cc[C],temp32,temp16,D_REG);
	D_REG = temp32;
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	CycleCounter+=InsCycles[md[NATIVE6309]][M64];
	break;

case ANDA_D: //94
	A_REG = A_REG & MemRead8(DPADDRESS(PC_REG++));
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	cc[V] = 0;
	CycleCounter+=InsCycles[md[NATIVE6309]][M43];
	break;

case BITA_D: //95
	temp8 = A_REG & MemRead8(DPADDRESS(PC_REG++));
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	cc[V] = 0;
	CycleCounter+=InsCycles[md[NATIVE6309]][M43];
	break;

case LDA_D: //96
	A_REG = MemRead8(DPADDRESS(PC_REG++));
	cc[Z] = ZTEST(A_REG);
	cc[N] = NTEST8(A_REG);
	cc[V] = 0;
	CycleCounter+=InsCycles[md[NATIVE6309]][M43];
	break;

case STA_D: //97
	MemWrite8( A_REG,DPADDRESS(PC_REG++));
	cc[Z] = ZTEST(A_REG);
	cc[N] = NTEST8(A_REG);
	cc[V] = 0;
	CycleCounter+=InsCycles[md[NATIVE6309]][M43];
	break;

case EORA_D: //98
	A_REG= A_REG ^ MemRead8(DPADDRESS(PC_REG++));
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	cc[V] = 0;
	CycleCounter+=InsCycles[md[NATIVE6309]][M43];
	break;

case ADCA_D: //99
	postbyte=MemRead8(DPADDRESS(PC_REG++));
	temp16= A_REG + postbyte + cc[C];
	cc[C] = (temp16 & 0x100)>>8;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,A_REG);
	cc[H] = ((A_REG ^ temp16 ^ postbyte) & 0x10)>>4;
	A_REG= (unsigned char)temp16;
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	CycleCounter+=InsCycles[md[NATIVE6309]][M43];
	break;

case ORA_D: //9A
	A_REG = A_REG | MemRead8(DPADDRESS(PC_REG++));
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	cc[V] = 0;
	CycleCounter+=InsCycles[md[NATIVE6309]][M43];
	break;

case ADDA_D: //9B
	postbyte=MemRead8(DPADDRESS(PC_REG++));
	temp16=A_REG+postbyte;
	cc[C] =(temp16 & 0x100)>>8;
	cc[H] = ((A_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,A_REG);
	A_REG= (unsigned char)temp16;
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	CycleCounter+=InsCycles[md[NATIVE6309]][M43];
	break;

case CMPX_D: //9C
	postword=MemRead16(DPADDRESS(PC_REG++));
	temp16= X_REG - postword ;
	cc[C] = temp16 > X_REG;
	cc[V] = OVERFLOW16(cc[C],postword,temp16,X_REG);
	cc[N] = NTEST16(temp16);
	cc[Z] = ZTEST(temp16);
	CycleCounter+=InsCycles[md[NATIVE6309]][M64];
	break;

case BSR_D: //9D
	temp16 = DPADDRESS(PC_REG++);
	S_REG--;
	MemWrite8(pc.B.lsb,S_REG--);
	MemWrite8(pc.B.msb,S_REG);
	PC_REG=temp16;
	CycleCounter+=InsCycles[md[NATIVE6309]][M76];
	break;

case LDX_D: //9E
	X_REG=MemRead16(DPADDRESS(PC_REG++));
	cc[Z] = ZTEST(X_REG);	
	cc[N] = NTEST16(X_REG);
	cc[V] = 0;
	CycleCounter+=InsCycles[md[NATIVE6309]][M54];
	break;

case STX_D: //9F
	MemWrite16(X_REG,DPADDRESS(PC_REG++));
	cc[Z] = ZTEST(X_REG);
	cc[N] = NTEST16(X_REG);
	cc[V] = 0;
	CycleCounter+=InsCycles[md[NATIVE6309]][M54];
	break;

case SUBA_X: //A0
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp16 = A_REG - postbyte;
	cc[C] =(temp16 & 0x100)>>8; 
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,A_REG);
	A_REG= (unsigned char)temp16;
	cc[Z] = ZTEST(A_REG);
	cc[N] = NTEST8(A_REG);
	CycleCounter+=4;
	break;	

case CMPA_X: //A1
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp8= A_REG-postbyte;
	cc[C] = temp8 > A_REG;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp8,A_REG);
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	CycleCounter+=4;
	break;

case SBCA_X: //A2
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp16=A_REG-postbyte-cc[C];
	cc[C] = (temp16 & 0x100)>>8;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,A_REG);
	A_REG = (unsigned char)temp16;
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	CycleCounter+=4;
	break;

case SUBD_X: //A3
	temp16= MemRead16(INDADDRESS(PC_REG++));
	temp32= D_REG-temp16;
	cc[C] = (temp32 & 0x10000)>>16;
	cc[V] = OVERFLOW16(cc[C],temp32,temp16,D_REG);
	D_REG = temp32;
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	CycleCounter+=InsCycles[md[NATIVE6309]][M65];
	break;

case ANDA_X: //A4
	A_REG = A_REG & MemRead8(INDADDRESS(PC_REG++));
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	cc[V] = 0;
	CycleCounter+=4;
	break;

case BITA_X:  //A5
	temp8 = A_REG & MemRead8(INDADDRESS(PC_REG++));
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	cc[V] = 0;
	CycleCounter+=4;
	break;

case LDA_X: //A6
	A_REG = MemRead8(INDADDRESS(PC_REG++));
	cc[Z] = ZTEST(A_REG);
	cc[N] = NTEST8(A_REG);
	cc[V] = 0;
	CycleCounter+=4;
	break;

case STA_X: //A7
	MemWrite8(A_REG,INDADDRESS(PC_REG++));
	cc[Z] = ZTEST(A_REG);
	cc[N] = NTEST8(A_REG);
	cc[V] = 0;
	CycleCounter+=4;
	break;

case EORA_X: //A8
	A_REG= A_REG ^ MemRead8(INDADDRESS(PC_REG++));
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	cc[V] = 0;
	CycleCounter+=4;
	break;

case ADCA_X: //A9	
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp16= A_REG + postbyte + cc[C];
	cc[C] = (temp16 & 0x100)>>8;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,A_REG);
	cc[H] = ((A_REG ^ temp16 ^ postbyte) & 0x10)>>4;
	A_REG= (unsigned char)temp16;
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	CycleCounter+=4;
	break;

case ORA_X: //AA
	A_REG= A_REG | MemRead8(INDADDRESS(PC_REG++));
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	cc[V] = 0;
	CycleCounter+=4;
	break;

case ADDA_X: //AB
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp16= A_REG+postbyte;
	cc[C] = (temp16 & 0x100)>>8;
	cc[H] = ((A_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,A_REG);
	A_REG = (unsigned char)temp16;
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	CycleCounter+=4;
	break;

case CMPX_X: //AC
	postword=MemRead16(INDADDRESS(PC_REG++));
	temp16= X_REG - postword ;
	cc[C] = temp16 > X_REG;
	cc[V] = OVERFLOW16(cc[C],postword,temp16,X_REG);
	cc[N] = NTEST16(temp16);
	cc[Z] = ZTEST(temp16);
	CycleCounter+=InsCycles[md[NATIVE6309]][M65];
	break;

case BSR_X: //AD
	temp16=INDADDRESS(PC_REG++);
	S_REG--;
	MemWrite8(pc.B.lsb,S_REG--);
	MemWrite8(pc.B.msb,S_REG);
	PC_REG=temp16;
	CycleCounter+=InsCycles[md[NATIVE6309]][M76];
	break;

case LDX_X: //AE
	X_REG=MemRead16(INDADDRESS(PC_REG++));
	cc[Z] = ZTEST(X_REG);
	cc[N] = NTEST16(X_REG);
	cc[V] = 0;
	CycleCounter+=5;
	break;

case STX_X: //AF
	MemWrite16(X_REG,INDADDRESS(PC_REG++));
	cc[Z] = ZTEST(X_REG);
	cc[N] = NTEST16(X_REG);
	cc[V] = 0;
	CycleCounter+=5;
	break;

case SUBA_E: //B0
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp16 = A_REG - postbyte;
	cc[C] = (temp16 & 0x100)>>8; 
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,A_REG);
	A_REG= (unsigned char)temp16;
	cc[Z] = ZTEST(A_REG);
	cc[N] = NTEST8(A_REG);
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M54];
	break;

case CMPA_E: //B1
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp8= A_REG-postbyte;
	cc[C] = temp8 > A_REG;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp8,A_REG);
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M54];
	break;

case SBCA_E: //B2
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp16=A_REG-postbyte-cc[C];
	cc[C] = (temp16 & 0x100)>>8;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,A_REG);
	A_REG = (unsigned char)temp16;
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M54];
	break;

case SUBD_E: //B3
	temp16=MemRead16(IMMADDRESS(PC_REG));
	temp32=D_REG-temp16;
	cc[C] = (temp32 & 0x10000)>>16;
	cc[V] = OVERFLOW16(cc[C],temp32,temp16,D_REG);
	D_REG= temp32;
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M75];
	break;

case ANDA_E: //B4
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	A_REG = A_REG & postbyte;
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M54];
	break;

case BITA_E: //B5
	temp8 = A_REG & MemRead8(IMMADDRESS(PC_REG));
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M54];
	break;

case LDA_E: //B6
	A_REG= MemRead8(IMMADDRESS(PC_REG));
	cc[Z] = ZTEST(A_REG);
	cc[N] = NTEST8(A_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M54];
	break;

case STA_E: //B7
	MemWrite8(A_REG,IMMADDRESS(PC_REG));
	cc[Z] = ZTEST(A_REG);
	cc[N] = NTEST8(A_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M54];
	break;

case EORA_E:  //B8
	A_REG = A_REG ^ MemRead8(IMMADDRESS(PC_REG));
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M54];
	break;

case ADCA_E: //B9
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp16= A_REG + postbyte + cc[C];
	cc[C] = (temp16 & 0x100)>>8;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,A_REG);
	cc[H] = ((A_REG ^ temp16 ^ postbyte) & 0x10)>>4;
	A_REG= (unsigned char)temp16;
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M54];
	break;

case ORA_E: //BA
	A_REG = A_REG | MemRead8(IMMADDRESS(PC_REG));
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M54];
	break;

case ADDA_E: //BB
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp16=A_REG+postbyte;
	cc[C] = (temp16 & 0x100)>>8;
	cc[H] = ((A_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,A_REG);
	A_REG = (unsigned char)temp16;
	cc[N] = NTEST8(A_REG);
	cc[Z] = ZTEST(A_REG);
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M54];
	break;

case CMPX_E: //BC
	postword=MemRead16(IMMADDRESS(PC_REG));
	temp16 = X_REG-postword;
	cc[C] = temp16 > X_REG;
	cc[V] = OVERFLOW16(cc[C],postword,temp16,X_REG);
	cc[N] = NTEST16(temp16);
	cc[Z] = ZTEST(temp16);
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M75];
	break;

case BSR_E: //BD
	postword=IMMADDRESS(PC_REG);
	PC_REG+=2;
	S_REG--;
	MemWrite8(pc.B.lsb,S_REG--);
	MemWrite8(pc.B.msb,S_REG);
	PC_REG=postword;
	CycleCounter+=InsCycles[md[NATIVE6309]][M87];
	break;

case LDX_E: //BE
	X_REG=MemRead16(IMMADDRESS(PC_REG));
	cc[Z] = ZTEST(X_REG);
	cc[N] = NTEST16(X_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M65];
	break;

case STX_E: //BF
	MemWrite16(X_REG,IMMADDRESS(PC_REG));
	cc[Z] = ZTEST(X_REG);
	cc[N] = NTEST16(X_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M65];
	break;

case SUBB_M: //C0
	postbyte=MemRead8(PC_REG++);
	temp16 = B_REG - postbyte;
	cc[C] = (temp16 & 0x100)>>8; 
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,B_REG);
	B_REG= (unsigned char)temp16;
	cc[Z] = ZTEST(B_REG);
	cc[N] = NTEST8(B_REG);
	CycleCounter+=2;
	break;

case CMPB_M: //C1
	postbyte=MemRead8(PC_REG++);
	temp8= B_REG-postbyte;
	cc[C] = temp8 > B_REG;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp8,B_REG);
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	CycleCounter+=2;
	break;

case SBCB_M: //C3
	postbyte=MemRead8(PC_REG++);
	temp16=B_REG-postbyte-cc[C];
	cc[C] = (temp16 & 0x100)>>8;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,B_REG);
	B_REG = (unsigned char)temp16;
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	CycleCounter+=2;
	break;

case ADDD_M: //C3
	temp16=IMMADDRESS(PC_REG);
	temp32= D_REG+ temp16;
	cc[C] = (temp32 & 0x10000)>>16;
	cc[V] = OVERFLOW16(cc[C],temp32,temp16,D_REG);
	D_REG= temp32;
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M43];
	break;

case ANDB_M: //C4 LOOK
	B_REG = B_REG & MemRead8(PC_REG++);
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	cc[V] = 0;
	CycleCounter+=2;
	break;

case BITB_M: //C5
	temp8 = B_REG & MemRead8(PC_REG++);
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	cc[V] = 0;
	CycleCounter+=2;
	break;

case LDB_M: //C6
	B_REG=MemRead8(PC_REG++);
	cc[Z] = ZTEST(B_REG);
	cc[N] = NTEST8(B_REG);
	cc[V] = 0;
	CycleCounter+=2;
	break;

case EORB_M: //C8
	B_REG = B_REG ^ MemRead8(PC_REG++);
	cc[N] =NTEST8(B_REG);
	cc[Z] =ZTEST(B_REG);
	cc[V] = 0;
	CycleCounter+=2;
	break;

case ADCB_M: //C9
	postbyte=MemRead8(PC_REG++);
	temp16= B_REG + postbyte + cc[C];
	cc[C] = (temp16 & 0x100)>>8;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,B_REG);
	cc[H] = ((B_REG ^ temp16 ^ postbyte) & 0x10)>>4;
	B_REG= (unsigned char)temp16;
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	CycleCounter+=2;
	break;

case ORB_M: //CA
	B_REG= B_REG | MemRead8(PC_REG++);
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	cc[V] = 0;
	CycleCounter+=2;
	break;

case ADDB_M: //CB
	postbyte=MemRead8(PC_REG++);
	temp16=B_REG+postbyte;
	cc[C] =(temp16 & 0x100)>>8;
	cc[H] = ((B_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,B_REG);
	B_REG= (unsigned char)temp16;
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	CycleCounter+=2;
	break;

case LDD_M: //CC
	D_REG=IMMADDRESS(PC_REG);
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=3;
	break;

case LDQ_M: //CD 6309 WORK
	Q_REG = MemRead32(PC_REG);
	PC_REG+=4;
	cc[Z] = ZTEST(Q_REG);
	cc[N] = NTEST32(Q_REG);
	cc[V] = 0;
	CycleCounter+=5;
	break;

case LDU_M: //CE
	U_REG = IMMADDRESS(PC_REG);
	cc[Z] = ZTEST(U_REG);
	cc[N] = NTEST16(U_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=3;
	break;

case SUBB_D: //D0
	postbyte=MemRead8( DPADDRESS(PC_REG++));
	temp16 = B_REG - postbyte;
	cc[C] =(temp16 & 0x100)>>8; 
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,B_REG);
	B_REG = (unsigned char)temp16;
	cc[Z] = ZTEST(B_REG);
	cc[N] = NTEST8(B_REG);
	CycleCounter+=InsCycles[md[NATIVE6309]][M43];
	break;

case CMPB_D: //D1
	postbyte=MemRead8(DPADDRESS(PC_REG++));
	temp8= B_REG-postbyte;
	cc[C] = temp8 > B_REG;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp8,B_REG);
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	CycleCounter+=InsCycles[md[NATIVE6309]][M43];
	break;

case SBCB_D: //D2
	postbyte=MemRead8(DPADDRESS(PC_REG++));
	temp16=B_REG-postbyte-cc[C];
	cc[C] = (temp16 & 0x100)>>8;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,B_REG);
	B_REG = (unsigned char)temp16;
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	CycleCounter+=InsCycles[md[NATIVE6309]][M43];
	break;

case ADDD_D: //D3
	temp16=MemRead16( DPADDRESS(PC_REG++));
	temp32= D_REG+ temp16;
	cc[C] =(temp32 & 0x10000)>>16;
	cc[V] = OVERFLOW16(cc[C],temp32,temp16,D_REG);
	D_REG= temp32;
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	CycleCounter+=InsCycles[md[NATIVE6309]][M64];
	break;

case ANDB_D: //D4 
	B_REG = B_REG & MemRead8(DPADDRESS(PC_REG++));
	cc[N] =NTEST8(B_REG);
	cc[Z] =ZTEST(B_REG);
	cc[V] = 0;
	CycleCounter+=InsCycles[md[NATIVE6309]][M43];
	break;

case BITB_D: //D5
	temp8 = B_REG & MemRead8(DPADDRESS(PC_REG++));
	cc[N] = NTEST8(temp8);
	cc[Z] =ZTEST(temp8);
	cc[V] = 0;
	CycleCounter+=InsCycles[md[NATIVE6309]][M43];
	break;

case LDB_D: //D6
	B_REG= MemRead8( DPADDRESS(PC_REG++));
	cc[Z] = ZTEST(B_REG);
	cc[N] = NTEST8(B_REG);
	cc[V] = 0;
	CycleCounter+=InsCycles[md[NATIVE6309]][M43];
	break;

case	STB_D: //D7
	MemWrite8( B_REG,DPADDRESS(PC_REG++));
	cc[Z] = ZTEST(B_REG);
	cc[N] = NTEST8(B_REG);
	cc[V] = 0;
	CycleCounter+=InsCycles[md[NATIVE6309]][M43];
	break;

case EORB_D: //D8	
	B_REG = B_REG ^ MemRead8(DPADDRESS(PC_REG++));
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	cc[V] = 0;
	CycleCounter+=InsCycles[md[NATIVE6309]][M43];
	break;

case ADCB_D: //D9
	postbyte=MemRead8(DPADDRESS(PC_REG++));
	temp16= B_REG + postbyte + cc[C];
	cc[C] = (temp16 & 0x100)>>8;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,B_REG);
	cc[H] = ((B_REG ^ temp16 ^ postbyte) & 0x10)>>4;
	B_REG = (unsigned char)temp16;
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	CycleCounter+=InsCycles[md[NATIVE6309]][M43];
	break;

case ORB_D: //DA
	B_REG = B_REG | MemRead8(DPADDRESS(PC_REG++));
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	cc[V] = 0;
	CycleCounter+=InsCycles[md[NATIVE6309]][M43];
	break;

case ADDB_D: //DB
	postbyte=MemRead8(DPADDRESS(PC_REG++));
	temp16= B_REG+postbyte;
	cc[C] = (temp16 & 0x100)>>8;
	cc[H] = ((B_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,B_REG);
	B_REG = (unsigned char)temp16;
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	CycleCounter+=InsCycles[md[NATIVE6309]][M43];
	break;

case LDD_D: //DC
	D_REG = MemRead16(DPADDRESS(PC_REG++));
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	cc[V] = 0;
	CycleCounter+=InsCycles[md[NATIVE6309]][M54];
	break;

case STD_D: //DD
	MemWrite16(D_REG,DPADDRESS(PC_REG++));
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	cc[V] = 0;
	CycleCounter+=InsCycles[md[NATIVE6309]][M54];
	break;

case LDU_D: //DE
	U_REG = MemRead16(DPADDRESS(PC_REG++));
	cc[Z] = ZTEST(U_REG);
	cc[N] = NTEST16(U_REG);
	cc[V] = 0;
	CycleCounter+=InsCycles[md[NATIVE6309]][M54];
	break;

case STU_D: //DF
	MemWrite16(U_REG,DPADDRESS(PC_REG++));
	cc[Z] = ZTEST(U_REG);
	cc[N] = NTEST16(U_REG);
	cc[V] = 0;
	CycleCounter+=InsCycles[md[NATIVE6309]][M54];
	break;

case SUBB_X: //E0
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp16 = B_REG - postbyte;
	cc[C] = (temp16 & 0x100)>>8; 
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,B_REG);
	B_REG = (unsigned char)temp16;
	cc[Z] = ZTEST(B_REG);
	cc[N] = NTEST8(B_REG);
	CycleCounter+=4;
	break;

case CMPB_X: //E1
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp8 = B_REG-postbyte;
	cc[C] = temp8 > B_REG;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp8,B_REG);
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	CycleCounter+=4;
	break;

case SBCB_X: //E2
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp16=B_REG-postbyte-cc[C];
	cc[C] = (temp16 & 0x100)>>8;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,B_REG);
	B_REG = (unsigned char)temp16;
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	CycleCounter+=4;
	break;

case ADDD_X: //E3 
	temp16=MemRead16(INDADDRESS(PC_REG++));
	temp32= D_REG+ temp16;
	cc[C] =(temp32 & 0x10000)>>16;
	cc[V] = OVERFLOW16(cc[C],temp32,temp16,D_REG);
	D_REG= temp32;
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	CycleCounter+=InsCycles[md[NATIVE6309]][M65];
	break;

case ANDB_X: //E4
	B_REG = B_REG & MemRead8(INDADDRESS(PC_REG++));
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	cc[V] = 0;
	CycleCounter+=4;
	break;

case BITB_X: //E5 
	temp8 = B_REG & MemRead8(INDADDRESS(PC_REG++));
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	cc[V] = 0;
	CycleCounter+=4;
	break;

case LDB_X: //E6
	B_REG=MemRead8(INDADDRESS(PC_REG++));
	cc[Z] = ZTEST(B_REG);
	cc[N] = NTEST8(B_REG);
	cc[V] = 0;
	CycleCounter+=4;
	break;

case STB_X: //E7
	MemWrite8(B_REG,CalculateEA( MemRead8(PC_REG++)));
	cc[Z] = ZTEST(B_REG);
	cc[N] = NTEST8(B_REG);
	cc[V] = 0;
	CycleCounter+=4;
	break;

case EORB_X: //E8
	B_REG= B_REG ^ MemRead8(INDADDRESS(PC_REG++));
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	cc[V] = 0;
	CycleCounter+=4;
	break;

case ADCB_X: //E9
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp16= B_REG + postbyte + cc[C];
	cc[C] = (temp16 & 0x100)>>8;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,B_REG);
	cc[H] = ((B_REG ^ temp16 ^ postbyte) & 0x10)>>4;
	B_REG= (unsigned char)temp16;
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	CycleCounter+=4;
	break;

case ORB_X: //EA 
	B_REG = B_REG | MemRead8(INDADDRESS(PC_REG++));
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	cc[V] = 0;
	CycleCounter+=4;

	break;

case ADDB_X: //EB
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp16=B_REG+postbyte;
	cc[C] =(temp16 & 0x100)>>8;
	cc[H] = ((B_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,B_REG);
	B_REG = (unsigned char)temp16;
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	CycleCounter+=4;
	break;

case LDD_X: //EC
	D_REG=MemRead16(INDADDRESS(PC_REG++));
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	cc[V] = 0;
	CycleCounter+=5;
	break;

case STD_X: //ED
	MemWrite16(D_REG,INDADDRESS(PC_REG++));
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	cc[V] = 0;
	CycleCounter+=5;
	break;

case LDU_X: //EE
	U_REG=MemRead16(INDADDRESS(PC_REG++));
	cc[Z] =ZTEST(U_REG);
	cc[N] =NTEST16(U_REG);
	cc[V] = 0;
	CycleCounter+=5;
	break;

case STU_X: //EF
	MemWrite16(U_REG,INDADDRESS(PC_REG++));
	cc[Z] = ZTEST(U_REG);
	cc[N] = NTEST16(U_REG);
	cc[V] = 0;
	CycleCounter+=5;
	break;

case SUBB_E: //F0
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp16 = B_REG - postbyte;
	cc[C] = (temp16 & 0x100)>>8; 
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,B_REG);
	B_REG= (unsigned char)temp16;
	cc[Z] = ZTEST(B_REG);
	cc[N] = NTEST8(B_REG);
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M54];
	break;

case CMPB_E: //F1
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp8= B_REG-postbyte;
	cc[C] = temp8 > B_REG;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp8,B_REG);
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M54];
	break;

case SBCB_E: //F2
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp16=B_REG-postbyte-cc[C];
	cc[C] = (temp16 & 0x100)>>8;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,B_REG);
	B_REG = (unsigned char)temp16;
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M54];
	break;

case ADDD_E: //F3
	temp16=MemRead16(IMMADDRESS(PC_REG));
	temp32= D_REG+ temp16;
	cc[C] =(temp32 & 0x10000)>>16;
	cc[V] = OVERFLOW16(cc[C],temp32,temp16,D_REG);
	D_REG = temp32;
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M76];
	break;

case ANDB_E:  //F4
	B_REG = B_REG & MemRead8(IMMADDRESS(PC_REG));
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M54];
	break;

case BITB_E: //F5
	temp8 = B_REG & MemRead8(IMMADDRESS(PC_REG));
	cc[N] = NTEST8(temp8);
	cc[Z] = ZTEST(temp8);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M54];
	break;

case LDB_E: //F6
	B_REG=MemRead8(IMMADDRESS(PC_REG));
	cc[Z] = ZTEST(B_REG);
	cc[N] = NTEST8(B_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M54];
	break;

case STB_E: //F7 
	MemWrite8(B_REG,IMMADDRESS(PC_REG));
	cc[Z] = ZTEST(B_REG);
	cc[N] = NTEST8(B_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M54];
	break;

case EORB_E: //F8
	B_REG = B_REG ^ MemRead8(IMMADDRESS(PC_REG));
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M54];
	break;

case ADCB_E: //F9
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp16= B_REG + postbyte + cc[C];
	cc[C] = (temp16 & 0x100)>>8;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,B_REG);
	cc[H] = ((B_REG ^ temp16 ^ postbyte) & 0x10)>>4;
	B_REG= (unsigned char)temp16;
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M54];
	break;

case ORB_E: //FA
	B_REG = B_REG | MemRead8(IMMADDRESS(PC_REG));
	cc[N] = NTEST8(B_REG);
	cc[Z] = ZTEST(B_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M54];
	break;

case ADDB_E: //FB
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp16=B_REG+postbyte;
	cc[C] =(temp16 & 0x100)>>8;
	cc[H] = ((B_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc[V] = OVERFLOW8(cc[C],postbyte,temp16,B_REG);
	B_REG= (unsigned char)temp16;
	cc[N] = NTEST8(B_REG);
	cc[Z] =ZTEST(B_REG);
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M54];
	break;

case LDD_E: //FC
	D_REG=MemRead16(IMMADDRESS(PC_REG));
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M65];
	break;

case STD_E: //FD
	MemWrite16(D_REG,IMMADDRESS(PC_REG));
	cc[Z] = ZTEST(D_REG);
	cc[N] = NTEST16(D_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M65];
	break;

case LDU_E: //FE
	U_REG= MemRead16(IMMADDRESS(PC_REG));
	cc[Z] = ZTEST(U_REG);
	cc[N] = NTEST16(U_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M65];
	break;

case STU_E: //FF
	MemWrite16(U_REG,IMMADDRESS(PC_REG));
	cc[Z] = ZTEST(U_REG);
	cc[N] = NTEST16(U_REG);
	cc[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[md[NATIVE6309]][M65];
	break;

default:
	InvalidInsHandler();
	break;
	}//End Switch
}//End While

return(CycleFor-CycleCounter);

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
	static unsigned short int ea=0;
	static signed char byte=0;
	static unsigned char Register;

	Register= ((postbyte>>5)&3)+1;

	if (postbyte & 0x80) 
	{
		switch (postbyte & 0x1F)
		{
		case 0:
			ea=(*xfreg16[Register]);
			(*xfreg16[Register])++;
			CycleCounter+=2;
			break;

		case 1:
			ea=(*xfreg16[Register]);
			(*xfreg16[Register])+=2;
			CycleCounter+=3;
			break;

		case 2:
			(*xfreg16[Register])-=1;
			ea=(*xfreg16[Register]);
			CycleCounter+=2;
			break;

		case 3:
			(*xfreg16[Register])-=2;
			ea=(*xfreg16[Register]);
			CycleCounter+=3;
			break;

		case 4:
			ea=(*xfreg16[Register]);
			break;

		case 5:
			ea=(*xfreg16[Register])+((signed char)B_REG);
			CycleCounter+=1;
			break;

		case 6:
			ea=(*xfreg16[Register])+((signed char)A_REG);
			CycleCounter+=1;
			break;

		case 7:
			ea=(*xfreg16[Register])+((signed char)E_REG);
			CycleCounter+=1;
			break;

		case 8:
			ea=(*xfreg16[Register])+(signed char)MemRead8(PC_REG++);
			CycleCounter+=1;
			break;

		case 9:
			ea=(*xfreg16[Register])+IMMADDRESS(PC_REG);
			CycleCounter+=4;
			PC_REG+=2;
			break;

		case 10:
			ea=(*xfreg16[Register])+((signed char)F_REG);
			CycleCounter+=1;
			break;

		case 11:
			ea=(*xfreg16[Register])+D_REG; //Changed to unsigned 03/14/2005 NG Was signed
			CycleCounter+=4;
			break;

		case 12:
			ea=(signed short)PC_REG+(signed char)MemRead8(PC_REG)+1; 
			CycleCounter+=1;
			PC_REG++;
			break;

		case 13: //MM
			ea=PC_REG+IMMADDRESS(PC_REG)+2; 
			CycleCounter+=5;
			PC_REG+=2;
			break;

		case 14:
			ea=(*xfreg16[Register])+W_REG; 
			CycleCounter+=4;
			break;

		case 15: //01111
			byte=(postbyte>>5)&3;
			switch (byte)
			{
			case 0:
				ea=W_REG;
				break;
			case 1:
				ea=W_REG+IMMADDRESS(PC_REG);
				PC_REG+=2;
				break;
			case 2:
				ea=W_REG;
				W_REG+=2;
				break;
			case 3:
				W_REG-=2;
				ea=W_REG;
				break;
			}
		break;

		case 16: //10000
			byte=(postbyte>>5)&3;
			switch (byte)
			{
			case 0:
				ea=MemRead16(W_REG);
				break;
			case 1:
				ea=MemRead16(W_REG+IMMADDRESS(PC_REG));
				PC_REG+=2;
				break;
			case 2:
				ea=MemRead16(W_REG);
				W_REG+=2;
				break;
			case 3:
				W_REG-=2;
				ea=MemRead16(W_REG);
				break;
			}
		break;


		case 17: //10001
			ea=(*xfreg16[Register]);
			(*xfreg16[Register])+=2;
			ea=MemRead16(ea);
			CycleCounter+=6;
			break;

		case 18: //10010
			CycleCounter+=6;
			break;

		case 19: //10011
			(*xfreg16[Register])-=2;
			ea=(*xfreg16[Register]);
			ea=MemRead16(ea);
			CycleCounter+=6;
			break;

		case 20: //10100
			ea=(*xfreg16[Register]);
			ea=MemRead16(ea);
			CycleCounter+=3;
			break;

		case 21: //10101
			ea=(*xfreg16[Register])+((signed char)B_REG);
			ea=MemRead16(ea);
			CycleCounter+=4;
			break;

		case 22: //10110
			ea=(*xfreg16[Register])+((signed char)A_REG);
			ea=MemRead16(ea);
			CycleCounter+=4;
			break;

		case 23: //10111
			ea=(*xfreg16[Register])+((signed char)E_REG);
			ea=MemRead16(ea);
			CycleCounter+=4;
			break;

		case 24: //11000
			ea=(*xfreg16[Register])+(signed char)MemRead8(PC_REG++);
			ea=MemRead16(ea);
			CycleCounter+=4;
			break;

		case 25: //11001
			ea=(*xfreg16[Register])+IMMADDRESS(PC_REG);
			ea=MemRead16(ea);
			CycleCounter+=7;
			PC_REG+=2;
			break;
		case 26: //11010
			ea=(*xfreg16[Register])+((signed char)F_REG);
			ea=MemRead16(ea);
			CycleCounter+=4;
			break;

		case 27: //11011
			ea=(*xfreg16[Register])+D_REG;
			ea=MemRead16(ea);
			CycleCounter+=7;
			break;

		case 28: //11100
			ea=(signed short)PC_REG+(signed char)MemRead8(PC_REG)+1; 
			ea=MemRead16(ea);
			CycleCounter+=4;
			PC_REG++;
			break;

		case 29: //11101
			ea=PC_REG+IMMADDRESS(PC_REG)+2; 
			ea=MemRead16(ea);
			CycleCounter+=8;
			PC_REG+=2;
			break;

		case 30: //11110
			ea=(*xfreg16[Register])+W_REG; 
			ea=MemRead16(ea);
			CycleCounter+=7;
			break;

		case 31: //11111
			ea=IMMADDRESS(PC_REG);
			ea=MemRead16(ea);
			CycleCounter+=8;
			PC_REG+=2;
			break;

		} //END Switch
	}
	else 
	{
		byte= (postbyte & 31);
		byte= (byte << 3);
		byte= byte /8;
		ea= *xfreg16[Register]+byte; //Was signed
		CycleCounter+=1;
	}
return(ea);
}



void setcc (unsigned char bincc)
{
	unsigned char bit;
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
	for (bit=0;bit<=7;bit++)
		md[bit]=!!(binmd & (1<<bit));
	return;
}

unsigned char getmd(void)
{
	unsigned char binmd=0,bit=0;
	for (bit=0;bit<=7;bit++)
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
	CycleCounter+=(12 + InsCycles[md[NATIVE6309]][M54]);	//One for each byte +overhead? Guessing from PSHS
	return;
}

unsigned int MemRead32(unsigned short Address)
{
	return ( (MemRead16(Address)<<16) | MemRead16(Address+2) );

}
void MemWrite32(unsigned int data,unsigned short Address)
{
	MemWrite16( data>>16,Address);
	MemWrite16( data & 0xFFFF,Address+2);
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

unsigned short GetPC(void)
{
	return(PC_REG);
}


int performDivQ(short divisor)
{
	int divOkay = 1;	//true
	if (divisor != 0)
	{
		stemp32 = Q_REG;
		stempW = stemp32 / divisor;
		if (stempW > 0xFFFF || stempW<SHRT_MIN)
		{
			//overflow, quotient is larger then can be stored in 16 bits
			//registers remain untouched, set flags accordingly
			cc[V] = 1;
			cc[N] = 0;
			cc[Z] = 0;
			cc[C] = 0;
		}
		else
		{
			W_REG = stempW;
			D_REG = stemp32 % divisor;
			cc[N] = W_REG >> 15;	//set to bit 15
			cc[Z] = ZTEST(W_REG);
			cc[C] = W_REG & 1;

			//check for overflow, quotient can't be represented by signed 16 bit value
			cc[V] = stempW > SHRT_MAX;
			if (cc[V])
				cc[N] = 1;
		}
	}
	else
	{
		divOkay = 0;
		DivbyZero();
	}
	return divOkay;
}
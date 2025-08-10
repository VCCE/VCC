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

#include <windows.h>
#include <stdio.h>
#include "defines.h"
#include "mc6809.h"
#include "mc6809defs.h"
#include "tcc1014mmu.h"
#include "OpDecoder.h"
#include "Disassembler.h"

//Global variables for CPU Emulation-----------------------

#define NTEST8(r) r>0x7F;
#define NTEST16(r) r>0x7FFF;
#define OTEST8(c,a,b,r) c ^ (((a^b^r)>>7) &1);
#define OTEST16(c,a,b,r) c ^ (((a^b^r)>>15)&1);
#define ZTEST(r) !r;

// Quiet legacy compiler warning about forcing int value to bool
#pragma warning( disable : 4800 )

typedef union
{
	unsigned short Reg;
	struct
	{
		unsigned char lsb,msb;
	} B;
} cpuregister;

#define D_REG	d.Reg
#define PC_REG	pc.Reg
#define X_REG	x.Reg
#define Y_REG	y.Reg
#define U_REG	u.Reg
#define S_REG	s.Reg
#define A_REG	d.B.msb
#define B_REG	d.B.lsb
#define DP_REG	dp.B.msb

static cpuregister pc,x,y,u,s,dp,d;
static std::array<bool, 8> cc;
static int CycleCounter=0;
static unsigned int SyncWaiting=0;
static unsigned int temp32;
static unsigned short temp16;
static unsigned char temp8;
static unsigned char InterruptLine[IS_MAX] = { 0 };
static VCC::DFF InterruptLatch;
static unsigned char Source=0,Dest=0;
static unsigned char postbyte=0;
static short unsigned postword=0;
static signed char *spostbyte=(signed char *)&postbyte;
static signed short *spostword=(signed short *)&postword;
static std::vector<unsigned short> CPUBreakpoints;
static std::vector<unsigned short> CPUTraceTriggers;
static int HaltedInsPending = 0;

//END Global variables for CPU Emulation-------------------

//Fuction Prototypes---------------------------------------
_inline unsigned short CalculateEA(unsigned char);
static void set_cc_flags(unsigned char);
static unsigned char get_cc_flags();
static void cpu_firq(void);
static void cpu_irq(void);
static void cpu_nmi(void);
static void Do_Opcode(int);
static void P2_Opcode(void);
static void P3_Opcode(void);

#include "CpuCommon.h"

//END Fuction Prototypes-----------------------------------
void MC6809Init(void)
{
}

void MC6809Reset(void)
{
	// Reset registers to 0
	D_REG = 0;
	X_REG = 0;
	Y_REG = 0;
	U_REG = 0;
	S_REG = 0;
	PC_REG = 0;
	DP_REG = 0;
	set_cc_flags(0);

	// Set the initial state of the CPU and flags
	cc[I]=1;
	cc[F]=1;
	SyncWaiting=0;
	pc.Reg=MemRead16(VRESET);	//PC gets its reset vector
	SetMapType(0);
}

VCC::CPUState MC6809GetState()
{
	VCC::CPUState regs = { 0 };

	regs.CC = get_cc_flags();
	regs.DP = DP_REG;
	regs.A = A_REG;
	regs.B = B_REG;
	regs.X = X_REG;
	regs.Y = Y_REG;
	regs.U = U_REG;
	regs.S = S_REG;
	regs.PC = PC_REG;

	regs.IsNative6309 = false;

	return regs;
}

void MC6809SetBreakpoints(const std::vector<unsigned short>& breakpoints)
{
	CPUBreakpoints = breakpoints;
}

void MC6809SetTraceTriggers(const std::vector<unsigned short>& triggers)
{
	CPUTraceTriggers = triggers;
}


// This function performs a read from a register index specified in a
// TFR or EXG instruction. This function only returns the correct value
// for EXG instructions when the first register in the exchange (MSN) is
// an 8 bit register. The value is returned as a 16 bit value for all
// registers based on the behavior of the 6809 CPU as follows:
//
//	* All 16 bit registers return their respective values.
//	* ACCA and ACCB return their values with the MSB set to 0xFF.
//	* Condition Code (CC) and Data Pointer (DP) return their values in
//    both the MSG and LSB.
//  * Invalid register encodings return a constant value of 0xFFFF.
//
static uint16_t MC6809ReadTfrExgRegister(uint8_t reg)
{
	uint16_t result;

	switch (reg & 0x0F)
	{
	case  0:	// D
		result = D_REG;
		break;

	case  1:	// X
		result = X_REG;
		break;

	case  2:	// Y
		result = Y_REG;
		break;

	case  3:	// U
		result = U_REG;
		break;

	case  4:	// S
		result = S_REG;
		break;

	case  5:	// PC
		result = PC_REG;
		break;

	case  8:	// A
		result = 0xff00 | A_REG;
		break;

	case  9:	// B
		result = 0xff00 | B_REG;
		break;

	case 10:	// CC
	{
		const unsigned char ccflags = get_cc_flags();
		result = (uint16_t)(ccflags << 8) | ccflags;
		break;
	}

	case 11:	// DP
		result = (uint16_t)(DP_REG << 8) | DP_REG;
		break;  

	default:
		result = 0xffff;
		break;
	}

	return result;
}

// This function performs a read from a register index specified in an
// EXG instruction when the first register in the exchange (MSN) is
// a 16 bit register. The value is returned as a 16 bit value for all
// registers based on the behavior of the 6809 CPU as follows:
//
//	* All 16 bit registers return their respective values.
//	* All 8 bit registers return respective value with the MSB set to 0xFF.
//  * Invalid register encodings return a constant value of 0xFFFF.
//
static uint16_t MC6809ReadExgRegister(uint8_t reg)
{
	uint16_t result;

	switch (reg & 0x0F)
	{
	case  0:	// D
		result = D_REG;
		break;

	case  1:	// X
		result = X_REG;
		break;

	case  2:	// Y
		result = Y_REG;
		break;

	case  3:	// U
		result = U_REG;
		break;

	case  4:	// S
		result = S_REG;
		break;

	case  5:	// PC
		result = PC_REG;
		break;

	case  8:	// A
		result = 0xff00 | A_REG;
		break;

	case  9:	// B
		result = 0xff00 | B_REG;
		break;

	case 10:	// CC
		result = 0xff00 | get_cc_flags();
		break;

	case 11:	// DP
		result = 0xff00 | DP_REG;
		break;

	default:
		result = 0xffff;
		break;
	}

	return result;
}

// This function performs a write to a register index specified in a
// TFR or EXG instruction.
static void MC6809WriteTfrExgRegister(uint8_t reg, uint16_t value)
{
	switch(reg & 0x0F)
	{
	case  0:	// D
		D_REG = value;
		break;

	case  1:	// X
		X_REG = value;
		break;

	case  2:	// Y
		Y_REG = value;
		break;

	case  3:	// U
		U_REG = value;
		break;

	case  4:	// S
		S_REG = value;
		break;

	case  5:	// PC
		PC_REG = value;
		break;

	case  8:	// A
		A_REG = value & 0xff;
		break;

	case  9:	// B
		B_REG = value & 0xff;
		break;

	case 10:	// CC
		set_cc_flags(value & 0xff);
		break;

	case 11:	// DP
		DP_REG = value & 0xff;
		break;

	default:
		// Invalid register encoding, do nothing
		break;
	}
}



// Do instructions for CycleFor cycles. Return number cycles over.
int MC6809Exec(int CycleFor)
{
	static unsigned char opcode=0;
	extern int JS_Ramp_Clock;
	int PrevCycleCount = 0;
	CycleCounter=0;

	// Instruction Loop
	while (CycleCounter<CycleFor) {

		// CPU is halted.
		if (EmuState.Debugger.IsHalted()) {
			return(CycleFor - CycleCounter);
		}
		// CPU is stepping.
		if (EmuState.Debugger.IsStepping()) {
			Do_Opcode(CycleFor);
			EmuState.Debugger.Halt();
			return(CycleFor - CycleCounter);
		}
		// Halted instruction pending.
		if (HaltedInsPending) {
			VCC::ApplyHaltpoints(0);
			Do_Opcode(CycleFor);
			VCC::ApplyHaltpoints(1);
			HaltedInsPending = 0;
			return(CycleFor - CycleCounter);
		}

		LatchInterrupts();

		if (NMI())
			cpu_nmi();
		else if (FIRQ() && !cc[F])
			cpu_firq();
		else if (IRQ() && !cc[I])
			cpu_irq();

		// Wait for Sync
		if (SyncWaiting==1)	// Note: Assert interrupt clears sync waiting
			return(0);

		// Any CPU Breakpoints set?
		if (!EmuState.Debugger.IsStepping() && !CPUBreakpoints.empty()) {
			if (find(CPUBreakpoints.begin(), CPUBreakpoints.end(), pc.Reg) != CPUBreakpoints.end())
			{
				EmuState.Debugger.Halt();
				return(CycleFor - CycleCounter);
			}
		}

		// Is the execution trace enabled - but currently not running?
		if (EmuState.Debugger.IsTracingEnabled() && !EmuState.Debugger.IsTracing())
		{
			// Only Start Tracing when we hit a start trigger.
			if (!CPUTraceTriggers.empty())
			{
				if (find(CPUTraceTriggers.begin(), CPUTraceTriggers.end(), pc.Reg) != CPUTraceTriggers.end()) {
					EmuState.Debugger.TraceStart();
				}
			} else {
				// Otherwise start right away.
				EmuState.Debugger.TraceStart();
			}
		}

		// Trace is running.
		if (EmuState.Debugger.IsTracing()) {
			EmuState.Debugger.TraceCaptureBefore(CycleCounter, MC6809GetState());
		}

		// Do an instruction
		Do_Opcode(CycleFor);

		// After instruction trace capture
		if (EmuState.Debugger.IsTracing()) {
			EmuState.Debugger.TraceCaptureAfter(CycleCounter, MC6809GetState());
		}
		// Advance the JoyStick Ramp timer
		if (JS_Ramp_Clock < 0xFFFF) {
			JS_Ramp_Clock += CycleCounter-PrevCycleCount;
		}

		PrevCycleCount = CycleCounter;

	} // End instruction loop

	return(CycleFor-CycleCounter);

} // End MC6809Exec


// Execute an instruction
void Do_Opcode(int CycleFor)
{
	static unsigned char msn,lsn; //Most signifcant, least significant nibbles

	switch (MemRead8(pc.Reg++)) {

	case NEG_D: //0
		temp16=(dp.Reg |MemRead8(pc.Reg++));
		postbyte=MemRead8(temp16);
		temp8=0-postbyte;
		cc[C]=temp8>0;
		cc[V]= (postbyte==0x80);
		cc[N]= NTEST8(temp8);
		cc[Z]= ZTEST(temp8);
		MemWrite8(temp8,temp16);
		CycleCounter+=6;
		break;

	case COM_D: //3
		temp16=(dp.Reg |MemRead8(pc.Reg++));
		temp8=MemRead8(temp16);
		temp8=0xFF-temp8;
		cc[Z]= ZTEST(temp8);
		cc[N]= NTEST8(temp8);
		cc[C]=1;
		cc[V] = 0;
		MemWrite8(temp8,temp16);
		CycleCounter+=6;
		break;

	case LSR_D: //4
		temp16=(dp.Reg |MemRead8(pc.Reg++));
		temp8=MemRead8(temp16);
		cc[C]= temp8 & 1;
		temp8= temp8 >>1;
		cc[Z]= ZTEST(temp8);
		cc[N]=0;
		MemWrite8(temp8,temp16);
		CycleCounter+=6;
		break;

	case ROR_D: //6
		temp16=(dp.Reg |MemRead8(pc.Reg++));
		temp8=MemRead8(temp16);
		postbyte= cc[C]<<7;
		cc[C]= temp8 & 1;
		temp8= (temp8 >> 1)| postbyte;
		cc[Z]= ZTEST(temp8);
		cc[N]= NTEST8(temp8);
		MemWrite8(temp8,temp16);
		CycleCounter+=6;
		break;

	case ASR_D: //7
		temp16=(dp.Reg |MemRead8(pc.Reg++));
		temp8=MemRead8(temp16);
		cc[C]= temp8 & 1;
		temp8 = (temp8 & 0x80) | (temp8 >>1);
		cc[Z]= ZTEST(temp8);
		cc[N]= NTEST8(temp8);
		MemWrite8(temp8,temp16);
		CycleCounter+=6;
		break;

	case ASL_D: //8
		temp16=(dp.Reg |MemRead8(pc.Reg++));
		temp8=MemRead8(temp16);
		cc[C]= (temp8 & 0x80) >>7;
		cc[V]= cc[C] ^ ((temp8 & 0x40) != 0);
		temp8= temp8 <<1;
		cc[N]= NTEST8(temp8);
		cc[Z]= ZTEST(temp8);
		MemWrite8(temp8,temp16);
		CycleCounter+=6;
		break;

	case ROL_D:	//9
		temp16=(dp.Reg |MemRead8(pc.Reg++));
		temp8=MemRead8(temp16);
		postbyte=cc[C];
		cc[C]=(temp8 & 0x80)>>7;
		cc[V]= cc[C] ^ ((temp8 & 0x40) != 0);
		temp8 = (temp8<<1) | postbyte;
		cc[Z]= ZTEST(temp8);
		cc[N]= NTEST8(temp8);
		MemWrite8(temp8,temp16);
		CycleCounter+=6;
		break;

	case DEC_D: //A
		temp16=(dp.Reg |MemRead8(pc.Reg++));
		temp8=MemRead8(temp16)-1;
		cc[Z]= ZTEST(temp8);
		cc[N]= NTEST8(temp8);
		cc[V]= temp8==0x7F;
		MemWrite8(temp8,temp16);
		CycleCounter+=6;
		break;

	case INC_D: //C
		temp16=(dp.Reg |MemRead8(pc.Reg++));
		temp8=MemRead8(temp16)+1;
		cc[Z]= ZTEST(temp8);
		cc[V]= temp8==0x80;
		cc[N]= NTEST8(temp8);
		MemWrite8(temp8,temp16);
		CycleCounter+=6;
		break;

	case TST_D: //D
		temp8=MemRead8((dp.Reg |MemRead8(pc.Reg++)));
		cc[Z]= ZTEST(temp8);
		cc[N]= NTEST8(temp8);
		cc[V] = 0;
		CycleCounter+=6;
		break;

	case JMP_D: //E
		pc.Reg= ((dp.Reg |MemRead8(pc.Reg)));
		CycleCounter+=3;
		break;

	case CLR_D: //F
		MemWrite8(0,(dp.Reg |MemRead8(pc.Reg++)));
		cc[Z]=1;
		cc[N]=0;
		cc[V] = 0;
		cc[C]=0;
		CycleCounter+=6;
		break;

	case Page2: //10
		P2_Opcode();
		break;

	case Page3: //11
		P3_Opcode();
		break;

	case NOP_I: //12
		CycleCounter+=2;
		break;

	case SYNC_I: //13
		CycleCounter=CycleFor;
		SyncWaiting=1;
		break;

	case HALT: //15
		if (EmuState.Debugger.Halt_Enabled()) {
			HaltedInsPending = 1;
			VCC::ApplyHaltpoints(0);
			EmuState.Debugger.Halt();
			PC_REG -= 1;
		} else {
			CycleCounter+=2;
		}
		break;

	case LBRA_R: //16
		*spostword=MemRead16(pc.Reg);
		pc.Reg+=2;
		pc.Reg+=*spostword;
		CycleCounter+=5;
		break;

	case LBSR_R: //17
		*spostword=MemRead16(pc.Reg);
		pc.Reg+=2;
		s.Reg--;
		MemWrite8(pc.B.lsb,s.Reg--);
		MemWrite8(pc.B.msb,s.Reg);
		pc.Reg+=*spostword;
		CycleCounter+=9;
		break;

	case DAA_I: //19
	//	MessageBox(0,"DAA","Ok",0);
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
		cc[C]|=((temp16 & 0x100) != 0);
		A_REG= temp16 & 0xFF;
		cc[N]= NTEST8(A_REG);
		cc[Z]= ZTEST(A_REG);
		CycleCounter+=2;
		break;

	case ORCC_M: //1A
		postbyte=MemRead8(pc.Reg++);
		temp8=get_cc_flags();
		temp8 = (temp8 | postbyte);
		set_cc_flags(temp8);
		CycleCounter+=3;
		break;

	case ANDCC_M: //1C
		postbyte=MemRead8(pc.Reg++);
		temp8=get_cc_flags();
		temp8 = (temp8 & postbyte);
		set_cc_flags(temp8);
		CycleCounter+=3;
		break;

	case SEX_I: //1D
//		A_REG = ((signed char)B_REG < 0) ? 0xff : 0x00;
		A_REG= 0-(B_REG>>7);
		cc[Z]= ZTEST(D_REG);
		cc[N]= NTEST16(D_REG);
		CycleCounter+=2;
		break;

	case EXG_M: //1E
		postbyte=MemRead8(pc.Reg);
		++pc.Reg;
		{
			// Get the indexes of the first and second registers.
			const unsigned char first_register = postbyte >> 4;
			const unsigned char second_register = postbyte & 0x0f;

			// Exchange differs if 16 bit register or 8 bit register is first
			uint16_t first_value;
			uint16_t second_value;
			if ((postbyte & 0x80) != 0)
			{
				// The first register is an 8 bit register which enables
				// TFR/EXG behavior when reading 8 bit registers.
				first_value = MC6809ReadTfrExgRegister(first_register);
				second_value = MC6809ReadTfrExgRegister(second_register);
			}
			else
			{
				// The first register is a 16 bit register which has unique
				// behavior when reading 8 bit registers.
				first_value = MC6809ReadExgRegister(first_register);
				second_value = MC6809ReadExgRegister(second_register);
			}

			MC6809WriteTfrExgRegister(second_register, first_value);
			MC6809WriteTfrExgRegister(first_register, second_value);
		}

		CycleCounter+=8;
		break;

	case TFR_M: //1F
		postbyte = MemRead8(pc.Reg);
		++pc.Reg;
		{
			Source = postbyte >> 4; // Source register
			Dest = postbyte & 15; // Destination register

			// Move the register value from the source to the destination.
			const uint16_t value = MC6809ReadTfrExgRegister(Source);
			MC6809WriteTfrExgRegister(Dest, value);
			break;
		}

		CycleCounter += 6;
		break;

	case BRA_R: //20
		*spostbyte=MemRead8(pc.Reg++);
		pc.Reg+=*spostbyte;
		CycleCounter+=3;
		break;

	case BRN_R: //21
		CycleCounter+=3;
		pc.Reg++;
		break;

	case BHI_R: //22
		if  (!(cc[C] | cc[Z]))
			pc.Reg+=(signed char)MemRead8(pc.Reg);
		pc.Reg++;
		CycleCounter+=3;
		break;

	case BLS_R: //23
		if (cc[C] | cc[Z])
			pc.Reg+=(signed char)MemRead8(pc.Reg);
		pc.Reg++;
		CycleCounter+=3;
		break;

	case BHS_R: //24
		if (!cc[C])
			pc.Reg+=(signed char)MemRead8(pc.Reg);
		pc.Reg++;
		CycleCounter+=3;
		break;

	case BLO_R: //25
		if (cc[C])
			pc.Reg+=(signed char)MemRead8(pc.Reg);
		pc.Reg++;
		CycleCounter+=3;
		break;

	case BNE_R: //26
		if (!cc[Z])
			pc.Reg+=(signed char)MemRead8(pc.Reg);
		pc.Reg++;
		CycleCounter+=3;
		break;

	case BEQ_R: //27
		if (cc[Z])
			pc.Reg+=(signed char)MemRead8(pc.Reg);
		pc.Reg++;
		CycleCounter+=3;
		break;

	case BVC_R: //28
		if (!cc[V])
			pc.Reg+=(signed char)MemRead8(pc.Reg);
		pc.Reg++;
		CycleCounter+=3;
		break;

	case BVS_R: //29
		if ( cc[V])
			pc.Reg+=(signed char)MemRead8(pc.Reg);
		pc.Reg++;
		CycleCounter+=3;
		break;

	case BPL_R: //2A
		if (!cc[N])
			pc.Reg+=(signed char)MemRead8(pc.Reg);
		pc.Reg++;
		CycleCounter+=3;
		break;

	case BMI_R: //2B
		if ( cc[N])
			pc.Reg+=(signed char)MemRead8(pc.Reg);
		pc.Reg++;
		CycleCounter+=3;
		break;

	case BGE_R: //2C
		if (! (cc[N] ^ cc[V]))
			pc.Reg+=(signed char)MemRead8(pc.Reg);
		pc.Reg++;
		CycleCounter+=3;
		break;

	case BLT_R: //2D
		if ( cc[V] ^ cc[N])
			pc.Reg+=(signed char)MemRead8(pc.Reg);
		pc.Reg++;
		CycleCounter+=3;
		break;

	case BGT_R: //2E
		if ( !( cc[Z] | (cc[N]^cc[V] ) ))
			pc.Reg+=(signed char)MemRead8(pc.Reg);
		pc.Reg++;
		CycleCounter+=3;
		break;

	case BLE_R: //2F
		if ( cc[Z] | (cc[N]^cc[V]) )
			pc.Reg+=(signed char)MemRead8(pc.Reg);
		pc.Reg++;
		CycleCounter+=3;
		break;

	case LEAX_X: //30
		x.Reg=CalculateEA(MemRead8(pc.Reg++));
		cc[Z]= ZTEST(x.Reg);
		CycleCounter+=4;
		break;

	case LEAY_X: //31
		y.Reg=CalculateEA(MemRead8(pc.Reg++));
		cc[Z]= ZTEST(y.Reg);
		CycleCounter+=4;
		break;

	case LEAS_X: //32
		s.Reg=CalculateEA(MemRead8(pc.Reg++));
		CycleCounter+=4;
		break;

	case LEAU_X: //33
		u.Reg=CalculateEA(MemRead8(pc.Reg++));
		CycleCounter+=4;
		break;

	case PSHS_M: //34
		postbyte=MemRead8(pc.Reg++);
		if (postbyte & 0x80)
		{
			MemWrite8( pc.B.lsb,--s.Reg);
			MemWrite8( pc.B.msb,--s.Reg);
			CycleCounter+=2;
		}
		if (postbyte & 0x40)
		{
			MemWrite8( u.B.lsb,--s.Reg);
			MemWrite8( u.B.msb,--s.Reg);
			CycleCounter+=2;
		}
		if (postbyte & 0x20)
		{
			MemWrite8( y.B.lsb,--s.Reg);
			MemWrite8( y.B.msb,--s.Reg);
			CycleCounter+=2;
		}
		if (postbyte & 0x10)
		{
			MemWrite8( x.B.lsb,--s.Reg);
			MemWrite8( x.B.msb,--s.Reg);
			CycleCounter+=2;
		}
		if (postbyte & 0x08)
		{
			MemWrite8( dp.B.msb,--s.Reg);
			CycleCounter+=1;
		}
		if (postbyte & 0x04)
		{
			MemWrite8(B_REG,--s.Reg);
			CycleCounter+=1;
		}
		if (postbyte & 0x02)
		{
			MemWrite8(A_REG,--s.Reg);
			CycleCounter+=1;
		}
		if (postbyte & 0x01)
		{
			MemWrite8(get_cc_flags(),--s.Reg);
			CycleCounter+=1;
		}

		CycleCounter+=5;
		break;

	case PULS_M: //35
		postbyte=MemRead8(pc.Reg++);
		if (postbyte & 0x01)
		{
			set_cc_flags(MemRead8(s.Reg++));
			CycleCounter+=1;
		}
		if (postbyte & 0x02)
		{
			A_REG=MemRead8(s.Reg++);
			CycleCounter+=1;
		}
		if (postbyte & 0x04)
		{
			B_REG=MemRead8(s.Reg++);
			CycleCounter+=1;
		}
		if (postbyte & 0x08)
		{
			dp.B.msb=MemRead8(s.Reg++);
			CycleCounter+=1;
		}
		if (postbyte & 0x10)
		{
			x.B.msb=MemRead8( s.Reg++);
			x.B.lsb=MemRead8( s.Reg++);
			CycleCounter+=2;
		}
		if (postbyte & 0x20)
		{
			y.B.msb=MemRead8( s.Reg++);
			y.B.lsb=MemRead8( s.Reg++);
			CycleCounter+=2;
		}
		if (postbyte & 0x40)
		{
			u.B.msb=MemRead8( s.Reg++);
			u.B.lsb=MemRead8( s.Reg++);
			CycleCounter+=2;
		}
		if (postbyte & 0x80)
		{
			pc.B.msb=MemRead8( s.Reg++);
			pc.B.lsb=MemRead8( s.Reg++);
			CycleCounter+=2;
		}
		CycleCounter+=5;
		break;

	case PSHU_M: //36
		postbyte=MemRead8(pc.Reg++);
		if (postbyte & 0x80)
		{
			MemWrite8( pc.B.lsb,--u.Reg);
			MemWrite8( pc.B.msb,--u.Reg);
			CycleCounter+=2;
		}
		if (postbyte & 0x40)
		{
			MemWrite8( s.B.lsb,--u.Reg);
			MemWrite8( s.B.msb,--u.Reg);
			CycleCounter+=2;
		}
		if (postbyte & 0x20)
		{
			MemWrite8( y.B.lsb,--u.Reg);
			MemWrite8( y.B.msb,--u.Reg);
			CycleCounter+=2;
		}
		if (postbyte & 0x10)
		{
			MemWrite8( x.B.lsb,--u.Reg);
			MemWrite8( x.B.msb,--u.Reg);
			CycleCounter+=2;
		}
		if (postbyte & 0x08)
		{
			MemWrite8( dp.B.msb,--u.Reg);
			CycleCounter+=1;
		}
		if (postbyte & 0x04)
		{
			MemWrite8(B_REG,--u.Reg);
			CycleCounter+=1;
		}
		if (postbyte & 0x02)
		{
			MemWrite8(A_REG,--u.Reg);
			CycleCounter+=1;
		}
		if (postbyte & 0x01)
		{
			MemWrite8(get_cc_flags(),--u.Reg);
			CycleCounter+=1;
		}
		CycleCounter+=5;
		break;

	case PULU_M: //37
		postbyte=MemRead8(pc.Reg++);
		if (postbyte & 0x01)
		{
			set_cc_flags(MemRead8(u.Reg++));
			CycleCounter+=1;
		}
		if (postbyte & 0x02)
		{
			A_REG=MemRead8(u.Reg++);
			CycleCounter+=1;
		}
		if (postbyte & 0x04)
		{
			B_REG=MemRead8(u.Reg++);
			CycleCounter+=1;
		}
		if (postbyte & 0x08)
		{
			dp.B.msb=MemRead8(u.Reg++);
			CycleCounter+=1;
		}
		if (postbyte & 0x10)
		{
			x.B.msb=MemRead8( u.Reg++);
			x.B.lsb=MemRead8( u.Reg++);
			CycleCounter+=2;
		}
		if (postbyte & 0x20)
		{
			y.B.msb=MemRead8( u.Reg++);
			y.B.lsb=MemRead8( u.Reg++);
			CycleCounter+=2;
		}
		if (postbyte & 0x40)
		{
			s.B.msb=MemRead8( u.Reg++);
			s.B.lsb=MemRead8( u.Reg++);
			CycleCounter+=2;
		}
		if (postbyte & 0x80)
		{
			pc.B.msb=MemRead8( u.Reg++);
			pc.B.lsb=MemRead8( u.Reg++);
			CycleCounter+=2;
		}
		CycleCounter+=5;
		break;

	case RTS_I: //39
		pc.B.msb=MemRead8(s.Reg++);
		pc.B.lsb=MemRead8(s.Reg++);
		CycleCounter+=5;
		break;

	case ABX_I: //3A
		x.Reg=x.Reg+B_REG;
		CycleCounter+=3;
		break;

	case RTI_I: //3B
		set_cc_flags(MemRead8(s.Reg++));
		CycleCounter+=6;
		if (cc[E])
		{
			A_REG=MemRead8(s.Reg++);
			B_REG=MemRead8(s.Reg++);
			dp.B.msb=MemRead8(s.Reg++);
			x.B.msb=MemRead8(s.Reg++);
			x.B.lsb=MemRead8(s.Reg++);
			y.B.msb=MemRead8(s.Reg++);
			y.B.lsb=MemRead8(s.Reg++);
			u.B.msb=MemRead8(s.Reg++);
			u.B.lsb=MemRead8(s.Reg++);
			CycleCounter+=9;
		}
		pc.B.msb=MemRead8(s.Reg++);
		pc.B.lsb=MemRead8(s.Reg++);
		break;

	case CWAI_I: //3C
		postbyte=MemRead8(pc.Reg++);
		set_cc_flags(get_cc_flags() & postbyte);
		CycleCounter=CycleFor;
		SyncWaiting=1;
		break;

	case MUL_I: //3D
		D_REG = A_REG * B_REG;
		cc[C]= B_REG >0x7F;
		cc[Z]= ZTEST(D_REG);
		CycleCounter+=11;
		break;

	case RESET:	//Undocumented
		MC6809Reset();
		break;

	case SWI1_I: //3F
		cc[E]=1;
		MemWrite8( pc.B.lsb,--s.Reg);
		MemWrite8( pc.B.msb,--s.Reg);
		MemWrite8( u.B.lsb,--s.Reg);
		MemWrite8( u.B.msb,--s.Reg);
		MemWrite8( y.B.lsb,--s.Reg);
		MemWrite8( y.B.msb,--s.Reg);
		MemWrite8( x.B.lsb,--s.Reg);
		MemWrite8( x.B.msb,--s.Reg);
		MemWrite8( dp.B.msb,--s.Reg);
		MemWrite8(B_REG,--s.Reg);
		MemWrite8(A_REG,--s.Reg);
		MemWrite8(get_cc_flags(),--s.Reg);
		pc.Reg=MemRead16(VSWI);
		CycleCounter+=19;
		cc[I]=1;
		cc[F]=1;
		break;

	case NEGA_I: //40
		temp8= 0-A_REG;
		cc[C]= temp8>0;
		cc[V]= A_REG==0x80; //cc[C] ^ ((A_REG^temp8)>>7);
		cc[N]= NTEST8(temp8);
		cc[Z]= ZTEST(temp8);
		A_REG= temp8;
		CycleCounter+=2;
		break;

	case COMA_I: //43
		A_REG= 0xFF- A_REG;
		cc[Z]= ZTEST(A_REG);
		cc[N]= NTEST8(A_REG);
		cc[C]= 1;
		cc[V]= 0;
		CycleCounter+=2;
		break;

	case LSRA_I: //44
		cc[C]= A_REG & 1;
		A_REG= A_REG>>1;
		cc[Z]= ZTEST(A_REG);
		cc[N]= 0;
		CycleCounter+=2;
		break;

	case RORA_I: //46
		postbyte=cc[C]<<7;
		cc[C]= A_REG & 1;
		A_REG= (A_REG>>1) | postbyte;
		cc[Z]= ZTEST(A_REG);
		cc[N]= NTEST8(A_REG);
		CycleCounter+=2;
		break;

	case ASRA_I: //47
		cc[C]= A_REG & 1;
		A_REG= (A_REG & 0x80) | (A_REG >> 1);
		cc[Z]= ZTEST(A_REG);
		cc[N]= NTEST8(A_REG);
		CycleCounter+=2;
		break;

	case ASLA_I: //48 JF
		cc[C]= A_REG > 0x7F;
		cc[V]=  cc[C] ^((A_REG & 0x40) != 0);
		A_REG= A_REG<<1;
		cc[N]= NTEST8(A_REG);
		cc[Z]= ZTEST(A_REG);
		CycleCounter+=2;
		break;

	case ROLA_I: //49
		postbyte=cc[C];
		cc[C]= A_REG > 0x7F;
		cc[V]= cc[C] ^ ((A_REG & 0x40) != 0);
		A_REG= (A_REG<<1) | postbyte;
		cc[Z]= ZTEST(A_REG);
		cc[N]= NTEST8(A_REG);
		CycleCounter+=2;
		break;

	case DECA_I: //4A
		A_REG--;
		cc[Z]= ZTEST(A_REG);
		cc[V]= A_REG==0x7F;
		cc[N]= NTEST8(A_REG);
		CycleCounter+=2;
		break;

	case INCA_I: //4C
		A_REG++;
		cc[Z]= ZTEST(A_REG);
		cc[V]= A_REG==0x80;
		cc[N]= NTEST8(A_REG);
		CycleCounter+=2;
		break;

	case TSTA_I: //4D
		cc[Z]= ZTEST(A_REG);
		cc[N]= NTEST8(A_REG);
		cc[V]= 0;
		CycleCounter+=2;
		break;

	case CLRA_I: //4F
		A_REG= 0;
		cc[C]=0;
		cc[V] = 0;
		cc[N]=0;
		cc[Z]=1;
		CycleCounter+=2;
		break;

	case NEGB_I: //50
		temp8= 0-B_REG;
		cc[C]= temp8>0;
		cc[V]= B_REG == 0x80;
		cc[N]= NTEST8(temp8);
		cc[Z]= ZTEST(temp8);
		B_REG= temp8;
		CycleCounter+=2;
		break;

	case COMB_I: //53
		B_REG= 0xFF- B_REG;
		cc[Z]= ZTEST(B_REG);
		cc[N]= NTEST8(B_REG);
		cc[C]= 1;
		cc[V]= 0;
		CycleCounter+=2;
		break;

	case LSRB_I: //54
		cc[C]= B_REG & 1;
		B_REG= B_REG>>1;
		cc[Z]= ZTEST(B_REG);
		cc[N]= 0;
		CycleCounter+=2;
		break;

	case RORB_I: //56
		postbyte=cc[C]<<7;
		cc[C]= B_REG & 1;
		B_REG= (B_REG>>1) | postbyte;
		cc[Z]= ZTEST(B_REG);
		cc[N]= NTEST8(B_REG);
		CycleCounter+=2;
		break;

	case ASRB_I: //57
		cc[C]= B_REG & 1;
		B_REG= (B_REG & 0x80) | (B_REG >> 1);
		cc[Z]= ZTEST(B_REG);
		cc[N]= NTEST8(B_REG);
		CycleCounter+=2;
		break;

	case ASLB_I: //58
		cc[C]= B_REG > 0x7F;
		cc[V]=  cc[C] ^((B_REG & 0x40) != 0);
		B_REG= B_REG<<1;
		cc[N]= NTEST8(B_REG);
		cc[Z]= ZTEST(B_REG);
		CycleCounter+=2;
		break;

	case ROLB_I: //59
		postbyte=cc[C];
		cc[C]= B_REG > 0x7F;
		cc[V]= cc[C] ^ ((B_REG & 0x40) != 0);
		B_REG= (B_REG<<1) | postbyte;
		cc[Z]= ZTEST(B_REG);
		cc[N]= NTEST8(B_REG);
		CycleCounter+=2;
		break;

	case DECB_I: //5A
		B_REG--;
		cc[Z]= ZTEST(B_REG);
		cc[V]= B_REG==0x7F;
		cc[N]= NTEST8(B_REG);
		CycleCounter+=2;
		break;

	case INCB_I: //5C
		B_REG++;
		cc[Z]= ZTEST(B_REG);
		cc[V]= B_REG==0x80;
		cc[N]= NTEST8(B_REG);
		CycleCounter+=2;
		break;

	case TSTB_I: //5D
		cc[Z]= ZTEST(B_REG);
		cc[N]= NTEST8(B_REG);
		cc[V]= 0;
		CycleCounter+=2;
		break;

	case CLRB_I: //5F
		B_REG=0;
		cc[C]=0;
		cc[N]=0;
		cc[V] = 0;
		cc[Z]=1;
		CycleCounter+=2;
		break;

	case NEG_X: //60
		temp16=CalculateEA(MemRead8(pc.Reg++));
		postbyte=MemRead8(temp16);
		temp8= 0-postbyte;
		cc[C]= temp8>0;
		cc[V]= postbyte == 0x80;
		cc[N]= NTEST8(temp8);
		cc[Z]= ZTEST(temp8);
		MemWrite8(temp8,temp16);
		CycleCounter+=6;
		break;

	case COM_X: //63
		temp16=CalculateEA(MemRead8(pc.Reg++));
		temp8=MemRead8(temp16);
		temp8= 0xFF-temp8;
		cc[Z]= ZTEST(temp8);
		cc[N]= NTEST8(temp8);
		cc[V]= 0;
		cc[C]= 1;
		MemWrite8(temp8,temp16);
		CycleCounter+=6;
		break;

	case LSR_X: //64
		temp16=CalculateEA(MemRead8(pc.Reg++));
		temp8=MemRead8(temp16);
		cc[C]= temp8 & 1;
		temp8= temp8 >>1;
		cc[Z]= ZTEST(temp8);
		cc[N]= 0;
		MemWrite8(temp8,temp16);
		CycleCounter+=6;
		break;

	case ROR_X: //66
		temp16=CalculateEA(MemRead8(pc.Reg++));
		temp8=MemRead8(temp16);
		postbyte=cc[C]<<7;
		cc[C]= (temp8 & 1);
		temp8= (temp8>>1) | postbyte;
		cc[Z]= ZTEST(temp8);
		cc[N]= NTEST8(temp8);
		MemWrite8(temp8,temp16);
		CycleCounter+=6;
		break;

	case ASR_X: //67
		temp16=CalculateEA(MemRead8(pc.Reg++));
		temp8=MemRead8(temp16);
		cc[C]= temp8 & 1;
		temp8= (temp8 & 0x80) | (temp8 >>1);
		cc[Z]= ZTEST(temp8);
		cc[N]= NTEST8(temp8);
		MemWrite8(temp8,temp16);
		CycleCounter+=6;
		break;

	case ASL_X: //68
		temp16=CalculateEA(MemRead8(pc.Reg++));
		temp8= MemRead8(temp16);
		cc[C]= temp8 > 0x7F;
		cc[V]= cc[C] ^ ((temp8 & 0x40) != 0);
		temp8= temp8<<1;
		cc[N]= NTEST8(temp8);
		cc[Z]= ZTEST(temp8);
		MemWrite8(temp8,temp16);
		CycleCounter+=6;
		break;

	case ROL_X: //69
		temp16=CalculateEA(MemRead8(pc.Reg++));
		temp8=MemRead8(temp16);
		postbyte=cc[C];
		cc[C]= temp8 > 0x7F;
		cc[V]= ( cc[C] ^ ((temp8 & 0x40) != 0));
		temp8= ((temp8<<1) | postbyte);
		cc[Z]= ZTEST(temp8);
		cc[N]= NTEST8(temp8);
		MemWrite8(temp8,temp16);
		CycleCounter+=6;
		break;

	case DEC_X: //6A
		temp16=CalculateEA(MemRead8(pc.Reg++));
		temp8=MemRead8(temp16);
		temp8--;
		cc[Z]= ZTEST(temp8);
		cc[N]= NTEST8(temp8);
		cc[V]= (temp8==0x7F);
		MemWrite8(temp8,temp16);
		CycleCounter+=6;
		break;

	case INC_X: //6C
		temp16=CalculateEA(MemRead8(pc.Reg++));
		temp8=MemRead8(temp16);
		temp8++;
		cc[V]= (temp8 == 0x80);
		cc[N]= NTEST8(temp8);
		cc[Z]= ZTEST(temp8);
		MemWrite8(temp8,temp16);
		CycleCounter+=6;
		break;

	case TST_X: //6D
		temp8=MemRead8(CalculateEA(MemRead8(pc.Reg++)));
		cc[Z]= ZTEST(temp8);
		cc[N]= NTEST8(temp8);
		cc[V]= 0;
		CycleCounter+=6;
		break;

	case JMP_X: //6E
		pc.Reg=CalculateEA(MemRead8(pc.Reg++));
		CycleCounter+=3;
		break;

	case CLR_X: //6F
		MemWrite8(0,CalculateEA(MemRead8(pc.Reg++)));
		cc[C]= 0;
		cc[N]= 0;
		cc[V]= 0;
		cc[Z]= 1;
		CycleCounter+=6;
		break;

	case NEG_E: //70
		temp16=MemRead16(pc.Reg);
		postbyte=MemRead8(temp16);
		temp8=0-postbyte;
		cc[C]= temp8>0;
		cc[V]= postbyte == 0x80;
		cc[N]= NTEST8(temp8);
		cc[Z]= ZTEST(temp8);
		MemWrite8(temp8,temp16);
		pc.Reg+=2;
		CycleCounter+=7;
		break;

	case COM_E: //73
		temp16=MemRead16(pc.Reg);
		temp8=MemRead8(temp16);
		temp8=0xFF-temp8;
		cc[Z]= ZTEST(temp8);
		cc[N]= NTEST8(temp8);
		cc[C]= 1;
		cc[V]= 0;
		MemWrite8(temp8,temp16);
		pc.Reg+=2;
		CycleCounter+=7;
		break;

	case LSR_E:  //74
		temp16=MemRead16(pc.Reg);
		temp8=MemRead8(temp16);
		cc[C]= temp8 & 1;
		temp8= temp8>>1;
		cc[Z]= ZTEST(temp8);
		cc[N]= 0;
		MemWrite8(temp8,temp16);
		pc.Reg+=2;
		CycleCounter+=7;
		break;

	case ROR_E: //76
		temp16=MemRead16(pc.Reg);
		temp8=MemRead8(temp16);
		postbyte=cc[C]<<7;
		cc[C]= temp8 & 1;
		temp8= (temp8>>1) | postbyte;
		cc[Z]= ZTEST(temp8);
		cc[N]= NTEST8(temp8);
		MemWrite8(temp8,temp16);
		pc.Reg+=2;
		CycleCounter+=7;
		break;

	case ASR_E: //77
		temp16=MemRead16(pc.Reg);
		temp8=MemRead8(temp16);
		cc[C]= temp8 & 1;
		temp8= (temp8 & 0x80) | (temp8 >>1);
		cc[Z]= ZTEST(temp8);
		cc[N]= NTEST8(temp8);
		MemWrite8(temp8,temp16);
		pc.Reg+=2;
		CycleCounter+=7;
		break;

	case ASL_E: //78
		temp16=MemRead16(pc.Reg);
		temp8= MemRead8(temp16);
		cc[C]= temp8 > 0x7F;
		cc[V]= cc[C] ^ ((temp8 & 0x40) != 0);
		temp8= temp8<<1;
		cc[N]= NTEST8(temp8);
		cc[Z]= ZTEST(temp8);
		MemWrite8(temp8,temp16);
		pc.Reg+=2;
		CycleCounter+=7;
		break;

	case ROL_E: //79
		temp16=MemRead16(pc.Reg);
		temp8=MemRead8(temp16);
		postbyte=cc[C];
		cc[C]= temp8 > 0x7F;
		cc[V]= cc[C] ^  ((temp8 & 0x40) != 0);
		temp8= ((temp8<<1) | postbyte);
		cc[Z]= ZTEST(temp8);
		cc[N]= NTEST8(temp8);
		MemWrite8(temp8,temp16);
		pc.Reg+=2;
		CycleCounter+=7;
		break;

	case DEC_E: //7A
		temp16=MemRead16(pc.Reg);
		temp8=MemRead8(temp16);
		temp8--;
		cc[Z]= ZTEST(temp8);
		cc[N]= NTEST8(temp8);
		cc[V]= temp8==0x7F;
		MemWrite8(temp8,temp16);
		pc.Reg+=2;
		CycleCounter+=7;
		break;

	case INC_E: //7C
		temp16=MemRead16(pc.Reg);
		temp8=MemRead8(temp16);
		temp8++;
		cc[Z]= ZTEST(temp8);
		cc[V]= temp8==0x80;
		cc[N]= NTEST8(temp8);
		MemWrite8(temp8,temp16);
		pc.Reg+=2;
		CycleCounter+=7;
		break;

	case TST_E: //7D
		temp8=MemRead8(MemRead16(pc.Reg));
		cc[Z]= ZTEST(temp8);
		cc[N]= NTEST8(temp8);
		cc[V]= 0;
		pc.Reg+=2;
		CycleCounter+=7;
		break;

	case JMP_E: //7E
		pc.Reg=MemRead16(pc.Reg);
		CycleCounter+=4;
		break;

	case CLR_E: //7F
		MemWrite8(0,MemRead16(pc.Reg));
		cc[C]= 0;
		cc[N]= 0;
		cc[V]= 0;
		cc[Z]= 1;
		pc.Reg+=2;
		CycleCounter+=7;
		break;

	case SUBA_M: //80
		postbyte=MemRead8(pc.Reg++);
		temp16 = A_REG - postbyte;
		cc[C]=(temp16 & 0x100)>>8;
		cc[V]= OTEST8(cc[C],postbyte,temp16,A_REG);
		A_REG=(temp16 & 0xFF);
		cc[Z]= ZTEST(A_REG);
		cc[N]= NTEST8(A_REG);
		CycleCounter+=2;
		break;

	case CMPA_M: //81
		postbyte=MemRead8(pc.Reg++);
		temp8= A_REG-postbyte;
		cc[C]= temp8 > A_REG;
		cc[V]= OTEST8(cc[C],postbyte,temp8,A_REG);
		cc[N]= NTEST8(temp8);
		cc[Z]= ZTEST(temp8);
		CycleCounter+=2;
		break;

	case SBCA_M:  //82
		postbyte=MemRead8(pc.Reg++);
		temp16=A_REG-postbyte-cc[C];
		cc[C]= (temp16 & 0x100)>>8;
		cc[V]= OTEST8(cc[C],postbyte,temp16,A_REG);
		A_REG = (temp16 & 0xFF);
		cc[N]= NTEST8(A_REG);
		cc[Z]= ZTEST(A_REG);
		CycleCounter+=2;
		break;

	case SUBD_M: //83
		temp16=MemRead16(pc.Reg);
		temp32=D_REG-temp16;
		cc[C]=(temp32 & 0x10000)>>16;
		cc[V]=OTEST16(cc[C],temp32,temp16,D_REG);
		D_REG=(temp32 & 0xFFFF);
		cc[Z]= ZTEST(D_REG);
		cc[N]= NTEST16(D_REG);
		pc.Reg+=2;
		CycleCounter+=4;
		break;

	case ANDA_M: //84
		A_REG = A_REG & MemRead8(pc.Reg++);
		cc[N]= NTEST8(A_REG);
		cc[Z]= ZTEST(A_REG);
		cc[V]= 0;
		CycleCounter+=2;
		break;

	case BITA_M: //85
		temp8= A_REG & MemRead8(pc.Reg++);
		cc[N]= NTEST8(temp8);
		cc[Z]= ZTEST(temp8);
		cc[V]= 0;
		CycleCounter+=2;
		break;

	case LDA_M: //86
		A_REG= MemRead8(pc.Reg++);
		cc[Z]= ZTEST(A_REG);
		cc[N]= NTEST8(A_REG);
		cc[V]= 0;
		CycleCounter+=2;
		break;

	case EORA_M: //88
		A_REG= A_REG ^ MemRead8(pc.Reg++);
		cc[N]= NTEST8(A_REG);
		cc[Z]= ZTEST(A_REG);
		cc[V]= 0;
		CycleCounter+=2;
		break;

	case ADCA_M: //89
		postbyte=MemRead8(pc.Reg++);
		temp16= A_REG + postbyte + cc[C];
		cc[C]= (temp16 & 0x100)>>8;
		cc[V]= OTEST8(cc[C],postbyte,temp16,A_REG);
		cc[H]= ((A_REG ^ temp16 ^ postbyte) & 0x10)>>4;
		A_REG= (temp16 & 0xFF);
		cc[N]= NTEST8(A_REG);
		cc[Z]= ZTEST(A_REG);
		CycleCounter+=2;
		break;

	case ORA_M: //8A
		A_REG = A_REG | MemRead8(pc.Reg++);
		cc[N]= NTEST8(A_REG);
		cc[Z]= ZTEST(A_REG);
		cc[V]= 0;
		CycleCounter+=2;
		break;

	case ADDA_M: //8B
		postbyte=MemRead8(pc.Reg++);
		temp16=A_REG+postbyte;
		cc[C]=(temp16 & 0x100)>>8;
		cc[H]= ((A_REG ^ postbyte ^ temp16) & 0x10)>>4;
		cc[V]=OTEST8(cc[C],postbyte,temp16,A_REG);
		A_REG=(temp16 & 0xFF);
		cc[N]= NTEST8(A_REG);
		cc[Z]= ZTEST(A_REG);
		CycleCounter+=2;
		break;

	case CMPX_M: //8C
		postword=MemRead16(pc.Reg);
		temp16 = x.Reg-postword;
		cc[C]= temp16 > x.Reg;
		cc[V]= OTEST16(cc[C],postword,temp16,X_REG);
		cc[N]= NTEST16(temp16);
		cc[Z] = ZTEST(temp16);
		pc.Reg+=2;
		CycleCounter+=4;
		break;

	case BSR_R: //8D
		*spostbyte=MemRead8(pc.Reg++);
		s.Reg--;
		MemWrite8(pc.B.lsb,s.Reg--);
		MemWrite8(pc.B.msb,s.Reg);
		pc.Reg+=*spostbyte;
		CycleCounter+=7;
		break;

	case LDX_M: //8E
		x.Reg= MemRead16(pc.Reg);
		cc[Z]= ZTEST(x.Reg);
		cc[N]= NTEST16(x.Reg);
		cc[V]= 0;
		pc.Reg+=2;
		CycleCounter+=3;
		break;

	case SUBA_D: //90
		postbyte=MemRead8( (dp.Reg |MemRead8(pc.Reg++)));
		temp16 = A_REG - postbyte;
		cc[C]=(temp16 & 0x100)>>8;
		cc[V]= OTEST8(cc[C],postbyte,temp16,A_REG);
		A_REG=(temp16 & 0xFF);
		cc[Z]= ZTEST(A_REG);
		cc[N]= NTEST8(A_REG);
		CycleCounter+=4;
		break;

	case CMPA_D: //91
		postbyte=MemRead8((dp.Reg |MemRead8(pc.Reg++)));
		temp8= A_REG-postbyte;
		cc[C]= temp8 > A_REG;
		cc[V]= OTEST8(cc[C],postbyte,temp8,A_REG);
		cc[N]= NTEST8(temp8);
		cc[Z]= ZTEST(temp8);
		CycleCounter+=4;
		break;

	case SBCA_D: //92
		postbyte=MemRead8((dp.Reg |MemRead8(pc.Reg++)));
		temp16=A_REG-postbyte-cc[C];
		cc[C]= (temp16 & 0x100)>>8;
		cc[V]= OTEST8(cc[C],postbyte,temp16,A_REG);
		A_REG = (temp16 & 0xFF);
		cc[N]= NTEST8(A_REG);
		cc[Z]= ZTEST(A_REG);
		CycleCounter+=4;
		break;

	case SUBD_D: //93
		temp16=MemRead16((dp.Reg |MemRead8(pc.Reg++)));
		temp32=D_REG-temp16;
		cc[C]=(temp32 & 0x10000)>>16;
		cc[V]= OTEST16(cc[C],temp32,temp16,D_REG);
		D_REG=(temp32 & 0xFFFF);
		cc[Z]= ZTEST(D_REG);
		cc[N]= NTEST16(D_REG);
		CycleCounter+=6;
		break;

	case ANDA_D: //94
		A_REG = A_REG & MemRead8((dp.Reg |MemRead8(pc.Reg++)));
		cc[N]= NTEST8(A_REG);
		cc[Z]= ZTEST(A_REG);
		cc[V]= 0;
		CycleCounter+=4;
		break;

	case BITA_D: //95
		temp8 = A_REG & MemRead8((dp.Reg |MemRead8(pc.Reg++)));
		cc[N]= NTEST8(temp8);
		cc[Z]= ZTEST(temp8);
		cc[V]= 0;
		CycleCounter+=4;
		break;

	case LDA_D: //96
		A_REG= MemRead8((dp.Reg |MemRead8(pc.Reg++)));
		cc[Z]= ZTEST(A_REG);
		cc[N]= NTEST8(A_REG);
		cc[V]= 0;
		CycleCounter+=4;
		break;

	case STA_D: //97
		MemWrite8( A_REG,(dp.Reg |MemRead8(pc.Reg++)));
		cc[Z]= ZTEST(A_REG);
		cc[N]= NTEST8(A_REG);
		cc[V]= 0;
		CycleCounter+=4;
		break;

	case EORA_D: //98
		A_REG= A_REG ^ MemRead8((dp.Reg |MemRead8(pc.Reg++)));
		cc[N]= NTEST8(A_REG);
		cc[Z]= ZTEST(A_REG);
		cc[V]= 0;
		CycleCounter+=4;
		break;

	case ADCA_D: //99
		postbyte=MemRead8((dp.Reg |MemRead8(pc.Reg++)));
		temp16= A_REG + postbyte + cc[C];
		cc[C]= (temp16 & 0x100)>>8;
		cc[V]= OTEST8(cc[C],postbyte,temp16,A_REG);
		cc[H]= ((A_REG ^ temp16 ^ postbyte) & 0x10)>>4;
		A_REG= (temp16 & 0xFF);
		cc[N]= NTEST8(A_REG);
		cc[Z]= ZTEST(A_REG);
		CycleCounter+=4;
		break;

	case ORA_D: //9A
		A_REG = A_REG | MemRead8((dp.Reg |MemRead8(pc.Reg++)));
		cc[N]= NTEST8(A_REG);
		cc[Z]= ZTEST(A_REG);
		cc[V]= 0;
		CycleCounter+=4;
		break;

	case ADDA_D: //9B
		postbyte=MemRead8((dp.Reg |MemRead8(pc.Reg++)));
		temp16=A_REG+postbyte;
		cc[C]=(temp16 & 0x100)>>8;
		cc[H]= ((A_REG ^ postbyte ^ temp16) & 0x10)>>4;
		cc[V]=OTEST8(cc[C],postbyte,temp16,A_REG);
		A_REG=(temp16 & 0xFF);
		cc[N]= NTEST8(A_REG);
		cc[Z] = ZTEST(A_REG);
		CycleCounter+=4;
		break;

	case CMPX_D: //9C
		postword=MemRead16((dp.Reg |MemRead8(pc.Reg++)));
		temp16= x.Reg - postword ;
		cc[C]= temp16 > x.Reg;
		cc[V]= OTEST16(cc[C],postword,temp16,X_REG);
		cc[N]= NTEST16(temp16);
		cc[Z] = ZTEST(temp16);
		CycleCounter+=6;
		break;

	case BSR_D: //9D
		temp16=(dp.Reg |MemRead8(pc.Reg++));
		s.Reg--;
		MemWrite8(pc.B.lsb,s.Reg--);
		MemWrite8(pc.B.msb,s.Reg);
		pc.Reg=temp16;
		CycleCounter+=7;
		break;

	case LDX_D: //9E
		x.Reg=MemRead16((dp.Reg |MemRead8(pc.Reg++)));
		cc[Z]= ZTEST(x.Reg);
		cc[N]= NTEST16(x.Reg);
		cc[V]= 0;
		CycleCounter+=5;
		break;

	case STX_D: //9F
		MemWrite16(x.Reg,(dp.Reg |MemRead8(pc.Reg++)));
		cc[Z]= ZTEST(x.Reg);
		cc[N]= NTEST16(x.Reg);
		cc[V]= 0;
		CycleCounter+=5;
		break;

	case SUBA_X: //A0
		postbyte=MemRead8(CalculateEA(MemRead8(pc.Reg++)));
		temp16 = A_REG - postbyte;
		cc[C]=(temp16 & 0x100)>>8;
		cc[V]= OTEST8(cc[C],postbyte,temp16,A_REG);
		A_REG=(temp16 & 0xFF);
		cc[Z] = ZTEST(A_REG);
		cc[N]=NTEST8(A_REG);
		CycleCounter+=4;
		break;

	case CMPA_X: //A1
		postbyte=MemRead8(CalculateEA(MemRead8(pc.Reg++)));
		temp8= A_REG-postbyte;
		cc[C]= temp8 > A_REG;
		cc[V]= OTEST8(cc[C],postbyte,temp8,A_REG);
		cc[N]= NTEST8(temp8);
		cc[Z]= ZTEST(temp8);
		CycleCounter+=4;
		break;

	case SBCA_X: //A2
		postbyte=MemRead8(CalculateEA(MemRead8(pc.Reg++)));
		temp16=A_REG-postbyte-cc[C];
		cc[C]= (temp16 & 0x100)>>8;
		cc[V]= OTEST8(cc[C],postbyte,temp16,A_REG);
		A_REG = (temp16 & 0xFF);
		cc[N]= NTEST8(A_REG);
		cc[Z]= ZTEST(A_REG);
		CycleCounter+=4;
		break;

	case SUBD_X: //A3
		temp16=MemRead16(CalculateEA(MemRead8(pc.Reg++)));
		temp32=D_REG-temp16;
		cc[C]= (temp32 & 0x10000)>>16;
		cc[V]= OTEST16(cc[C],temp32,temp16,D_REG);
		D_REG= (temp32 & 0xFFFF);
		cc[Z]= ZTEST(D_REG);
		cc[N]= NTEST16(D_REG);
		CycleCounter+=6;
		break;

	case ANDA_X: //A4
		A_REG= A_REG & MemRead8(CalculateEA(MemRead8(pc.Reg++)));
		cc[N]= NTEST8(A_REG);
		cc[Z]= ZTEST(A_REG);
		cc[V]= 0;
		CycleCounter+=4;
		break;

	case BITA_X:  //A5
		temp8 =A_REG & MemRead8(CalculateEA(MemRead8(pc.Reg++)));
		cc[N]= NTEST8(temp8);
		cc[Z]= ZTEST(temp8);
		cc[V]= 0;
		CycleCounter+=4;
		break;

	case LDA_X: //A6
		A_REG= MemRead8(CalculateEA(MemRead8(pc.Reg++)));
		cc[Z]= ZTEST(A_REG);
		cc[N]= NTEST8(A_REG);
		cc[V]= 0;
		CycleCounter+=4;
		break;

	case STA_X: //A7
		MemWrite8(A_REG,CalculateEA(MemRead8(pc.Reg++)));
		cc[Z]= ZTEST(A_REG);
		cc[N]= NTEST8(A_REG);
		cc[V]= 0;
		CycleCounter+=4;
		break;

	case EORA_X: //A8
		A_REG= A_REG ^ MemRead8(CalculateEA(MemRead8(pc.Reg++)));
		cc[N]= NTEST8(A_REG);
		cc[Z]= ZTEST(A_REG);
		cc[V]= 0;
		CycleCounter+=4;
		break;

	case ADCA_X: //A9
		postbyte=MemRead8(CalculateEA(MemRead8(pc.Reg++)));
		temp16= A_REG + postbyte + cc[C];
		cc[C]= (temp16 & 0x100)>>8;
		cc[V]= OTEST8(cc[C],postbyte,temp16,A_REG);
		cc[H]= ((A_REG ^ temp16 ^ postbyte) & 0x10)>>4;
		A_REG= (temp16 & 0xFF);
		cc[N]= NTEST8(A_REG);
		cc[Z]= ZTEST(A_REG);
		CycleCounter+=4;
		break;

	case ORA_X: //AA
		A_REG= A_REG | MemRead8(CalculateEA(MemRead8(pc.Reg++)));
		cc[N]= NTEST8(A_REG);
		cc[Z]= ZTEST(A_REG);
		cc[V]= 0;
		CycleCounter+=4;
		break;

	case ADDA_X: //AB
		postbyte=MemRead8(CalculateEA(MemRead8(pc.Reg++)));
		temp16=A_REG+postbyte;
		cc[C]=(temp16 & 0x100)>>8;
		cc[H]= ((A_REG ^ postbyte ^ temp16) & 0x10)>>4;
		cc[V]=OTEST8(cc[C],postbyte,temp16,A_REG);
		A_REG=(temp16 & 0xFF);
		cc[N]= NTEST8(A_REG);
		cc[Z] = ZTEST(A_REG);
		CycleCounter+=4;
		break;

	case CMPX_X: //AC
		postword=MemRead16(CalculateEA(MemRead8(pc.Reg++)));
		temp16= x.Reg - postword ;
		cc[C]= temp16 > x.Reg;
		cc[V]= OTEST16(cc[C],postword,temp16,X_REG);
		cc[N]= NTEST16(temp16);
		cc[Z] = ZTEST(temp16);
		CycleCounter+=6;
		break;

	case BSR_X: //AD
		temp16=CalculateEA(MemRead8(pc.Reg++));

		s.Reg--;
		MemWrite8(pc.B.lsb,s.Reg--);
		MemWrite8(pc.B.msb,s.Reg);
		pc.Reg=temp16;
		CycleCounter+=7;
		break;

	case LDX_X: //AE
		x.Reg=MemRead16(CalculateEA(MemRead8(pc.Reg++)));
		cc[Z]= ZTEST(x.Reg);
		cc[N]= NTEST16(x.Reg);
		cc[V]= 0;
		CycleCounter+=5;
		break;

	case STX_X: //AF
		MemWrite16(x.Reg,CalculateEA(MemRead8(pc.Reg++)));
		cc[Z]= ZTEST(x.Reg);
		cc[N]= NTEST16(x.Reg);
		cc[V] = 0;
		CycleCounter+=5;
		break;

	case SUBA_E: //B0
		postbyte=MemRead8(MemRead16(pc.Reg));
		temp16 = A_REG - postbyte;
		cc[C]=(temp16 & 0x100)>>8;
		cc[V]= OTEST8(cc[C],postbyte,temp16,A_REG);
		A_REG=(temp16 & 0xFF);
		cc[Z] = ZTEST(A_REG);
		cc[N]=NTEST8(A_REG);
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case	CMPA_E: //B1
		postbyte=MemRead8(MemRead16(pc.Reg));
		temp8= A_REG-postbyte;
		cc[C]= temp8 > A_REG;
		cc[V]= OTEST8(cc[C],postbyte,temp8,A_REG);
		cc[N]= NTEST8(temp8);
		cc[Z]= ZTEST(temp8);
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case SBCA_E: //B2
		postbyte=MemRead8(MemRead16(pc.Reg));
		temp16=A_REG-postbyte-cc[C];
		cc[C]= (temp16 & 0x100)>>8;
		cc[V]= OTEST8(cc[C],postbyte,temp16,A_REG);
		A_REG = (temp16 & 0xFF);
		cc[N]= NTEST8(A_REG);
		cc[Z]= ZTEST(A_REG);
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case SUBD_E: //B3
		temp16=MemRead16(MemRead16(pc.Reg));
		temp32=D_REG-temp16;
		cc[C]= (temp32 & 0x10000)>>16;
		cc[V]= OTEST16(cc[C],temp32,temp16,D_REG);
		D_REG= (temp32 & 0xFFFF);
		cc[Z]= ZTEST(D_REG);
		cc[N]= NTEST16(D_REG);
		pc.Reg+=2;
		CycleCounter+=7;
		break;

	case ANDA_E: //B4
		postbyte=MemRead8(MemRead16(pc.Reg));
		A_REG = A_REG & postbyte;
		cc[N]= NTEST8(A_REG);
		cc[Z]= ZTEST(A_REG);
		cc[V]= 0;
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case BITA_E: //B5
		temp8 = A_REG & MemRead8(MemRead16(pc.Reg));
		cc[N]= NTEST8(temp8);
		cc[Z]= ZTEST(temp8);
		cc[V]= 0;
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case LDA_E: //B6
		A_REG= MemRead8(MemRead16(pc.Reg));
		cc[Z]= ZTEST(A_REG);
		cc[N]= NTEST8(A_REG);
		cc[V]= 0;
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case STA_E: //B7
		MemWrite8(A_REG,MemRead16(pc.Reg));
		cc[Z]= ZTEST(A_REG);
		cc[N]= NTEST8(A_REG);
		cc[V]= 0;
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case EORA_E:  //B8
		A_REG = A_REG ^ MemRead8(MemRead16(pc.Reg));
		cc[N]= NTEST8(A_REG);
		cc[Z]= ZTEST(A_REG);
		cc[V]= 0;
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case ADCA_E: //B9
		postbyte=MemRead8(MemRead16(pc.Reg));
		temp16= A_REG + postbyte + cc[C];
		cc[C]= (temp16 & 0x100)>>8;
		cc[V]= OTEST8(cc[C],postbyte,temp16,A_REG);
		cc[H]= ((A_REG ^ temp16 ^ postbyte) & 0x10)>>4;
		A_REG= (temp16 & 0xFF);
		cc[N]= NTEST8(A_REG);
		cc[Z]= ZTEST(A_REG);
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case ORA_E: //BA
		A_REG = A_REG | MemRead8(MemRead16(pc.Reg));
		cc[N]= NTEST8(A_REG);
		cc[Z]= ZTEST(A_REG);
		cc[V]= 0;
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case ADDA_E: //BB
		postbyte=MemRead8(MemRead16(pc.Reg));
		temp16=A_REG+postbyte;
		cc[C]= (temp16 & 0x100)>>8;
		cc[H]= ((A_REG ^ postbyte ^ temp16) & 0x10)>>4;
		cc[V]= OTEST8(cc[C],postbyte,temp16,A_REG);
		A_REG= (temp16 & 0xFF);
		cc[N]= NTEST8(A_REG);
		cc[Z]= ZTEST(A_REG);
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case CMPX_E: //BC
		postword=MemRead16(MemRead16(pc.Reg));
		temp16 = x.Reg-postword;
		cc[C]= temp16 > x.Reg;
		cc[V]= OTEST16(cc[C],postword,temp16,X_REG);
		cc[N]= NTEST16(temp16);
		cc[Z]= ZTEST(temp16);
		pc.Reg+=2;
		CycleCounter+=7;
		break;

	case BSR_E: //BD
		postword=MemRead16(pc.Reg);
		pc.Reg+=2;
		s.Reg--;
		MemWrite8(pc.B.lsb,s.Reg--);
		MemWrite8(pc.B.msb,s.Reg);
		pc.Reg=postword;
		CycleCounter+=8;
		break;

	case LDX_E: //BE
		x.Reg=MemRead16(MemRead16(pc.Reg));
		cc[Z]= ZTEST(x.Reg);
		cc[N]= NTEST16(x.Reg);
		cc[V]= 0;
		pc.Reg+=2;
		CycleCounter+=6;
		break;

	case STX_E: //BF
		MemWrite16(x.Reg,MemRead16(pc.Reg));
		cc[Z]= ZTEST(x.Reg);
		cc[N]= NTEST16(x.Reg);
		cc[V]= 0;
		pc.Reg+=2;
		CycleCounter+=6;
		break;

	case SUBB_M: //C0
		postbyte=MemRead8(pc.Reg++);
		temp16 = B_REG - postbyte;
		cc[C]=(temp16 & 0x100)>>8;
		cc[V]= OTEST8(cc[C],postbyte,temp16,B_REG);
		B_REG=(temp16 & 0xFF);
		cc[Z] = ZTEST(B_REG);
		cc[N]=NTEST8(B_REG);
		CycleCounter+=2;
		break;

	case CMPB_M: //C1
		postbyte=MemRead8(pc.Reg++);
		temp8= B_REG-postbyte;
		cc[C]= temp8 > B_REG;
		cc[V]= OTEST8(cc[C],postbyte,temp8,B_REG);
		cc[N]= NTEST8(temp8);
		cc[Z]= ZTEST(temp8);
		CycleCounter+=2;
		break;

	case SBCB_M: //C3
		postbyte=MemRead8(pc.Reg++);
		temp16=B_REG-postbyte-cc[C];
		cc[C]= (temp16 & 0x100)>>8;
		cc[V]= OTEST8(cc[C],postbyte,temp16,B_REG);
		B_REG = (temp16 & 0xFF);
		cc[N]= NTEST8(B_REG);
		cc[Z]= ZTEST(B_REG);
		CycleCounter+=2;
		break;

	case ADDD_M: //C3
		temp16=MemRead16(pc.Reg);
		temp32= D_REG+ temp16;
		cc[C]= (temp32 & 0x10000)>>16;
		cc[V]= OTEST16(cc[C],temp32,temp16,D_REG);
		D_REG= (temp32 & 0xFFFF);
		cc[Z]= ZTEST(D_REG);
		cc[N]= NTEST16(D_REG);
		pc.Reg+=2;
		CycleCounter+=4;
		break;

	case ANDB_M: //C4
//		postbyte=MemRead8(pc.Reg++);
		B_REG = B_REG & MemRead8(pc.Reg++);
		cc[N]= NTEST8(B_REG);
		cc[Z]= ZTEST(B_REG);
		cc[V] = 0;
		CycleCounter+=2;
		break;

	case BITB_M: //C5
		temp8 = B_REG & MemRead8(pc.Reg++);
		cc[N]= NTEST8(temp8);
		cc[Z]= ZTEST(temp8);
		cc[V]= 0;
		CycleCounter+=2;
		break;

	case LDB_M: //C6
		B_REG=MemRead8(pc.Reg++);
		cc[Z]= ZTEST(B_REG);
		cc[N]= NTEST8(B_REG);
		cc[V]= 0;
		CycleCounter+=2;
		break;

	case EORB_M: //C8
//		postbyte=MemRead8(pc.Reg++);
		B_REG = B_REG ^ MemRead8(pc.Reg++);
		cc[N]=NTEST8(B_REG);
		cc[Z] = ZTEST(B_REG);
		cc[V] = 0;
		CycleCounter+=2;
		break;

	case ADCB_M: //C9
		postbyte=MemRead8(pc.Reg++);
		temp16= B_REG + postbyte + cc[C];
		cc[C]= (temp16 & 0x100)>>8;
		cc[V]= OTEST8(cc[C],postbyte,temp16,B_REG);
		cc[H]= ((B_REG ^ temp16 ^ postbyte) & 0x10)>>4;
		B_REG= (temp16 & 0xFF);
		cc[N]= NTEST8(B_REG);
		cc[Z]= ZTEST(B_REG);
		CycleCounter+=2;
		break;

	case ORB_M: //CA
//		postbyte=MemRead8(pc.Reg++);
		B_REG = B_REG | MemRead8(pc.Reg++);
		cc[N]= NTEST8(B_REG);
		cc[Z] = ZTEST(B_REG);
		cc[V] = 0;
		CycleCounter+=2;
		break;

	case ADDB_M: //CB
		postbyte=MemRead8(pc.Reg++);
		temp16=B_REG+postbyte;
		cc[C]=(temp16 & 0x100)>>8;
		cc[H]= ((B_REG ^ postbyte ^ temp16) & 0x10)>>4;
		cc[V]=OTEST8(cc[C],postbyte,temp16,B_REG);
		B_REG=(temp16 & 0xFF);
		cc[N]= NTEST8(B_REG);
		cc[Z] = ZTEST(B_REG);
		CycleCounter+=2;
		break;

	case LDD_M: //CC
		D_REG=MemRead16(pc.Reg);
		cc[Z]= ZTEST(D_REG);
		cc[N]= NTEST16(D_REG);
		cc[V]= 0;
		pc.Reg+=2;
		CycleCounter+=3;
		break;

	case LDU_M: //CE
		u.Reg=MemRead16(pc.Reg);
		cc[Z]= ZTEST(u.Reg);
		cc[N]= NTEST16(u.Reg);
		cc[V]= 0;
		pc.Reg+=2;
		CycleCounter+=3;
		break;

	case SUBB_D: //D0
		postbyte=MemRead8( (dp.Reg |MemRead8(pc.Reg++)));
		temp16 = B_REG - postbyte;
		cc[C]=(temp16 & 0x100)>>8;
		cc[V]= OTEST8(cc[C],postbyte,temp16,B_REG);
		B_REG=(temp16 & 0xFF);
		cc[Z] = ZTEST(B_REG);
		cc[N]= NTEST8(B_REG);
		CycleCounter+=4;
		break;

	case CMPB_D: //D1
		postbyte=MemRead8((dp.Reg |MemRead8(pc.Reg++)));
		temp8= B_REG-postbyte;
		cc[C]= temp8 > B_REG;
		cc[V]= OTEST8(cc[C],postbyte,temp8,B_REG);
		cc[N]= NTEST8(temp8);
		cc[Z]= ZTEST(temp8);
		CycleCounter+=4;
		break;

	case SBCB_D: //D2
		postbyte=MemRead8((dp.Reg |MemRead8(pc.Reg++)));
		temp16=B_REG-postbyte-cc[C];
		cc[C]= (temp16 & 0x100)>>8;
		cc[V]= OTEST8(cc[C],postbyte,temp16,B_REG);
		B_REG = (temp16 & 0xFF);
		cc[N]= NTEST8(B_REG);
		cc[Z]= ZTEST(B_REG);
		CycleCounter+=4;
		break;

	case ADDD_D: //D3
		temp16=MemRead16( (dp.Reg |MemRead8(pc.Reg++)));
		temp32= D_REG+ temp16;
		cc[C]=(temp32 & 0x10000)>>16;
		cc[V]= OTEST16(cc[C],temp32,temp16,D_REG);
		D_REG= (temp32 & 0xFFFF);
		cc[Z]= ZTEST(D_REG);
		cc[N]= NTEST16(D_REG);
		CycleCounter+=6;
		break;

	case ANDB_D: //D4
//		postbyte=MemRead8((dp.Reg |MemRead8(pc.Reg++)));
		B_REG = B_REG & MemRead8((dp.Reg |MemRead8(pc.Reg++)));
		cc[N]=NTEST8(B_REG);
		cc[Z] = ZTEST(B_REG);
		cc[V] = 0;
		CycleCounter+=4;
		break;

	case BITB_D: //D5
		temp8 = B_REG & MemRead8((dp.Reg |MemRead8(pc.Reg++)));
		cc[N]= NTEST8(temp8);
		cc[Z] = ZTEST(temp8);
		cc[V] = 0;
		CycleCounter+=4;
		break;

	case LDB_D: //D6
		B_REG=MemRead8( (dp.Reg |MemRead8(pc.Reg++)));
		cc[Z] = ZTEST(B_REG);
		cc[N]= NTEST8(B_REG);
		cc[V] = 0;
		CycleCounter+=4;
		break;

	case STB_D: //D7
		MemWrite8( B_REG,(dp.Reg |MemRead8(pc.Reg++)));
		cc[Z] = ZTEST(B_REG);
		cc[N]= NTEST8(B_REG);
		cc[V] = 0;
		CycleCounter+=4;
		break;

	case EORB_D: //D8
//		postbyte=MemRead8((dp.Reg | MemRead8(pc.Reg++)));
		B_REG = B_REG ^ MemRead8((dp.Reg | MemRead8(pc.Reg++)));
		cc[N]= NTEST8(B_REG);
		cc[Z] = ZTEST(B_REG);
		cc[V] = 0;
		CycleCounter+=4;
		break;

	case ADCB_D: //D9
		postbyte=MemRead8((dp.Reg |MemRead8(pc.Reg++)));
		temp16= B_REG + postbyte + cc[C];
		cc[C]= (temp16 & 0x100)>>8;
		cc[V]= OTEST8(cc[C],postbyte,temp16,B_REG);
		cc[H]= ((B_REG ^ temp16 ^ postbyte) & 0x10)>>4;
		B_REG= (temp16 & 0xFF);
		cc[N]= NTEST8(B_REG);
		cc[Z]= ZTEST(B_REG);
		CycleCounter+=4;
		break;

	case ORB_D: //DA
//		postbyte=MemRead8((dp.Reg | MemRead8(pc.Reg++)));
		B_REG = B_REG | MemRead8((dp.Reg | MemRead8(pc.Reg++)));
		cc[N]= NTEST8(B_REG);
		cc[Z] = ZTEST(B_REG);
		cc[V] = 0;
		CycleCounter+=4;
		break;

	case ADDB_D: //DB
		postbyte=MemRead8((dp.Reg |MemRead8(pc.Reg++)));
		temp16=B_REG+postbyte;
		cc[C]=(temp16 & 0x100)>>8;
		cc[H]= ((B_REG ^ postbyte ^ temp16) & 0x10)>>4;
		cc[V]=OTEST8(cc[C],postbyte,temp16,B_REG);
		B_REG=(temp16 & 0xFF);
		cc[N]= NTEST8(B_REG);
		cc[Z] = ZTEST(B_REG);
		CycleCounter+=4;
		break;

	case LDD_D: //DC
		D_REG=MemRead16((dp.Reg | MemRead8(pc.Reg++)));
		cc[Z]= ZTEST(D_REG);
		cc[N]= NTEST16(D_REG);
		cc[V]= 0;
		CycleCounter+=5;
		break;

	case STD_D: //DD
		MemWrite16(D_REG,(dp.Reg |MemRead8(pc.Reg++)));
		cc[Z]= ZTEST(D_REG);
		cc[N]= NTEST16(D_REG);
		cc[V]= 0;
		CycleCounter+=5;
		break;

	case LDU_D: //DE
		u.Reg=MemRead16((dp.Reg |MemRead8(pc.Reg++)));
		cc[Z]= ZTEST(u.Reg);
		cc[N]= NTEST16(u.Reg);
		cc[V]= 0;
		CycleCounter+=5;
		break;

	case STU_D: //DF
		MemWrite16(u.Reg,(dp.Reg |MemRead8(pc.Reg++)));
		cc[Z]= ZTEST(u.Reg);
		cc[N]= NTEST16(u.Reg);
		cc[V]= 0;
		CycleCounter+=5;
		break;

	case SUBB_X: //E0
		postbyte=MemRead8(CalculateEA(MemRead8(pc.Reg++)));
		temp16 = B_REG - postbyte;
		cc[C]= (temp16 & 0x100)>>8;
		cc[V]= OTEST8(cc[C],postbyte,temp16,B_REG);
		B_REG= (temp16 & 0xFF);
		cc[Z]= ZTEST(B_REG);
		cc[N]= NTEST8(B_REG);
		CycleCounter+=4;
		break;

	case CMPB_X: //E1
		postbyte=MemRead8(CalculateEA(MemRead8(pc.Reg++)));
		temp8= B_REG-postbyte;
		cc[C]= temp8 > B_REG;
		cc[V]= OTEST8(cc[C],postbyte,temp8,B_REG);
		cc[N]= NTEST8(temp8);
		cc[Z]= ZTEST(temp8);
		CycleCounter+=4;
		break;

	case SBCB_X: //E2
		postbyte=MemRead8(CalculateEA(MemRead8(pc.Reg++)));
		temp16=B_REG-postbyte-cc[C];
		cc[C]= (temp16 & 0x100)>>8;
		cc[V]= OTEST8(cc[C],postbyte,temp16,B_REG);
		B_REG = (temp16 & 0xFF);
		cc[N]= NTEST8(B_REG);
		cc[Z]= ZTEST(B_REG);
		CycleCounter+=4;
		break;

	case ADDD_X: //E3
		temp16=MemRead16(CalculateEA(MemRead8(pc.Reg++)));
		temp32= D_REG+ temp16;
		cc[C]=(temp32 & 0x10000)>>16;
		cc[V]= OTEST16(cc[C],temp32,temp16,D_REG);
		D_REG= (temp32 & 0xFFFF);
		cc[Z]= ZTEST(D_REG);
		cc[N]= NTEST16(D_REG);
		CycleCounter+=6;
		break;

	case ANDB_X: //E4
//		postbyte=MemRead8(CalculateEA(MemRead8(pc.Reg++)));
		B_REG = B_REG & MemRead8(CalculateEA(MemRead8(pc.Reg++)));
		cc[N]= NTEST8(B_REG);
		cc[Z]= ZTEST(B_REG);
		cc[V]= 0;
		CycleCounter+=4;
		break;

	case BITB_X: //E5
		temp8 = B_REG & MemRead8(CalculateEA(MemRead8(pc.Reg++)));
		cc[N]= NTEST8(temp8);
		cc[Z]= ZTEST(temp8);
		cc[V] = 0;
		CycleCounter+=4;
		break;

	case LDB_X: //E6
		B_REG=MemRead8(CalculateEA(MemRead8(pc.Reg++)));
		cc[Z]= ZTEST(B_REG);
		cc[N]= NTEST8(B_REG);
		cc[V]= 0;
		CycleCounter+=4;
		break;

	case STB_X: //E7
//		postbyte=MemRead8(pc.Reg++);
		MemWrite8(B_REG,CalculateEA(MemRead8(pc.Reg++)));
		cc[Z]= ZTEST(B_REG);
		cc[N]= NTEST8(B_REG);
		cc[V]= 0;
		CycleCounter+=4;
		break;

	case EORB_X: //E8
//		postbyte=MemRead8(pc.Reg++);
//		temp16=CalculateEA(MemRead8(pc.Reg++));
		temp8=MemRead8(CalculateEA(MemRead8(pc.Reg++)));
		B_REG= B_REG ^ temp8;
		cc[N]= NTEST8(B_REG);
		cc[Z] = ZTEST(B_REG);
		cc[V] = 0;
		CycleCounter+=4;
		break;

	case ADCB_X: //E9
		postbyte=MemRead8(CalculateEA(MemRead8(pc.Reg++)));
		temp16= B_REG + postbyte + cc[C];
		cc[C]= (temp16 & 0x100)>>8;
		cc[V]= OTEST8(cc[C],postbyte,temp16,B_REG);
		cc[H]= ((B_REG ^ temp16 ^ postbyte) & 0x10)>>4;
		B_REG= (temp16 & 0xFF);
		cc[N]= NTEST8(B_REG);
		cc[Z]= ZTEST(B_REG);
		CycleCounter+=4;
		break;

	case ORB_X: //EA
//		postbyte=MemRead8(CalculateEA(MemRead8(pc.Reg++)));
		B_REG = B_REG | MemRead8(CalculateEA(MemRead8(pc.Reg++)));
		cc[N]= NTEST8(B_REG);
		cc[Z] = ZTEST(B_REG);
		cc[V] = 0;
		CycleCounter+=4;
		break;

	case ADDB_X: //EB
		postbyte=MemRead8(CalculateEA(MemRead8(pc.Reg++)));
		temp16=B_REG+postbyte;
		cc[C]=(temp16 & 0x100)>>8;
		cc[H]= ((B_REG ^ postbyte ^ temp16) & 0x10)>>4;
		cc[V]=OTEST8(cc[C],postbyte,temp16,B_REG);
		B_REG=(temp16 & 0xFF);
		cc[N]= NTEST8(B_REG);
		cc[Z] = ZTEST(B_REG);
		CycleCounter+=4;
		break;

	case LDD_X: //EC
//		postbyte=MemRead8(pc.Reg++);
		temp16=CalculateEA(MemRead8(pc.Reg++));
		D_REG=MemRead16(temp16);
		cc[Z]= ZTEST(D_REG);
		cc[N]= NTEST16(D_REG);
		cc[V] = 0;
		CycleCounter+=5;
		break;

	case STD_X: //ED
//		postbyte=MemRead8(pc.Reg++);
//		temp16=CalculateEA(MemRead8(pc.Reg++));
		MemWrite16(D_REG,CalculateEA(MemRead8(pc.Reg++)));
		cc[Z]= ZTEST(D_REG);
		cc[N]= NTEST16(D_REG);
		cc[V] = 0;
		CycleCounter+=5;
		break;

	case LDU_X: //EE
		u.Reg=MemRead16(CalculateEA(MemRead8(pc.Reg++)));
		cc[Z] = ZTEST(u.Reg);
		cc[N]=NTEST16(u.Reg);
		cc[V] = 0;
		CycleCounter+=5;
		break;

	case STU_X: //EF
//		postbyte=MemRead8(pc.Reg++);
//		temp16=CalculateEA(MemRead8(pc.Reg++));
		MemWrite16(u.Reg,CalculateEA(MemRead8(pc.Reg++)));
		cc[Z] = ZTEST(u.Reg);
		cc[N]=NTEST16(u.Reg);
		cc[V] = 0;
		CycleCounter+=5;
		break;

	case SUBB_E: //F0
		postbyte=MemRead8(MemRead16(pc.Reg));
		temp16 = B_REG - postbyte;
		cc[C]=(temp16 & 0x100)>>8;
		cc[V]= OTEST8(cc[C],postbyte,temp16,B_REG);
		B_REG=(temp16 & 0xFF);
		cc[Z] = ZTEST(B_REG);
		cc[N]= NTEST8(B_REG);
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case CMPB_E: //F1
		postbyte=MemRead8(MemRead16(pc.Reg));
		temp8= B_REG-postbyte;
		cc[C]= temp8 > B_REG;
		cc[V]= OTEST8(cc[C],postbyte,temp8,B_REG);
		cc[N]= NTEST8(temp8);
		cc[Z]= ZTEST(temp8);
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case SBCB_E: //F2
		postbyte=MemRead8(MemRead16(pc.Reg));
		temp16=B_REG-postbyte-cc[C];
		cc[C]= (temp16 & 0x100)>>8;
		cc[V]= OTEST8(cc[C],postbyte,temp16,B_REG);
		B_REG = (temp16 & 0xFF);
		cc[N]= NTEST8(B_REG);
		cc[Z]= ZTEST(B_REG);
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case ADDD_E: //F3
		temp16=MemRead16(MemRead16(pc.Reg));
		temp32= D_REG+ temp16;
		cc[C]=(temp32 & 0x10000)>>16;
		cc[V]= OTEST16(cc[C],temp32,temp16,D_REG);
		D_REG= (temp32 & 0xFFFF);
		cc[Z]= ZTEST(D_REG);
		cc[N]= NTEST16(D_REG);
		pc.Reg+=2;
		CycleCounter+=7;
		break;

	case ANDB_E:  //F4
//		postbyte=MemRead8(MemRead16(pc.Reg));
		B_REG = B_REG & MemRead8(MemRead16(pc.Reg));
		cc[N]= NTEST8(B_REG);
		cc[Z]= ZTEST(B_REG);
		cc[V]= 0;
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case BITB_E: //F5
		temp8 = B_REG & MemRead8(MemRead16(pc.Reg));
		cc[N]= NTEST8(temp8);
		cc[Z]= ZTEST(temp8);
		cc[V]= 0;
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case LDB_E: //F6
		B_REG=MemRead8(MemRead16(pc.Reg));
		cc[Z] = ZTEST(B_REG);
		cc[N]= NTEST8(B_REG);
		cc[V] = 0;
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case STB_E: //F7
		MemWrite8(B_REG,MemRead16(pc.Reg));
		cc[Z] = ZTEST(B_REG);
		cc[N] = NTEST8(B_REG);
		cc[V] = 0;
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case EORB_E: //F8
//		postbyte=MemRead8(MemRead16(pc.Reg));
		B_REG = B_REG ^ MemRead8(MemRead16(pc.Reg));
		cc[N]= NTEST8(B_REG);
		cc[Z] = ZTEST(B_REG);
		cc[V] = 0;
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case ADCB_E: //F9
		postbyte=MemRead8(MemRead16(pc.Reg));
		temp16= B_REG + postbyte + cc[C];
		cc[C]= (temp16 & 0x100)>>8;
		cc[V]= OTEST8(cc[C],postbyte,temp16,B_REG);
		cc[H]= ((B_REG ^ temp16 ^ postbyte) & 0x10)>>4;
		B_REG= (temp16 & 0xFF);
		cc[N]= NTEST8(B_REG);
		cc[Z]= ZTEST(B_REG);
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case ORB_E: //FA
//		postbyte=MemRead8(MemRead16(pc.Reg));
		B_REG = B_REG | MemRead8(MemRead16(pc.Reg));
		cc[N]= NTEST8(B_REG);
		cc[Z] = ZTEST(B_REG);
		cc[V] = 0;
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case ADDB_E: //FB
		postbyte=MemRead8(MemRead16(pc.Reg));
		temp16=B_REG+postbyte;
		cc[C]=(temp16 & 0x100)>>8;
		cc[H]= ((B_REG ^ postbyte ^ temp16) & 0x10)>>4;
		cc[V]= OTEST8(cc[C],postbyte,temp16,B_REG);
		B_REG=(temp16 & 0xFF);
		cc[N]= NTEST8(B_REG);
		cc[Z] = ZTEST(B_REG);
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case LDD_E: //FC
		D_REG=MemRead16(MemRead16(pc.Reg));
		cc[Z]= ZTEST(D_REG);
		cc[N]= NTEST16(D_REG);
		cc[V]= 0;
		pc.Reg+=2;
		CycleCounter+=6;
		break;

	case STD_E: //FD
		MemWrite16(D_REG,MemRead16(pc.Reg));
		cc[Z]= ZTEST(D_REG);
		cc[N]=NTEST16(D_REG);
		cc[V] = 0;
		pc.Reg+=2;
		CycleCounter+=6;
		break;

	case LDU_E: //FE
//		postword=MemRead16(pc.Reg);
		u.Reg=MemRead16(MemRead16(pc.Reg));
		cc[Z] = ZTEST(u.Reg);
		cc[N]= NTEST16(u.Reg);
		cc[V] = 0;
		pc.Reg+=2;
		CycleCounter+=6;
		break;

	case STU_E: //FF
		MemWrite16(u.Reg,MemRead16(pc.Reg));
		cc[Z] = ZTEST(u.Reg);
		cc[N]= NTEST16(u.Reg);
		cc[V] = 0;
		pc.Reg+=2;
		CycleCounter+=6;
		break;

	default:
//		MessageBox(0,"Unhandled Op","Ok",0);
		break;
	}//End Switch

} // Do_Opcode ENDS


void P2_Opcode(void)
{
	switch (MemRead8(pc.Reg++)) {

	case LBEQ_R: //1027
		if (cc[Z])
		{
			*spostword=MemRead16(pc.Reg);
			pc.Reg+=*spostword;
			CycleCounter+=1;
		}
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case LBRN_R: //1021
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case LBHI_R: //1022
		if  (!(cc[C] | cc[Z]))
		{
			*spostword=MemRead16(pc.Reg);
			pc.Reg+=*spostword;
			CycleCounter+=1;
		}
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case LBLS_R: //1023
		if (cc[C] | cc[Z])
		{
			*spostword=MemRead16(pc.Reg);
			pc.Reg+=*spostword;
			CycleCounter+=1;
		}
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case LBHS_R: //1024
		if (!cc[C])
		{
			*spostword=MemRead16(pc.Reg);
			pc.Reg+=*spostword;
			CycleCounter+=1;
		}
		pc.Reg+=2;
		CycleCounter+=6;
		break;

	case LBCS_R: //1025
		if (cc[C])
		{
			*spostword=MemRead16(pc.Reg);
			pc.Reg+=*spostword;
			CycleCounter+=1;
		}
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case LBNE_R: //1026
		if (!cc[Z])
		{
			*spostword=MemRead16(pc.Reg);
			pc.Reg+=*spostword;
			CycleCounter+=1;
		}
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case LBVC_R: //1028
		if ( !cc[V])
		{
			*spostword=MemRead16(pc.Reg);
			pc.Reg+=*spostword;
			CycleCounter+=1;
		}
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case LBVS_R: //1029
		if ( cc[V])
		{
			*spostword=MemRead16(pc.Reg);
			pc.Reg+=*spostword;
			CycleCounter+=1;
		}
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case LBPL_R: //102A
		if (!cc[N])
		{
			*spostword=MemRead16(pc.Reg);
			pc.Reg+=*spostword;
			CycleCounter+=1;
		}
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case LBMI_R: //102B
		if ( cc[N])
		{
			*spostword=MemRead16(pc.Reg);
			pc.Reg+=*spostword;
			CycleCounter+=1;
		}
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case LBGE_R: //102C
		if (! (cc[N] ^ cc[V]))
		{
			*spostword=MemRead16(pc.Reg);
			pc.Reg+=*spostword;
			CycleCounter+=1;
		}
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case LBLT_R: //102D
		if ( cc[V] ^ cc[N])
		{
			*spostword=MemRead16(pc.Reg);
			pc.Reg+=*spostword;
			CycleCounter+=1;
		}
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case LBGT_R: //102E
		if ( !( cc[Z] | (cc[N]^cc[V] ) ))
		{
			*spostword=MemRead16(pc.Reg);
			pc.Reg+=*spostword;
			CycleCounter+=1;
		}
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case LBLE_R:	//102F
		if ( cc[Z] | (cc[N]^cc[V]) )
		{
			*spostword=MemRead16(pc.Reg);
			pc.Reg+=*spostword;
			CycleCounter+=1;
		}
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case SWI2_I: //103F
		cc[E]=1;
		MemWrite8( pc.B.lsb,--s.Reg);
		MemWrite8( pc.B.msb,--s.Reg);
		MemWrite8( u.B.lsb,--s.Reg);
		MemWrite8( u.B.msb,--s.Reg);
		MemWrite8( y.B.lsb,--s.Reg);
		MemWrite8( y.B.msb,--s.Reg);
		MemWrite8( x.B.lsb,--s.Reg);
		MemWrite8( x.B.msb,--s.Reg);
		MemWrite8( dp.B.msb,--s.Reg);
		MemWrite8(B_REG,--s.Reg);
		MemWrite8(A_REG,--s.Reg);
		MemWrite8(get_cc_flags(),--s.Reg);
		pc.Reg=MemRead16(VSWI2);
		CycleCounter+=20;
		break;

	case CMPD_M: //1083
		postword=MemRead16(pc.Reg);
		temp16 = D_REG-postword;
		cc[C]= temp16 > D_REG;
		cc[V]= OTEST16(cc[C],postword,temp16,D_REG);//cc[C]^((postword^temp16^D_REG)>>15);
		cc[N]= NTEST16(temp16);
		cc[Z]= ZTEST(temp16);
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case CMPY_M: //108C
		postword=MemRead16(pc.Reg);
		temp16 = y.Reg-postword;
		cc[C]= temp16 > y.Reg;
		cc[V]= OTEST16(cc[C],postword,temp16,y.Reg);//cc[C]^((postword^temp16^y.Reg)>>15);
		cc[N]= NTEST16(temp16);
		cc[Z]= ZTEST(temp16);
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case LDY_M: //108E
		y.Reg= MemRead16(pc.Reg);
		cc[Z]= ZTEST(y.Reg);
		cc[N]= NTEST16(y.Reg);
		cc[V]= 0;
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case CMPD_D: //1093
		postword=MemRead16((dp.Reg |MemRead8(pc.Reg++)));
		temp16= D_REG - postword ;
		cc[C]= temp16 > D_REG;
		cc[V]= OTEST16(cc[C],postword,temp16,D_REG); //cc[C]^((postword^temp16^D_REG)>>15);
		cc[N]= NTEST16(temp16);
		cc[Z]= ZTEST(temp16);
		CycleCounter+=7;
		break;

	case CMPY_D:	//109C
		postword=MemRead16((dp.Reg |MemRead8(pc.Reg++)));
		temp16= y.Reg - postword ;
		cc[C]= temp16 > y.Reg;
		cc[V]= OTEST16(cc[C],postword,temp16,y.Reg);//cc[C]^((postword^temp16^y.Reg)>>15);
		cc[N]= NTEST16(temp16);
		cc[Z]= ZTEST(temp16);
		CycleCounter+=7;
		break;

	case LDY_D: //109E
		y.Reg=MemRead16((dp.Reg |MemRead8(pc.Reg++)));
		cc[Z]= ZTEST(y.Reg);
		cc[N]= NTEST16(y.Reg);
		cc[V]= 0;
		CycleCounter+=6;
		break;

	case STY_D: //109F
		MemWrite16(y.Reg,(dp.Reg |MemRead8(pc.Reg++)));
		cc[Z]= ZTEST(y.Reg);
		cc[N]= NTEST16(y.Reg);
		cc[V]= 0;
		CycleCounter+=6;
		break;

	case CMPD_X: //10A3
		postword=MemRead16(CalculateEA(MemRead8(pc.Reg++)));
		temp16= D_REG - postword ;
		cc[C]= temp16 > D_REG;
		cc[V]= OTEST16(cc[C],postword,temp16,D_REG);//cc[C]^((postword^temp16^D_REG)>>15);
		cc[N]= NTEST16(temp16);
		cc[Z]= ZTEST(temp16);
		CycleCounter+=7;
		break;

	case CMPY_X: //10AC
		postword=MemRead16(CalculateEA(MemRead8(pc.Reg++)));
		temp16= y.Reg - postword ;
		cc[C]= temp16 > y.Reg;
		cc[V]= OTEST16(cc[C],postword,temp16,Y_REG);
		cc[N]= NTEST16(temp16);
		cc[Z]= ZTEST(temp16);
		CycleCounter+=7;
		break;

	case LDY_X: //10AE
		y.Reg=MemRead16(CalculateEA(MemRead8(pc.Reg++)));
		cc[Z]= ZTEST(y.Reg);
		cc[N]= NTEST16(y.Reg);
		cc[V]= 0;
		CycleCounter+=6;
		break;

	case STY_X: //10AF
		MemWrite16(y.Reg,CalculateEA(MemRead8(pc.Reg++)));
		cc[Z]= ZTEST(y.Reg);
		cc[N]= NTEST16(y.Reg);
		cc[V]= 0;
		CycleCounter+=6;
		break;

	case CMPD_E: //10B3
		postword=MemRead16(MemRead16(pc.Reg));
		temp16 = D_REG-postword;
		cc[C]= temp16 > D_REG;
		cc[V]= OTEST16(cc[C],postword,temp16,D_REG);
		cc[N]= NTEST16(temp16);
		cc[Z]= ZTEST(temp16);
		pc.Reg+=2;
		CycleCounter+=8;
		break;

	case CMPY_E: //10BC
		postword=MemRead16(MemRead16(pc.Reg));
		temp16 = y.Reg-postword;
		cc[C]= temp16 > y.Reg;
		cc[V]= OTEST16(cc[C],postword,temp16,Y_REG);
		cc[N]= NTEST16(temp16);
		cc[Z]= ZTEST(temp16);
		pc.Reg+=2;
		CycleCounter+=8;
		break;

	case LDY_E: //10BE
		y.Reg=MemRead16(MemRead16(pc.Reg));
		cc[Z]= ZTEST(y.Reg);
		cc[N]= NTEST16(y.Reg);
		cc[V]= 0;
		pc.Reg+=2;
		CycleCounter+=7;
		break;

	case STY_E: //10BF
		MemWrite16(y.Reg,MemRead16(pc.Reg));
		cc[Z]= ZTEST(y.Reg);
		cc[N]= NTEST16(y.Reg);
		cc[V]= 0;
		pc.Reg+=2;
		CycleCounter+=7;
		break;

	case LDS_I:  //10CE
		s.Reg=MemRead16(pc.Reg);
		cc[Z]= ZTEST(s.Reg);
		cc[N]= NTEST16(s.Reg);
		cc[V] = 0;
		pc.Reg+=2;
		CycleCounter+=4;
		break;

	case LDS_D: //10DE
		s.Reg=MemRead16((dp.Reg |MemRead8(pc.Reg++)));
		cc[Z]= ZTEST(s.Reg);
		cc[N]= NTEST16(s.Reg);
		cc[V] = 0;
		CycleCounter+=6;
		break;

	case STS_D: //10DF
		MemWrite16(s.Reg,(dp.Reg |MemRead8(pc.Reg++)));
		cc[Z]= ZTEST(s.Reg);
		cc[N]= NTEST16(s.Reg);
		cc[V]= 0;
		CycleCounter+=6;
		break;

	case LDS_X: //10EE
		s.Reg=MemRead16(CalculateEA(MemRead8(pc.Reg++)));
		cc[Z]= ZTEST(s.Reg);
		cc[N]= NTEST16(s.Reg);
		cc[V]= 0;
		CycleCounter+=6;
		break;

	case STS_X: //10EF
		MemWrite16(s.Reg,CalculateEA(MemRead8(pc.Reg++)));
		cc[Z]= ZTEST(s.Reg);
		cc[N]= NTEST16(s.Reg);
		cc[V]= 0;
		CycleCounter+=6;
		break;

	case LDS_E: //10FE
		s.Reg=MemRead16(MemRead16(pc.Reg));
		cc[Z]= ZTEST(s.Reg);
		cc[N]= NTEST16(s.Reg);
		cc[V]= 0;
		pc.Reg+=2;
		CycleCounter+=7;
		break;

	case STS_E: //10FF
		MemWrite16(s.Reg,MemRead16(pc.Reg));
		cc[Z] = ZTEST(s.Reg);
		cc[N]= NTEST16(s.Reg);
		cc[V] = 0;
		pc.Reg+=2;
		CycleCounter+=7;
		break;

	default:
//		MessageBox(0,"Unhandled Op","Ok",0);
		break;
	}
} // P2_Opcode ends

void P3_Opcode(void)
{

	switch (MemRead8(pc.Reg++)) {

	case BREAK: //113E
		if (EmuState.Debugger.Break_Enabled()) {
			EmuState.Debugger.Halt();
		} else {
			CycleCounter+=4;
		}
		break;

	case SWI3_I: //113F
		cc[E]=1;
		MemWrite8( pc.B.lsb,--s.Reg);
		MemWrite8( pc.B.msb,--s.Reg);
		MemWrite8( u.B.lsb,--s.Reg);
		MemWrite8( u.B.msb,--s.Reg);
		MemWrite8( y.B.lsb,--s.Reg);
		MemWrite8( y.B.msb,--s.Reg);
		MemWrite8( x.B.lsb,--s.Reg);
		MemWrite8( x.B.msb,--s.Reg);
		MemWrite8( dp.B.msb,--s.Reg);
		MemWrite8(B_REG,--s.Reg);
		MemWrite8(A_REG,--s.Reg);
		MemWrite8(get_cc_flags(),--s.Reg);
		pc.Reg=MemRead16(VSWI3);
		CycleCounter+=20;
		break;

	case CMPU_M: //1183
		postword=MemRead16(pc.Reg);
		temp16 = u.Reg-postword;
		cc[C]= temp16 > u.Reg;
		cc[V]= OTEST16(cc[C],postword,temp16,U_REG);
		cc[N]= NTEST16(temp16);
		cc[Z]= ZTEST(temp16);
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case CMPS_M: //118C
		postword=MemRead16(pc.Reg);
		temp16 = s.Reg-postword;
		cc[C]= temp16 > s.Reg;
		cc[V]= OTEST16(cc[C],postword,temp16,S_REG);
		cc[N]= NTEST16(temp16);
		cc[Z]= ZTEST(temp16);
		pc.Reg+=2;
		CycleCounter+=5;
		break;

	case CMPU_D: //1193
		postword=MemRead16((dp.Reg |MemRead8(pc.Reg++)));
		temp16= u.Reg - postword ;
		cc[C]= temp16 > u.Reg;
		cc[V]= OTEST16(cc[C],postword,temp16,U_REG);
		cc[N]= NTEST16(temp16);
		cc[Z]= ZTEST(temp16);
		CycleCounter+=7;
		break;

	case CMPS_D: //119C
		postword=MemRead16((dp.Reg |MemRead8(pc.Reg++)));
		temp16= s.Reg - postword ;
		cc[C]= temp16 > s.Reg;
		cc[V]= OTEST16(cc[C],postword,temp16,S_REG);
		cc[N]= NTEST16(temp16);
		cc[Z]= ZTEST(temp16);
		CycleCounter+=7;
		break;

	case CMPU_X: //11A3
		postword=MemRead16(CalculateEA(MemRead8(pc.Reg++)));
		temp16= u.Reg - postword ;
		cc[C]= temp16 > u.Reg;
		cc[V]= OTEST16(cc[C],postword,temp16,U_REG);
		cc[N]= NTEST16(temp16);
		cc[Z]= ZTEST(temp16);
		CycleCounter+=7;
		break;

	case CMPS_X:  //11AC
		postword=MemRead16(CalculateEA(MemRead8(pc.Reg++)));
		temp16= s.Reg - postword ;
		cc[C]= temp16 > s.Reg;
		cc[V]= OTEST16(cc[C],postword,temp16,S_REG);
		cc[N]= NTEST16(temp16);
		cc[Z]= ZTEST(temp16);
		CycleCounter+=7;
		break;

	case CMPU_E: //11B3
		postword=MemRead16(MemRead16(pc.Reg));
		temp16 = u.Reg-postword;
		cc[C]= temp16 > u.Reg;
		cc[V]= OTEST16(cc[C],postword,temp16,U_REG);
		cc[N]= NTEST16(temp16);
		cc[Z]= ZTEST(temp16);
		pc.Reg+=2;
		CycleCounter+=8;
		break;

	case CMPS_E: //11BC
		postword=MemRead16(MemRead16(pc.Reg));
		temp16 = s.Reg-postword;
		cc[C]= temp16 > s.Reg;
		cc[V]= OTEST16(cc[C],postword,temp16,S_REG);
		cc[N]= NTEST16(temp16);
		cc[Z]= ZTEST(temp16);
		pc.Reg+=2;
		CycleCounter+=8;
		break;

	default:
//		MessageBox(0,"Unhandled Op","Ok",0);
		break;
	}

} // P3_Opcode ends

void cpu_firq(void)
{
	if (EmuState.Debugger.IsTracing())
		EmuState.Debugger.TraceCaptureInterruptServicing(INT_FIRQ, CycleCounter, MC6809GetState());

	cc[E] = 0; // Turn E flag off
	MemWrite8(pc.B.lsb, --s.Reg);
	MemWrite8(pc.B.msb, --s.Reg);
	MemWrite8(get_cc_flags(), --s.Reg);
	cc[I] = 1;
	cc[F] = 1;
	pc.Reg = MemRead16(VFIRQ);

	CycleCounter += 15;			// 10 Cycles to respond, 5 cycles to stack and load PC.

	if (EmuState.Debugger.IsTracing())
		EmuState.Debugger.TraceCaptureInterruptExecuting(INT_FIRQ, CycleCounter, MC6809GetState());
}

void cpu_irq(void)
{
	if (EmuState.Debugger.IsTracing())
		EmuState.Debugger.TraceCaptureInterruptServicing(INT_IRQ, CycleCounter, MC6809GetState());

	cc[E] = 1;
	MemWrite8(pc.B.lsb, --s.Reg);
	MemWrite8(pc.B.msb, --s.Reg);
	MemWrite8(u.B.lsb, --s.Reg);
	MemWrite8(u.B.msb, --s.Reg);
	MemWrite8(y.B.lsb, --s.Reg);
	MemWrite8(y.B.msb, --s.Reg);
	MemWrite8(x.B.lsb, --s.Reg);
	MemWrite8(x.B.msb, --s.Reg);
	MemWrite8(dp.B.msb, --s.Reg);
	MemWrite8(B_REG, --s.Reg);
	MemWrite8(A_REG, --s.Reg);
	MemWrite8(get_cc_flags(), --s.Reg);

	pc.Reg = MemRead16(VIRQ);
	cc[I] = 1;

	CycleCounter += 24;			// 10 Cycles to respond, 14 cycles to stack and load PC.

	if (EmuState.Debugger.IsTracing())
		EmuState.Debugger.TraceCaptureInterruptExecuting(INT_IRQ, CycleCounter, MC6809GetState());

	ClearIRQ();
}

void cpu_nmi(void)
{
	if (EmuState.Debugger.IsTracing())
		EmuState.Debugger.TraceCaptureInterruptServicing(INT_NMI, CycleCounter, MC6809GetState());

	cc[E] = 1;
	MemWrite8(pc.B.lsb, --s.Reg);
	MemWrite8(pc.B.msb, --s.Reg);
	MemWrite8(u.B.lsb, --s.Reg);
	MemWrite8(u.B.msb, --s.Reg);
	MemWrite8(y.B.lsb, --s.Reg);
	MemWrite8(y.B.msb, --s.Reg);
	MemWrite8(x.B.lsb, --s.Reg);
	MemWrite8(x.B.msb, --s.Reg);
	MemWrite8(dp.B.msb, --s.Reg);
	MemWrite8(B_REG, --s.Reg);
	MemWrite8(A_REG, --s.Reg);
	MemWrite8(get_cc_flags(), --s.Reg);
	cc[I] = 1;
	cc[F] = 1;
	pc.Reg = MemRead16(VNMI);

	CycleCounter += 24;			// 10 Cycles to respond, 14 cycles to stack and load PC.

	if (EmuState.Debugger.IsTracing())
		EmuState.Debugger.TraceCaptureInterruptExecuting(INT_NMI, CycleCounter, MC6809GetState());

	ClearNMI();
}

void set_cc_flags (unsigned char bincc)
{
	unsigned char bit;
	for (bit=0;bit<=7;bit++)
		cc[bit]=!!(bincc & (1<<bit));
	return;
}

unsigned char get_cc_flags(void)
{
	unsigned char bincc=0,bit=0;
	for (bit=0;bit<=7;bit++)
		if (cc[bit])
			bincc=bincc | (1<<bit);
		return(bincc);
}

void MC6809AssertInterupt(InterruptSource src, Interrupt interrupt)
{
	assert(src >= IS_NMI && src < IS_MAX);
	assert(interrupt >= INT_IRQ && interrupt <= INT_NMI);

	InterruptLine[src] |= Bit(interrupt);
	if (SyncWaiting || interrupt == INT_NMI)
		LatchInterrupts();
	SyncWaiting = 0;

	if (EmuState.Debugger.IsTracing())
		EmuState.Debugger.TraceCaptureInterruptRequest(interrupt, CycleCounter, MC6809GetState());
}

void MC6809DeAssertInterupt(InterruptSource src, Interrupt interrupt)
{
	assert(src >= IS_NMI && src < IS_MAX);
	assert(interrupt >= INT_IRQ && interrupt <= INT_NMI);

	InterruptLine[src] &= BitMask(interrupt);
}

void MC6809ForcePC(unsigned short NewPC)
{
	ClearInterrupts();
	pc.Reg=NewPC;
	SyncWaiting=0;
	return;
}

static unsigned short CalculateEA(unsigned char postbyte)
{
	static const std::array<unsigned short*, 4> indexableRegisters =
	{
		&X_REG,
		&Y_REG,
		&U_REG,
		&S_REG
	};

	static unsigned short int ea=0;
	static signed char byte=0;
	static unsigned char Register;

	Register = ((postbyte >> 5) & 3);

	if (postbyte & 0x80)
	{
		switch (postbyte & 0x1F)
		{
		case 0:
			ea=(*indexableRegisters[Register]);
			(*indexableRegisters[Register])++;
			CycleCounter+=2;
			break;

		case 1:
			ea=(*indexableRegisters[Register]);
			(*indexableRegisters[Register])+=2;
			CycleCounter+=3;
			break;

		case 2:
			(*indexableRegisters[Register])-=1;
			ea=(*indexableRegisters[Register]);
			CycleCounter+=2;
			break;

		case 3:
			(*indexableRegisters[Register])-=2;
			ea=(*indexableRegisters[Register]);
			CycleCounter+=3;
			break;

		case 4:
			ea=(*indexableRegisters[Register]);
			break;

		case 5:
			ea=(*indexableRegisters[Register])+((signed char)B_REG);
			CycleCounter+=1;
			break;

		case 6:
			ea=(*indexableRegisters[Register])+((signed char)A_REG);
			CycleCounter+=1;
			break;

		case 7:
//			ea=(*indexableRegisters[Register])+((signed char)E_REG);
			CycleCounter+=1;
			break;

		case 8:
			ea=(*indexableRegisters[Register])+(signed char)MemRead8(pc.Reg++);
			CycleCounter+=1;
			break;

		case 9:
			ea=(*indexableRegisters[Register])+MemRead16(pc.Reg);
			CycleCounter+=4;
			pc.Reg+=2;
			break;

		case 10:
//			ea=(*indexableRegisters[Register])+((signed char)F_REG);
			CycleCounter+=1;
			break;

		case 11:
			ea=(*indexableRegisters[Register])+D_REG; //Changed to unsigned 03/14/2005 NG Was signed
			CycleCounter+=4;
			break;

		case 12:
			ea=(signed short)pc.Reg+(signed char)MemRead8(pc.Reg)+1;
			CycleCounter+=1;
			pc.Reg++;
			break;

		case 13: //MM
			ea=pc.Reg+MemRead16(pc.Reg)+2;
			CycleCounter+=5;
			pc.Reg+=2;
			break;

		case 14:
//			ea=(*indexableRegisters[Register])+W_REG;
			CycleCounter+=4;
			break;

		case 15: //01111
			byte=(postbyte>>5)&3;
			switch (byte)
			{
			case 0:
//				ea=W_REG;
				break;
			case 1:
//				ea=W_REG+MemRead16(pc.Reg);
				pc.Reg+=2;
				break;
			case 2:
//				ea=W_REG;
//				W_REG+=2;
				break;
			case 3:
//				W_REG-=2;
//				ea=W_REG;
				break;
			}
		break;

		case 16: //10000
			byte=(postbyte>>5)&3;
			switch (byte)
			{
			case 0:
//				ea=MemRead16(W_REG);
				break;
			case 1:
//				ea=MemRead16(W_REG+MemRead16(pc.Reg));
				pc.Reg+=2;
				break;
			case 2:
//				ea=MemRead16(W_REG);
//				W_REG+=2;
				break;
			case 3:
//				W_REG-=2;
//				ea=MemRead16(W_REG);
				break;
			}
		break;


		case 17: //10001
			ea=(*indexableRegisters[Register]);
			(*indexableRegisters[Register])+=2;
			ea=MemRead16(ea);
			CycleCounter+=6;
			break;

		case 18: //10010
//			MessageBox(0,"Hitting undecoded 18","Ok",0);
			CycleCounter+=6;
			break;

		case 19: //10011
			(*indexableRegisters[Register])-=2;
			ea=(*indexableRegisters[Register]);
			ea=MemRead16(ea);
			CycleCounter+=6;
			break;

		case 20: //10100
			ea=(*indexableRegisters[Register]);
			ea=MemRead16(ea);
			CycleCounter+=3;
			break;

		case 21: //10101
			ea=(*indexableRegisters[Register])+((signed char)B_REG);
			ea=MemRead16(ea);
			CycleCounter+=4;
			break;

		case 22: //10110
			ea=(*indexableRegisters[Register])+((signed char)A_REG);
			ea=MemRead16(ea);
			CycleCounter+=4;
			break;

		case 23: //10111
//			ea=(*indexableRegisters[Register])+((signed char)E_REG);
			ea=MemRead16(ea);
			CycleCounter+=4;
			break;

		case 24: //11000
			ea=(*indexableRegisters[Register])+(signed char)MemRead8(pc.Reg++);
			ea=MemRead16(ea);
			CycleCounter+=4;
			break;

		case 25: //11001
			ea=(*indexableRegisters[Register])+MemRead16(pc.Reg);
			ea=MemRead16(ea);
			CycleCounter+=7;
			pc.Reg+=2;
			break;
		case 26: //11010
//			ea=(*indexableRegisters[Register])+((signed char)F_REG);
			ea=MemRead16(ea);
			CycleCounter+=4;
			break;

		case 27: //11011
			ea=(*indexableRegisters[Register])+D_REG;
			ea=MemRead16(ea);
			CycleCounter+=7;
			break;

		case 28: //11100
			ea=(signed short)pc.Reg+(signed char)MemRead8(pc.Reg)+1;
			ea=MemRead16(ea);
			CycleCounter+=4;
			pc.Reg++;
			break;

		case 29: //11101
			ea=pc.Reg+MemRead16(pc.Reg)+2;
			ea=MemRead16(ea);
			CycleCounter+=8;
			pc.Reg+=2;
			break;

		case 30: //11110
//			ea=(*indexableRegisters[Register])+W_REG;
			ea=MemRead16(ea);
			CycleCounter+=7;
			break;

		case 31: //11111
			ea=MemRead16(pc.Reg);
			ea=MemRead16(ea);
			CycleCounter+=8;
			pc.Reg+=2;
			break;

		} //END Switch
	}
	else
	{
		byte= (postbyte & 31);
		byte= (byte << 3);
		byte= byte /8;
		ea= *indexableRegisters[Register]+byte; //Was signed
		CycleCounter+=1;
	}
return(ea);
}


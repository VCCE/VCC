#pragma once
#include <array>
#include <vector>
#include <string>
#include "interrupts.h"

namespace VCC
{
	struct CPUState
	{
		//  
		unsigned char DP;
		unsigned char CC;
		unsigned char MD;
		//
		unsigned char A;
		unsigned char B;
		unsigned char E;
		unsigned char F;
		unsigned short X;
		unsigned short Y;
		unsigned short U;
		unsigned short S;
		unsigned short PC;
		unsigned short V;
		//
		bool IsNative6309;
		bool phyAddr;               // Decode using block relative addressing
		unsigned short block;       // Physical address = PC + block * 0x2000
	};

	// Trace Events
	enum class TraceEvent
	{
		Instruction,			// Instruction Executed
		IRQStart,				// Beginning of Interrupt Traces
		IRQRequested,			// Interrupt is being asserted
		IRQMasked,				// CPU acknowledged the interrupt, but it was masked, so it will be ignored
		IRQServicing,			// CPU acknowledged the interrupt, and will push state onto stack
		IRQExecuting,			// CPU is now passing control to the interrupt vector
		IRQEnd,					// End of Interrupt Traces
		ScreenStart,			// Beginning of Screen Traces
		ScreenVSYNCLow,			// Screen vertical synchronization start
		ScreenVSYNCHigh,		// Screen vertical synchronization end
		ScreenHSYNCLow,			// Screen HSYNC synchronization start
		ScreenHSYNCHigh,		// Screen HSYNC syncrhonization end
		ScreenTopBorder,		// Screen is drawing - top border
		ScreenRender,			// Screen is drawing - active region
		ScreenBottomBorder,		// Screen is drawing - bottom border,
		ScreenEnd,				// End of Screen Traces
		EmulatorCycle,			// Tracing Emulation Cycles
	};

	// CPU Execution Tracing
	struct CPUTrace
	{
		TraceEvent event;					// Trace Event type
		int frame;							// Frame being rendered
		int line;							// Line being rendered
		long cycleTime;						// Time since Trace start in CPU cycles
		unsigned short pc;					// Program Counter at time of event
		std::vector<unsigned char> bytes;	// Op code bytes
		std::string instruction;			// Instruction
		std::string operand;				// Instruction Operand
		CPUState startState;				// CPU State before instruction
		CPUState endState;					// CPU State after instruction
		int execCycles;						// Number of cycles indicated by emulator
		int decodeCycles;					// Number of cycles from OpDecoder
		int emulationState;					// State switch in emulation cycles
		std::vector<double> emulator;		// Emulation Tracing
	};

}

// make nth bit 0-7
inline constexpr uint8_t Bit(uint8_t n) { return 1 << n; }

// make mask of nth bit 0-7
inline constexpr uint8_t BitMask(uint8_t n) { return ~Bit(n); }

extern void (*CPUInit)(void);
extern int  (*CPUExec)(int);
extern void (*CPUReset)(void);
extern void (*CPUAssertInterupt)(InterruptSource, Interrupt);
extern void (*CPUDeAssertInterupt)(InterruptSource, Interrupt);
extern void (*CPUForcePC)(unsigned short);
extern void (*CPUSetBreakpoints)(const std::vector<unsigned short>&);
extern void (*CPUSetTraceTriggers)(const std::vector<unsigned short>&);
extern VCC::CPUState(*CPUGetState)();

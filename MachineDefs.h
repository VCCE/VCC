#pragma once
#include <array>
#include <vector>
#include <string>
#include <vcc/core/interrupts.h>

namespace VCC
{
	struct CPUState
	{
		//  
		unsigned char DP = 0;
		unsigned char CC = 0;
		unsigned char MD = 0;
		//
		unsigned char A = 0;
		unsigned char B = 0;
		unsigned char E = 0;
		unsigned char F = 0;
		unsigned short X = 0;
		unsigned short Y = 0;
		unsigned short U = 0;
		unsigned short S = 0;
		unsigned short PC = 0;
		unsigned short V = 0;
		//
		bool IsNative6309 = false;
		bool phyAddr = false;		// Decode using block relative addressing
		unsigned short block = 0;	// Physical address = PC + block * 0x2000
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
		TraceEvent event = TraceEvent::Instruction;	// Trace Event type
		int frame = 0;						// Frame being rendered
		int line = 0;						// Line being rendered
		long cycleTime = 0;					// Time since Trace start in CPU cycles
		unsigned short pc = 0;				// Program Counter at time of event
		std::vector<unsigned char> bytes;	// Op code bytes
		std::string instruction;			// Instruction
		std::string operand;				// Instruction Operand
		CPUState startState;				// CPU State before instruction
		CPUState endState;					// CPU State after instruction
		int execCycles = 0;					// Number of cycles indicated by emulator
		int decodeCycles = 0;				// Number of cycles from OpDecoder
		int emulationState = 0;				// State switch in emulation cycles
		std::vector<double> emulator;		// Emulation Tracing
	};

}

// make nth bit 0-7
constexpr uint8_t Bit(uint8_t n) { return 1 << n; }

// make mask of nth bit 0-7
constexpr uint8_t BitMask(uint8_t n) { return ~Bit(n); }

extern void (*CPUInit)();
extern int  (*CPUExec)(int);
extern void (*CPUReset)();
extern void (*CPUAssertInterupt)(InterruptSource, Interrupt);
extern void (*CPUDeAssertInterupt)(InterruptSource, Interrupt);
extern void (*CPUForcePC)(unsigned short);
extern void (*CPUSetBreakpoints)(const std::vector<unsigned short>&);
extern void (*CPUSetTraceTriggers)(const std::vector<unsigned short>&);
extern VCC::CPUState(*CPUGetState)();

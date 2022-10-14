#pragma once
#include <array>
#include <vector>


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
	};

}

//Common CPU defs
#define IRQ		1
#define FIRQ	2
#define NMI		3


extern void (*CPUInit)(void);
extern int  (*CPUExec)(int);
extern void (*CPUReset)(void);
extern void (*CPUAssertInterupt)(unsigned char, unsigned char);
extern void (*CPUDeAssertInterupt)(unsigned char);
extern void (*CPUForcePC)(unsigned short);
extern void (*CPUSetBreakpoints)(const std::vector<unsigned short>&);
extern VCC::CPUState(*CPUGetState)();
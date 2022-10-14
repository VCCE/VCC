#pragma once
#include <array>
#include "optional.hpp"


namespace VCC
{

	struct MMUState
	{
		bool Enabled;
		int ActiveTask;
		int RamVectors;
		int RomMap;

		std::array<int, 8>    Task0;
		std::array<int, 8>    Task1;
	};

	struct CPUState
	{
		//  
		unsigned char DP;
		unsigned char CC;
		tl::optional<unsigned char> MD;
		//
		unsigned char A;
		unsigned char B;
		tl::optional<unsigned char> E;
		tl::optional<unsigned char> F;
		unsigned short X;
		unsigned short Y;
		unsigned short U;
		unsigned short S;
		unsigned short PC;
		tl::optional<unsigned short> V;
	};

}
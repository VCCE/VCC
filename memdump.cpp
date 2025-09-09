//------------------------------------------------------------------
// Dump VCC memory for debugging purposes
// E J Jaquay 27sep24
//
// This file is part of VCC (Virtual Color Computer).
//
// VCC (Virtual Color Computer) is free software: you can redistribute
// it and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// VCC (Virtual Color Computer) is distributed in the hope that it will
// be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with VCC (Virtual Color Computer).  If not, see
// <http://www.gnu.org/licenses/>.
//
//------------------------------------------------------------------

#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include "defines.h"
#include "tcc1014mmu.h"
#include "memdump.h"

//---------------- private --------------------

// Path for dump file
char DumpFilePath[MAX_PATH]="VccDump";

// Get size of physical memory
int GetMemSize()
{
	int MemSize[4]={0x20000,0x80000,0x200000,0x800000};
	return MemSize[EmuState.RamSize & 3];
}

// Get current MMU registors
std::array<int,8> GetMmuRegs()
{
	VCC::MMUState mmu = GetMMUState();
	if (mmu.ActiveTask == 0)
		return mmu.Task0;
	else
		return mmu.Task1;
	// TODO adjust regs for ROM map
}

// Open dump file
int OpenDumpFile()
{
    int oflag = _O_CREAT | _O_TRUNC | _O_RDWR | _O_BINARY;
    int pmode = _S_IREAD | _S_IWRITE;
    return _open(DumpFilePath,oflag,pmode);
}

// Get size of physical memory
// Dump some memory to file
void blockdump(int fd, const unsigned char * ptr, int siz)
{
	while (siz > 0) {
		int cnt = _write(fd,ptr,siz);
		if (cnt <= 0) break;
		ptr += cnt;
		siz -= cnt;
	}
}

//---------------- public --------------------

// Set dump file path
void SetDumpPath(const char * dumpfile)
{
	if (strlen(dumpfile) > MAX_PATH) return;
	strncpy(DumpFilePath,dumpfile,MAX_PATH);
	return;
}

// Dump real memory
void MemDump(void)
{
	int fd = OpenDumpFile();
	unsigned char * ptr = Get_mem_pointer();
	int siz = GetMemSize();
	blockdump(fd,ptr,siz);
    _close(fd);
}

// Dump CPU memory
void CpuDump(void)
{
	std::array<int,8> regs = GetMmuRegs();
	unsigned char * pmem = Get_mem_pointer();
	int blk;

	int fd = OpenDumpFile();
	for (blk=0; blk<8; blk++) {
		unsigned char * ptr = pmem + regs[blk] * 0x2000;
		int siz = 0x2000;
		blockdump(fd,ptr,siz);
	}
    _close(fd);
}


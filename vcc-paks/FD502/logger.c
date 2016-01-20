#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tcc1014mmu.h"	//Need memread for CpuDump
#include "logger.h"

void WriteLog(char *Message,unsigned char Type)
{
	static HANDLE hout=NULL;
	static FILE  *disk_handle=NULL;
	unsigned long dummy;
	switch (Type)
	{
	case TOCONS:
		if (hout==NULL)
		{
			AllocConsole();
			hout=GetStdHandle(STD_OUTPUT_HANDLE);
			SetConsoleTitle("Logging Window"); 
		}
		WriteConsole(hout,Message,strlen(Message),&dummy,0);
		break;

	case TOFILE:
	if (disk_handle ==NULL)
		disk_handle=fopen("c:\\VccLog.txt","w");

	fprintf(disk_handle,"%s\r\n",Message);
	fflush(disk_handle);
	break;
	}

}


void CpuDump(void)
{
	FILE* disk_handle=NULL;
	int x;
	disk_handle=fopen("c:\\cpuspace_dump.txt","wb");
	for (x=0;x<=65535;x++)
		fprintf(disk_handle,"%c",MemRead8(x));
	fflush(disk_handle);
	fclose(disk_handle);
	return;
}



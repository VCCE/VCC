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

#include "logger.h"

void WriteLog(char *Message,unsigned char Type)
{
	static unsigned int Counter=1;
	static HANDLE hout=NULL;
	static FILE  *disk_handle=NULL;
	unsigned long dummy;
	char cTemp[512]="";
	switch (Type)
	{
	case TOCONS:
		if (hout==NULL)
		{
			AllocConsole();
			hout=GetStdHandle(STD_OUTPUT_HANDLE);
			SetConsoleTitle("Logging Window"); 
		}
		sprintf(cTemp,"%s",Message);
		WriteConsole(hout,cTemp,strlen(cTemp),&dummy,0);
		Counter++;
		break;

	case TOFILE:
	if (disk_handle ==NULL)
		disk_handle=fopen("c:\\VccLog.txt","w");

	fprintf(disk_handle,"%s\r\n",Message);
	fflush(disk_handle);
	break;
	}

}





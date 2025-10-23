#ifndef __IDEBUS_H__
#define __IDEBUS_H__
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
struct IDEINTERFACE {
	unsigned short	Data;
	unsigned char	Error[2];
	unsigned char	SecCount;
	unsigned char	SecNumber;
	unsigned char	Cylinderlsb;
	unsigned char	Cylindermsb;
	unsigned char	Head;
	unsigned char	Command;
	unsigned char	Status[2];
};

void IdeInit();
void IdeRegWrite(unsigned char,unsigned short);
unsigned short IdeRegRead(unsigned char);
void DiskStatus(char* text_buffer, size_t buffer_size);
unsigned char MountDisk(const char *,unsigned char );
unsigned char DropDisk(unsigned char);
void QueryDisk(unsigned char,char *);
//Status 
#define ERR		1	//Previous command ended in an error
#define IDX		2	//Unused
#define CORR	4	//Unused
#define	DRQ		8	//Data Request Ready (Sector buffer ready for transfer)
#define DSC		16	//Unused
#define	DF		32	//Write Fault
#define	RDY		64	//Ready for command
#define BUSY	128	//Controller is busy executing a command.

//Error Codes
#define AMNF	1	//Address Mark Not found
#define TKONF	2	//Track 0 not found
#define ABRT	4	//(Aborted Command) Set when Command Aborted due to drive error.
#define MCR		8	//(Media Change Request) Set to 0.
#define IDNF	16	//(ID Not Found) Set when Sector ID not found.
#define	MC		32	//(Media Changed) Set to 0.
#define UNC		64	//(Uncorrectable Data Error) Set when Uncorrectable Error is encountered.
#define BBK		128	//(Bad Block Detected) Set when a Bad Block is detected.



#define BUSYWAIT 5

#define MASTER 0
#define SLAVE 1


#endif


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

#include <Windows.h>
#include <stdio.h>
#include "fileops.h"

void ValidatePath(char *Path)
{
	char FullExePath[MAX_PATH]="";
	char TempPath[MAX_PATH]="";

	GetModuleFileName(nullptr,FullExePath,MAX_PATH);
	PathRemoveFileSpec(FullExePath);	//Get path to executable
	strcpy(TempPath,Path);			
	PathRemoveFileSpec(TempPath);		//Get path to Incomming file
	if (!strcmp(TempPath,FullExePath))	// If they match remove the Path
		PathStripPath(Path);
	return;
}

int CheckPath( char *Path)	//Return 1 on Error
{
	char TempPath[MAX_PATH]="";
	HANDLE hr=nullptr;

	if ((strlen(Path)==0) | (strlen(Path) > MAX_PATH))
		return 1;
	hr=CreateFile(Path,nullptr,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
	if (hr==INVALID_HANDLE_VALUE) //File Doesn't exist
	{
		GetModuleFileName(nullptr,TempPath,MAX_PATH);
		PathRemoveFileSpec(TempPath);
		if ( (strlen(TempPath)) + (strlen(Path)) > MAX_PATH)	//Resulting path is to large Bail.
			return 1;

		strcat(TempPath,Path);
		hr=CreateFile(TempPath,nullptr,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);			
		if (hr ==INVALID_HANDLE_VALUE)
			return 1;
		strcpy(Path,TempPath);
	}
	CloseHandle(hr);
	return 0;
}
// These are here to remove dependance on shlwapi.dll. ASCII only
void PathStripPath ( char *TextBuffer)  
{
	short Index=strlen(TextBuffer);
	char TempBuffer[MAX_PATH]="";
	if (strlen(TextBuffer) > MAX_PATH)	//Test for overflow
		return;

	while ( (Index>=0) & (TextBuffer[Index--] != '\\') );

	if (Index>0)
	{
		strcpy(TempBuffer, &TextBuffer[Index+2]);
		strcpy(TextBuffer,TempBuffer);
	}
	return;
}

BOOL PathRemoveFileSpec(char *Path)
{
	short unsigned Index=strlen(Path),Lenth=Index;
	if ( (Index==0) | (Index > MAX_PATH))
		return false;
	
	while ( (Index>0) & (Path[Index] != '\\') )
		Index--;
	while ( (Index>0) & (Path[Index] == '\\') )
		Index--;
	if (Index==0)
		return false;
	Path[Index+2]=0;
	return( !(strlen(Path) == Lenth));
}		

BOOL PathRemoveExtension(char *Path)
{
	short unsigned Index=strlen(Path),Lenth=Index;
	if ( (Index==0) | (Index > MAX_PATH))
		return false;
	
	while ( (Index>0) & (Path[Index--] != '.') );
	Path[Index+1]=0;
	return( !(strlen(Path) == Lenth));
}

char* PathFindExtension(char *Path)
{
	short unsigned Index=strlen(Path);
	if ( (Index==0) | (Index > MAX_PATH))
		return(&Path[strlen(Path)+1]);
	while ( (Index>0) & (Path[Index--] != '.') );
	return(&Path[Index+1]);
}

DWORD WritePrivateProfileInt(LPCTSTR SectionName,LPCTSTR KeyName,int KeyValue,LPCTSTR IniFileName)
{
	char Buffer[32]="";
	sprintf(Buffer,"%i",KeyValue);
	return(WritePrivateProfileString(SectionName,KeyName,Buffer,IniFileName));
}




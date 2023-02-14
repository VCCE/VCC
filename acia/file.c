//------------------------------------------------------------------
// Copyright E J Jaquay 2022
//
// This file is part of VCC (Virtual Color Computer).
//
// VCC (Virtual Color Computer) is free software: you can redistribute it
// and/or modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// VCC (Virtual Color Computer) is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// See the GNU General Public License for more details.  You should have
// received a copy of the GNU General Public License along with VCC
// (Virtual Color Computer). If not see <http://www.gnu.org/licenses/>.
//
//------------------------------------------------------------------

//------------------------------------------------------------------
// Input from or Output to windows file
//------------------------------------------------------------------

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "acia.h"
#include "logger.h"

FILE * FileStream = NULL;

// Open file. Binary mode so windows does not try to do line end LF
// translations. (OS9 uses CR for line endings)  File path is
// relative to %USERPROFILE%

int file_open()
{
    char filepath[MAX_PATH];
    strncpy(filepath,getenv("USERPROFILE"),MAX_PATH);
    strncat(filepath,"\\",MAX_PATH);
    switch (AciaComMode) {
    case COM_MODE_READ:
        strncat(filepath,AciaFileRdPath,MAX_PATH);
        FileStream = fopen(filepath,"rb");
        //PrintLogF("Opened %s %d\n",filepath,FileStream);
        break;
    case COM_MODE_WRITE:
        strcat(filepath,AciaFileWrPath);
        FileStream = fopen(filepath,"wb");
        //PrintLogF("Opened %s %d\n",filepath,FileStream);
        break;
    }
    return 0;
}

void file_close()
{
    //PrintLogF("Close %d\n",FileStream);
    if(FileStream) fclose(FileStream);
    FileStream = NULL;
}

// Read file.  If text remove LF characters
int file_read(char* buf,int siz)
{
    //PrintLogF("Read %d %d\n",FileStream, siz);
    int PrevChrCR=0; // True if last char read was a carriage return

    if (FileStream == NULL) {
        return -1;
    }

    int count;

    if (AciaTextMode) {
        count = 0;
        int chr = 0;

        while ((count < siz) && (chr != EOF)) {
            chr = fgetc(FileStream);
            if (chr == EOF) {
                if (!(PrevChrCR)) buf[count++] = '\r';
                buf[count++] = EOFCHR;
            } else {
                if ( (chr != '\n') || !(PrevChrCR) ) buf[count++] = chr;
                PrevChrCR = (chr == '\r') ? 1 : 0;
            }
        }
    } else {
        count = fread(buf,1,siz,FileStream);
    }

    return count;
}

// Write file.  If text skip LF chars and convert CR to CRLF
int  file_write(char* buf,int siz)
{
    //PrintLogF("Write %d %d\n",FileStream, siz);
    int count = 0;
    if (FileStream && (siz > 0)) {
        if (AciaTextMode) {
            for (int n=0; n<siz; n++) {
                count++;
                char chr = buf[n];
                if (chr != '\n') fputc(chr,FileStream);
                if (chr == '\r') fputc('\n',FileStream);
            }
        } else {
            count = fwrite(buf,1,siz,FileStream);
        }
    }
    return count;
}


/*
Copyright E J Jaquay 2022
This file is part of VCC (Virtual Color Computer).
    VCC (Virtual Color Computer) is free software: you can redistribute it
    and/or modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation, either version 3 of the License,
    or (at your option) any later version.

    VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with VCC (Virtual Color Computer).  If not, see <http://www.gnu.org/licenses/>.
*/

//------------------------------------------------------------------
// Read/Write file 
//------------------------------------------------------------------

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "acia.h"
#include "logger.h"

FILE * stream = NULL;

// Open file. See fopen for.  If there is no 'b' character in
// the mode file will considered to be text and CR/CRLF 
// translations will be done during I/O

int file_open()
{
PrintLogF("O %s\n",File_FilePath);
    char * mode = "wb";   //TODO make user changeable

	stream = fopen(File_FilePath,mode);
	if (stream) {
        return 0;
    } else {
        return errno;
	}
}

void file_close()
{
PrintLogF("C %s\n",File_FilePath);
    if(stream) fclose(stream); 
	stream = NULL;
}

// Read file.  If text convert CRLF to CR
int file_read(char* buf,int siz)
{
    int count;
    if (stream) 
        count = fread(buf,1,siz,stream);
    else
        count = 0;
    return count;
}

// Write file.  If text convert CR to CRLF
// TODO make text/bin mode user changeable

int  file_write(char* buf,int siz)
{
    int count = 0;
    if (stream) {
        for (int n=0; n<siz; n++) {
            count++;
            fputc(buf[n],stream);
            // if text convert CR to CRLF.
            if (buf[n] == '\r') fputc('\n',stream);
        }
       // count = fwrite(buf,1,siz,stream);
    } else {
        count = 0;
    }

PrintLogF("W %d %d\n",siz,count);

    return count;
}


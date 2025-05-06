#ifndef __COCO3_H__
#define __COCO3_H__
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

struct DisplayDetails
{
	int contentRows = 0;
	int topBorderRows = 0;
	int bottomBorderRows = 0;

	int contentColumns = 0;
	int leftBorderColumns = 0;
	int rightBorderColumns = 0;
};


//unsigned short RenderFrame (unsigned char);
void SetClockSpeed(unsigned int Cycles);
void SetLinesperScreen(unsigned char Lines);
DisplayDetails GetDisplayDetails(const int clientWidth, const int clientHeight);
void SetHorzInteruptState(unsigned char);
void SetVertInteruptState(unsigned char);
void SetSndOutMode(unsigned char);
float RenderFrame (SystemState *);

void SetTimerInteruptState(unsigned char);
void SetTimerClockRate (unsigned char);	
void SetInteruptTimer(unsigned int);
void MiscReset(void);
void PasteBASICWithNew();
void PasteBASIC();
void PasteText();
void QueueText(char *);
void CopyText();
void FlipArtifacts();
unsigned int SetAudioRate(unsigned int);

#endif

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

#ifndef _tcc1014graphics_h_
#define _tcc1014graphics_h_

/*****************************************************************************/

#include "tcc1014.h"

/*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
	
	void SetGimeVdgOffset(gime_t * pGIME, unsigned char );
	void SetGimeVdgMode(gime_t * pGIME, unsigned char);
	void SetGimeVdgMode2(gime_t * pGIME, unsigned char);

	void SetVerticalOffsetRegister(gime_t * pGIME, unsigned short);
    void SetVertScrollRegister(gime_t * pGIME, unsigned char data);

	void SetCompatMode(gime_t * pGIME, unsigned char);
	void SetGimePallet(gime_t * pGIME, unsigned char,unsigned char);
	void SetGimeVmode(gime_t * pGIME, unsigned char);
	void SetGimeVres(gime_t * pGIME, unsigned char);
	void SetGimeHorzOffset(gime_t * pGIME, unsigned char);
	void SetGimeBorderColor(gime_t * pGIME, unsigned char);
	
	void SetVideoBank(gime_t * pGIME, unsigned char);
	
    //void SetBlinkState(gime_t * pGIME, int);
	
#ifdef __cplusplus
}
#endif
	
/*****************************************************************************/

#endif // _tcc1014graphics_h_

#ifndef __MPI_H__
#define __MPI_H__
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

#include "../MachineDefs.h"

//Misc
constexpr auto MAX_LOADSTRING = 100u;
constexpr auto QUERY = 255u;

// FIXME: These need to be turned into an enum and the signature of functions
// that use them updated. These are also duplicated everywhere and need to be
// consolidated in one gdmf place.
#define	HEAD 0
#define SLAVE 1
#define STANDALONE 2

// FIXME: These typedefs are duplicated across more if not all projects and
// need to be consolidated in one place.
typedef void (*DYNAMICMENUCALLBACK)( const char *,int, int);
typedef void (*GETNAME)(char *,char *,DYNAMICMENUCALLBACK); 
typedef void (*CONFIGIT)(unsigned char); 
typedef void (*HEARTBEAT) (void);
typedef unsigned char (*PACKPORTREAD)(unsigned char);
typedef void (*PACKPORTWRITE)(unsigned char,unsigned char);
typedef void (*PAKINTERUPT)(unsigned char, unsigned char);

typedef unsigned char (*MEMREAD8)(unsigned short);
typedef void (*SETCART)(unsigned char);

typedef void (*SETCARTPOINTER)(SETCART);
typedef void (*MEMWRITE8)(unsigned char,unsigned short);
typedef void (*MODULESTATUS)(char *);

typedef void (*DMAMEMPOINTERS) ( MEMREAD8,MEMWRITE8);
typedef void (*SETINTERUPTCALLPOINTER) (PAKINTERUPT);
typedef unsigned short (*MODULEAUDIOSAMPLE)(void);
typedef void (*MODULERESET)(void);
typedef void (*SETINIPATH)(char *);

#endif

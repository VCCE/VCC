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

#ifndef _file_h_
#define _file_h_

/*****************************************************************************/

#include "xTypes.h"

/****************************************************************************/
/*
 Coco file types - ROM pak / plug-in / etc
 
 See also: sysGetFileType
 */

typedef enum filetype_e
{
    COCO_FILE_NONE = 0,
    
    COCO_PAK_ROM,
    COCO_PAK_PLUGIN,
    COCO_CPU_PLUGIN,

    COCO_CASSETTE_CAS,
    COCO_CASSETTE_WAVE,
    
    COCO_ADDONMOD,
    
    COCO_VDISK_FLOPPY,
    
    IMAGE,
    //VIDEO,
    
    //COCO_PERIPHERAL,
    //COCO_VDISK_VHD_XXX,
    //COCO_VDISK_ISO
} filetype_e;

typedef struct pair_t pair_t;
struct pair_t
{
    size_t           offset;
    unsigned char    value;
};

typedef struct filetype_t filetype_t;
struct filetype_t
{
    filetype_e      type;
    const char *    pExt;
	const char *    pDesc;
	pair_t          sig[4];
    
} ;

/*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
	
    extern filetype_t g_filetypelist[];
    
	filetype_e 		sysGetFileType(const char * path);
    const char *    sysGetFileTypeExtension(filetype_e fileType);

    result_t        fileGetExtension(const char * path, char * pDst, size_t szDst);
    result_t        fileGetFilename(const char * path, char * pDst, size_t szDst);
    result_t        fileGetFilenameNoExtension(const char * path, char * pDst, size_t szDst);

    result_t        fileTouch(const char * path);
    result_t        fileLoad(const char * path, void ** ppBuffer, size_t * pszBufferSize);
    
	/** Open a dialog to select a file */
	char *          sysGetPathnameFromUser(filetype_e * fileTypes, const char * pStartPath);
	char *          sysGetSavePathnameFromUser(filetype_e * fileTypes, const char * pStartPath);

#ifdef __cplusplus
}
#endif

/*****************************************************************************/

#endif // _file_h_


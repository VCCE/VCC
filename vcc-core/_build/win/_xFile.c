/*************************************************************************************/
/*
*/
/*************************************************************************************/
/*
 */
/*************************************************************************************/

#include "file.h"

#if !(defined _WIN32)
#	error "WINDOWS implementation"
#endif

#include <assert.h>

#include <Windows.h>

/*************************************************************************************/
/**
	@param fileTypes Types array terminated with the type COCO_FILE_NONE

	@return Windows file dialog filter string.  Must be released with free()
*/
char * convertFileTypesArray(filetype_e * fileTypes)
{
	char			filter[MAX_PATH] = "";
	filetype_t *    curType;

	if (fileTypes != NULL)
	{
		// walk user specified types
		int i = 0;
		while (fileTypes[i] != COCO_FILE_NONE)
		{
			// look for all entries that match
			curType = &g_filetypelist[0];
			while (curType->type != COCO_FILE_NONE
				&& curType->type != 0
				)
			{
				filetype_e fileType = fileTypes[i];

				if (fileType == curType->type)
				{
					if (strlen(filter) > 0)
					{
						strcat(filter, ";");
					}

					// add the extension for this type
					sprintf(strend(filter), "%s (*.%s), *.%s", curType->pDesc, curType->pExt, curType->pExt);
				}

				// next
				curType++;
			}

			i++;
		}

		// add the extension for this type
		if (strlen(filter) > 0)
		{
			strcat(filter, ";");
		}
		sprintf(strend(filter), "All files (*.*), *");

		return strdup(filter);
	}

	return NULL;
}

/*************************************************************************************/
/**
	Using platform specific UI, ask the user for a file (platform specific pathname object).

	Used for loading external ROM, ROM Pak, device Pak, etc
*/
char * sysGetPathnameFromUser(filetype_e * fileTypes, const char * pStartPath)
{
	OPENFILENAME	ofn;
	char *			pPathname = NULL;
	char			szFileName[MAX_PATH] = "";
	char *			filters = NULL;
	const char *	pcszTitle = "Open file";

	filters = convertFileTypesArray(fileTypes);

	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = (HWND)GetForegroundWindow();
	ofn.lpstrFilter = (const char *)filters;		// filter string
	ofn.nFilterIndex = 1;								// current filter index
	ofn.lpstrFile = szFileName;					// contains full path and filename on return
	ofn.nMaxFile = MAX_PATH;				// sizeof lpstrFile
	ofn.lpstrFileTitle = NULL;							// filename and extension only
	ofn.nMaxFileTitle = MAX_PATH;				// sizeof lpstrFileTitle
	ofn.lpstrInitialDir = NULL; //pStartPath ;					// initial directory
	ofn.lpstrTitle = pcszTitle;						// title bar string
	ofn.Flags = 0; //OFN_HIDEREADONLY;
	if (GetOpenFileName(&ofn))
	{
		pPathname = strdup(szFileName);
	}

	if (filters != NULL)
	{
		free(filters);
		filters = NULL;
	}

	return pPathname;
}

/*************************************************************************************/

char * sysGetSavePathnameFromUser(filetype_e * fileTypes, const char * pStartPath)
{
	OPENFILENAME	ofn;
	char *			pPathname = NULL;
	char			szFileName[MAX_PATH] = "";
	char *			filters = NULL;
	const char *	pcszTitle = "Save file";

	filters = convertFileTypesArray(fileTypes);

	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = (HWND)GetForegroundWindow();
	ofn.lpstrFilter = (const char *)filters;	// filter string
	ofn.nFilterIndex = 1;						// current filter index
	ofn.lpstrFile = szFileName;					// contains full path and filename on return
	ofn.nMaxFile = sizeof(szFileName-1);		// sizeof lpstrFile
	ofn.lpstrFileTitle = NULL;					// filename and extension only
	ofn.nMaxFileTitle = MAX_PATH;				// sizeof lpstrFileTitle
	ofn.lpstrInitialDir = pStartPath ;			// initial directory
	ofn.lpstrTitle = pcszTitle;					// title bar string
	ofn.Flags = 0; //OFN_HIDEREADONLY;
	if (GetSaveFileName(&ofn))
	{
		pPathname = strdup(szFileName);
	}

	if (filters != NULL)
	{
		free(filters);
		filters = NULL;
	}

	return pPathname;
}

/*************************************************************************************/

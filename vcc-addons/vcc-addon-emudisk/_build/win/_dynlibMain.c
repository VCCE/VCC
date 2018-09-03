/****************************************************************************/
/*
*/
/****************************************************************************/

#include "EmuDsk.h"
#include "cloud9.h"

#include <windows.h>
#include "resource.h" 

/****************************************************************************/

static HINSTANCE	g_hinstDLL = NULL;

/****************************************************************************/

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpReserved )  // reserved
{
	if (  fdwReason == DLL_PROCESS_ATTACH )
	{
		//
		// init
		//
		g_hinstDLL = hinstDLL;
		
		// we use the module name for the config section
		LoadString(g_hinstDLL, IDS_MODULE_NAME, g_szModuleName, MAX_LOADSTRING);
		LoadString(g_hinstDLL, IDS_CATNUMBER, g_szCatNumber, MAX_LOADSTRING);	

		EmuDskInit();

		return(1);
	}
	else if ( fdwReason == DLL_PROCESS_DETACH )
	{
		//
		// Clean Up 
		//
		EmuDskTerm();

		return(1);
	}

	return(1);
}

/****************************************************************************/

LRESULT CALLBACK Config(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch ( message )
	{
		case WM_INITDIALOG:
			SendDlgItemMessage(hDlg,IDC_CHECK_ENABLE,BM_SETCHECK,EnableCloud9RTC(QUERY),0);

			return TRUE; 
		break;

		case WM_COMMAND:
			switch ( LOWORD(wParam) )
			{
				case IDOK:
					EnableCloud9RTC((unsigned char)SendDlgItemMessage(hDlg,IDC_CHECK_ENABLE,BM_GETCHECK,0,0));
					SaveConfig();
					EndDialog(hDlg, LOWORD(wParam));
					return TRUE;
				break;

				case IDCLOSE:
				case IDCANCEL:
					EndDialog(hDlg, LOWORD(wParam));
				break;
			}

			return TRUE;
		break;

	}
    return FALSE;
}

/****************************************************************************/

void EmuDskConfig()
{
	DialogBox(g_hinstDLL, (LPCTSTR)IDD_DIALOG_EMUDSK_CONFIG, GetForegroundWindow(), (DLGPROC)Config);
}

/****************************************************************************/


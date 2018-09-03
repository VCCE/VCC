#include "xDebug.h"

LRESULT CALLBACK Config(HWND, UINT, WPARAM, LPARAM);


/****************************************************************************/

LRESULT CALLBACK Config(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	unsigned char BTemp=0;
	switch (message)
	{
		case WM_INITDIALOG:
			SendDlgItemMessage(hDlg,IDC_CLOCK,BM_SETCHECK,ClockEnabled,0);
			SendDlgItemMessage(hDlg,IDC_READONLY,BM_SETCHECK,ClockReadOnly,0);
			SendDlgItemMessage (hDlg,IDC_BASEADDR, CB_ADDSTRING, NULL,(LPARAM) "40");
			SendDlgItemMessage (hDlg,IDC_BASEADDR, CB_ADDSTRING, NULL,(LPARAM) "50");
			SendDlgItemMessage (hDlg,IDC_BASEADDR, CB_ADDSTRING, NULL,(LPARAM) "60");
			SendDlgItemMessage (hDlg,IDC_BASEADDR, CB_ADDSTRING, NULL,(LPARAM) "70");
			SendDlgItemMessage (hDlg,IDC_BASEADDR, CB_SETCURSEL,BaseAddr,(LPARAM)0);
			EnableWindow(GetDlgItem(hDlg, IDC_CLOCK), TRUE);
			if (BaseAddr==3)
			{
				ClockEnabled=0;
				SendDlgItemMessage(hDlg,IDC_CLOCK,BM_SETCHECK,ClockEnabled,0);
				EnableWindow(GetDlgItem(hDlg, IDC_CLOCK), FALSE);
			}
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
					BaseAddr=(unsigned char)SendDlgItemMessage(hDlg,IDC_BASEADDR,CB_GETCURSEL,0,0);
					ClockEnabled=(unsigned char)SendDlgItemMessage(hDlg,IDC_CLOCK,BM_GETCHECK,0,0);
					ClockReadOnly=(unsigned char)SendDlgItemMessage(hDlg,IDC_READONLY,BM_GETCHECK,0,0);
					BaseAddress=BaseTable[BaseAddr & 3];
					SetClockWrite(!ClockReadOnly);
					SaveConfig();
					EndDialog(hDlg, LOWORD(wParam));

					break;

				case IDCANCEL:
					EndDialog(hDlg, LOWORD(wParam));
					break;

				case IDC_CLOCK:

					break;

				case IDC_READONLY:
					break;

				case IDC_BASEADDR:
					BTemp=(unsigned char)SendDlgItemMessage(hDlg,IDC_BASEADDR,CB_GETCURSEL,0,0);
					EnableWindow(GetDlgItem(hDlg, IDC_CLOCK), TRUE);
					if (BTemp==3)
					{
						ClockEnabled=0;
						SendDlgItemMessage(hDlg,IDC_CLOCK,BM_SETCHECK,ClockEnabled,0);
						EnableWindow(GetDlgItem(hDlg, IDC_CLOCK), FALSE);
					}
					break;
			}
	}
	return(0);
}

/****************************************************************************/
/*
	DLL Main entry point (Windows)
*/
BOOL WINAPI DllMain(
	HINSTANCE hinstDLL,  // handle to DLL module
	DWORD fdwReason,     // reason for calling function
	LPVOID lpReserved)  // reserved
{
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			// one-time init
			g_hinstDLL = hinstDLL;
		}
		break;

		case DLL_PROCESS_DETACH:
		{
			// one time destruction
		}
		break;
	}

	return(1);
}


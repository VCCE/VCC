
#include "acia.h"

#define MAX_LOADSTRING 200


//------------------------------------------------------------------------
// DLL entry and exports 
//------------------------------------------------------------------------

typedef void (*DYNAMICMENUCALLBACK)( char *,int, int);
typedef void (*ASSERTINTERUPT) (unsigned char,unsigned char);

LRESULT CALLBACK Config(HWND, UINT, WPARAM, LPARAM);

void (*DynamicMenuCallback)( char *,int, int)=NULL;
void BuildDynaMenu(void);
void LoadConfig(void);
void SaveConfig(void);

//------------------------------------------------------------------------
// Globals private to acia.c 
//------------------------------------------------------------------------

static HINSTANCE g_hinstDLL=NULL;

//------------------------------------------------------------------------
//  DLL Entry  
//------------------------------------------------------------------------

BOOL APIENTRY DllMain( HINSTANCE  hinstDLL,
                       DWORD  reason,
                       LPVOID lpReserved )
{
    if (reason == DLL_PROCESS_ATTACH) g_hinstDLL = hinstDLL;
    if (reason == DLL_PROCESS_DETACH) sc6551_close();
    return TRUE;
}

//-----------------------------------------------------------------------
//  Register the DLL
//-----------------------------------------------------------------------
__declspec(dllexport) void
ModuleName(char *ModName,char *CatNumber,DYNAMICMENUCALLBACK Temp)
{
	LoadString(g_hinstDLL,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
	LoadString(g_hinstDLL,IDS_CATNUMBER,CatNumber, MAX_LOADSTRING);		
	DynamicMenuCallback =Temp;
	if (DynamicMenuCallback  != NULL)
			BuildDynaMenu();
	return ;
}

//-----------------------------------------------------------------------
// Export write to port
//-----------------------------------------------------------------------
__declspec(dllexport) void
PackPortWrite(unsigned char Port,unsigned char Data)
{
    sc6551_write(Data,Port);
    return;
}

//-----------------------------------------------------------------------
// Export read from port
//-----------------------------------------------------------------------
__declspec(dllexport) unsigned char 
PackPortRead(unsigned char Port)
{
    return sc6551_read(Port);
}

//-----------------------------------------------------------------------
// This captures the transfer point for the CPU assert interupt 
//-----------------------------------------------------------------------
__declspec(dllexport) void AssertInterupt(ASSERTINTERUPT Dummy)
{
	AssertInt=Dummy;
	return;
}

//-----------------------------------------------------------------------
//
//-----------------------------------------------------------------------
__declspec(dllexport) void ModuleStatus(char *status)
{
    if (sc6551_initialized) {
        if (AciaComType == 0) {
            if (ConsoleLineInput) strcpy(status,"LineMode");
            else strcpy(status,"Console"); 
        } else {
            strcpy(status,"AciaUnknown");
        }
    } else {
        strcpy(status,"Acia");
    }
    return;
}

//-----------------------------------------------------------------------
//
//-----------------------------------------------------------------------
__declspec(dllexport) void ModuleConfig(unsigned char MenuID)
{
    DialogBox(g_hinstDLL, (LPCTSTR)IDD_PROPPAGE, NULL, (DLGPROC)Config);
	BuildDynaMenu();
	return;
}

//-----------------------------------------------------------------------
//
//-----------------------------------------------------------------------
__declspec(dllexport) void SetIniPath (char *IniFilePath)
{
//	strcpy(IniFile,IniFilePath);
//	LoadConfig();
	return;
}

//-----------------------------------------------------------------------
//  Add config option to Cartridge menu
//-----------------------------------------------------------------------
void BuildDynaMenu(void)
{
	DynamicMenuCallback("",0,0);     // begin
	DynamicMenuCallback("",6000,0);  // seperator
	DynamicMenuCallback("ACIA Config",5016,STANDALONE); // Config
	DynamicMenuCallback("",1,0);     // end
}

//-----------------------------------------------------------------------
//  Config dialog. Allow user to select com; Console, TCP, etc
//-----------------------------------------------------------------------
LRESULT CALLBACK Config(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND hwndOwner; 
	RECT rc, rcDlg, rcOwner; 

	switch (message)
	{
		case WM_INITDIALOG:

            if ((hwndOwner = GetParent(hDlg)) == NULL) {
                hwndOwner = GetDesktopWindow();
            }

            GetWindowRect(hwndOwner, &rcOwner); 
			GetWindowRect(hDlg, &rcDlg); 
			CopyRect(&rc, &rcOwner); 

			OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top); 
			OffsetRect(&rc, -rc.left, -rc.top); 
			OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom); 

		    SetWindowPos(hDlg, 
                 HWND_TOP, 
                 rcOwner.left + (rc.right / 2), 
                 rcOwner.top + (rc.bottom / 2), 
                 0, 0,          // Ignores size arguments. 
                 SWP_NOSIZE); 
// Send initial settings  here
//               SendDlgItemMessage(hDlg,IDC_FOO,WM_SETTEXT,strlen(msg),msg);
			return TRUE; 
		break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
// config stuff here
					EndDialog(hDlg, LOWORD(wParam));
					return TRUE;
				break;

				case IDHELP:
					return TRUE;
				break;

				case IDCANCEL:
					EndDialog(hDlg, LOWORD(wParam));
				break;
			}
			return TRUE;
		break;

	}
    return FALSE;
}


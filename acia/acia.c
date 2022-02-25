
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
static int AciaComType; // Communications type 0=console

//------------------------------------------------------------------------
//  DLL Entry  
//------------------------------------------------------------------------

BOOL APIENTRY DllMain( HINSTANCE  hinstDLL,
                       DWORD  reason,
                       LPVOID lpReserved )
{
    if (reason == DLL_PROCESS_DETACH) sc6551_close();
//    else sc6551_initialized = 0;
    return TRUE;
}

//-----------------------------------------------------------------------
//  Setup menu and return module name
//-----------------------------------------------------------------------

__declspec(dllexport) void
ModuleName(char *ModName,char *CatNumber,DYNAMICMENUCALLBACK Temp)
{
	LoadString(g_hinstDLL,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
	LoadString(g_hinstDLL,IDS_CATNUMBER,CatNumber, MAX_LOADSTRING);		
	strcpy(ModName,"ACIA");
	DynamicMenuCallback =Temp;
	if (DynamicMenuCallback  != NULL)
			BuildDynaMenu();
	return ;
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
__declspec(dllexport) void SetIniPath (char *IniFilePath)
{
//	strcpy(IniFile,IniFilePath);
//	LoadConfig();
	return;
}

//-----------------------------------------------------------------------
//
//-----------------------------------------------------------------------
void BuildDynaMenu(void)
{
	if (DynamicMenuCallback == NULL)
		MessageBox(0,"No good","Ok",0);
	DynamicMenuCallback("",0,0);
	DynamicMenuCallback("",0,0);
	DynamicMenuCallback("",0,0);
	DynamicMenuCallback("",0,0);
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
// config stuff here
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
//----------------------------------------------------------------
// Dispatch I/0 to communication type used.
// Hooks allow sc6551 to do communications to selected media
//----------------------------------------------------------------

// Open com
void acia_open_com() {
	switch (AciaComType) {
	case 0: // Legacy Console
        console_open();
		break;
	}
}

void acia_close_com() {
	switch (AciaComType) {
	case 0: // Console
        console_close();
		break;
	}
}

int acia_write_com(char * buf,int len) { // returns bytes written
	switch (AciaComType) {
	case 0: // Legacy Console
        return console_write(buf,len);
        break;
    }
    return 0;
}

int acia_read_com(char * buf,int len) {  // returns bytes read
	switch (AciaComType) {
	case 0: // Legacy Console
        return console_read(buf,len);
    }
    return 0;
}


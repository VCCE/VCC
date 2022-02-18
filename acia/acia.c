
#include "acia.h"

static int ComType; // Communications type 0=console

//------------------------------------------------------------------------
// DLL entry and exports 
//------------------------------------------------------------------------

#define MAX_LOADSTRING 200

// vcc stuff

typedef void (*DYNAMICMENUCALLBACK)( char *,int, int);
typedef void (*ASSERTINTERUPT) (unsigned char,unsigned char);

static HINSTANCE g_hinstDLL=NULL;
LRESULT CALLBACK Config(HWND, UINT, WPARAM, LPARAM);

void (*DynamicMenuCallback)( char *,int, int)=NULL;
void BuildDynaMenu(void);
void LoadConfig(void);
void SaveConfig(void);

BOOL APIENTRY DllMain( HINSTANCE  hinstDLL,
                       DWORD  reason,
                       LPVOID lpReserved )
{
    switch (reason) {
    case DLL_PROCESS_ATTACH:
//        sc6551_init();
    case DLL_PROCESS_DETACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}

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

__declspec(dllexport) void ModuleConfig(unsigned char MenuID)
{
		DialogBox(g_hinstDLL, (LPCTSTR)IDD_PROPPAGE, NULL, (DLGPROC)Config);
		BuildDynaMenu();
		return;
}

// Export write to port
__declspec(dllexport) void
PackPortWrite(unsigned char Port,unsigned char Data)
{
    sc6551_write(Data,Port);
    return;
}

// Export read from port
__declspec(dllexport) unsigned char 
PackPortRead(unsigned char Port)
{
    return sc6551_read(Port);
}

// This captures the transfer point for the CPU assert interupt 
__declspec(dllexport) void AssertInterupt(ASSERTINTERUPT Dummy)
{
	AssertInt=Dummy;
	return;
}

void BuildDynaMenu(void)
{
	if (DynamicMenuCallback == NULL)
		MessageBox(0,"No good","Ok",0);
	DynamicMenuCallback("",0,0);
	DynamicMenuCallback("",0,0);
	DynamicMenuCallback("",0,0);
	DynamicMenuCallback("",0,0);
	
}

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
// Acia com hooks. These dispatch to communication type used
//----------------------------------------------------------------

// Open com
void acia_open_com() {
	switch (ComType) {
	case 0: // Legacy Console
        console_open();
		break;
	}
}

void acia_write_com(unsigned char chr) {
	switch (ComType) {
	case 0: // Legacy Console
        console_write(chr);
        break;
    }
}

unsigned char acia_read_com() {
	switch (ComType) {
	case 0: // Legacy Console
        return console_read();
    }
    return 0;
}

void acia_close_com() {
	switch (ComType) {
	case 0: // Console
        console_close();
		break;
	}
}


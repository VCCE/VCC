/****************************************************************************/
/*
	Emulated hard disk (EmuDsk)

	Mapped to $FF80-$FF86

	TODO: x-plat open filename
	TODO: x-plat / revised dynamic menus
	TODO: x-plat message box
	TODO: x-plat config
*/
/****************************************************************************/

#include "EmuDsk.h"
#include "cc3vhd.h"
#include "cloud9.h"

#include "vccDefines.h"
#include "vccFile.h"
#include "vccPAKAPI.h"

#include "xAssert.h"
#include "xConf.h"

#include <io.h>

#include <windows.h>

/****************************************************************************/

static pfnvccpak_dynmenucb_t	g_pfnDynMenuCB	= NULL;

static char		VHD_FileName[2][MAX_PATH_LENGTH]		= {"",""};
static char		g_szConfFilename[MAX_PATH_LENGTH]		= "";
char			g_szModuleName[MAX_LOADSTRING]			= "";
char			g_szCatNumber[MAX_LOADSTRING]			= "";

/****************************************************************************/

void EmuDskInit()
{
}

/****************************************************************************/

void EmuDskTerm()
{
	UnmountHD(0);
	UnmountHD(1);
}

/****************************************************************************/

unsigned char EnableCloud9RTC(unsigned char Tmp)
{
	if ( Tmp != QUERY )
	{
		bCloud9RTCEnable = Tmp;
	}
	return (bCloud9RTCEnable);
}

/****************************************************************************/

void LoadHardDisk(int iDiskNum)
{
	OPENFILENAME ofn ;	

	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize       = sizeof (OPENFILENAME) ;
	ofn.hwndOwner         = GetForegroundWindow();
	ofn.lpstrFilter       =	"HardDisk Images (*.VHD,*.OS9)\0*.VHD;*.OS9\0\0";	// filter VHD images
	ofn.nFilterIndex      = 1 ;								// current filter index
	ofn.lpstrFile         = VHD_FileName[iDiskNum] ;				// contains full path and filename on return
	ofn.nMaxFile          = MAX_PATH;						// sizeof lpstrFile
	ofn.lpstrFileTitle    = NULL;							// filename and extension only
	ofn.nMaxFileTitle     = MAX_PATH ;						// sizeof lpstrFileTitle
	ofn.lpstrInitialDir   = NULL;							// initial directory
	ofn.lpstrTitle        = TEXT("Load HardDisk Image") ;	// title bar string
	ofn.Flags             = OFN_HIDEREADONLY;
	
	if ( GetOpenFileName (&ofn) )
	{
		if ( MountHD(VHD_FileName[iDiskNum],iDiskNum) == 0 )
		{
			MessageBox(NULL,"Can't open file","Error",0);
		}
	}
}

/****************************************************************************/

void BuildDynaMenu(void)
{
	char TempMsg[512]="";
	char TempBuf[MAX_PATH_LENGTH]="";

	g_pfnDynMenuCB("",0,0);
	g_pfnDynMenuCB( "",6000,0);
	g_pfnDynMenuCB( "EmuDsk HD 0",6000,HEAD);
	g_pfnDynMenuCB( "Insert",5010,SLAVE);
	strcpy(TempMsg,"Eject: ");
	strcpy(TempBuf,VHD_FileName[0]);
	PathStripPath (TempBuf);
	strcat(TempMsg,TempBuf);
	g_pfnDynMenuCB( TempMsg,5011,SLAVE);

	g_pfnDynMenuCB( "EmuDsk HD 1",6001,HEAD);
	g_pfnDynMenuCB( "Insert",5012,SLAVE);
	strcpy(TempMsg,"Eject: ");
	strcpy(TempBuf,VHD_FileName[1]);
	PathStripPath (TempBuf);
	strcat(TempMsg,TempBuf);
	g_pfnDynMenuCB( TempMsg,5013,SLAVE);

	g_pfnDynMenuCB( "EmuDsk Config",5014,STANDALONE);

	g_pfnDynMenuCB( "",1,0);
}

/****************************************************************************/

void LoadConfig()
{
	GetPrivateProfileString(g_szModuleName,"VHDImage","",VHD_FileName[0],MAX_PATH_LENGTH,g_szConfFilename);
	CheckPath(VHD_FileName[0]);
	if ( _access(VHD_FileName[0],6) == -1 )
	{
		strcpy(VHD_FileName[0],"");
		WritePrivateProfileString(g_szModuleName,"VHDImage",VHD_FileName[0] ,g_szConfFilename);
	}
	else
	{
		MountHD(VHD_FileName[0],0);
	}
	
	GetPrivateProfileString(g_szModuleName,"VHDImage1","",VHD_FileName[1],MAX_PATH,g_szConfFilename);
	CheckPath(VHD_FileName[1]);
	if ( _access(VHD_FileName[1],6) == -1 )
	{
		strcpy(VHD_FileName[1],"");
		WritePrivateProfileString(g_szModuleName,"VHDImage1",VHD_FileName[1] ,g_szConfFilename);
	}
	else
	{
		MountHD(VHD_FileName[1],1);
	}

	bCloud9RTCEnable = GetPrivateProfileInt(g_szModuleName,"Cloud9RTC",1,g_szConfFilename);  

	BuildDynaMenu();
}

/****************************************************************************/

void SaveConfig(void)
{
	ValidatePath(VHD_FileName[0]);
	WritePrivateProfileString(g_szModuleName,"VHDImage",VHD_FileName[0] ,g_szConfFilename);

	ValidatePath(VHD_FileName[1]);
	WritePrivateProfileString(g_szModuleName,"VHDImage1",VHD_FileName[1] ,g_szConfFilename);

	WritePrivateProfileInt(g_szModuleName,"Cloud9RTC",bCloud9RTCEnable ,g_szConfFilename);
}

/****************************************************************************/

XAPI_EXPORT void VCCPakInit(const char * pcszConfFilename, char * pszModuleName, char * pszCatNumber, pfnvccpak_dynmenucb_t pfncbDynMenu)
{
	strcpy(pszModuleName,g_szModuleName);
	strcpy(pszCatNumber,g_szCatNumber);

	g_pfnDynMenuCB = pfncbDynMenu;

	strcpy(g_szConfFilename,pcszConfFilename);
	LoadConfig();
}

/****************************************************************************/

XAPI_EXPORT void VCCPakConfig(unsigned char MenuID)
{
	switch ( MenuID )
	{
	case 10:
		LoadHardDisk(0);
		SaveConfig();
		BuildDynaMenu();
		break;

	case 11:
		UnmountHD(0);
		strcpy(VHD_FileName[0] ,"");
		SaveConfig();
		BuildDynaMenu();
		break;

	case 12:
		LoadHardDisk(1);
		SaveConfig();
		BuildDynaMenu();
		break;

	case 13:
		UnmountHD(1);
		strcpy(VHD_FileName[1] ,"");
		SaveConfig();
		BuildDynaMenu();
		break;

	case 14:
		EmuDskConfig();
		break;

	default:
		assert(0);

	}
}

/****************************************************************************/

XAPI_EXPORT void VCCPakPortWrite(mmu_t * pMMU, u8_t Data, u8_t Port)
{
	VHDWrite(pMMU,Data,Port);
}

/****************************************************************************/

XAPI_EXPORT unsigned char VCCPakPortRead(mmu_t * pMMU, u8_t Port)
{
	switch ( Port )
	{
		case CLOCK_PORT:
		case CLOCK_PORT + 1:
		case CLOCK_PORT + 4:
			if ( bCloud9RTCEnable )
			{
				return ReadTime(Port);
			}

			return 0;
			break;

		default:
			return VHDRead(pMMU,Port);
			break;
	}	//End port switch
}

/****************************************************************************/

XAPI_EXPORT void VCCPakStatus(char *MyStatus)
{
	DiskStatus(MyStatus);
}

/****************************************************************************/


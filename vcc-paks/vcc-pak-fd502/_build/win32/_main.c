#include <windows.h>
#include <winioctl.h>

static HINSTANCE g_hinstDLL;

LRESULT CALLBACK Config(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK NewDisk(HWND,UINT, WPARAM, LPARAM);


BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpReserved )  // reserved
{
	if (fdwReason == DLL_PROCESS_DETACH ) //Clean Up 
	{
		for (unsigned char Drive=0;Drive<=3;Drive++)
			unmount_disk_image(Drive);
	}
	else
	{
		g_hinstDLL=hinstDLL;
		RealDisks=InitController();
	}
	return(1);
}

void BuildDynaMenu(void)
{
	char TempMsg[64]="";
	char TempBuf[MAX_PATH]="";
	if (DynamicMenuCallback ==NULL)
		MessageBox(0,"No good","Ok",0);
	DynamicMenuCallback("",0,0);
	DynamicMenuCallback( "",6000,0);
	DynamicMenuCallback( "FD-502 Drive 0",6000,HEAD);
	DynamicMenuCallback( "Insert",5010,SLAVE);
	strcpy(TempMsg,"Eject: ");
	strcpy(TempBuf,Drive[0].ImageName);
	PathStripPath(TempBuf);
	strcat(TempMsg,TempBuf);
	DynamicMenuCallback( TempMsg,5011,SLAVE);

	DynamicMenuCallback( "FD-502 Drive 1",6000,HEAD);
	DynamicMenuCallback( "Insert",5012,SLAVE);
	strcpy(TempMsg,"Eject: ");
	strcpy(TempBuf,Drive[1].ImageName);
	PathStripPath(TempBuf);
	strcat(TempMsg,TempBuf);
	DynamicMenuCallback( TempMsg,5013,SLAVE);

	DynamicMenuCallback( "FD-502 Drive 2",6000,HEAD);
	DynamicMenuCallback( "Insert",5014,SLAVE);
	strcpy(TempMsg,"Eject: ");
	strcpy(TempBuf,Drive[2].ImageName);
	PathStripPath(TempBuf);
	strcat(TempMsg,TempBuf);
	DynamicMenuCallback( TempMsg,5015,SLAVE);
//NEW
	DynamicMenuCallback( "FD-502 Drive 3",6000,HEAD);
	DynamicMenuCallback( "Insert",5017,SLAVE);
	strcpy(TempMsg,"Eject: ");
	strcpy(TempBuf,Drive[3].ImageName);
	PathStripPath(TempBuf);
	strcat(TempMsg,TempBuf);
	DynamicMenuCallback( TempMsg,5018,SLAVE);
//NEW 
	DynamicMenuCallback( "FD-502 Config",5016,STANDALONE);
	DynamicMenuCallback( "",1,0);
}

long CreateDisk (unsigned char Disk)
{
	NewDiskNumber=Disk;
	DialogBox(g_hinstDLL, (LPCTSTR)IDD_NEWDISK, NULL, (DLGPROC)NewDisk);
	return(0);
}

LRESULT CALLBACK NewDisk(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	unsigned char temp=0,temp2=0;
	static unsigned char NewDiskType=DMK,NewDiskTracks=2,DblSided=1;
	char Dummy[MAX_PATH]="";
	long DiskType[3]={IDC_JVC,IDC_VDK,IDC_DMK};
	long DiskTracks[3]={IDC_TR35,IDC_TR40,IDC_TR80};

	switch (message)
	{
		case WM_INITDIALOG:
			for (temp=0;temp<=2;temp++)
				SendDlgItemMessage(hDlg,DiskType[temp],BM_SETCHECK,(temp==NewDiskType),0);
			for (temp=0;temp<=3;temp++)
				SendDlgItemMessage(hDlg,DiskTracks[temp],BM_SETCHECK,(temp==NewDiskTracks),0);
			SendDlgItemMessage(hDlg,IDC_DBLSIDE,BM_SETCHECK,DblSided,0);
			strcpy(Dummy,TempFileName);
			PathStripPath(Dummy);
			SendDlgItemMessage(hDlg,IDC_TEXT1,WM_SETTEXT,strlen(Dummy),(LPARAM)(LPCSTR)Dummy);	
			return TRUE; 
		break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_DMK:
				case IDC_JVC:
				case IDC_VDK:
					for (temp=0;temp<=2;temp++)
						if (LOWORD(wParam)==DiskType[temp])
						{
							for (temp2=0;temp2<=2;temp2++)
								SendDlgItemMessage(hDlg,DiskType[temp2],BM_SETCHECK,0,0);
							SendDlgItemMessage(hDlg,DiskType[temp],BM_SETCHECK,1,0);
							NewDiskType=temp;
						}
				break;

				case IDC_TR35:
				case IDC_TR40:
				case IDC_TR80:
					for (temp=0;temp<=2;temp++)
						if (LOWORD(wParam)==DiskTracks[temp])
						{
							for (temp2=0;temp2<=2;temp2++)
								SendDlgItemMessage(hDlg,DiskTracks[temp2],BM_SETCHECK,0,0);
							SendDlgItemMessage(hDlg,DiskTracks[temp],BM_SETCHECK,1,0);
							NewDiskTracks=temp;
						}
				break;

				case IDC_DBLSIDE:
					DblSided=!DblSided;
					SendDlgItemMessage(hDlg,IDC_DBLSIDE,BM_SETCHECK,DblSided,0);
				break;

				case IDOK:
					EndDialog(hDlg, LOWORD(wParam));
					PathRemoveExtension(TempFileName);
					strcat(TempFileName,".dsk");
					if( CreateDiskHeader(TempFileName,NewDiskType,NewDiskTracks,DblSided))
					{
						strcpy(TempFileName,"");
						MessageBox(0,"Can't create File","Error",0);
					}
					return TRUE;
				break;

				case IDCANCEL:
					EndDialog(hDlg, LOWORD(wParam));
					CreateFlag=0;
					return FALSE;
				break;
			}
			return TRUE;
		break;
	}
    return FALSE;
}

void LoadConfig(void)
{
	char ModName[MAX_LOADSTRING]="";
	unsigned char Index=0;
	char Temp[16]="";
	char DiskName[MAX_PATH]="";
	unsigned int RetVal=0;
	HANDLE hr=NULL;
	LoadString(g_hinstDLL,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
	SelectRom = GetPrivateProfileInt(ModName,"DiskRom",1,IniFile);  //0 External 1=TRSDOS 2=RGB Dos
	GetPrivateProfileString(ModName,"RomPath","",RomFileName,MAX_PATH,IniFile);
	PersistDisks=GetPrivateProfileInt(ModName,"Persist",1,IniFile);  
	CheckPath(RomFileName);
	LoadExtRom(RomFileName);
	if (PersistDisks)
		for (Index=0;Index<4;Index++)
		{
			sprintf(Temp,"Disk#%i",Index);
			GetPrivateProfileString(ModName,Temp,"",DiskName,MAX_PATH,IniFile);
			if (strlen(DiskName))
			{
				RetVal=mount_disk_image(DiskName,Index);
				if (RetVal)
				{
					if ( (!strcmp(DiskName,"*Floppy A:")) )	//RealDisks
						PhysicalDriveA=Index+1;
					if ( (!strcmp(DiskName,"*Floppy B:")) )
						PhysicalDriveB=Index+1;
				}
			}
		}
	BuildDynaMenu();
	return;
}

void SaveConfig(void)
{
	unsigned char Index=0;
	char ModName[MAX_LOADSTRING]="";
	char Temp[16]="";
	LoadString(g_hinstDLL,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
	ValidatePath(RomFileName);
	WritePrivateProfileInt(ModName,"DiskRom",SelectRom ,IniFile);
	WritePrivateProfileString(ModName,"RomPath",RomFileName,IniFile);
	WritePrivateProfileInt(ModName,"Persist",PersistDisks ,IniFile);
	if (PersistDisks)
		for (Index=0;Index<4;Index++)
		{	
			sprintf(Temp,"Disk#%i",Index);
			WritePrivateProfileString(ModName,Temp,Drive[Index].ImageName,IniFile);
		}
	return;
}

void Load_Disk(unsigned char disk)
{
	HANDLE hr=NULL;
	OPENFILENAME ofn ;	
	char Dummy[MAX_PATH]="";
	unsigned char FileNotSelected=0;
	if (DialogOpen ==1)	//Only allow 1 dialog open 
		return;
	DialogOpen=1;
	FileNotSelected=1;
	while (FileNotSelected)
	{
		CreateFlag=1; 
		memset(&ofn,0,sizeof(ofn));
		ofn.lStructSize       = sizeof (OPENFILENAME) ;
		ofn.hwndOwner         = NULL;
		ofn.Flags             = OFN_HIDEREADONLY;
		ofn.hInstance         = GetModuleHandle(0);
		ofn. lpstrDefExt      ="dsk";
		ofn.lpstrFilter       =	"Disk Images\0*.DSK;*.OS9\0\0";	// filter string "Disks\0*.DSK\0\0";
		ofn.nFilterIndex      = 0 ;								// current filter index
		ofn.lpstrFile         = TempFileName	 ;				// contains full path and filename on return
		ofn.nMaxFile          = MAX_PATH;						// sizeof lpstrFile
		ofn.lpstrFileTitle    = NULL;							// filename and extension only
		ofn.nMaxFileTitle     = MAX_PATH ;						// sizeof lpstrFileTitle
		ofn.lpstrInitialDir   = Dummy;							// initial directory
		ofn.lpstrTitle        = "Insert Disk Image" ;			// title bar string

		if ( GetOpenFileName(&ofn) )
		{
			hr=CreateFile(TempFileName,NULL,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
			if (hr==INVALID_HANDLE_VALUE) 
			{
				NewDiskNumber=disk;
				DialogBox(g_hinstDLL, (LPCTSTR)IDD_NEWDISK, NULL, (DLGPROC)NewDisk);	//CreateFlag =0 on cancel
			}
			else
				CloseHandle(hr);

			if (CreateFlag==1)
			{
				if (mount_disk_image(TempFileName,disk)==0)	
					MessageBox(NULL,"Can't open file","Error",0);
				else
				{
					FileNotSelected=0;
					SaveConfig();
				}
			}
			
		}
		else
			FileNotSelected=0;

		DialogOpen=0;

	}
	return;
}

LRESULT CALLBACK Config(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static unsigned char CurrentDisk=0,temp=0,temp2=0;
	long ChipChoice[3]={IDC_EXTROM,IDC_TRSDOS,IDC_RGB};
	long VirtualDrive[2]={IDC_DISKA,IDC_DISKB};
	char VirtualNames[5][16]={"None","Drive 0","Drive 1","Drive 2","Drive 3"};
	OPENFILENAME ofn ;	

	switch (message)
	{
		case WM_INITDIALOG:
			TempSelectRom=SelectRom;
			if (!RealDisks)
			{
				EnableWindow(GetDlgItem(hDlg, IDC_DISKA), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_DISKB), FALSE);
				PhysicalDriveA=0;
				PhysicalDriveB=0;
			}

			OldPhysicalDriveA=PhysicalDriveA;
			OldPhysicalDriveB=PhysicalDriveB;
			strcpy(TempRomFileName,RomFileName);
			SendDlgItemMessage(hDlg,IDC_KBLEDS,BM_SETCHECK,UseKeyboardLeds(QUERY),0);
			SendDlgItemMessage(hDlg,IDC_TURBO,BM_SETCHECK,SetTurboDisk(QUERY),0);
			SendDlgItemMessage(hDlg,IDC_PERSIST,BM_SETCHECK,PersistDisks,0);
			for (temp=0;temp<=3;temp++)
				SendDlgItemMessage(hDlg,ChipChoice[temp],BM_SETCHECK,(temp==TempSelectRom),0);
			for (temp=0;temp<2;temp++)
				for (temp2=0;temp2<5;temp2++)
						SendDlgItemMessage (hDlg,VirtualDrive[temp], CB_ADDSTRING, NULL,(LPARAM) VirtualNames[temp2]);

//			GetDlgItem(hDlg,IDC_DISKA)->EnableWindow(FALSE); 
//			SendDlgItemMessage (hDlg, IDC_DISKA,WM_ENABLE  ,(WPARAM)0,(LPARAM)0);
			SendDlgItemMessage (hDlg, IDC_DISKA,CB_SETCURSEL,(WPARAM)PhysicalDriveA,(LPARAM)0);
			SendDlgItemMessage (hDlg, IDC_DISKB,CB_SETCURSEL,(WPARAM)PhysicalDriveB,(LPARAM)0);
			SendDlgItemMessage (hDlg,IDC_ROMPATH,WM_SETTEXT,strlen(TempRomFileName),(LPARAM)(LPCSTR)TempRomFileName);
			return TRUE; 
		break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
					UseKeyboardLeds((unsigned char)SendDlgItemMessage(hDlg,IDC_KBLEDS,BM_GETCHECK,0,0));
					SetTurboDisk((unsigned char)SendDlgItemMessage(hDlg,IDC_TURBO,BM_GETCHECK,0,0));
					PersistDisks=(unsigned char) SendDlgItemMessage(hDlg,IDC_PERSIST,BM_GETCHECK,0,0);
					PhysicalDriveA=(unsigned char)SendDlgItemMessage(hDlg,IDC_DISKA,CB_GETCURSEL,0,0);
					PhysicalDriveB=(unsigned char)SendDlgItemMessage(hDlg,IDC_DISKB,CB_GETCURSEL,0,0);
					if (!RealDisks)
					{
						PhysicalDriveA=0;
						PhysicalDriveB=0;
					//	MessageBox(0,"Wrong Version or Driver not installed","FDRAWCMD Driver",0);
					}
					else
					{
						if (PhysicalDriveA != OldPhysicalDriveA)	//Drive changed
						{					
							if (OldPhysicalDriveA!=0)
								unmount_disk_image(OldPhysicalDriveA-1);
							if (PhysicalDriveA!=0)
								mount_disk_image("*Floppy A:",PhysicalDriveA-1);
						}
						if (PhysicalDriveB != OldPhysicalDriveB)	//Drive changed
						{					
							if (OldPhysicalDriveB!=0)
								unmount_disk_image(OldPhysicalDriveB-1);
							if (PhysicalDriveB!=0)
								mount_disk_image("*Floppy B:",PhysicalDriveB-1);
						}	
					}
					EndDialog(hDlg, LOWORD(wParam));
					SelectRom=TempSelectRom;
					strcpy(RomFileName,TempRomFileName );
					CheckPath(RomFileName);
					LoadExtRom(RomFileName);
					SaveConfig();
					return TRUE;
				break;

				case IDC_EXTROM:
				case IDC_TRSDOS:
				case IDC_RGB:
					for (temp=0;temp<=3;temp++)
						if (LOWORD(wParam)==ChipChoice[temp])
						{
							for (temp2=0;temp2<=3;temp2++)
								SendDlgItemMessage(hDlg,ChipChoice[temp2],BM_SETCHECK,0,0);
							SendDlgItemMessage(hDlg,ChipChoice[temp],BM_SETCHECK,1,0);
							TempSelectRom=temp;
						}
				break;

				case IDC_BROWSE:
					memset(&ofn,0,sizeof(ofn));
					ofn.lStructSize       = sizeof (OPENFILENAME) ;
					ofn.hwndOwner         = NULL;
					ofn.lpstrFilter       =	"Disk Rom Images\0*.rom\0\0";	// filter ROM images
					ofn.nFilterIndex      = 1 ;								// current filter index
					ofn.lpstrFile         = TempRomFileName ;						// contains full path and filename on return
					ofn.nMaxFile          = MAX_PATH;						// sizeof lpstrFile
					ofn.lpstrFileTitle    = NULL;							// filename and extension only
					ofn.nMaxFileTitle     = MAX_PATH ;						// sizeof lpstrFileTitle
					ofn.lpstrInitialDir   = NULL;							// initial directory
					ofn.lpstrTitle        = TEXT("Disk Rom Image") ;	// title bar string
					ofn.Flags             = OFN_HIDEREADONLY;
					GetOpenFileName (&ofn);
						SendDlgItemMessage(hDlg,IDC_ROMPATH,WM_SETTEXT,strlen(TempRomFileName),(LPARAM)(LPCSTR)TempRomFileName);
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

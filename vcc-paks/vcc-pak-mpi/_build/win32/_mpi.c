/*
 *  _mpi.c
 *  VCC-X
 *
 *  Created by Wes Gale on 11-02-14.
 *  Copyright 2011 Magellan Interactive Inc. All rights reserved.
 *
 */

/*********************************************************************/
/*********************************************************************/

#include <windows.h>
#include <commctrl.h>

#include "resource.h" 

/*********************************************************************/
/*
 Dynamic library interface
*/

static HINSTANCE g_hinstDLL	=	NULL;

LRESULT CALLBACK Config(HWND,UINT,WPARAM,LPARAM);

BOOL WINAPI DllMain(
					HINSTANCE hinstDLL,  // handle to DLL module
					DWORD fdwReason,     // reason for calling function
					LPVOID lpReserved )  // reserved
{
	if ( fdwReason == DLL_PROCESS_DETACH ) //Clean Up 
	{
		WriteConfig();
		
		for(Temp=0;Temp<4;Temp++)
		{
			UnloadModule(Temp);
		}
		
		return(1);
	}
	
	g_hinstDLL = hinstDLL;
	return(1);
}

LRESULT CALLBACK Config(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	unsigned short EDITBOXS[4]={IDC_EDIT1,IDC_EDIT2,IDC_EDIT3,IDC_EDIT4};
	unsigned short INSERTBTN[4]={ID_INSERT1,ID_INSERT2,ID_INSERT3,ID_INSERT4};
	unsigned short REMOVEBTN[4]={ID_REMOVE1,ID_REMOVE2,ID_REMOVE3,ID_REMOVE4};
	unsigned short CONFIGBTN[4]={ID_CONFIG1,ID_CONFIG2,ID_CONFIG3,ID_CONFIG4};
	char ConfigText[1024]="";
	
	unsigned char Temp=0;
	switch (message)
	{
		case WM_INITDIALOG:
			for (Temp=0;Temp<4;Temp++)
				SendDlgItemMessage(hDlg,EDITBOXS[Temp],WM_SETTEXT,5,(LPARAM)(LPCSTR) SlotLabel[Temp] );
			SendDlgItemMessage(hDlg,IDC_PAKSELECT,TBM_SETRANGE,TRUE,MAKELONG(0,3) );
			SendDlgItemMessage(hDlg,IDC_PAKSELECT,TBM_SETPOS,TRUE,SwitchSlot);
			ReadModuleParms(SwitchSlot,ConfigText);
			SendDlgItemMessage(hDlg,IDC_MODINFO,WM_SETTEXT,strlen(ConfigText),(LPARAM)(LPCSTR)ConfigText );
			
			return TRUE; 
			break;
			
		case WM_COMMAND:
			switch (LOWORD(wParam))
		{
			case IDOK:
				WriteConfig();
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
				break;
				
		} //End switch LOWORD
			
			for (Temp=0;Temp<4;Temp++)
			{
				if ( LOWORD(wParam) == INSERTBTN[Temp] )
				{
					LoadCartDLL(Temp,ModulePaths[Temp]);
					for (Temp=0;Temp<4;Temp++)
						SendDlgItemMessage(hDlg,EDITBOXS[Temp],WM_SETTEXT,strlen(SlotLabel[Temp]),(LPARAM)(LPCSTR)SlotLabel[Temp] );
				}
			}
			
			for (Temp=0;Temp<4;Temp++)
			{
				if ( LOWORD(wParam) == REMOVEBTN[Temp] )
				{
					UnloadModule(Temp);	
					SendDlgItemMessage(hDlg,EDITBOXS[Temp],WM_SETTEXT,strlen(SlotLabel[Temp]),(LPARAM)(LPCSTR)SlotLabel[Temp] );
				}
			}
			
			for (Temp=0;Temp<4;Temp++)
			{
				if ( LOWORD(wParam) == CONFIGBTN[Temp] )
				{
					if (ConfigModuleCalls[Temp] != NULL)
						ConfigModuleCalls[Temp](NULL);
				}
			}
			return TRUE;
			break;
			
		case WM_HSCROLL:
			SwitchSlot=(unsigned char) SendDlgItemMessage(hDlg,IDC_PAKSELECT,TBM_GETPOS,(WPARAM) 0, (WPARAM) 0);
			SpareSelectSlot= SwitchSlot;
			ChipSelectSlot= SwitchSlot;
			ReadModuleParms(SwitchSlot,ConfigText);
			SendDlgItemMessage(hDlg,IDC_MODINFO,WM_SETTEXT,strlen(ConfigText),(LPARAM)(LPCSTR)ConfigText );
			PakSetCart(0);
			if (CartForSlot[SpareSelectSlot]==1)
				PakSetCart(1);
			break;
	}
	
    return FALSE;
}




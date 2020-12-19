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

#define NO_WARN_MBCS_MFC_DEPRECATION
#define _WIN32_WINNT 0x05010000 // I want to support XP

#include <afxwin.h>
#include <commctrl.h>	// Windows common controls
#include "defines.h"
#include "ddraw.h"
#include "stdio.h"
#include "DirectDrawInterface.h"
#include "resource.h"
#include "tcc1014graphics.h"
#include "throttle.h"
#include "config.h"
#include <iostream>
#include <string>

//Global Variables for Direct Draw funcions
LPDIRECTDRAW        g_pDD		= NULL;  // The DirectDraw object
LPDIRECTDRAWCLIPPER g_pClipper	= NULL;  // Clipper for primary surface
LPDIRECTDRAWSURFACE g_pDDS		= NULL;  // Primary surface
LPDIRECTDRAWSURFACE g_pDDSBack	= NULL;  // Back surface

static IDirectDrawPalette* ddpal;		//Needed for 8bit Palette mode
static HWND hwndStatusBar=NULL;
static RECT WindowDefaultSize;
static unsigned int StatusBarHeight=0;
static TCHAR szTitle[MAX_LOADSTRING];			// The title bar text
static TCHAR szWindowClass[MAX_LOADSTRING];	// The title bar text
static HINSTANCE g_hInstance;
static int g_nCmdShow;
static 	WNDCLASSEX wcex;
static unsigned char InfoBand=1;
static unsigned char Resizeable=1;
static unsigned char ForceAspect=1;
static char StatusText[255]="";
static unsigned int Color=0;
static POINT RememberWinSize;

//Function Prototypes for this module
extern LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM); //Callback for the main window
//void DisplayFlip(SystemState *);


BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	g_hInstance=hInstance;
	g_nCmdShow=nCmdShow;
	AfxInitRichEdit();
	LoadString(g_hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(g_hInstance, IDS_APP_TITLE, szWindowClass, MAX_LOADSTRING);
	return TRUE;
}

bool CreateDDWindow(SystemState *CWState)
{
	HRESULT hr;
	DDSURFACEDESC ddsd;				// A structure to describe the surfaces we want
	RECT rStatBar;
	RECT rc;
	unsigned char ColorValues[4]={0,85,170,255};
	PALETTEENTRY pal[256];
	memset(&ddsd, 0, sizeof(ddsd));	// Clear all members of the structure to 0
	ddsd.dwSize = sizeof(ddsd);		// The first parameter of the structure must contain the size of the structure
	rc.top=0;
	rc.left=0;
	//rc.right = CWState->WindowSize.x;
	//rc.bottom = CWState->WindowSize.y;

	if (GetRememberSize()) {
		POINT pp = GetIniWindowSize();
		rc.right = pp.x;
		rc.bottom = pp.y;
	}
	else {
		rc.right = CWState->WindowSize.x;
		rc.bottom = CWState->WindowSize.y;
	}

	if (CWState->WindowHandle != NULL) //If its go a value it must be a mode switch
	{
		if (g_pDD != NULL)
			g_pDD->Release();	//Destroy the current Window
		DestroyWindow(CWState->WindowHandle);
		UnregisterClass(wcex.lpszClassName,wcex.hInstance);
	}
	wcex.cbSize = sizeof(WNDCLASSEX);	//And Rebuilt it from scratch
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= g_hInstance;
	wcex.hIcon			= LoadIcon(g_hInstance, (LPCTSTR)IDI_COCO3);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)GetStockObject(BLACK_BRUSH); 
	wcex.lpszMenuName	= (LPCSTR)IDR_MENU;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_COCO3);
	if (CWState->FullScreen)
	{
		wcex.lpszMenuName	=NULL;	//Fullscreen has no Menu Bar and no Mouse pointer
		wcex.hCursor		= LoadCursor(g_hInstance, MAKEINTRESOURCE(IDC_NONE));
	}
	if (!RegisterClassEx(&wcex))
		return FALSE;
	switch (CWState->FullScreen)
	{
	case 0: //Windowed Mode
		
		// Calculates the required size of the window rectangle, based on the desired client-rectangle size
		// The window rectangle can then be passed to the CreateWindow function to create a window whose client area is the desired size.
		::AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, TRUE); 
		
		// We create the Main window 
		CWState->WindowHandle = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 
											 rc.right-rc.left, rc.bottom-rc.top, 
											 NULL, NULL, g_hInstance, NULL);
		if (!CWState->WindowHandle)	// Can't create window
		   return FALSE;
		
		// Create the Status Bar Window at the bottom
		hwndStatusBar = ::CreateStatusWindow(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | CCS_BOTTOM , "Ready", CWState->WindowHandle, 2);
		if (!hwndStatusBar) // Can't create Status bar
		   return FALSE;

		// Retrieves the dimensions of the bounding rectangle of the specified window
		// The dimensions are given in screen coordinates that are relative to the upper-left corner of the screen.
		::GetWindowRect(hwndStatusBar, &rStatBar); // Get the size of the Status bar
		StatusBarHeight = rStatBar.bottom - rStatBar.top; // Calculate its height

		// Get the size of main window and add height of status bar then resize Main
		// re-using rStatBar RECT even though it's the main window
		::GetWindowRect(CWState->WindowHandle, &rStatBar);  
		::MoveWindow(CWState->WindowHandle, rStatBar.left, rStatBar.top, // using MoveWindow to resize 
											rStatBar.right - rStatBar.left, (rStatBar.bottom + StatusBarHeight) - rStatBar.top,
//											rStatBar.right - rStatBar.left, rStatBar.bottom - rStatBar.top,
											1);
		::SendMessage(hwndStatusBar, WM_SIZE, 0, 0); // Redraw Status bar in new position

		::GetWindowRect(CWState->WindowHandle, &WindowDefaultSize);	// And save the Final size of the Window 
		::ShowWindow(CWState->WindowHandle, g_nCmdShow);
		::UpdateWindow(CWState->WindowHandle);

		// Create an instance of a DirectDraw object
		hr = ::DirectDrawCreate(NULL, &g_pDD, NULL);
		if (hr) return FALSE;
		
		// Initialize the DirectDraw object
		hr = g_pDD->SetCooperativeLevel(CWState->WindowHandle, DDSCL_NORMAL);	// Set DDSCL_NORMAL to use windowed mode
		if (hr) return FALSE;
		
		ddsd.dwFlags = DDSD_CAPS ; 
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
		
		// Create our Primary Surface
		hr = g_pDD->CreateSurface(&ddsd, &g_pDDS, NULL);
		if (hr) return FALSE;
		ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS ;
		ddsd.dwWidth  = CWState->WindowSize.x;								// Make our off-screen surface 
		ddsd.dwHeight = CWState->WindowSize.y;
		ddsd.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY;				// Try to create back buffer in video RAM
		hr = g_pDD->CreateSurface(&ddsd, &g_pDDSBack, NULL);	
		if (hr)													// If not enough Video Ram 			
		{
			ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY;			// Try to create back buffer in System RAM
			hr = g_pDD->CreateSurface(&ddsd, &g_pDDSBack, NULL);
			if (hr)	return FALSE;								//Giving Up
			MessageBox(0,"Creating Back Buffer in System Ram!\nThis will be slower","Performance Warning",0);
		}

		hr= g_pDD->GetDisplayMode(&ddsd);
		if (hr) return FALSE;
		hr = g_pDD->CreateClipper(0, &g_pClipper, NULL);		// Create the clipper using the DirectDraw object
		if (hr) return FALSE;
		hr = g_pClipper->SetHWnd(0, CWState->WindowHandle);						// Assign your window's HWND to the clipper
		if (hr) return FALSE;
		hr = g_pDDS->SetClipper(g_pClipper);					// Attach the clipper to the primary surface
		if (hr) return FALSE;
		hr= g_pDDSBack->Lock( NULL, &ddsd, DDLOCK_WAIT, NULL );
		if (hr) return FALSE;
		hr = g_pDDSBack->Unlock( NULL );						// Unlock surface
		if (hr) return FALSE;
		break;


	case 1:	//Full Screen Mode
		ddsd.lPitch=0;
		ddsd.ddpfPixelFormat.dwRGBBitCount=0;
		CWState->WindowHandle = CreateWindow(szWindowClass, NULL, WS_POPUP | WS_VISIBLE,0, 0, CWState->WindowSize.x, CWState->WindowSize.y, NULL, NULL, g_hInstance, NULL);
		if (!CWState->WindowHandle )
		   return FALSE;
		GetWindowRect(CWState->WindowHandle,&WindowDefaultSize);
		ShowWindow(CWState->WindowHandle, g_nCmdShow);
		UpdateWindow(CWState->WindowHandle);
		hr = DirectDrawCreate( NULL, &g_pDD, NULL );		// Initialize DirectDraw
		if (hr) return FALSE;
		hr = g_pDD->SetCooperativeLevel(CWState->WindowHandle, DDSCL_EXCLUSIVE|DDSCL_FULLSCREEN|DDSCL_NOWINDOWCHANGES);
		if (hr) return FALSE;
		hr = g_pDD->SetDisplayMode(CWState->WindowSize.x, CWState->WindowSize.y, 32);	// Set 640x480x32 Bit full-screen mode
		if (hr) return FALSE;
		ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP;
		ddsd.dwBackBufferCount = 1;
		hr = g_pDD->CreateSurface(&ddsd, &g_pDDS, NULL);
		if (hr) return FALSE;
		ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;
		g_pDDS->GetAttachedSurface(&ddsd.ddsCaps, &g_pDDSBack);
		hr= g_pDD->GetDisplayMode(&ddsd);
		if (hr) return FALSE;
//***********************************TEST*****************************************
		for ( unsigned short i=0;i<=63;i++)
		{
			pal[i+128].peBlue= ColorValues[(i & 8 ) >>2 | (i & 1) ];
			pal[i+128].peGreen=ColorValues[(i & 16) >>3 | (i & 2) >>1];
			pal[i+128].peRed=  ColorValues[(i & 32) >>4 | (i & 4) >>2];
			pal[i+128].peFlags = PC_RESERVED | PC_NOCOLLAPSE;
		}
		g_pDD->CreatePalette(DDPCAPS_8BIT | DDPCAPS_ALLOW256,pal,&ddpal,NULL);
		g_pDDS->SetPalette(ddpal); // Set pallete for Primary surface
//**********************************END TEST***************************************
	break;
	}
	return TRUE;
}



/*--------------------------------------------------------------------------*/
// Checks if the memory associated with surfaces is lost and restores if necessary.
void CheckSurfaces()
{
	if (g_pDDS)		// Check the primary surface
		if (g_pDDS->IsLost() == DDERR_SURFACELOST)
			g_pDDS->Restore();

	
	if (g_pDDSBack)		// Check the back buffer
		if (g_pDDSBack->IsLost() == DDERR_SURFACELOST)
			g_pDDSBack->Restore();

//	g_pDDS->SetPalette(ddpal);
}
/*--------------------------------------------------------------------------*/

void DisplayFlip(SystemState *DFState)	// Double buffering flip
{
	using namespace std;
	static HRESULT hr;
	static RECT    rcSrc;  // source blit rectangle
	static RECT    rcDest; // destination blit rectangle
	static RECT	Temp;
	static POINT   p;	

	if (DFState->FullScreen)	// if we're windowed do the blit, else just Flip
		hr = g_pDDS->Flip(NULL,DDFLIP_NOVSYNC |DDFLIP_DONOTWAIT ); //DDFLIP_WAIT
	else
	{
		p.x = 0; p.y = 0;
		
		// The ClientToScreen function converts the client-area coordinates of a specified point to screen coordinates.
		// in other word the client rectangle of the main windows 0, 0 (upper-left corner) 
		// in a screen x,y coords which is put back into p  
		::ClientToScreen(DFState->WindowHandle, &p);  // find out where on the primary surface our window lives

		// get the actual client rectangle, which is always 0,0 - w,h
		::GetClientRect(DFState->WindowHandle, &rcDest);

		// The OffsetRect function moves the specified rectangle by the specified offsets
		// add the delta screen point we got above, which gives us the client rect in screen coordinates.
		::OffsetRect(&rcDest, p.x, p.y);
		
		// our destination rectangle is going to be 
		::SetRect(&rcSrc, 0, 0, DFState->WindowSize.x, DFState->WindowSize.y);
		
		if (Resizeable)
		{
			rcDest.bottom -= StatusBarHeight;

			if (ForceAspect) // Adjust the Aspect Ratio if window is resized
			{
				float srcWidth = (float)DFState->WindowSize.x;
				float srcHeight = (float)DFState->WindowSize.y;
				float srcRatio = srcWidth / srcHeight;

				// change this to use the existing rcDest and the calc, w = right-left & h = bottom-top, 
				//                         because rcDest has already been converted to screen cords, right?   
				static RECT rcClient;
				::GetClientRect(DFState->WindowHandle, &rcClient);  // x,y is always 0,0 so right, bottom is w,h
				rcClient.bottom -= StatusBarHeight;
				
				float clientWidth = (float)rcClient.right;
				float clientHeight = (float)rcClient.bottom;
				float clientRatio = clientWidth / clientHeight;

				float dstWidth = 0, dstHeight = 0;

				if (clientRatio > srcRatio)
				{
					dstWidth = srcWidth * clientHeight / srcHeight;
					dstHeight = clientHeight;
				}
				else
				{
					dstWidth = clientWidth;
					dstHeight = srcHeight * clientWidth / srcWidth;
				}

				float dstX = (clientWidth - dstWidth) / 2; 
				float dstY = (clientHeight - dstHeight) / 2;

				static POINT pDstLeftTop;
				pDstLeftTop.x = (long)dstX; pDstLeftTop.y = (long)dstY;
				::ClientToScreen(DFState->WindowHandle, &pDstLeftTop);

				static POINT pDstRightBottom;
				pDstRightBottom.x = (long)(dstX + dstWidth); pDstRightBottom.y = (long)(dstY + dstHeight);
				::ClientToScreen(DFState->WindowHandle, &pDstRightBottom);

				::SetRect(&rcDest, pDstLeftTop.x, pDstLeftTop.y, pDstRightBottom.x, pDstRightBottom.y);
			}
		}
		else
		{
			// this does not seem ideal, it lets you begin to resize and immediately resizes it back ... causing a lot of flicker.
			rcDest.right = rcDest.left + DFState->WindowSize.x;
			rcDest.bottom = rcDest.top + DFState->WindowSize.y;
			::GetWindowRect(DFState->WindowHandle, &Temp);
			::MoveWindow(DFState->WindowHandle, Temp.left, Temp.top, WindowDefaultSize.right - WindowDefaultSize.left, WindowDefaultSize.bottom-WindowDefaultSize.top, 1);
		}
		
		if (g_pDDSBack == NULL)
			MessageBox(0, "Odd", "Error", 0); // yes, odd error indeed!! (??)
		                                      // especially since we go ahead and use it below!
	
		hr = g_pDDS->Blt(&rcDest, g_pDDSBack, &rcSrc, DDBLT_WAIT , NULL); // DDBLT_WAIT
	}

	static RECT CurScreen;
	::GetClientRect(DFState->WindowHandle, &CurScreen);
	//CurScreen.bottom -= StatusBarHeight;
	int clientWidth = (int)CurScreen.right;
	int clientHeight = (int)CurScreen.bottom;
	RememberWinSize.x = clientWidth; // Used for saving new window size to the ini file.
	RememberWinSize.y = clientHeight-StatusBarHeight;
	return;
}

unsigned char LockScreen(SystemState *LSState)
{

	HRESULT	hr;
	DDSURFACEDESC ddsd;				// A structure to describe the surfaces we want
	memset(&ddsd, 0, sizeof(ddsd));	// Clear all members of the structure to 0
	ddsd.dwSize = sizeof(ddsd);		// The first parameter of the structure must contain the size of the structure
	CheckSurfaces();
	
	// Lock entire surface, wait if it is busy, return surface memory pointer
	hr = g_pDDSBack->Lock( NULL, &ddsd, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL );
	if (hr)
	{
//		MessageBox(0,"Can't Lock surface","Error",0);
		return(1);
	}
	switch (ddsd.ddpfPixelFormat.dwRGBBitCount)
	{
		case 8:
			LSState->SurfacePitch=ddsd.lPitch;
			LSState->BitDepth=0;
		break;
		case 15:
		case 16:
			LSState->SurfacePitch=ddsd.lPitch/2;
			LSState->BitDepth=1;
		break;
		case 24:
			MessageBox(0,"24 Bit color is currnetly unsupported","Ok",0);
			exit(0);
			LSState->SurfacePitch=ddsd.lPitch;
			LSState->BitDepth=2;
		break;
		case 32:
			LSState->SurfacePitch=ddsd.lPitch/4;
			LSState->BitDepth=3;
		break;
		default:
			MessageBox(0,"Unsupported Color Depth!","Error",0);
			return 1;
		break;
	}
	if (ddsd.lpSurface==NULL)
		MessageBox(0,"Returning NULL!!","ok",0);
	LSState->PTRsurface8=(unsigned char *)ddsd.lpSurface;
	LSState->PTRsurface16=(unsigned short *)ddsd.lpSurface;
	LSState->PTRsurface32=(unsigned int *)ddsd.lpSurface;
	return(0);
}

void UnlockScreen(SystemState *USState)
{
	static HRESULT hr;
	static size_t Index=0;
	static HDC hdc;
	if (USState->FullScreen  & InfoBand) //Put StatusText for full screen here
	{
		g_pDDSBack->GetDC(&hdc);
		SetBkColor( hdc, RGB( 0, 0, 0 ) );
		SetTextColor(hdc, RGB(255,255,255));
		Index=strlen(StatusText);
		for (;Index<132;Index++)
			StatusText[Index]=32;
		StatusText[Index]=0;
		TextOut(hdc, 0, 0, StatusText, 132); 
		g_pDDSBack->ReleaseDC(hdc);  
	}
	hr = g_pDDSBack->Unlock( NULL );

	DisplayFlip(USState);
	return;
}

void SetStatusBarText( char *TextBuffer,SystemState *STState)
{
	if (!STState->FullScreen)
	{
		SendMessage(hwndStatusBar,WM_SETTEXT ,strlen(TextBuffer),(LPARAM)(LPCSTR)TextBuffer);
		SendMessage(hwndStatusBar,WM_SIZE,0,0);
	}
	else
		strcpy(StatusText,TextBuffer);
	return;
}

void Cls(unsigned int ClsColor,SystemState *CLState)
{
	CLState->ResetPending=3; //Tell Main loop to hold Emu
	Color=ClsColor;
	return;
}

void DoCls(SystemState *CLStatus)
{
	unsigned short x=0,y=0;

	if(LockScreen(CLStatus))
		return;
	switch (CLStatus->BitDepth)
	{
	case 0:
		for (y=0;y<480;y++)
			for (x=0;x<640;x++)
				CLStatus->PTRsurface8[x + (y*CLStatus->SurfacePitch) ]=Color|128;
			break;
	case 1:
		for (y=0;y<480;y++)
			for (x=0;x<640;x++)
				CLStatus->PTRsurface16[x + (y*CLStatus->SurfacePitch) ]=Color;
			break;
	case 2:
		for (y=0;y<480;y++)
			for (x=0;x<640;x++)
			{
				CLStatus->PTRsurface8[(x*3) + (y*CLStatus->SurfacePitch) ]=(Color & 0xFF0000)>>16;
				CLStatus->PTRsurface8[(x*3)+1 + (y*CLStatus->SurfacePitch) ]=(Color & 0x00FF00)>>8;
				CLStatus->PTRsurface8[(x*3)+2 + (y*CLStatus->SurfacePitch) ]=(Color & 0xFF);
			}
			break;
	case 3:
		for (y=0;y<480;y++)
			for (x=0;x<640;x++)
				CLStatus->PTRsurface32[x + (y*CLStatus->SurfacePitch) ]=Color;
			break;
	default:
		return;
	}

	UnlockScreen(CLStatus);
	return;
}

unsigned char SetInfoBand( unsigned char Tmp)
{
	if (Tmp!=QUERY)
		InfoBand=Tmp;
	return(InfoBand);
}

unsigned char SetResize(unsigned char Tmp)
{
	if (Tmp!=QUERY)
		Resizeable=Tmp;
	return(Resizeable);
}

unsigned char SetAspect (unsigned char Tmp)
{
	if (Tmp!=QUERY)
		ForceAspect=Tmp;
	return(ForceAspect);
}


float Static(SystemState *STState)
{
	unsigned short x=0;
	static unsigned short y=0;
//	unsigned char Depth=0;
	unsigned char Temp=0;
	static unsigned short TextX=0,TextY=0;
	static unsigned char Counter,Counter1=32;
	static char Phase=1;
	static char Message[]=" Signal Missing! Press F9";
	static unsigned char GreyScales[4]={128,135,184,191};
	HDC hdc;
	LockScreen(STState);
	if (STState->PTRsurface32==NULL)
		return(0);
//	y=(y+1) % 480;
	switch (STState->BitDepth)
	{
	case 0:
		
		for (y=0;y<480;y+=2)
			for (x=0;x<160;x++)
			{
				Temp=rand() & 3;
				STState->PTRsurface32[x + (y*STState->SurfacePitch>>2) ]= GreyScales[Temp]|(GreyScales[Temp]<<8)|(GreyScales[Temp]<<16) | (GreyScales[Temp]<<24);
				STState->PTRsurface32[x + ((y+1)*STState->SurfacePitch>>2) ]= GreyScales[Temp]|(GreyScales[Temp]<<8)|(GreyScales[Temp]<<16) | (GreyScales[Temp]<<24);
			}
			break;
	case 1:
		for (y=0;y<480;y+=2)
			for (x=0;x<320;x++)
			{
				Temp=rand() & 31;
				STState->PTRsurface32[x + (y*STState->SurfacePitch>>1) ]=Temp | (Temp <<6) | (Temp<<11) | (Temp<<16) |(Temp<<22)| (Temp<<27);
				STState->PTRsurface32[x + ((y+1)*STState->SurfacePitch>>1) ]=Temp | (Temp <<6) | (Temp<<11) | (Temp<<16) |(Temp<<22)| (Temp<<27);
			}
			break;
	case 2:
		for (y=0;y<480;y++)
			for (x=0;x<640;x++)
			{
				STState->PTRsurface8[(x*3) + (y*STState->SurfacePitch) ]=Temp;
				STState->PTRsurface8[(x*3)+1 + (y*STState->SurfacePitch) ]=Temp<<8;
				STState->PTRsurface8[(x*3)+2 + (y*STState->SurfacePitch) ]=Temp <<16;
			}
			break;
	case 3:
		for (y=0;y<480;y++)
			for (x=0;x<640;x++)
			{
				Temp=rand() & 255;
				STState->PTRsurface32[x + (y*STState->SurfacePitch) ]=Temp | (Temp<<8) | (Temp <<16);
			}
			break;
	default:
		return(0);
	}
	g_pDDSBack->GetDC(&hdc);
	SetBkColor( hdc, 0);
	SetTextColor(hdc, RGB(Counter1<<2,Counter1<<2,Counter1<<2));
	TextOut(hdc, TextX, TextY, Message, strlen(Message)); 
	Counter++;
	Counter1+=Phase;
	if ((Counter1==60) | (Counter1==20))
		Phase=-Phase;
	Counter%=60; //about 1 seconds
	if (!Counter)
	{
		TextX=rand()%580;
		TextY=rand()%470;
	}
	g_pDDSBack->ReleaseDC(hdc);  
	UnlockScreen(STState);
	return(CalculateFPS());
}
POINT GetCurWindowSize() {
	return (RememberWinSize);
}
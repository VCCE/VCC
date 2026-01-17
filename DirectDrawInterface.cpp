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

#ifdef _LEGACY_VCC
#pragma warning( disable : 4800 )
#endif

#define NO_WARN_MBCS_MFC_DEPRECATION
#define _WIN32_WINNT 0x05010000 // I want to support XP

#include "BuildConfig.h"
#include <vcc/util/limits.h>
#include <Windows.h>
#include <CommCtrl.h>	// Windows common controls
#include "defines.h"
#include "stdio.h"
#include "DirectDrawInterface.h"
#include "resource.h"
#include "tcc1014graphics.h"
#include "throttle.h"
#include "config.h"
#include <iostream>
#include <string>
#include "IDisplay.h"
#include "Screenshot.h"

#include "OpenGL.h"
#include "OpenGLFontConsola.h"
#include "DirectX.h"
#include "DisplayNull.h"
#include "DisplayNone.h"

#if USE_DEBUG_AUDIOTAPE
#include "IDisplayDebug.h"
#endif // USE_DEBUG_AUDIOTAPE

using namespace VCC; // after all includes

static IDisplay* g_Display = nullptr;
static ISystemState* g_SystemState = nullptr;

static IDisplayDirectX* g_DirectXDisplay = nullptr;
static IDisplayOpenGL* g_OpenGLDisplay = nullptr;
static const OpenGLFont* g_DisplayFont = nullptr;

#if USE_OPENGL
using ClassOpenGL = OpenGL;
#else
using ClassOpenGL = DisplayNull;
#endif

#if USE_DIRECTX
using ClassDirectX = DirectX;
#else
using ClassDirectX = DisplayNull;
#endif

static HWND hwndStatusBar=nullptr;
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
static Rect RememberWin = { CW_USEDEFAULT, CW_USEDEFAULT, DefaultWidth, DefaultHeight };
static bool UseOpenGL = true;

//Function Prototypes for this module
extern LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM); //Callback for the main window

// load font on demand, otherwise false if not supported
bool LoadOpenGLFont()
{
#if USE_OPENGL
	return g_OpenGLDisplay && (g_DisplayFont || g_OpenGLDisplay->LoadFont(&g_DisplayFont, IDB_FONT_CONSOLA, OpenGLFontConsola, 32, 127) == OpenGL::OK);
#else // !USE_OPENGL
	return false;
#endif // !USE_OPENGL
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	g_hInstance=hInstance;
	g_nCmdShow=nCmdShow;
	LoadLibrary("RICHED20.DLL");
	LoadString(g_hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(g_hInstance, IDS_APP_TITLE, szWindowClass, MAX_LOADSTRING);
	return TRUE;
}

template<typename T, typename Interface>
bool InitGraphics(Interface** ptr, SystemState* CWState, int w, int h)
{
	if (!g_SystemState) g_SystemState = new SystemStatePtr(CWState);
	if (!*ptr) *ptr = new T(g_SystemState);
	if (!g_Display) g_Display = *ptr;
	if (g_Display && g_Display->Setup(CWState->WindowHandle, w, h, StatusBarHeight, CWState->FullScreen == 1) == IDisplay::OK)
		return true;
	g_Display = nullptr;
	*ptr = nullptr;
	return false;
};

bool CreateNullWindow(SystemState* CWState)
{
	IDisplayOpenGL* display = nullptr;
	return InitGraphics<DisplayNone>(&display, CWState, 640, 480);
}

bool CreateDDWindow(SystemState *CWState)
{
	RECT rStatBar;
	RECT rc = { 0, 0, DefaultWidth, DefaultHeight };

	auto isRememberSize = GetRememberSize();

	if (isRememberSize)
	{
		auto& windowRect = GetIniWindowRect();
		rc = windowRect.GetRect();

		// if x or y undefined keep size but use default position
		if (windowRect.IsDefaultX() || windowRect.IsDefaultY())
			isRememberSize = false;
	}

	int xPos = rc.left;
	int yPos = rc.top;

	// if not remembering window, use default positioning
	if (!isRememberSize)
	{
		xPos = CW_USEDEFAULT;
		yPos = 0;
	}

	HMONITOR hmon;

	if (CWState->WindowHandle != nullptr) //If its go a value it must be a mode switch
	{
		hmon = MonitorFromWindow(CWState->WindowHandle,	MONITOR_DEFAULTTONEAREST);
		CloseScreen();
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
	wcex.hCursor		= LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)GetStockObject(BLACK_BRUSH); 
	wcex.lpszMenuName	= (LPCSTR)IDR_MENU;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_COCO3);
	if (CWState->FullScreen)
	{
		wcex.lpszMenuName	=nullptr;	//Fullscreen has no Menu Bar and no Mouse pointer
		wcex.hCursor		= LoadCursor(g_hInstance, MAKEINTRESOURCE(IDC_NONE));
	}
	if (!RegisterClassEx(&wcex))
		return false;
	if (CWState->FullScreen == 0)
	{
		//Windowed Mode
		
		// Calculates the required size of the window rectangle, based on the desired client-rectangle size
		// The window rectangle can then be passed to the CreateWindow function to create a window whose client area is the desired size.
		::AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, TRUE); 

		// We create the Main window 
		CWState->WindowHandle = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, xPos, yPos,
											 rc.right - rc.left, rc.bottom - rc.top,
											 nullptr, nullptr, g_hInstance, nullptr);
		if (!CWState->WindowHandle)	// Can't create window
		   return false;
		
		// Create the Status Bar Window at the bottom
		hwndStatusBar = ::CreateStatusWindow(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | CCS_BOTTOM , "Ready", CWState->WindowHandle, 2);
		if (!hwndStatusBar) // Can't create Status bar
		   return false;

		// Retrieves the dimensions of the bounding rectangle of the specified window
		// The dimensions are given in screen coordinates that are relative to the upper-left corner of the screen.
		::GetWindowRect(hwndStatusBar, &rStatBar); // Get the size of the Status bar
		StatusBarHeight = rStatBar.bottom - rStatBar.top; // Calculate its height

		// Get the size of main window and add height of status bar then resize Main
		// re-using rStatBar RECT even though it's the main window
		::GetWindowRect(CWState->WindowHandle, &rStatBar);  
		::MoveWindow(CWState->WindowHandle, rStatBar.left, rStatBar.top, // using MoveWindow to resize 
											rStatBar.right - rStatBar.left, (rStatBar.bottom + StatusBarHeight) - rStatBar.top,
											1);
		::SendMessage(hwndStatusBar, WM_SIZE, 0, 0); // Redraw Status bar in new position
	}
	else	//Full Screen Mode
	{
		CaptureCurrentWindowRect();

		MONITORINFO mi = { sizeof(mi) };
		if (!GetMonitorInfo(hmon, &mi))
			return false;

		CWState->WindowHandle = CreateWindow(
			szWindowClass,
			nullptr,
			WS_POPUP | WS_VISIBLE,
			mi.rcMonitor.left,
			mi.rcMonitor.top,
			mi.rcMonitor.right - mi.rcMonitor.left,
			mi.rcMonitor.bottom - mi.rcMonitor.top,
											 nullptr, nullptr, g_hInstance, 0);
	}

	ShowWindow(CWState->WindowHandle, g_nCmdShow);
	UpdateWindow(CWState->WindowHandle);

	HWND windowHandle = CWState->WindowHandle;
	if (windowHandle)
	{
		::GetClientRect(windowHandle, &rStatBar);
		int w = rStatBar.right - rStatBar.left;
		int h = rStatBar.bottom - rStatBar.top - StatusBarHeight;

		// attempt to use opengl
		if (UseOpenGL && !InitGraphics<ClassOpenGL>(&g_OpenGLDisplay, CWState, w, h))
			UseOpenGL = false;

		// otherwise attempt to use directx
		if (!UseOpenGL && !InitGraphics<ClassDirectX>(&g_DirectXDisplay, CWState, w, h))
			return false;

		SetAspect(ForceAspect);
	}

	return true;
}

/*--------------------------------------------------------------------------*/


bool IsMaximized(SystemState* DFState)
{
	WINDOWPLACEMENT wp;
	memset(&wp, 0, sizeof(wp));
	wp.length = sizeof(wp);
	GetWindowPlacement(DFState->WindowHandle, &wp);
	return wp.showCmd == SW_MAXIMIZE;
}


void DisplayFlip(SystemState *DFState)	// Double buffering flip
{
	if (g_Display)
	{
		g_Display->Render();
#if USE_OPENGL
		if (DFState->FullScreen && InfoBand)
		{
			if (LoadOpenGLFont())
			{
				OpenGL::Rect renderRect;
				g_OpenGLDisplay->GetRect(OpenGL::OPT_RECT_RENDER, &renderRect);
				g_OpenGLDisplay->RenderBox(0, 0, renderRect.w, 25, VCC::ColorBlack, true);
				g_OpenGLDisplay->RenderText(g_DisplayFont, 10, 18, 20, StatusText);
			}
		}

#if USE_DEBUG_AUDIOTAPE
		// put these here for now as it needs to be on top
		DebugPrint(10, 250, 20, "Audio Out");
		DebugPrint(10, 400, 20, "Audio In");
		DebugPrint(10, 515, 20, "Motor");
		DebugPrint(10, 545, 20, "MUX");
#endif // USE_DEBUG_AUDIOTAPE
#endif // USE_OPENGL

		g_Display->Present();
	}

	static RECT CurWindow;
	::GetWindowRect(DFState->WindowHandle, &CurWindow);
	static RECT CurScreen;
	::GetClientRect(DFState->WindowHandle, &CurScreen);
	int clientWidth = (int)CurScreen.right;
	int clientHeight = (int)CurScreen.bottom;

	if (!IsMaximized(DFState) && !DFState->FullScreen && !DFState->Exiting)
	{
#if USE_DEFAULT_POSITIONING
		// preserve default positioning (like older versions):
		RememberWin.x = RememberWin.IsDefaultX() ? CW_USEDEFAULT : CurWindow.left;
		RememberWin.y = RememberWin.IsDefaultY() ? CW_USEDEFAULT : CurWindow.top;
#else // !USE_DEFAULT_POSITIONING
		// remember positioning:
		RememberWin.x = CurWindow.left;
		RememberWin.y = CurWindow.top;
#endif // !USE_DEFAULT_POSITIONING
	
		// remember size:
		RememberWin.w = clientWidth; // Used for saving new window size to the ini file.
		RememberWin.h = clientHeight - StatusBarHeight;
	}
	return;
}

unsigned char LockScreen()
{
	if (!g_Display) 
		return 0;

	if (g_Display->LockSurface() != IDisplay::OK)
		return 1;

	return 0;
}

void UnlockScreen(SystemState *USState)
{
	if (g_DirectXDisplay && InfoBand)
		g_DirectXDisplay->RenderStatusLine(StatusText);

	if (g_Display->UnlockSurface() != IDisplay::OK)
		return;

	DisplayFlip(USState);
	return;
}

void SetStatusBarText(const char *TextBuffer,const SystemState *STState)
{
	if (!STState->FullScreen)
	{
		SendMessage(hwndStatusBar,WM_SETTEXT,0, reinterpret_cast<LPARAM>(TextBuffer));
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

	if(LockScreen())
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
	return InfoBand;
}

unsigned char SetResize(unsigned char Tmp)
{
	if (Tmp != QUERY)
	{
		Resizeable = Tmp;
		if (g_Display)
			g_Display->SetOption(IDisplay::OPT_FLAG_RESIZEABLE, Resizeable);
	}
	return Resizeable;
}

unsigned char SetAspect (unsigned char Tmp)
{
	if (Tmp != QUERY)
	{
		ForceAspect = Tmp;
		if (g_Display)
			g_Display->SetOption(IDisplay::OPT_FLAG_ASPECT, ForceAspect);
	}
	return ForceAspect;
}

void DisplaySignalLostMessage()
{
#if USE_OPENGL
	if (LoadOpenGLFont())
	{
		const float fontSize = 20;
		const char* message = " Signal Lost! Press F9 ";
		const float messageWidth = strlen(message) * fontSize * g_DisplayFont->glyphs->advance;
		const float baseLine = fontSize - 4;

		OpenGL::Rect renderRect;
		g_OpenGLDisplay->GetRect(OpenGL::OPT_RECT_RENDER, &renderRect);
		float x = (renderRect.w - messageWidth) / 2;
		float y = (renderRect.h - fontSize) / 2;
		g_OpenGLDisplay->RenderBox(x, y, messageWidth, fontSize, VCC::ColorBlack, true);
		g_OpenGLDisplay->RenderText(g_DisplayFont, x, y + baseLine, fontSize, message);
	}
#endif // USE_OPENGL
}

float Static(SystemState *STState)
{
	unsigned short x=0;
	static unsigned short y=0;
	unsigned char Temp=0;
	static unsigned char GreyScales[4] = { 128,135,184,191 };

	if (g_Display == nullptr)
		return 0;

	if (g_Display->LockSurface() != IDisplay::OK)
		return 0;

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
		return 0;
	}

	// need to do during lock
	if (g_DirectXDisplay)
		g_DirectXDisplay->RenderSignalLostMessage();

	if (g_Display->UnlockSurface() != IDisplay::OK)
		return 0;

	g_Display->Render();
	DisplaySignalLostMessage();
	g_Display->Present();

	return CalculateFPS();
}

const Rect& GetCurWindowSize() 
{
	return RememberWin;
}

int GetRenderWindowStatusBarHeight()
{
	return StatusBarHeight;
}


POINT GetForcedAspectBorderPadding()
{
	if (g_Display)
	{
		IDisplay::Rect area;
		g_Display->GetRect(IDisplay::OPT_RECT_DISPLAY, &area);
		return { (long)area.x, (long)area.y };
	}
	return { 0,0 };
}

void CloseScreen()
{
	if (g_Display)
	{
		g_Display->Cleanup();
		delete g_Display;
		delete g_SystemState;
		g_Display = nullptr;
		g_SystemState = nullptr;

		// alias
		g_DirectXDisplay = nullptr;
		g_OpenGLDisplay = nullptr;
		g_DisplayFont = nullptr; 
	}
}

void DebugDrawLine(float x1, float y1, float x2, float y2, Pixel color)
{
	if (g_Display)
		g_Display->DebugDrawLine(x1, y1, x2, y2, color);
}

void DebugDrawBox(float x, float y, float w, float h, Pixel color)
{
	if (g_Display)
		g_Display->DebugDrawBox(x, y, w, h, color);
}

void DebugPrint(float x, float y, float size, const char* str)
{
	if (g_OpenGLDisplay && LoadOpenGLFont())
		g_OpenGLDisplay->RenderText(g_DisplayFont, x, y, size, str);
}

void DumpScreenshot(const char* fname)
{
	using namespace Screenshot;

	auto result = Snap(g_Display, fname);

	if (result != OK)
	{
		char msg[128];
		sprintf(msg, "Unable to take screenshot! (%d)", result);
		MessageBox(nullptr, msg, "Error", 0);
	}
}

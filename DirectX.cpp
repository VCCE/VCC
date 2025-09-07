//  Copyright 2015 by Joseph Forgion
//  This file is part of VCC (Virtual Color Computer).
//
//  VCC (Virtual Color Computer) is free software: you can redistribute it and/or
//  modify it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  VCC (Virtual Color Computer) is distributed in the hope that it will be
//  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License along
//  with VCC. If not, see <http://www.gnu.org/licenses/>.
//
//  2025/04/10 - Craig Allsop - Add OpenGL rendering.
//

#include "DirectX.h"

#if USE_DIRECTX

#include "DirectDrawInterface.h"
#include "ddraw.h"

namespace VCC
{
    //
    // Private wrapper on result code for logging errors
    //
    static int Result(int code)
    {
        if (code != IDisplay::OK)
        {
            char message[256];
            snprintf(message, 64, "DirectX error %d\nCheck DirectX support", code);
            MessageBox(nullptr, message, "Error", 0);
            //PrintLogC("OpenGL Error: %d\n", code);
        }
        return code;
    }

    namespace Detail
    {
        //Global Variables for Direct Draw funcions
        LPDIRECTDRAW        g_pDD;       // The DirectDraw object
        LPDIRECTDRAWCLIPPER g_pClipper;  // Clipper for primary surface
        LPDIRECTDRAWSURFACE g_pDDS;      // Primary surface
        LPDIRECTDRAWSURFACE g_pDDSBack;  // Back surface
        IDirectDrawPalette* ddpal;       // Needed for 8bit Palette mode
        bool isInitialized;
        bool isFullscreen;
        bool forceAspect;
        bool resizeable;
        int statusBarHeight;
        RECT WindowDefaultSize;
        POINT ForcedAspectBorderPadding;

        // surface while locked
        void* surface;
        int bitDepth;
        long surfacePitch;
    }

    DirectX::DirectX(ISystemState* state) : state(state)
    {
        using namespace Detail;
        isInitialized = false;
        isFullscreen = false;
        forceAspect = false;
        statusBarHeight = 0;
        resizeable = true;
        WindowDefaultSize = {};
        ForcedAspectBorderPadding = {};
        g_pDDSBack = nullptr;
        g_pDDS = nullptr;
        g_pClipper = nullptr;
        g_pDD = nullptr;
        ddpal = nullptr;
        surface = nullptr;
        bitDepth = 0;
        surfacePitch = 0;
    }

    int DirectX::Setup(void* hwnd, int width, int height, int statusHeight, bool fullscreen)
    {
        using namespace Detail;

        isFullscreen = fullscreen;
        statusBarHeight = statusHeight;

        GetWindowRect((HWND)hwnd, &WindowDefaultSize);

        HRESULT hr;
        PALETTEENTRY pal[256];
        DDSURFACEDESC ddsd;				// A structure to describe the surfaces we want
        unsigned char ColorValues[4] = { 0,85,170,255 };

        memset(&ddsd, 0, sizeof(ddsd));	// Clear all members of the structure to 0
        ddsd.dwSize = sizeof(ddsd);		// The first parameter of the structure must contain the size of the structure

        // Create an instance of a DirectDraw object
        hr = ::DirectDrawCreate(nullptr, &g_pDD, nullptr);
        if (hr) return Result(ERR_UNKNOWN);

        VCC::Rect windowRect = { 0, 0, 640, 480 };

        if (fullscreen)
        {
            ddsd.lPitch = 0;
            ddsd.ddpfPixelFormat.dwRGBBitCount = 0;
            hr = DirectDrawCreate(nullptr, &g_pDD, nullptr);		// Initialize DirectDraw
            if (hr) return Result(ERR_UNKNOWN);
            hr = g_pDD->SetCooperativeLevel((HWND)hwnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_NOWINDOWCHANGES);
            if (hr) return Result(ERR_UNKNOWN);
            hr = g_pDD->SetDisplayMode(windowRect.w, windowRect.h, 32);	// Set 640x480x32 Bit full-screen mode
            if (hr) return Result(ERR_UNKNOWN);
            ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
            ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP;
            ddsd.dwBackBufferCount = 1;
            hr = g_pDD->CreateSurface(&ddsd, &g_pDDS, nullptr);
            if (hr) return Result(ERR_UNKNOWN);
            ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;
            g_pDDS->GetAttachedSurface(&ddsd.ddsCaps, &g_pDDSBack);
            hr = g_pDD->GetDisplayMode(&ddsd);
            if (hr) return Result(ERR_UNKNOWN);
            //***********************************TEST*****************************************
            for (unsigned short i = 0;i <= 63;i++)
            {
                pal[i + 128].peBlue = ColorValues[(i & 8) >> 2 | (i & 1)];
                pal[i + 128].peGreen = ColorValues[(i & 16) >> 3 | (i & 2) >> 1];
                pal[i + 128].peRed = ColorValues[(i & 32) >> 4 | (i & 4) >> 2];
                pal[i + 128].peFlags = PC_RESERVED | PC_NOCOLLAPSE;
            }
            g_pDD->CreatePalette(DDPCAPS_8BIT | DDPCAPS_ALLOW256, pal, &ddpal, nullptr);
            g_pDDS->SetPalette(ddpal); // Set pallete for Primary surface
            //**********************************END TEST***************************************
        }
        else
        {
            // Initialize the DirectDraw object
            hr = g_pDD->SetCooperativeLevel((HWND)hwnd, DDSCL_NORMAL);	// Set DDSCL_NORMAL to use windowed mode
            if (hr) return Result(ERR_UNKNOWN);

            ddsd.dwFlags = DDSD_CAPS;
            ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

            // Create our Primary Surface
            hr = g_pDD->CreateSurface(&ddsd, &g_pDDS, nullptr);
            if (hr) return Result(ERR_UNKNOWN);
            ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
            ddsd.dwWidth = windowRect.w;								// Make our off-screen surface 
            ddsd.dwHeight = windowRect.h;
            ddsd.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY;				// Try to create back buffer in video RAM
            hr = g_pDD->CreateSurface(&ddsd, &g_pDDSBack, nullptr);
            if (hr)													// If not enough Video Ram 			
            {
                ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY;			// Try to create back buffer in System RAM
                hr = g_pDD->CreateSurface(&ddsd, &g_pDDSBack, nullptr);
                if (hr)	return Result(ERR_UNKNOWN);								//Giving Up
                MessageBox(nullptr, "Creating Back Buffer in System Ram\n", "Performance Warning", 0);
            }

            hr = g_pDD->GetDisplayMode(&ddsd);
            if (hr) return Result(ERR_UNKNOWN);
            hr = g_pDD->CreateClipper(0, &g_pClipper, nullptr);		// Create the clipper using the DirectDraw object
            if (hr) return Result(ERR_UNKNOWN);
            hr = g_pClipper->SetHWnd(0, (HWND)hwnd);				// Assign your window's HWND to the clipper
            if (hr) return Result(ERR_UNKNOWN);
            hr = g_pDDS->SetClipper(g_pClipper);					// Attach the clipper to the primary surface
            if (hr) return Result(ERR_UNKNOWN);
            hr = g_pDDSBack->Lock(nullptr, &ddsd, DDLOCK_WAIT, nullptr);
            if (hr) return Result(ERR_UNKNOWN);
            hr = g_pDDSBack->Unlock(nullptr);						// Unlock surface
            if (hr) return Result(ERR_UNKNOWN);
        }

        return Result(OK);
    }

    int DirectX::Render()
    {
        using namespace std;
        using namespace Detail;

        static HRESULT hr;
        static RECT    rcSrc;  // source blit rectangle
        static RECT    rcDest; // destination blit rectangle
        static RECT	Temp;
        static POINT   p;

        HWND hwnd;
        if (state->GetWindowHandle((void**)&hwnd) != OK)
            return Result(ERR_UNKNOWN);

        VCC::Rect windowRect;
        if (state->GetRect(1, &windowRect) != OK)
            return Result(ERR_UNKNOWN);

        ForcedAspectBorderPadding = { 0,0 };

        if (isFullscreen)	// if we're windowed do the blit, else just Flip
            hr = g_pDDS->Flip(nullptr, DDFLIP_NOVSYNC | DDFLIP_DONOTWAIT); //DDFLIP_WAIT
        else
        {
            p.x = 0; p.y = 0;

            // The ClientToScreen function converts the client-area coordinates of a specified point to screen coordinates.
            // in other word the client rectangle of the main windows 0, 0 (upper-left corner) 
            // in a screen x,y coords which is put back into p  
            ::ClientToScreen(hwnd, &p);  // find out where on the primary surface our window lives

            // get the actual client rectangle, which is always 0,0 - w,h
            ::GetClientRect(hwnd, &rcDest);

            // The OffsetRect function moves the specified rectangle by the specified offsets
            // add the delta screen point we got above, which gives us the client rect in screen coordinates.
            ::OffsetRect(&rcDest, p.x, p.y);

            // our destination rectangle is going to be 
            ::SetRect(&rcSrc, 0, 0, windowRect.w, windowRect.h);

            if (resizeable)
            {
                rcDest.bottom -= statusBarHeight;

                if (forceAspect) // Adjust the Aspect Ratio if window is resized
                {
                    float srcWidth = (float)windowRect.w;
                    float srcHeight = (float)windowRect.h;
                    float srcRatio = srcWidth / srcHeight;

                    // change this to use the existing rcDest and the calc, w = right-left & h = bottom-top, 
                    //                         because rcDest has already been converted to screen cords, right?   
                    static RECT rcClient;
                    ::GetClientRect(hwnd, &rcClient);  // x,y is always 0,0 so right, bottom is w,h
                    rcClient.bottom -= statusBarHeight;

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
                    ForcedAspectBorderPadding = pDstLeftTop;

                    ::ClientToScreen(hwnd, &pDstLeftTop);

                    static POINT pDstRightBottom;
                    pDstRightBottom.x = (long)(dstX + dstWidth); pDstRightBottom.y = (long)(dstY + dstHeight);
                    ::ClientToScreen(hwnd, &pDstRightBottom);

                    ::SetRect(&rcDest, pDstLeftTop.x, pDstLeftTop.y, pDstRightBottom.x, pDstRightBottom.y);
                }
            }
            else
            {
                // this does not seem ideal, it lets you begin to resize and immediately resizes it back ... causing a lot of flicker.
                rcDest.right = rcDest.left + windowRect.w;
                rcDest.bottom = rcDest.top + windowRect.h;
                ::GetWindowRect(hwnd, &Temp);
                ::MoveWindow(hwnd, Temp.left, Temp.top, WindowDefaultSize.right - WindowDefaultSize.left, WindowDefaultSize.bottom - WindowDefaultSize.top, 1);
            }

            if (g_pDDSBack == nullptr)
                return Result(ERR_UNKNOWN);

            hr = g_pDDS->Blt(&rcDest, g_pDDSBack, &rcSrc, DDBLT_WAIT, nullptr); // DDBLT_WAIT
            if (hr) return Result(ERR_UNKNOWN);
        }

        return Result(OK);
    }

    int DirectX::Present()
    {
        return Result(OK);
    }

    int DirectX::Cleanup()
    {
        using namespace Detail;
        if (g_pDD != nullptr)
            g_pDD->Release();	//Destroy the current Window

        return Result(OK);
    }

    int DirectX::SetOption(int flagOption, bool enabled)
    {
        using namespace Detail;
        //if (!isInitialized) return Result(ERR_NOTINITIALIZED);
        switch (flagOption)
        {
            case OPT_FLAG_ASPECT: forceAspect = enabled; break;
            case OPT_FLAG_RESIZEABLE: resizeable = enabled; break;
            default: return Result(ERR_BADOPTION);
        }
        return Result(OK);
    }

    int DirectX::GetSurface(Pixel** pixels)
    {
        using namespace Detail;
        if (!surface) return Result(ERR_UNKNOWN);
        if (bitDepth == 3)
        {
            *pixels = (Pixel*)surface;
            return Result(OK);
        }
        return Result(ERR_UNSUPPORTED);
    }

    int DirectX::GetRect(int rectOption, Rect* rect)
    {
        switch (rectOption)
        {
            case OPT_RECT_DISPLAY: GetDisplayArea(rect); break;
            //case OPT_RECT_RENDER: GetRenderArea(rect); break;
            case OPT_RECT_SURFACE: GetSurfaceArea(rect); break;
            default: return Result(ERR_BADOPTION);
        }
        return Result(OK);
    }

    int DirectX::LockSurface()
    {
        using namespace Detail;

        CheckSurfaces();

        HRESULT	hr;
        DDSURFACEDESC ddsd;				// A structure to describe the surfaces we want
        memset(&ddsd, 0, sizeof(ddsd));	// Clear all members of the structure to 0
        ddsd.dwSize = sizeof(ddsd);		// The first parameter of the structure must contain the size of the structure

        // Lock entire surface, wait if it is busy, return surface memory pointer
        hr = g_pDDSBack->Lock(nullptr, &ddsd, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, nullptr);
        if (hr)
        {
            //		MessageBox(0,"Can't Lock surface","Error",0);
            return Result(ERR_UNKNOWN);
        }

        switch (ddsd.ddpfPixelFormat.dwRGBBitCount)
        {
            case 8:
                surfacePitch = ddsd.lPitch;
                bitDepth = 0;
                break;
            case 15:
            case 16:
                surfacePitch = ddsd.lPitch / 2;
                bitDepth = 1;
                break;
            case 24:
                MessageBox(nullptr, "24 Bit color is currently unsupported", "Ok", 0);
                exit(0);
                surfacePitch = ddsd.lPitch;
                bitDepth = 2;
                break;
            case 32:
                surfacePitch = ddsd.lPitch / 4;
                bitDepth = 3;
                break;
            default:
                MessageBox(nullptr, "Unsupported Color Depth!", "Error", 0);
                return 1;
                break;
        }
        if (ddsd.lpSurface == nullptr)
            MessageBox(nullptr, "Returning NULL!!", "ok", 0);

        surface = ddsd.lpSurface;
        state->SetSurface(surface, bitDepth, surfacePitch);

        return Result(OK);
    }

    int DirectX::UnlockSurface()
    {
        using namespace Detail;

        HRESULT hr;
        hr = g_pDDSBack->Unlock(nullptr);
        surface = nullptr;
        if (hr) return Result(ERR_UNKNOWN);

        return Result(OK);
    }

    void DirectX::GetSurfaceArea(Rect* rect)
    {
        rect->x = 0;
        rect->y = 0;
        rect->w = 640;
        rect->h = 480;
    }

    void DirectX::GetDisplayArea(Rect* rect)
    {
        using namespace Detail;
        rect->x = (float)ForcedAspectBorderPadding.x;
        rect->y = (float)ForcedAspectBorderPadding.y;
        rect->w = 640;
        rect->h = 480;
    }

    void DirectX::CheckSurfaces()
    {
        using namespace Detail;

        if (g_pDDS)		// Check the primary surface
            if (g_pDDS->IsLost() == DDERR_SURFACELOST)
                g_pDDS->Restore();

        if (g_pDDSBack)		// Check the back buffer
            if (g_pDDSBack->IsLost() == DDERR_SURFACELOST)
                g_pDDSBack->Restore();

    }

    int DirectX::RenderSignalLostMessage()
    {
        using namespace Detail;
        static unsigned short TextX = rand() % 580, TextY = rand() % 470;
        static unsigned char Counter, Counter1 = 32;
        static char Phase = 1;
        static char Message[] = " Signal Missing! Press F9";

        HDC hdc;
        g_pDDSBack->GetDC(&hdc);
        SetBkColor(hdc, 0);
        SetTextColor(hdc, RGB(Counter1 << 2, Counter1 << 2, Counter1 << 2));
        TextOut(hdc, TextX, TextY, Message, strlen(Message));
        Counter++;
        Counter1 += Phase;
        if (Counter1 == 60 || Counter1 == 20)
            Phase = -Phase;
        Counter %= 60; //about 1 seconds
        if (!Counter)
        {
            TextX = rand() % 580;
            TextY = rand() % 470;
        }
        g_pDDSBack->ReleaseDC(hdc);
        return Result(OK);
    }

    int DirectX::RenderStatusLine(char * statusText)
    {
        using namespace Detail;
        size_t Index = 0;
        HDC hdc;
        //Put StatusText for full screen here
        if (isFullscreen)
        {
            g_pDDSBack->GetDC(&hdc);
            SetBkColor(hdc, RGB(0, 0, 0));
            SetTextColor(hdc, RGB(255, 255, 255));
            Index = strlen(statusText);
            for (;Index < 132;Index++)
                statusText[Index] = 32;
            statusText[Index] = 0;
            TextOut(hdc, 0, 0, statusText, 132);
            g_pDDSBack->ReleaseDC(hdc);
        }
        return Result(OK);
    }
}

#endif // USE_DIRECTX


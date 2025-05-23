#include "DirectX.h"

#if USE_DIRECTX

#include "DirectDrawInterface.h"

using namespace VCC;

DirectX::DirectX(SystemState* state) : state(state)
{
}

int DirectX::Setup(void* hwnd, int width, int height, int statusHeight)
{
    return OK;
}

int DirectX::Render()
{
    return OK;
}

int DirectX::Present()
{
    return OK;
}

int DirectX::Cleanup()
{
    return OK;
}

int DirectX::SetOption(int flagOption, bool enabled)
{
    return ERR_UNSUPPORTED;
}

int DirectX::GetSurface(Pixel** pixels)
{
    if (state->BitDepth == 3)
    {
        *pixels = (Pixel*)(state->PTRsurface32);
        return OK;
    }
    return ERR_UNSUPPORTED;
}

int DirectX::GetRect(int rectOption, Rect* rect)
{
    switch (rectOption)
    {
        //case OPT_RECT_DISPLAY: GetDisplayArea(rect); break;
        //case OPT_RECT_RENDER: GetRenderArea(rect); break;
        case OPT_RECT_SURFACE: GetSurfaceArea(rect); break;
        default: return ERR_BADOPTION;
    }
    return OK;
}

int DirectX::LockSurface()
{
    if (LockScreen(state) == 0) return OK;
    return ERR_UNSUPPORTED;
}

int DirectX::UnlockSurface()
{
    UnlockScreen(state);
    return OK;
}

void DirectX::GetSurfaceArea(Rect* rect)
{
    rect->x = 0;
    rect->y = 0;
    rect->w = 640;
    rect->h = 480;
}

#endif // USE_DIRECTX


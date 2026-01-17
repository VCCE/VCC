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

#include "OpenGL.h"
#include "BuildConfig.h"
#include <Windows.h>
#include <CommCtrl.h>
#include <gl/GL.h>
#include "GL/glext.h"
#include "GL/wglext.h"
#include <vcc/util/logger.h>

#if USE_OPENGL

#if USE_DEBUG_LINES
#include <vector>
#endif

typedef HGLRC(WINAPI* wglprocCreateContextAttribsARB)(HDC, HGLRC, const int*);
typedef BOOL(WINAPI* wglprocChoosePixelFormatARB)(HDC, const int*, const FLOAT*, UINT, int*, UINT*);
typedef BOOL(WINAPI* wglprocSwapIntervalEXT)(int interval);

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32")

static const LPCSTR ViewClass = "VCC_ViewWnd";
static const LPCSTR OpenGLClass = "VCC_OpenGLWnd";

namespace VCC
{
	//
	// Private wrapper on result code for logging errors
	//
	static int Result(int code)
	{
		if (code != OpenGL::OK)
		{
			char message[256];
			snprintf(message,64,"OpenGL error %d\nCheck OpenGL support",code);
			MessageBox(nullptr,message,"Error",0);
			// FIXME: This is needed and should not be commented out. Wrap it conditional
			// either here or in the debug log functions.
			//PrintLogC("OpenGL Error: %d\n", code);
		}
		return code;
	}

	//
	// Impliementation detail of OpenGL interface.
	//
	struct OpenGL::Detail
	{
		const int SubClassId = 1;
		const int GLSubClassId = 2;
		const int SurfaceWidth = 640; // coco surface size
		const int SurfaceHeight = 480;
		const float PalFreq = 60.0f;
		const float NtscFreq = 50.0f;

		bool isInitialized; // if already initialized

		HWND hWnd; // window handle of OpenGL window
		HDC hDC;   // its context
		HGLRC hRC;

		int width; // width of OpenGL window
		int height; // height of OpenGL window
		int statusHeight; // height of status bar
		bool aspect; // true if aspect preserved, otherwise fit
		bool ntsc; // use ntsc 50hz, otherwise 60hz
		Pixel* pixels; // cpu side surface buffer

		GLuint texId; // OpenGL texture on gpu
		wglprocSwapIntervalEXT wglSwapIntervalEXT;

		ISystemState* state;

		#if USE_DEBUG_LINES
		struct Line { float x1; float y1; float x2; float y2; Pixel color; };
		std::vector<Line> debugLines;
		#endif // USE_DEBUG_LINES

		OpenGL::Detail(ISystemState *state)
			: isInitialized(false), hWnd(nullptr), hDC(nullptr), hRC(nullptr)
			, width(0), height(0), statusHeight(0), aspect(true), ntsc(false)
			, pixels(nullptr), texId(0), wglSwapIntervalEXT(nullptr), state(state)
		{
		}

		// intercepted parent window messages
		static LRESULT __stdcall SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
		{
			switch (uMsg)
			{
			case WM_ERASEBKGND:
				return 1;

			case WM_SIZE:
			{
				auto detail = reinterpret_cast<OpenGL::Detail*>(dwRefData);
				detail->Resize(LOWORD(lParam), HIWORD(lParam));
				break;
			}

			case WM_NCDESTROY:
				RemoveWindowSubclass(hWnd, &SubclassProc, uIdSubclass);
				break;
			};

			return DefSubclassProc(hWnd, uMsg, wParam, lParam);
		}

		static LRESULT __stdcall GLSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
		{
			switch (uMsg)
			{
			case WM_ERASEBKGND:
				return 1;

			// prevent child window taking mouse messages
			case WM_NCHITTEST:
				return HTTRANSPARENT;

			case WM_NCDESTROY:
				RemoveWindowSubclass(hWnd, &SubclassProc, uIdSubclass);
				break;
			};

			return DefSubclassProc(hWnd, uMsg, wParam, lParam);
		}

		void Resize(int width, int height)
		{
			// windows callback with client size, leave space for status bar.
			height -= statusHeight;
			this->width = width;
			this->height = height;
			if (hWnd) SetWindowPos(hWnd, nullptr, 0, 0, width, height, SWP_NOCOPYBITS);
		}

		int Setup(void* hwnd, int width, int height, int statusHeight)
		{
			if (isInitialized)
				return Result(ERR_INITIALIZED);

			HWND hTempWnd = nullptr;
			HDC hTempDC = nullptr;
			HGLRC hTempRC = nullptr;

			auto hInstance = GetModuleHandle(nullptr);

			auto cleanupTempWindow = [&](int err)
			{
				if (hTempRC) { wglMakeCurrent(nullptr, nullptr); wglDeleteContext(hTempRC); }
				if (hTempDC) ReleaseDC(hTempWnd, hTempDC);
				if (hTempWnd) DestroyWindow(hTempWnd);
				UnregisterClass(ViewClass, hInstance);
				return err;
			};

			auto cleanupWindow = [&](int err)
			{
				Cleanup();
				return cleanupTempWindow(err);
			};

			this->width = width;
			this->height = height;
			this->statusHeight = statusHeight;

			WNDCLASS wc;
			memset(&wc, 0, sizeof(wc));
			wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
			wc.lpfnWndProc = DefWindowProc;
			wc.hInstance = GetModuleHandle(nullptr);
			wc.lpszClassName = ViewClass;
			if (!RegisterClass(&wc))
				return ERR_TMPREGISTERCLASS;

			// Create a temporary context to get address of wgl extensions.
			hTempWnd = CreateWindowEx(WS_EX_APPWINDOW, ViewClass, "Simple", WS_VISIBLE | WS_POPUP | WS_MAXIMIZE,
											0, 0, 0, 0, nullptr, nullptr, hInstance, nullptr);

			if (!hTempWnd) 
				return Result(ERR_TMPCREATEWINDOW);

			hTempDC = GetDC(hTempWnd);
			if (!hTempDC) 
				return Result(cleanupTempWindow(ERR_TMPGETDC));

			PIXELFORMATDESCRIPTOR pfd;
			memset(&pfd, 0, sizeof(pfd));
			pfd.nSize = sizeof(pfd);
			pfd.nVersion = 1;
			pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW;
			pfd.cColorBits = 24;
			pfd.iPixelType = PFD_TYPE_RGBA;
			pfd.iLayerType = PFD_MAIN_PLANE;

			int iPF;
			if (!(iPF = ChoosePixelFormat(hTempDC, &pfd)))
				return Result(cleanupTempWindow(ERR_TMPCHOOSEFORMAT));

			if (!SetPixelFormat(hTempDC, iPF, &pfd))
				return Result(cleanupTempWindow(ERR_TMPSETPIXELFORMAT));

			if (!(hTempRC = wglCreateContext(hTempDC)))
				return Result(cleanupTempWindow(ERR_TMPCREATECONTEXT));

			if (!wglMakeCurrent(hTempDC, hTempRC))
				return Result(cleanupTempWindow(ERR_TMPMAKECONTEXT));

			// Function pointers returned by wglGetProcAddress are tied to the render context they were obtained with.
			wglprocCreateContextAttribsARB wglCreateContextAttribsARB = (wglprocCreateContextAttribsARB)wglGetProcAddress("wglCreateContextAttribsARB");
			wglprocChoosePixelFormatARB wglChoosePixelFormatARB = (wglprocChoosePixelFormatARB)wglGetProcAddress("wglChoosePixelFormatARB");
			wglSwapIntervalEXT = (wglprocSwapIntervalEXT)wglGetProcAddress("wglSwapIntervalEXT");

			if (wglChoosePixelFormatARB && wglCreateContextAttribsARB && wglSwapIntervalEXT)
			{
				WNDCLASS wc = { 0 };
				wc.lpfnWndProc = DefWindowProc;
				wc.hInstance = hInstance;
				wc.hbrBackground = nullptr;
				wc.lpszClassName = OpenGLClass;
				wc.style = CS_OWNDC;

				if (!RegisterClass(&wc))
					return Result(cleanupTempWindow(ERR_REGISTERCLASS));

				SetWindowSubclass((HWND)hwnd, &SubclassProc, SubClassId, (DWORD_PTR)this);

				hWnd = CreateWindowEx(0, OpenGLClass, nullptr, WS_VISIBLE | WS_CHILD, 0, 0,
										width, height, (HWND)hwnd, 0, hInstance, this);

				if (!hWnd)
				{
					UnregisterClass(OpenGLClass, hInstance);
					return Result(cleanupTempWindow(ERR_CREATEWINDOW));
				}

				SetWindowSubclass((HWND)hWnd, &GLSubclassProc, GLSubClassId, (DWORD_PTR)this);

				hDC = GetDC(hWnd);

				if (!hDC)
					return Result(cleanupWindow(ERR_GETDC));

				int attribs[] = {
					WGL_DRAW_TO_WINDOW_ARB, TRUE,
					WGL_DOUBLE_BUFFER_ARB, TRUE,
					WGL_SUPPORT_OPENGL_ARB, TRUE,
					WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
					WGL_COLOR_BITS_ARB, 24,
					WGL_RED_BITS_ARB, 8,
					WGL_GREEN_BITS_ARB, 8,
					WGL_BLUE_BITS_ARB, 8,
					WGL_DEPTH_BITS_ARB, 0,
					WGL_STENCIL_BITS_ARB, 0,
					0, 0
				};

				UINT numFormats;
				BOOL success = wglChoosePixelFormatARB(hDC, attribs, nullptr, 1, &iPF, &numFormats);
				DescribePixelFormat(hDC, iPF, sizeof(pfd), &pfd);

				if (!(success && numFormats >= 1 && SetPixelFormat(hDC, iPF, &pfd)))
					return Result(cleanupWindow(ERR_SETPIXELFORMAT));

				// request OpenGl 3.0
				int contextAttribs[] =
				{
					WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
					WGL_CONTEXT_MINOR_VERSION_ARB, 0,
					0, 0
				};

				hRC = wglCreateContextAttribsARB(hDC, nullptr, contextAttribs);
				if (!hRC)
					return Result(cleanupWindow(ERR_GLVERSION));

				// release temporary window
				cleanupTempWindow(OK);
			}
			else return Result(ERR_MISSINGAPIS);

			// the off screen surface
			pixels = new Pixel[SurfaceWidth * SurfaceHeight];
			memset(pixels, 0, SurfaceWidth * SurfaceHeight * sizeof(Pixel));

			// create texture for rendering surface
			wglMakeCurrent(hDC, hRC);
			glViewport(0, 0, width, height);
			glGenTextures(1, &texId);
			glBindTexture(GL_TEXTURE_2D, texId);
			glEnable(GL_TEXTURE_2D);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SurfaceWidth, SurfaceHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
			wglMakeCurrent(nullptr, nullptr);
			isInitialized = true;
			return Result(OK);
		}

		int Render()
		{
			if (!isInitialized) return Result(ERR_NOTINITIALIZED);

			wglMakeCurrent(hDC, hRC);
			glViewport(0, 0, width, height);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(0, (float)width, (float)height, 0, -1, 1);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			// select surface texture
			glBindTexture(GL_TEXTURE_2D, texId);
			glEnable(GL_TEXTURE_2D);

			// update it
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SurfaceWidth, SurfaceHeight, GL_BGRA, GL_UNSIGNED_BYTE, pixels);

			Rect area;
			GetDisplayArea(&area);

			// aliases - don't panic
			const auto& x = area.x;
			const auto& y = area.y;
			const auto& w = area.w;
			const auto& h = area.h;

			// draw main surface in the middle
			glColor3f(1, 1, 1);
			glBegin(GL_QUADS);
				glTexCoord2f(0, 0); glVertex3f(x, y, 0);
				glTexCoord2f(1, 0); glVertex3f(x + w, y, 0);
				glTexCoord2f(1, 1); glVertex3f(x + w, y + h, 0);
				glTexCoord2f(0, 1); glVertex3f(x, y + h, 0);
			glEnd();

			#if USE_BLACKEDGES
			// Switch to black color for edges
			glDisable(GL_TEXTURE_2D);
			#define glTexCoord2f(a,b) glColor3f(0,0,0)
			#endif // USE_BLACKEDGES

			glBegin(GL_QUADS);
			if (x > 0)
			{
				// fill left/right edge
				glTexCoord2f(0, 0); glVertex3f(0, 0, 0);
				glTexCoord2f(0, 0); glVertex3f(x, 0, 0);
				glTexCoord2f(0, 1); glVertex3f(x, h, 0);
				glTexCoord2f(0, 1); glVertex3f(0, h, 0);

				glTexCoord2f(1, 0); glVertex3f(x + w, y, 0);
				glTexCoord2f(1, 0); glVertex3f((float)width, y, 0);
				glTexCoord2f(1, 1); glVertex3f((float)width, y + h, 0);
				glTexCoord2f(1, 1); glVertex3f(x + w, y + h, 0);
			}
			else if (y > 0)
			{
				// fill top/bottom edge
				glTexCoord2f(0, 0); glVertex3f(0, 0, 0);
				glTexCoord2f(1, 0); glVertex3f(w, 0, 0);
				glTexCoord2f(1, 0); glVertex3f(w, y, 0);
				glTexCoord2f(0, 0); glVertex3f(0, y, 0);

				glTexCoord2f(0, 1); glVertex3f(0, y + h, 0);
				glTexCoord2f(1, 1); glVertex3f(w, y + h, 0);
				glTexCoord2f(1, 1); glVertex3f(w, (float)height, 0);
				glTexCoord2f(0, 1); glVertex3f(0, (float)height, 0);
			}

			#if USE_BLACKEDGES
			#undef glTexCoord2f
			#endif // USE_BLACKEDGES

			glEnd();

			#if USE_DEBUG_LINES
			// draw some debug lines
			if (debugLines.size())
			{
				Pixel lastColor(debugLines[0].color);
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glBegin(GL_LINES);
				glColor4ub(lastColor.r, lastColor.g, lastColor.b, lastColor.a);
				for (size_t i = 0; i < debugLines.size(); ++i)
				{
					if (lastColor != debugLines[i].color)
					{
						lastColor = debugLines[i].color;
						glColor4ub(lastColor.r, lastColor.g, lastColor.b, lastColor.a);
					}
					glVertex3f(debugLines[i].x1, debugLines[i].y1, 0);
					glVertex3f(debugLines[i].x2, debugLines[i].y2, 0);
				}
				glEnd();
				glDisable(GL_BLEND);
				debugLines.clear();
			}
			#endif // USE_DEBUG_LINES

			wglMakeCurrent(nullptr, nullptr);
			return Result(OK);
		}

		int Cleanup()
		{
			if (!isInitialized) return Result(ERR_NOTINITIALIZED);
			if (pixels) delete pixels;
			if (hDC) ReleaseDC(hWnd, hDC);
			if (hWnd) DestroyWindow(hWnd);
			UnregisterClass(OpenGLClass, GetModuleHandle(nullptr));
			isInitialized = false;
			return Result(OK);
		}

		int GetSurface(Pixel** pixels) const
		{
			if (!isInitialized) return Result(ERR_NOTINITIALIZED);
			*pixels = this->pixels;
			return Result(OK);
		}

		int SetOption(int option, bool enabled)
		{
			if (!isInitialized) return Result(ERR_NOTINITIALIZED);
			switch (option)
			{
				case OPT_FLAG_ASPECT: aspect = enabled; break;
				case OPT_FLAG_NTSC: ntsc = enabled; break;
				case OPT_FLAG_RESIZEABLE: break;
				default: return Result(ERR_BADOPTION);
			}
			return Result(OK);
		}

		int GetRect(int rectOption, Rect* rect)
		{
			if (!isInitialized) return Result(ERR_NOTINITIALIZED);
			switch (rectOption)
			{
				case OPT_RECT_DISPLAY: GetDisplayArea(rect); break;
				case OPT_RECT_RENDER: GetRenderArea(rect); break;
				case OPT_RECT_SURFACE: GetSurfaceArea(rect); break;
				default: return Result(ERR_BADOPTION);
			}
			return Result(OK);
		}

		int RenderText(const OpenGLFont* font, float x, float y, float size, const char* text) const
		{
			if (!isInitialized) return Result(ERR_NOTINITIALIZED);

			int length = strlen(text);
			wglMakeCurrent(hDC, hRC);
			glViewport(0, 0, width, height);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(0, (float)width, (float)height, 0, -1, 1);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glBindTexture(GL_TEXTURE_2D, font->texture);
			glEnable(GL_TEXTURE_2D);
			glBegin(GL_QUADS);

			glColor3f(1, 1, 1);
			for (int i = 0; i < length; ++i)
			{
				auto ch = text[i];
				if (ch < font->start || ch >= font->end) continue;
				const auto glyph = font->glyphs + ch - font->start;
				if (ch != ' ')
				{
					const auto &ad = glyph->atlasDimensions;
					const auto& d = glyph->dimensions;

					float u1 = ad.l / font->textureWidth;
					float v1 = ad.b / font->textureHeight;
					float u2 = ad.r / font->textureWidth;
					float v2 = ad.t / font->textureHeight;

					glTexCoord2f(u1, v1); glVertex3f(x + size * d.l, y + size * d.b, 0);
					glTexCoord2f(u2, v1); glVertex3f(x + size * d.r, y + size * d.b, 0);
					glTexCoord2f(u2, v2); glVertex3f(x + size * d.r, y + size * d.t, 0);
					glTexCoord2f(u1, v2); glVertex3f(x + size * d.l, y + size * d.t, 0);
				}
				x += glyph->advance * size;
			}
			glEnd();
			wglMakeCurrent(nullptr, nullptr);

			return Result(OK);
		}

		int Present() const
		{
			if (!isInitialized) return Result(ERR_NOTINITIALIZED);
			wglMakeCurrent(hDC, hRC);
			// present
			glFlush();
			// don't sync with monitor, if monitor is 30hz we want vcc
			// running at 60 still, refresh is controlled in throttle.
			wglSwapIntervalEXT(0);
			SwapBuffers(hDC);
			wglMakeCurrent(nullptr, nullptr);
			return Result(OK);
		}

		int LoadFont(const OpenGLFont** outFont, int bitmapRes, const OpenGLFontGlyph* glyphs, int start, int end) const
		{
			*outFont = nullptr;
			if (!isInitialized) return Result(ERR_NOTINITIALIZED);

			HBITMAP hBitmap = nullptr;
			HDC dcBitmap = nullptr;
			COLORREF* bitmapPixels = nullptr;

			auto cleanup = [&](int code)
			{
				if (hBitmap) DeleteObject(hBitmap);
				if (dcBitmap) ReleaseDC(nullptr, dcBitmap);
				if (bitmapPixels) delete bitmapPixels;
				return code;
			};

			// load bitmap resource
			auto hInstance = GetModuleHandle(nullptr);
			hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(bitmapRes));
			if (!hBitmap) return Result(cleanup(ERR_FONTHBITMAP));
			dcBitmap = CreateCompatibleDC(nullptr);
			if (!dcBitmap) return Result(cleanup(ERR_FONTDC));
			SelectObject(dcBitmap, hBitmap);

			BITMAP bm;
			::GetObject(hBitmap, sizeof(bm), &bm);

			BITMAPINFO bmpInfo;
			bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmpInfo.bmiHeader.biWidth = bm.bmWidth;
			bmpInfo.bmiHeader.biHeight = -bm.bmHeight;
			bmpInfo.bmiHeader.biPlanes = 1;
			bmpInfo.bmiHeader.biBitCount = 24;
			bmpInfo.bmiHeader.biCompression = BI_RGB;
			bmpInfo.bmiHeader.biSizeImage = 0;

			// get bitmap data
			bitmapPixels = new COLORREF[bm.bmWidth * bm.bmHeight];
			if (GetDIBits(dcBitmap, hBitmap, 0, bm.bmHeight, bitmapPixels, &bmpInfo, DIB_RGB_COLORS) == 0)
				return Result(cleanup(ERR_FONTGETDIBITS));

			// upload texture
			GLuint textureId = 0;
			wglMakeCurrent(hDC, hRC);
			glGenTextures(1, &textureId);
			glBindTexture(GL_TEXTURE_2D, textureId);
			glEnable(GL_TEXTURE_2D);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);			
			glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.bmWidth, bm.bmHeight, 0, GL_BGR, GL_UNSIGNED_BYTE, bitmapPixels);
			if (glGetError()) return Result(cleanup(ERR_FONTTEXIMAGE2D));
			wglMakeCurrent(nullptr, nullptr);

			// allocate font
			OpenGLFont* font = new OpenGLFont();
			font->glyphs = glyphs;
			font->start = start;
			font->end = end;
			font->texture = textureId;
			font->textureWidth = (float)bm.bmWidth;
			font->textureHeight = (float)bm.bmHeight;

			*outFont = font;
			return Result(cleanup(OK));
		}

		#if USE_DEBUG_LINES
		void DebugDrawLine(float x1, float y1, float x2, float y2, Pixel color)
		{
			Line line = { x1, y1, x2, y2, color };
			debugLines.push_back(line);
		}
		#endif // USE_DEBUG_LINES

		int RenderBox(float x, float y, float w, float h, Pixel color, bool filled) const
		{
			if (!isInitialized) return Result(ERR_NOTINITIALIZED);
			float x2 = x + w;
			float y2 = y + h;
			wglMakeCurrent(hDC, hRC);
			glViewport(0, 0, width, height);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(0, (float)width, (float)height, 0, -1, 1);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glColor3ub(color.r, color.g, color.b);
			if (filled)
			{
				glBegin(GL_QUADS);
					glVertex3f(x, y, 0);
					glVertex3f(x + w, y, 0);
					glVertex3f(x + w, y + h, 0);
					glVertex3f(x, y + h, 0);
				glEnd();
			}
			else
			{
				glBegin(GL_LINES);
					glVertex3f(x, y, 0);
					glVertex3f(x + w, y, 0);

					glVertex3f(x + w, y, 0);
					glVertex3f(x + w, y + h, 0);

					glVertex3f(x + w, y + h, 0);
					glVertex3f(x, y + h, 0);

					glVertex3f(x, y + h, 0);
					glVertex3f(x, y, 0);
				glEnd();
			}
			wglMakeCurrent(nullptr, nullptr);
			return Result(OK);
		}

		int LockSurface()
		{
			if (!isInitialized) return Result(ERR_NOTINITIALIZED);
			state->SetSurface(pixels, 3, DefaultWidth);
			return Result(OK);
		}

	private:

		// returns the box where display should be rendered
		void GetDisplayArea(Rect* area) const
		{
			area->w = (float)width;
			area->h = (float)height;

			// scale to fit display with aspect kept
			if (aspect)
			{
				// keep aspect ratio, ntsc = 50hz
				float freqScale = ntsc ? PalFreq / NtscFreq : 1.0f;
				float dw = (float)SurfaceWidth;
				float dh = (float)SurfaceHeight * freqScale;
				float rx = area->w / dw;
				float ry = area->h / dh;
				float r = rx > ry ? ry : rx;

				area->w = r * dw;
				area->h = r * dh;
			}

			// center on display
			area->x = (width - area->w) / 2.0f;
			area->y = (height - area->h) / 2.0f;
		}

		void GetRenderArea(Rect* area) const
		{
			area->x = area->y = 0;
			area->w = (float)width;
			area->h = (float)height;
		}

		void GetSurfaceArea(Rect* area) const
		{
			area->x = area->y = 0;
			area->w = (float)SurfaceWidth;
			area->h = (float)SurfaceHeight;
		}
	};

	//////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// Forwarding functions to implementation.
	//

	int OpenGL::GetSurface(Pixel** pixels)
	{
		if (detail) 
			return detail->GetSurface(pixels);
		return Result(ERR_NOTINITIALIZED);
	}

	int OpenGL::Setup(void* hwnd, int width, int height, int statusHeight, bool fullscreen)
	{
		detail = new Detail(state);
		return detail->Setup(hwnd, width, height, statusHeight);
	}

	int OpenGL::Render()
	{
		if (detail) 
			return detail->Render();
		return Result(ERR_NOTINITIALIZED);
	}

	int OpenGL::Cleanup()
	{
		if (detail)
		{
			int result = detail->Cleanup();
			delete detail;
			detail = nullptr;
			return result;
		}
		return Result(ERR_NOTINITIALIZED);
	}

	int OpenGL::Present()
	{
		if (detail)
			return detail->Present();
		return Result(ERR_NOTINITIALIZED);
	}

	int OpenGL::SetOption(int option, bool enabled)
	{
		if (detail)
			return detail->SetOption(option, enabled);
		return Result(ERR_NOTINITIALIZED);
	}
	
	int OpenGL::RenderBox(float x, float y, float w, float h, Pixel color, bool filled)
	{
		if (detail)
			return detail->RenderBox(x,y,w,h,color,filled);
		return Result(ERR_NOTINITIALIZED);
	}

	int OpenGL::GetRect(int rectOption, Rect* area)
	{
		if (detail)
			return detail->GetRect(rectOption, area);
		return Result(ERR_NOTINITIALIZED);
	}

	int OpenGL::RenderText(const OpenGLFont* font, float x, float y, float size, const char* text)
	{
		if (detail)
			return detail->RenderText(font, x, y, size, text);
		return Result(ERR_NOTINITIALIZED);
	}

	int OpenGL::LoadFont(const OpenGLFont** outFont, int bitmapRes, const OpenGLFontGlyph* glyphs, int start, int end)
	{
		if (detail)
			return detail->LoadFont(outFont, bitmapRes, glyphs, start, end);
		return Result(ERR_NOTINITIALIZED);
	}

	int OpenGL::LockSurface()
	{
		if (detail)
			return detail->LockSurface();
		return Result(ERR_NOTINITIALIZED);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////

	void OpenGL::DebugDrawLine(float x1, float y1, float x2, float y2, Pixel color)
	{
		#if USE_DEBUG_LINES
		if (detail)
			detail->DebugDrawLine(x1, y1, x2, y2, color);
		#endif // USE_DEBUG_LINES
	}
	void OpenGL::DebugDrawBox(float x, float y, float w, float h, Pixel color)
	{
		#if USE_DEBUG_LINES
		if (detail)
		{
			float x2 = x + w;
			float y2 = y + h;
			detail->DebugDrawLine(x, y, x2, y, color);
			detail->DebugDrawLine(x2, y, x2, y2, color);
			detail->DebugDrawLine(x2, y2, x, y2, color);
			detail->DebugDrawLine(x, y2, x, y, color);
		}
		#endif // USE_DEBUG_LINES
	}
}

#endif // USE_OPENGL

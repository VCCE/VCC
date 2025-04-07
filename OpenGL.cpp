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

	2025/04/05 - Craig Allsop - add initial setup OpenGL class
*/

#include "OpenGL.h"

#include <Windows.h>
#include <commctrl.h>
#include <gl/GL.h>
#include "gl/glext.h"
#include "gl/wglext.h"
#include <string>

typedef HGLRC(*wglprocCreateContextAttribsARB)(HDC, HGLRC, const int*);
typedef BOOL(*wglprocChoosePixelFormatARB)(HDC, const int*, const FLOAT*, UINT, int*, UINT*);

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32")

static const LPCSTR ViewClass = "VCC_ViewWnd";
static const LPCSTR OpenGLClass = "VCC_OpenGLWnd";

namespace VCC
{
	struct OpenGL::Detail
	{
		const int SubClassId = 1;
		const int SurfaceWidth = 640; // coco surface size
		const int SurfaceHeight = 480;

		HWND hWnd; // window handle of OpenGL window
		HDC hDC;   // its context
		HGLRC hRC;

		int width; // width of OpenGL window
		int height; // height of OpenGL window
		int statusHeight; // height of status bar
		Pixel* pixelsRGBA; // cpu side surface buffer

		GLuint texId; // OpenGL texture on gpu

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

		void Resize(int width, int height)
		{
			// windows callback with client size, leave space for status bar.
			height -= statusHeight;
			this->width = width;
			this->height = height;
			if (hWnd) SetWindowPos(hWnd, NULL, 0, 0, width, height, SWP_NOCOPYBITS);
		}

		bool Setup(void* hwnd, int width, int height, int statusHeight)
		{
			pixelsRGBA = new Pixel[SurfaceWidth * SurfaceHeight];
			memset(pixelsRGBA, 0, SurfaceWidth * SurfaceHeight * sizeof(int));

			this->width = width;
			this->height = height;
			this->statusHeight = statusHeight;

			auto hInstance = GetModuleHandle(NULL);

			WNDCLASS wc;
			memset(&wc, 0, sizeof(wc));
			wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
			wc.lpfnWndProc = DefWindowProc;
			wc.hInstance = GetModuleHandle(NULL);
			wc.lpszClassName = ViewClass;
			RegisterClass(&wc);

			/* Create a temporaray context to get address of wgl extensions. */

			HWND hTempWnd = CreateWindowEx(WS_EX_APPWINDOW, ViewClass, "Simple", WS_VISIBLE | WS_POPUP | WS_MAXIMIZE,
											0, 0, 0, 0, NULL, NULL, hInstance, NULL);

			if (!hTempWnd)
			{
				auto e = GetLastError();
				return FALSE;
			}

			HDC hTempDC = GetDC(hTempWnd);
			if (!hTempDC)
			{
				DestroyWindow(hTempWnd);
				return FALSE;
			}

			PIXELFORMATDESCRIPTOR pfd;
			memset(&pfd, 0, sizeof(pfd));
			pfd.nSize = sizeof(pfd);
			pfd.nVersion = 1;
			pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
			pfd.cColorBits = 24;
			pfd.cDepthBits = 24;
			pfd.iPixelType = PFD_TYPE_RGBA;
			pfd.iLayerType = PFD_MAIN_PLANE;

			HGLRC hTempRC;
			int iPF;
			if ((!(iPF = ChoosePixelFormat(hTempDC, &pfd)) || !SetPixelFormat(hTempDC, iPF, &pfd)) ||
				(!(hTempRC = wglCreateContext(hTempDC)) || !wglMakeCurrent(hTempDC, hTempRC)))
			{
				ReleaseDC(hTempWnd, hTempDC);
				DestroyWindow(hTempWnd);
				return FALSE;
			}

			// Function pointers returned by wglGetProcAddress are tied to the render context they were obtained with.
			wglprocCreateContextAttribsARB wglCreateContextAttribsARB = (wglprocCreateContextAttribsARB)wglGetProcAddress("wglCreateContextAttribsARB");
			wglprocChoosePixelFormatARB wglChoosePixelFormatARB = (wglprocChoosePixelFormatARB)wglGetProcAddress("wglChoosePixelFormatARB");

			if (wglChoosePixelFormatARB && wglCreateContextAttribsARB)
			{
				WNDCLASS wc = { 0 };
				wc.lpfnWndProc = DefWindowProc;
				wc.hInstance = hInstance;
				wc.hbrBackground = NULL;
				wc.lpszClassName = OpenGLClass;
				wc.style = CS_OWNDC;

				if (!RegisterClass(&wc))
				{
					return false;
				}

				SetWindowSubclass((HWND)hwnd, &SubclassProc, SubClassId, (DWORD_PTR)this);

				hWnd = CreateWindowEx(0, OpenGLClass, NULL, WS_VISIBLE | WS_CHILD, 0, 0,
										width, height, (HWND)hwnd, 0, hInstance, this);

				if (!hWnd)
				{
					wglMakeCurrent(NULL, NULL);
					wglDeleteContext(hTempRC);
					ReleaseDC(hTempWnd, hTempDC);
					DestroyWindow(hTempWnd);

					return FALSE;
				}

				hDC = GetDC(hWnd);

				if (!hDC)
				{
					DestroyWindow(hWnd);

					wglMakeCurrent(NULL, NULL);
					wglDeleteContext(hTempRC);
					ReleaseDC(hTempWnd, hTempDC);
					DestroyWindow(hTempWnd);

					return FALSE;
				}

				int attribs[] = {
					WGL_DRAW_TO_WINDOW_ARB, TRUE,
					WGL_DOUBLE_BUFFER_ARB, TRUE,
					WGL_SUPPORT_OPENGL_ARB, TRUE,
					WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
					WGL_COLOR_BITS_ARB, 24,
					WGL_RED_BITS_ARB, 8,
					WGL_GREEN_BITS_ARB, 8,
					WGL_BLUE_BITS_ARB, 8,
					WGL_DEPTH_BITS_ARB, 24,
					WGL_STENCIL_BITS_ARB, 8,
					0, 0
				};

				UINT numFormats;
				BOOL success = wglChoosePixelFormatARB( hDC, attribs, NULL, 1, &iPF, &numFormats);
				DescribePixelFormat(hDC, iPF, sizeof(pfd), &pfd);

				if (!(success && numFormats >= 1 && SetPixelFormat(hDC, iPF, &pfd)))
				{
					ReleaseDC(hWnd, hDC);
					DestroyWindow(hWnd);

					wglMakeCurrent(NULL, NULL);
					wglDeleteContext(hTempRC);
					ReleaseDC(hTempWnd, hTempDC);
					DestroyWindow(hTempWnd);

					return FALSE;
				}

				// request OpenGl 3.0
				int contextAttribs[] =
				{
					WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
					WGL_CONTEXT_MINOR_VERSION_ARB, 0,
					0, 0
				};

				hRC = wglCreateContextAttribsARB(hDC, NULL, contextAttribs);
				if (!hRC)
				{
					ReleaseDC(hWnd, hDC);
					DestroyWindow(hWnd);

					wglMakeCurrent(NULL, NULL);
					wglDeleteContext(hTempRC);
					ReleaseDC(hTempWnd, hTempDC);
					DestroyWindow(hTempWnd);
					return FALSE;
				}

				// release temporary window
				wglMakeCurrent(NULL, NULL);
				wglDeleteContext(hTempRC);
				ReleaseDC(hTempWnd, hTempDC);
				DestroyWindow(hTempWnd);
			}

			wglMakeCurrent(hDC, hRC);
			glViewport(0, 0, width, height);
			glGenTextures(1, &texId);
			glBindTexture(GL_TEXTURE_2D, texId);
			glEnable(GL_TEXTURE_2D);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SurfaceWidth, SurfaceHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRGBA);
			wglMakeCurrent(NULL, NULL);
			return true;
		}

		bool Render()
		{
			float w = (float)width;
			float h = (float)height;
			wglMakeCurrent(hDC, hRC);
			glViewport(0, 0, width, height);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(0, w, h, 0, -1, 1);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			glBindTexture(GL_TEXTURE_2D, texId);
			glEnable(GL_TEXTURE_2D);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SurfaceWidth, SurfaceHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRGBA);

			// keep aspect ratio, ntsc = 50hz
			bool isNTSC = true;
			float ntsc = isNTSC ? 60.0f / 50.0f : 1.0f;
			float dw = (float)SurfaceWidth;
			float dh = (float)SurfaceHeight * ntsc;
			float rx = w / dw;
			float ry = h / dh;
			float r = rx > ry ? ry : rx;

			// scale to fit display
			w = r * dw;
			h = r * dh;

			// center on display
			float x = (width - w) / 2.0f;
			float y = (height - h) / 2.0f;

			glBegin(GL_QUADS);
				glTexCoord2f(0, 0); glVertex3f(x, y, 0);
				glTexCoord2f(1, 0); glVertex3f(x+w, y, 0);
				glTexCoord2f(1, 1); glVertex3f(x+w, y+h, 0);
				glTexCoord2f(0, 1); glVertex3f(x, y+h, 0);
			glEnd();

			glFlush();
			SwapBuffers(hDC);
			wglMakeCurrent(NULL, NULL);
			return true;
		}

		void Cleanup()
		{
		}

		Pixel* Surface()
		{
			return pixelsRGBA;
		}
	};

	Pixel* OpenGL::Surface()
	{
		if (detail) return detail->Surface();
		return nullptr;
	}

	bool OpenGL::Setup(void* hwnd, int width, int height, int statusHeight)
	{
		detail = new Detail();
		return detail->Setup(hwnd, width, height, statusHeight);
	}

	void OpenGL::Render()
	{
		if (detail) detail->Render();
	}

	void OpenGL::Cleanup()
	{
		if (detail)	detail->Cleanup();
		delete detail;
		detail = nullptr;
	}
}
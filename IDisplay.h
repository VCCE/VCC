#pragma once

namespace VCC
{
	typedef unsigned int Pixel;

	struct IDisplay
	{
		// setup display:
		//	width = full window width
		//	height = full window height not including status bar
		//  statusHeight = height to leave for status bar
		virtual bool Setup(void* hwnd, int width, int height, int statusHeight) { return false; }

		//
		// render the surface to the screen
		//
		virtual void Render() {}

		//
		// return the virtual surface (RGBA format) to draw into
		//
		virtual Pixel* Surface() { return nullptr; }

		virtual void Cleanup() {}
	};
}

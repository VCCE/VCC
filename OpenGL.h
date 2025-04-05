#pragma once

#include "IDisplay.h"

namespace VCC
{
	struct OpenGL : public IDisplay
	{
		OpenGL() : detail(nullptr) {}

		bool Setup(void* hwnd, int width, int height, int statusHeight) override;
		void Render() override;
		void Cleanup() override;
		Pixel* Surface() override;

	private:
		struct Detail;
		Detail* detail;
	};
}


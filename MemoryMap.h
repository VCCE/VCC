//------------------------------------------------------------------
// Display VCC memory for debugging
//
// This file is part of VCC (Virtual Color Computer).
// Copyright 2015 by Joseph Forgione
//
// VCC (Virtual Color Computer) is free software: you can redistribute
// it and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// VCC (Virtual Color Computer) is distributed in the hope that it will
// be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with VCC (Virtual Color Computer).  If not, see
// <http://www.gnu.org/licenses/>.
//
// Memory Map Display - Part of the Debugger package for VCC
// Authors: Mike Rojas, Chet Simpson
//
//------------------------------------------------------------------

#pragma once
#include <Windows.h>
#include "tcc1014graphics.h"

namespace VCC::Debugger::UI
{
	struct MemoryBackBufferInfo : BackBufferInfo
	{
		HPEN pen = nullptr;
		HFONT font = nullptr;

		HBITMAP fontBitmap = nullptr;
		HBITMAP fontRedBitmap = nullptr;
		HBITMAP fontAsciiBitmap = nullptr;
		HDC fontDC = nullptr;
		HDC fontRedDC = nullptr;
		HDC fontAsciiDC = nullptr;

		int dataWidth = 0;
		int dataHeight = 0;
		int dataStride = 0;
		char* data = nullptr;

		void Init(HWND hDlgWnd);
		void CleanupFonts(HWND hWnd);
		void CleanupPen();
		void CleanupFont();
		void Cleanup(HWND hWnd);
	};

	struct MemoryWindow
	{
		// Enum for memory type being examined
		enum AddrMode
		{
			Cpu,
			Real,
			ROM,
			PAK,
			NotSet
		};
		
		enum ViewMode
		{
			VM_ASCII,
			VM_SG4,
			VM_PMODE0_S0,
			VM_PMODE0_S1,
			VM_PMODE1_S0,
			VM_PMODE1_S1,
			VM_PMODE2_S0,
			VM_PMODE2_S1,
			VM_PMODE3_S0,
			VM_PMODE3_S1,
			VM_PMODE4_RGB_S0,
			VM_PMODE4_RGB_S1,
			VM_PMODE4_NTSC,
			VM_TEXT40,
			VM_TEXT80,
			VM_HSCREEN1,
			VM_HSCREEN2,
			VM_HSCREEN3,
			VM_HSCREEN4,
			VM_MAX
		};

		HWND hDlgMem;
		HWND hScrollBar;
		HWND hHorzScrollBar;
		HWND hEditAdrBeg;
		HWND hEditAdrEnd;
		HWND hEditVal;
		HWND hStatic;

		GimeGpu memGpu;
		unsigned char *ramCache = nullptr;
		unsigned int ramPos = -1;

		AddrMode addrMode = AddrMode::NotSet;
		ViewMode viewMode = VM_SG4;

		static const std::string cMemoryWindow;
		static const std::string cMemoryWindow2;
		static const std::string cWindowSizeX;
		static const std::string cWindowSizeY;
		static const std::string cWindowPosX;
		static const std::string cWindowPosY;
		static const std::string cAddress;
		static const std::string cAddressMode;
		static const std::string cViewMode;
		static const std::string cHex;
		static const std::string cWide;
		static const std::string cDataWidth;
		static const std::string cDataPosX;

		int winIndex;
		int memSize;
		int memOffset;
		int selectionRangeBeg;
		int selectionRangeEnd;
		unsigned char *rom;
		bool isEditing;
		int showHex;
		int showWideView;
		int editAddress;
		int dataWidth;
		int dataPosX;

		// Backing buffer used for painting memory data
		MemoryBackBufferInfo BackBuf;

		MemoryWindow(int index);

		struct Measurements;

		void SetEditing(bool);
		void FlashDialogWindow();
		void WriteMemory(int,unsigned char);
		void SetBackBuffer(const RECT&);
		void CreateScrollBar(const RECT&);
		void DrawForm(HDC,LPCRECT);
		bool DrawMemory(HDC,LPCRECT);
		void SetEditPosition(int,int);
		void LocateMemory();
		void ExportMemory();
		void CommitValue();
		void CommitWidth();
		void CommitWidth(const char *value);
		void SelectWidth();
		void SetMemType();
		void InitializeDialog(HWND);
		void DoScroll(WPARAM);
		void DoHorzScroll(WPARAM);
		int CStrToHex(const char *);
		void ResetMemoryCache();
		void ResizeWindow(int width, int height);
		void RepaintAll();
		void UpdateVertScrollBar();
		void UpdateHorzScrollBar();
		void UpdateWidthDisplay();
		void SetViewType();
		void SaveSettings();
		void LoadSettings();
		void SetupDataWidth(int delta);
		void Shutdown();
		void OnTimer(HWND hDlg);
		void Paint(HWND hDlg);
		void Help(HWND hDlg);
		void Export();
		void MemType(HWND hDlg);
		void LeftButton(int x, int y);
		void Escape();
		void SelectAdrEnd();
		void SelectAdrBeg();
		void SetWindowRect();
		void ViewType();
		bool Init();

		unsigned char ReadMemory(int addr);
	};

	void OpenMemoryMapWindow(HINSTANCE instance, HWND parent);
}

/*****************************************************************************/
/*
Vcc Copyright 2015 by Joseph Forgione.
This file is part of VCC (Virtual Color Computer)

	VCC (Virtual Color Computer) is free software: you can redistribute 
	it and/or modify it under the terms of the GNU General Public License 
	as published by the Free Software Foundation, either version 3 of the 
	License, or (at your option) any later version.

	VCC (Virtual Color Computer) is distributed in the hope that it will 
	be useful, but WITHOUT ANY WARRANTY; without even the implied warranty 
	of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with VCC (Virtual Color Computer).  If not, see 
	<http://www.gnu.org/licenses/>.

    Keyboard Edit author: E J Jaquay 2021
*/

#include <windows.h>
#include <windowsx.h>
#include <Shlwapi.h>
#include <stdio.h>
#include <intrin.h>

#include "defines.h"
#include "config.h"

#include "logger.h"
#include "keyboard.h"
#include "keyboardLayout.h"
#include "keyboardEdit.h"
#include "keynames.h"

// Modifier Button id's
static int ModBtnCode[4] = {0,IDC_KEYBTN_LSHIFT,IDC_KEYBTN_CTRL,IDC_KEYBTN_ALT}; 

// Modifier ScanCodes 
static int ModScanCode[4] = {0,DIK_LSHIFT,DIK_LCONTROL,DIK_LMENU};

// Modifier Names
static char *ModName[4] = {"","Shift","Ctrl","Alt"};

// Windows handles
HWND  hKeyMapDlg = NULL;     // Key map dialog
HWND  hText_PC = NULL;       // Control to display PC Key.
HWND  hText_CC = NULL;       // Control to display selected CoCo Keys
HWND  hText_MapFile = NULL;  // Control to display current map file name

// Subclass Text_PC control processing to capture all PC keystrokes
LRESULT CALLBACK SubText_PCproc(HWND,UINT,WPARAM,LPARAM);
WNDPROC Text_PCproc;

// CoCo and Host PC buttons and keys that are currently selected
// Note Left and Right modifiers are tied together
int CC_KeySelected;  // BtnId
int CC_ModSelected;  // 0,1,2,3
int PC_KeySelected;  // ScanCode
int PC_ModSelected;  // 0,1,2,3

// Current CoCo key button states (up/down)
// Used to raise currently down buttons
BOOL CoCoKeySet;      // BtnId
BOOL CoCoModSet;      // 0,1,2,3

// Pointer for selected translation entry
keytranslationentry_t * pKeyTran = NULL;

// Counter to limit the number of popup error messages
int UserWarned = 0;

// Background and Font for dynamic text boxes
HBRUSH hbrTextBox = NULL;
HFONT  hfTextBox  = NULL;

// Keymap data changed flag
BOOL KeyMapChanged = FALSE;

// Forward references
BOOL  InitKeymapDialog(HWND);
BOOL  Process_CoCoKey(int);
BOOL  SelectCustKeymapFile();
BOOL  SaveCustKeymap();
BOOL  WriteKeymap(char*); 
BOOL  SetCustomKeymap();
BOOL  ClrCustomKeymap();
BOOL  SetControlFont(WPARAM,LPARAM);
void  CoCoModifier(int);
void  ShowCoCoKey();
void  SetPCmod(int);
void  ShowPCkey();
void  SetCoCokey();
void  DoKeyDown(WPARAM,LPARAM);
void  ShowMapError(int, char *);
void  SetDialogFocus(HWND);
int   GetKeymapLine (char*, keytranslationentry_t *, int);
int   CustKeyTransLen();
char *GenKeymapLine(keytranslationentry_t *);

// Lookup functions for keyname tables
static struct CoCoKey * cctable_rowcol_lookup(unsigned char, unsigned char);
static struct CoCoKey * cctable_keyid_lookup(int);
static struct CoCoKey * cocotable_keyname_lookup(char *);
static struct PCScanCode * scantable_scancode_lookup(int);
static struct PCScanCode * scantable_keyname_lookup(char *);

/**************************************************/
/*          Load custom keymap from file          */
/**************************************************/

int LoadCustomKeyMap(char* keymapfile)
{
    FILE *keymap;
    char buf[256];
    int  ndx  = 0;
    int  lnum = 0;

	// Do nothing if keymap file path is empty
	if (*keymapfile == '\0') return 1;

    // Open the keymap file. Abort operation if open fails.
    if ((keymap = fopen(keymapfile,"r")) == NULL) {
        if (! UserWarned) {
            UserWarned = 1;
            sprintf(buf,"%s open failed. Using defaults",  keymapfile);
            MessageBox(GetActiveWindow(),buf,"Warning",0);
        }
        return 1;
    }
    UserWarned = 0;

    // clear the custom keymap.
    memset ( keyTranslationsCustom, 0,
			 sizeof(keytranslationentry_t)*MAX_CTRANSTBLSIZ);

    // load keymap from file.
    while (fgets(buf,250,keymap)) {
        lnum++;
		// Get line from file, save valid entries in table
        if (GetKeymapLine(buf, &keyTranslationsCustom[ndx],lnum)==0) {
            ndx++;
            if (ndx > MAX_CTRANSTBLSIZ-1) { 
				// Translation table overflow! (Can this actually happen?) 
				// TODO: If it does correct condition and warn user
				// ShowMapError(lnum,"Translation table full!");
                fclose(keymap);
                return 2;
            }
        }
    }

    fclose(keymap);
    return 0;
}

//-----------------------------------------------------
// Parse a line from keymap file into trans table entry
//-----------------------------------------------------
int GetKeymapLine ( char* line, keytranslationentry_t * trans, int lnum)
{
    char *pStr;
    static struct PCScanCode * pPCScanCode; 
    static struct CoCoKey * pCoCoKey;

    // pc scancode -> ScanCode1
    pStr = strtok(line, " \t\n\r");
    if ((pStr == NULL) || (*pStr == '#')) return 1; // A comment, skip it

	pPCScanCode = scantable_keyname_lookup(pStr);
	if (pPCScanCode == NULL) {
	    ShowMapError(lnum,"PC Scancode not recognized");
        return 2;
    }
    trans->ScanCode1 = pPCScanCode->ScanCode;

    // pc modifier -> ScanCode2
    pStr = strtok(NULL, " \t\n\r");
    if (pStr == NULL) {
        ShowMapError(lnum,"PC Scancode modifier missing");
        return 2;
    }
	
    // Generate scancode for modifier 1=shift,2=ctrl,3=alt
	switch(*pStr) {
    case '0':
        trans->ScanCode2 = 0;
        break;
    case '1':
        trans->ScanCode2 = DIK_LSHIFT;
        break;
    case '2':
        trans->ScanCode2 = DIK_LCONTROL;
        break;
    case '3':
        trans->ScanCode2 = DIK_LMENU;
        break;
    default:
        ShowMapError(lnum,"PC key modifier not recognized");
        return 2;
    }

    // CoCo Key -> Row1, Col1
	pStr = strtok(NULL, " \t\n\r");
    if (pStr == NULL) {
        ShowMapError(lnum,"CoCo keyname missing");
        return 2;
    }
    pCoCoKey = cocotable_keyname_lookup(pStr);
	if (pCoCoKey == NULL) {
		ShowMapError(lnum,"CoCo keyname not recognized");
        return 2;
    }
	trans->Row1 = 1 << pCoCoKey->row;
    trans->Col1 = pCoCoKey->col;

    // CoCo Key modifer -> Row2, Col2
    pStr = strtok(NULL, " \t\n\r");

    // If modifier missing accept input anyway
    if (pStr == NULL) {
        trans->Row2 = 0;
        trans->Col2 = 0;
        return 0;
    }

    // Generate row and col for modifier 1=shift,2=ctrl,3=alt
	switch(*pStr) {
    case '0':
        trans->Row2 = 0;
        trans->Col2 = 0;
        break;
    case '1':
        trans->Row2 = 64;
        trans->Col2 = 7;
        break;
    case '2':
        trans->Row2 = 64;
        trans->Col2 = 4;
        break;
    case '3':
        trans->Row2 = 64;
        trans->Col2 = 3;
        break;
    default:
        ShowMapError(lnum,"CoCo key modifier not recognized");
        return 2;
    }
    return 0;
}

//------------------------------------------------------
// Save custom keymap to file 
//-----------------------------------------------------
BOOL WriteKeymap(char* keymapfile) 
{
    keytranslationentry_t * pTran;
	pTran = keyTranslationsCustom;

    FILE *omap;
	FILE *kmap;
	char tmpfile[MAX_PATH];
	char tmppath[MAX_PATH];

	char buf[256];

    // Create a temporary file to hold the keymap
	GetTempPath(MAX_PATH,tmppath);
	GetTempFileName(tmppath,"KM_",0,tmpfile);

	// Open it for write
	kmap = fopen(tmpfile,"w");
	if (kmap == NULL) {
        MessageBox(hKeyMapDlg,tmpfile,"open error",0);
		return FALSE;
	}

    // If there was already a file by name specified
	if (PathFileExists(keymapfile)) {
		// Open the existing file
		omap = fopen(keymapfile,"r");
	    if (omap == NULL) {
            MessageBox(hKeyMapDlg,keymapfile,"open error",0);
		    return FALSE;
	    }
		// Copy comments to temporary file
		while (fgets(buf,256,omap)) { 
		   	if (*buf != '#') break;
		    if( fputs(buf,kmap) < 0) {
                MessageBox(hKeyMapDlg,keymapfile,"read error",0);
                fclose(kmap);
		        return FALSE;
		    }
		}
		// Close and delete it
        fclose(omap);
	    DeleteFile(keymapfile);
	}

    // Copy in contents of custom key translation table
	while (pTran->ScanCode1 != 0) {
		if( fputs(GenKeymapLine(pTran),kmap) < 0) {
            MessageBox(hKeyMapDlg,tmpfile,"write error",0);
            fclose(kmap);
		    return FALSE;
		}
		pTran++;
	}
    fclose(kmap);

	// Rename temp file to keymapfile
	if (!MoveFile(tmpfile,keymapfile)) {
        MessageBox(hKeyMapDlg,tmpfile,"Rename error",0);
		return FALSE;
	}

	return TRUE;
}

//------------------------------------------------------
// Convert translation record for keymap file
//-----------------------------------------------------
char * GenKeymapLine( keytranslationentry_t * pTran )
{
	static char txt[64];
	static struct PCScanCode * pSC; 
    static struct CoCoKey * pCC; 
	int CCmod = 0;
	int PCmod = 0;

	// Determine PC key and modifier
	if (pTran->ScanCode1 == 0) return NULL;
	if (pTran->Row1 == 0) return NULL;

    if (pTran->ScanCode2 != 0) {
	    switch (pTran->ScanCode1) {
	    case DIK_LSHIFT:   
		    PCmod = 1;
		    break;
	    case DIK_LCONTROL: 
		    PCmod = 2;
		    break;
	    case DIK_LMENU:
		    PCmod = 3;
		    break;
		}
    }

    if ((pTran->ScanCode2 != 0) & (PCmod != 0)) {
		pSC = scantable_scancode_lookup(pTran->ScanCode2);
	} else {
	    pSC = scantable_scancode_lookup(pTran->ScanCode1);
	    switch (pTran->ScanCode2) {
		case DIK_LSHIFT:   
		    PCmod = 1;
		    break;
		case DIK_LCONTROL:
		    PCmod = 2;
		    break;
		case DIK_LMENU:
		    PCmod = 3;
		    break;
		}
	}

	 
	if (pSC == NULL) return NULL;

	// Determine CoCo key and modifier
    if (pTran->Row2 != 0) {
        if (pTran->Row1 == 64) {
		    if (pTran->Col1 == 7) {         // shift
			    CCmod = 1;
		    } else if (pTran->Col1 == 4) {  // ctrl
			    CCmod = 2;
		    } else if (pTran->Col1 == 3) {  // alt
			    CCmod = 3;
		    }
		}
	}
	
    if ((pTran->Row2 != 0) & (CCmod != 0)) {
		pCC = cctable_rowcol_lookup(pTran->Row2, pTran->Col2); 
	} else {
		pCC = cctable_rowcol_lookup(pTran->Row1, pTran->Col1); 
        if (pTran->Row2 == 64) {
		    if (pTran->Col2 == 7) {
			    CCmod = 1;
		    } else if (pTran->Col2 == 4) {
			    CCmod = 2;
		    } else if (pTran->Col2 == 3) {
			    CCmod = 3;
		    }
	    }
    }	
	if (pCC == NULL) return NULL;

	sprintf(txt," DIK_%-12s %d COCO_%-10s %d\n",
			pSC->keyname, PCmod, pCC->keyname, CCmod);

	return txt;
}

//-----------------------------------------------------
// Show message box for custom keymap load errors
//-----------------------------------------------------

void ShowMapError(int lnum, char *msg)
{
	if (UserWarned < 8) {
        char fullmsg[128];
        sprintf(fullmsg,"Line %d %s",  lnum,msg);
        MessageBox(hKeyMapDlg,fullmsg,"Error",  0);
	} else if (UserWarned == 8) {
        MessageBox(hKeyMapDlg,"Too many errors to show",  "Error",  0);
	}
    UserWarned++;
}

//-----------------------------------------------------
// Lookup sctable by scan code name. Binary search.
//-----------------------------------------------------
static struct 
PCScanCode * scantable_keyname_lookup(char * keyname) 
{
    int first = 0;
    int last = (sizeof(sctable)/sizeof(sctable[0]))-1;
    int mid = last / 2;

	int cmp = strncmp(keyname,"DIK_",  4);
	if (cmp != 0) return NULL;

	while (first < last) {
        cmp = strcmp(keyname+4,sctable[mid].keyname);
		if (cmp == 0) return &sctable[mid];
		if (cmp > 0) {
            first = mid + 1;
        } else {
            last = mid;
        }
        mid = (first+last)/2;
    }
    return NULL;
}

//-----------------------------------------------------
// Lookup sctable by numeric scancode.
//-----------------------------------------------------
static struct
PCScanCode * scantable_scancode_lookup(int ScanCode)
{
    int ndx;

	// Build sctable_ndx on first call
	if (*sctable_ndx == 0) {
	    // Init sctable_ndx with unsupported entries
		for (int sc=0; sc<256; sc++) sctable_ndx[sc] = -1;
		// Populate valid sctable_ndx entries 
	    ndx = 0;
		while (sctable[ndx].ScanCode > 0) {
			sctable_ndx[sctable[ndx].ScanCode] = ndx;
			ndx++;
		}
	}

	// Lookup using sctable_ndx
	ScanCode = ScanCode & 0XFF;  // impose good scan code range 
	ndx = sctable_ndx[ScanCode]; // get index
	if (ndx < 0) return NULL;    // return NULL if unsupported
	return &sctable[ndx];        // else return pointer to selected entry
}

//-----------------------------------------------------
// Lookup CoCo key by keyname.  Binary search.
//-----------------------------------------------------
static struct 
CoCoKey * cocotable_keyname_lookup(char * keyname)
{
    int first = 0;

	int last = (sizeof(cctable)/sizeof(cctable[0]))-1; // 56
    int mid = last / 2;

	int cmp = strncmp(keyname,"COCO_",  5);
    if (cmp != 0) return NULL;

	while (first < last) {
        cmp = strcmp(keyname+5,cctable[mid].keyname);
        if (cmp == 0) return &cctable[mid];
		if (cmp > 0) {
            first = mid + 1;
        } else {
            last = mid;
        }
        mid = (first+last)/2;
    }
    return NULL;
}

//-----------------------------------------------------
// Lookup CoCo key by button id.  Sequential search.
//-----------------------------------------------------
static struct 
CoCoKey * cctable_keyid_lookup(int id) 
{
	struct CoCoKey *p;
	if (CC_KeySelected) {
        p = cctable;
		while (p->id > 0) { 
			if (p->id == CC_KeySelected) {
				return p;
				break;
			}
			p++; 
    	}
	}
	return NULL;
}

//-----------------------------------------------------
// Lookup CoCo key by row mask and col.
//-----------------------------------------------------
static struct 
CoCoKey * cctable_rowcol_lookup(unsigned char RowMask, unsigned char Col)
{
	unsigned long Row = 0;
	unsigned long Mask = RowMask;
	int ndx;

	// Convert RowMask, Col to index cctable_ndx
	_BitScanForward(&Row,Mask);
	ndx = Col + (Row<<3);
	if (ndx > 55) return NULL;
	return &cctable[cctable_ndx[ndx]];
}

/**************************************************/
/*              Keymap Edit Dialog                */
/**************************************************/

BOOL CALLBACK KeyMapProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
//	PrintLogC("%04x %08x %08x\n",msg,wParam,lParam);
	switch (msg) {
	case WM_INITDIALOG:
        return InitKeymapDialog(hWnd);
	case WM_CLOSE:
		return EndDialog(hWnd,wParam);
	case WM_CTLCOLORSTATIC:
		return SetControlFont(wParam,lParam);
	case WM_COMMAND:
		switch(LOWORD(wParam)) {
        case IDC_KEYMAP_EXIT:
			return EndDialog(hWnd,wParam);
		case IDC_LOAD_KEYMAP:
			return SelectCustKeymapFile();
		case IDC_SAVE_KEYMAP:
			return SaveCustKeymap();
		case IDC_SET_CUST_KEYMAP:
			return SetCustomKeymap();
	    case IDC_CLR_CUST_KEYMAP:
			return ClrCustomKeymap();
		default:
			return Process_CoCoKey(LOWORD(wParam));
		}
	}
    return FALSE; 
}

//-----------------------------------------------------
// Initialize key map dialog
//-----------------------------------------------------
BOOL InitKeymapDialog(HWND hWnd) 
{
	// Save dialog window handle as global
	hKeyMapDlg = hWnd;

	// Save handles for data display controls as globals
	hText_CC = GetDlgItem(hWnd,IDC_CCKEY_TXT);
	hText_PC = GetDlgItem(hWnd,IDC_PCKEY_TXT);
    hText_MapFile = GetDlgItem(hWnd,IDC_KEYMAP_FILE);

	if (hfTextBox == NULL) 
		hfTextBox = CreateFont (14, 0, 0, 0, FW_MEDIUM, FALSE, 
				FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, 
				CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
				DEFAULT_PITCH | FF_DONTCARE, TEXT("Tahoma"));

    if (hbrTextBox == NULL)
        hbrTextBox = CreateSolidBrush(RGB(255,255,255));

	// Initialize selected keys information
	CC_KeySelected = 0;
	CC_ModSelected = 0;
	PC_KeySelected = 0;
	PC_ModSelected = 0;
    CoCoKeySet = 0;
	CoCoModSet = 0;
    
	EnableWindow(GetDlgItem(hKeyMapDlg,IDC_SAVE_KEYMAP),KeyMapChanged);
	EnableWindow(GetDlgItem(hKeyMapDlg,IDC_SET_CUST_KEYMAP),FALSE);
	EnableWindow(GetDlgItem(hKeyMapDlg,IDC_CLR_CUST_KEYMAP),FALSE);

    // keymap filename
	SendMessage(hText_MapFile, WM_SETTEXT, 0, (LPARAM)GetKeyMapFilePath());

	// Subclass hText_PC to handle PC keyboard
	Text_PCproc = (WNDPROC)GetWindowLongPtr(hText_PC, GWLP_WNDPROC);
	SetWindowLongPtr(hText_PC, GWLP_WNDPROC, (LONG_PTR)SubText_PCproc);

	SetDialogFocus(hText_PC);

	return FALSE;
}

//-----------------------------------------------------
// Set font for text boxes
//-----------------------------------------------------
BOOL SetControlFont(WPARAM wParam, LPARAM lParam) {
	if (((HWND) lParam == hText_PC) |
	    ((HWND) lParam == hText_CC) |
	    ((HWND) lParam == hText_MapFile)) {
	    SelectObject((HDC)wParam,hfTextBox); 
        return (INT_PTR)hbrTextBox;
	}	
	return FALSE;
}

//-----------------------------------------------------
// Select custom keymap file
//-----------------------------------------------------
BOOL SelectCustKeymapFile() {

    OPENFILENAME ofn ;
    char FileSelected[MAX_PATH];
	strncpy (FileSelected,GetKeyMapFilePath(),MAX_PATH);

    // Prompt user for filename
    memset(&ofn,0,sizeof(ofn));
    ofn.lStructSize     = sizeof (OPENFILENAME) ;
    ofn.hwndOwner       = hKeyMapDlg;
    ofn.lpstrFilter     = "Keymap Files\0*.keymap\0\0";
    ofn.nFilterIndex    = 1;
    ofn.lpstrFile       = FileSelected;
    ofn.nMaxFile        = MAX_PATH;
    ofn.lpstrFileTitle  = NULL;
    ofn.nMaxFileTitle   = MAX_PATH;
    ofn.lpstrInitialDir = AppDirectory();
    ofn.lpstrTitle      = TEXT("Select Keymap file");
    ofn.Flags           = OFN_HIDEREADONLY
						| OFN_PATHMUSTEXIST;

    if ( GetOpenFileName (&ofn)) {
		// Load or create new keymap file
	    if (ofn.nFileExtension==0) strcat(FileSelected,".keymap");
	    if (PathFileExists(FileSelected)) {
            LoadCustomKeyMap(FileSelected);
		} else {
			char txt[MAX_PATH+32];
			strcpy (txt,FileSelected);
			strcat (txt," does not exist. Create it?");
		    if (MessageBox(hKeyMapDlg,txt,"Warning",MB_YESNO)==IDYES) 
				WriteKeymap(FileSelected);
		}
        SetKeyMapFilePath(FileSelected);

		strncpy (GetKeyMapFilePath(),FileSelected,MAX_PATH);
	    SendMessage(hText_MapFile, WM_SETTEXT, 0, (LPARAM)GetKeyMapFilePath());
	}
	
	SetDialogFocus(hText_PC);
	return TRUE;
}

//-----------------------------------------------------
// Save custom keymap file
//-----------------------------------------------------
BOOL SaveCustKeymap() {
    if (WriteKeymap(GetKeyMapFilePath())) KeyMapChanged = FALSE;
	EnableWindow(GetDlgItem(hKeyMapDlg,IDC_SAVE_KEYMAP),KeyMapChanged);
	SetDialogFocus(hText_PC);
	return TRUE;
}

//-----------------------------------------------------
// Set mapping for currently selected key
//-----------------------------------------------------
BOOL SetCustomKeymap() {

	int ModCol;
	int ModRow;
	static struct CoCoKey *p;

	if (pKeyTran == NULL) {

		// If no entry and no coco key to map do nothing
		if ( (CC_KeySelected + CC_ModSelected) == 0 ) return TRUE;

		p = cctable_keyid_lookup(CC_KeySelected);
		int len = 0;
	    pKeyTran = keyTranslationsCustom;

		while (pKeyTran->ScanCode1 > 0) {
		    len++;
		    pKeyTran++;
	    }
	    if (len >= MAX_CTRANSTBLSIZ) {
			// table overflow!!
		    // TODO: Warn user
			return TRUE;
		}
		pKeyTran->ScanCode1 = PC_KeySelected;
		pKeyTran->ScanCode2 = ModScanCode[PC_ModSelected];
	}

    // Generate row and col for modifier
	switch(CC_ModSelected) {
    case 1:
        ModCol = 7;
		ModRow = 64;
        break;
    case 2:
        ModCol = 4;
		ModRow = 64;
        break;
    case 3:
        ModCol = 3;
		ModRow = 64;
        break;
	default:
	    ModCol = 0;
	    ModRow = 0;
		break;
	}

	// Update custom translation table
    if (CC_KeySelected != 0) {  // BtnId
		p = cctable_keyid_lookup(CC_KeySelected);
		if (p) {
			pKeyTran->Row1 = 1<<(p->row);
		    pKeyTran->Col1 = p->col;
		} else {
			// Internal error, invalid Map!
		    // TODO: Warn user
			return TRUE;
		}
		pKeyTran->Row2 = ModRow;
		pKeyTran->Col2 = ModCol;

	} else if (CC_ModSelected != 0) {
		pKeyTran->Row1 = ModRow;
		pKeyTran->Col1 = ModCol;

	} else {
		// Remove PC key from translation table
		//TODO: Warn user key is being unmapped?
		while (pKeyTran->ScanCode1 > 0) {
		    memcpy(pKeyTran,pKeyTran+1,sizeof(keytranslationentry_t));
		    pKeyTran++;
	    }
	}

    // Update runtime table
	if (GetKeyboardLayout() == kKBLayoutCustom)
			vccKeyboardBuildRuntimeTable((keyboardlayout_e) kKBLayoutCustom);

	// Disable set button
	EnableWindow(GetDlgItem(hKeyMapDlg,IDC_SET_CUST_KEYMAP),FALSE);

	// Set map changed flag
    KeyMapChanged = TRUE;
	EnableWindow(GetDlgItem(hKeyMapDlg,IDC_SAVE_KEYMAP),KeyMapChanged);

    // Reset focus
	SetDialogFocus(hText_PC);
    return TRUE;
}

//-----------------------------------------------------
// Clear current key selection
//-----------------------------------------------------
BOOL ClrCustomKeymap() {
	PC_KeySelected = 0;
	PC_ModSelected = 0;
	ShowPCkey();
	SetCoCokey();
	EnableWindow(GetDlgItem(hKeyMapDlg,IDC_SET_CUST_KEYMAP),FALSE);
	SetDialogFocus(hText_PC);
	return TRUE;
}

//-----------------------------------------------------
// Select or toggle coco shift, ctrl, or alt (1, 2, or 3)
//-----------------------------------------------------
void CoCoModifier(int Modifier)
{
	if (CC_ModSelected != Modifier) {
		CC_ModSelected = Modifier;
	} else {
		CC_ModSelected = 0;
	}
}

//-----------------------------------------------------
// SetFocus for key map dialog
//-----------------------------------------------------
void SetDialogFocus(HWND hCtrl) {
    SendMessage(hKeyMapDlg,WM_NEXTDLGCTL,(WPARAM)hCtrl,TRUE);
}

// Select coco key(s) as per button id
void CoCoSelect(int BtnId) 
{
	switch (BtnId) {
	case IDC_KEYBTN_RSHIFT:
	case IDC_KEYBTN_LSHIFT:
		CoCoModifier(1);
		break;
	case IDC_KEYBTN_CTRL:
		CoCoModifier(2);
        break;
	case IDC_KEYBTN_ALT:
		CoCoModifier(3);
        break;
	default:
    	if (CC_KeySelected != BtnId) {
			CC_KeySelected = BtnId;
		} else {
			CC_KeySelected = 0;
		}
	}
}

//-----------------------------------------------------
// Handle CoCo key button press
//-----------------------------------------------------
BOOL Process_CoCoKey(int BtnId)
{

	// Return FALSE if not a CoCo key button 
	if (BtnId < IDC_KEYBTN_FIRST) return FALSE;
	if (BtnId > IDC_KEYBTN_LAST) return FALSE;
	SetDialogFocus(hText_PC);
    if ( (PC_KeySelected != 0 ) | ( PC_ModSelected !=0 ) ) {
	    CoCoSelect(BtnId);
	    ShowCoCoKey();
	    EnableWindow(GetDlgItem(hKeyMapDlg,IDC_SET_CUST_KEYMAP),TRUE);
	} else {
		MessageBox(hKeyMapDlg,"Press PC key to map first","Error",0);
	}
	return TRUE;
}

//-----------------------------------------------------
// Set up/down state of modifier key button(s)
//-----------------------------------------------------
void SetModKeyState (int mod, int state) 
{
    switch (mod) {
	case 1:
	    Button_SetState(GetDlgItem(hKeyMapDlg,IDC_KEYBTN_LSHIFT),state);
	    Button_SetState(GetDlgItem(hKeyMapDlg,IDC_KEYBTN_RSHIFT),state);
		break;
	case 2:
	    Button_SetState(GetDlgItem(hKeyMapDlg,IDC_KEYBTN_CTRL),state);
		break;
	case 3:
	    Button_SetState(GetDlgItem(hKeyMapDlg,IDC_KEYBTN_ALT),state);
    }
}

//-----------------------------------------------------
// Display CoCo keys 
//-----------------------------------------------------
void ShowCoCoKey()
{
	char str[64];
	char * keytxt = "";
	char * modtxt = "";
	struct CoCoKey *p;

	// set coco keyboard buttons 
	if (CC_KeySelected != CoCoKeySet) {
		if (CoCoKeySet) {
	       Button_SetState(GetDlgItem(hKeyMapDlg,CoCoKeySet),0);
        }
		if (CC_KeySelected) {
	       Button_SetState(GetDlgItem(hKeyMapDlg,CC_KeySelected),1);
		}
	    CoCoKeySet = CC_KeySelected;
	}
		  
	if (CC_ModSelected != CoCoModSet ) {
        SetModKeyState (CoCoModSet, 0);
		SetModKeyState (CC_ModSelected, 1);
		CoCoModSet = CC_ModSelected;
	}

	// Show coco keys names in editbox
	if (CC_KeySelected) {
        p = cctable_keyid_lookup(CC_KeySelected);
		if (p != NULL) {
			if (p->label != NULL) {
			    keytxt = p->label;
		    } else {
			    keytxt = p->keyname;   
			}
	   	}
	}

	sprintf(str,"%s %s",ModName[CC_ModSelected],keytxt);
	SendMessage(hText_CC, WM_SETTEXT, 0, (LPARAM)str);
}

//-----------------------------------------------------
// Handle messages for PC key name text box. This will 
// get all Keyboard events while dialog is running.
//-----------------------------------------------------

LRESULT CALLBACK 
SubText_PCproc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int ScanCode;
    int Extended;
	switch (msg) {
	case WM_GETDLGCODE:
		return DLGC_WANTALLKEYS;
	case WM_SYSKEYUP:
	case WM_KEYUP:
	case WM_CHAR:
		return 0;
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
        ScanCode=(lParam >> 16) & 0xFF;
        Extended=(lParam >> 24) & 1;
		if (Extended && (ScanCode!=DIK_NUMLOCK)) ScanCode += 0x80;
	    switch (ScanCode) {
		case DIK_LWIN:
			return 0;
	    case DIK_LSHIFT:
	    case DIK_RSHIFT:
			SetPCmod(1);
			break;
		case DIK_LCONTROL:
	    case DIK_RCONTROL:
			SetPCmod(2);
			break;
	    case DIK_LMENU:
	    case DIK_RMENU:
			SetPCmod(3);
			break;
	    default:
			if (PC_KeySelected == ScanCode) {
				PC_KeySelected = 0;
			} else {
		       PC_KeySelected = ScanCode;
			}
			break;
	    }
		ShowPCkey();
        SetCoCokey();
		return 0;
	}
	// Stock text control does everything else
	return (CallWindowProc(Text_PCproc,hWnd,msg,wParam,lParam));
}

//-----------------------------------------------------
// Select or toggle PC shift, ctrl, or alt 
//-----------------------------------------------------
void SetPCmod(int ModNum)
{
	if (ModNum == PC_ModSelected) {
		PC_ModSelected = 0;
	} else {
		PC_ModSelected = ModNum;
	}
}

//-----------------------------------------------------
// Display PC keys names that are selected
//-----------------------------------------------------
void ShowPCkey()
{
	char str[64];
	char * keytxt = "";
	char * modtxt = "";
    struct PCScanCode * entry;
	if (PC_KeySelected>0) { 
		entry = scantable_scancode_lookup(PC_KeySelected);
		if (entry == NULL) {
			keytxt = "";
			PC_KeySelected=0;
		} else {
			keytxt = entry->label;
			if (keytxt == NULL) keytxt = entry->keyname;
		}
	}
	sprintf(str,"%s %s",ModName[PC_ModSelected],keytxt);
	SendMessage(hText_PC, WM_SETTEXT, 0, (LPARAM)str);

	if ( (PC_KeySelected > 0) | (PC_ModSelected > 0) ) {
		EnableWindow(GetDlgItem(hKeyMapDlg,IDC_CLR_CUST_KEYMAP),TRUE);
	} else {
		EnableWindow(GetDlgItem(hKeyMapDlg,IDC_CLR_CUST_KEYMAP),FALSE);
	}
}

//-----------------------------------------------------
// Set CoCo key as per current map 
//-----------------------------------------------------
void SetCoCokey()
{
    static struct CoCoKey * pCoCoKey;

	// Clear selected coco keys
    CC_KeySelected = 0;
	CC_ModSelected = 0;

    // Scan Custom translation table for match on PC key(s) selected
	int mod = ModScanCode[PC_ModSelected];
	int s1 = PC_KeySelected + ( mod << 8 );
	int s2 = ( PC_KeySelected << 8 ) + mod; 

	pKeyTran = keyTranslationsCustom;
	while (pKeyTran->ScanCode1 != 0) { 
        int m = pKeyTran->ScanCode1 + (pKeyTran->ScanCode2 << 8);
		if ( (m == s1) | (m == s2) ) break;
		pKeyTran++;
	}

    // If match found select mapped coco keys (if any)
	if (pKeyTran->ScanCode1 != 0) {
		if (pKeyTran->Row1 != 0) {
			pCoCoKey = cctable_rowcol_lookup(pKeyTran->Row1, pKeyTran->Col1);
	        if (pCoCoKey) CoCoSelect(pCoCoKey->id);
		}
		if (pKeyTran->Row2 != 0) {
	        pCoCoKey = cctable_rowcol_lookup(pKeyTran->Row2, pKeyTran->Col2);
	        if (pCoCoKey) CoCoSelect(pCoCoKey->id);
		}
	} else {
		pKeyTran = NULL; 
	}

    // Disable set button
	EnableWindow(GetDlgItem(hKeyMapDlg,IDC_SET_CUST_KEYMAP),FALSE);

	// Show results
    ShowCoCoKey();
}


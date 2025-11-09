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

#include "BuildConfig.h"
#include <Windows.h>
#include "defines.h"
#include "joystickinput.h"
#include <vcc/utils/logger.h>

#if USE_DEBUG_RAMP
#include "IDisplayDebug.h"
#endif // USE_DEBUG_RAMP

// JoyStick configuration
// These were renamed from Left and Right to preserve maintainer sanity
//   JS.UseMouse:  0=keyboard,1=mouse,2=audio,3=joystick
//   JS.HiRes:     0=lowres,1=software,2=tandy,3=ccmax
JoyStick LeftJS;
JoyStick RightJS;

/*
/ Coco joysticks had restricted resolution due to use of 6 bit DAC compare
/ But a 640x225 screen resolution is possible for CoCo3 screen.
/ As a result many programs desire a higher resolution pointing device
/
/ Higher than 6 bit resolution can be obtained using 6 bit DAC with
/ special code that rapidly compares joystick values to the DAC voltage
/ as it changes also CocoMax cart used a ADC that retuned an 8 bit value.
/
/ Windows HID joysticks and mice have at least 16 bit resolution. So
/ code has been added to support higher resolution using one of the
/ two above methods.
/
/ To allow for increased resolution stick values are stored in
/ fourteen bits.  This means the second byte contains a 0-63
/ value which makes initial DAC comparisons easier.
*/

// for DoubleSpeedFlag
extern SystemState EmuState;

// Clock cycles since Joystick ramp started
extern int JS_Ramp_Clock=0;

// Software high resolution disabled
// DAC change is used for software high resolution joystick.
// It is used to simulate the normal DAC comparator time delay.
// static int DAC_Change;
// The following rising and falling values per MPU cycle were scaled
// from "deep scan" figure in "HI-RES INTERFACE" by Kowalski, Gault,
// and Marentes.  Unfortunatly the magic program to make software high
// resolution work uses the one cycle timing difference between LDB ,X
// (4 cycles) and LDB, $FF00 (5 cycles) to get 10x resolution. Because
// of the way the MPU is emulated Vcc does not know the addressing mode
// at the time of the memory access and DAC compare. It therefore can
// only supply 5 meaningful samples instead of 10. As a result the best
// Vcc can do is 5x resolution.
//static int DAC_Rising[10] ={256,256,256,256, 48, 32, 24, 20,  0,  0};
//static int DAC_Falling[10]={256,256,256,256,224,116, 64, 56, 48,  0};
//  static int DAC_Rising[10] ={256,256,256,256, 40, 40, 22, 22,  0,  0};
//  static int DAC_Falling[10]={256,256,256,256,160,160, 60, 60, 24, 24};

// Hires ramp flag
static int JS_Ramp_On;

// Hires ramp constants. Determined during testing
constexpr auto TANDYRAMPMIN  = 1200u;
constexpr auto TANDYRAMPMAX  = 10950u;
constexpr auto TANDYRAMPMUL  = 37u;
constexpr auto CCMAXRAMPMIN  = 800u;
constexpr auto CCMAXRAMPMAX  = 14000u;
constexpr auto CCMAXRAMPMUL  = 21u;

static int sticktarg = 0;    // Target stick cycle count

// Joystick values  (0-16383)
constexpr auto STICKMAX = 16383u;
constexpr auto STICKMID = 8191u;
unsigned int LeftStickX = STICKMID;
unsigned int LeftStickY = STICKMID;
unsigned int RightStickX = STICKMID;
unsigned int RightStickY = STICKMID;

static unsigned char LeftStickNumber = 0;
static unsigned char RightStickNumber = 0;

// Button states
static unsigned char LeftButton1Status = 0;
static unsigned char RightButton1Status = 0;
static unsigned char LeftButton2Status = 0;
static unsigned char RightButton2Status = 0;

// FIXME Direct input not working for ARM - disable joysticks for arm builds
#ifdef _M_ARM
unsigned int Joysticks[MAXSTICKS] = {nullptr};
#else
static LPDIRECTINPUTDEVICE8 Joysticks[MAXSTICKS];
#endif

char StickName[MAXSTICKS][STRLEN];
static unsigned char JoyStickIndex=0;

#ifdef _M_ARM
#else
static LPDIRECTINPUT8 di;
BOOL CALLBACK enumCallback(const DIDEVICEINSTANCE* , VOID* );
BOOL CALLBACK enumAxesCallback(const DIDEVICEOBJECTINSTANCE* , VOID* );
#endif

static unsigned char CurrentStick;

unsigned int get_pot_value(unsigned char);
inline int vccJoystickType();

/*****************************************************************************/
// Locate connected joysticks.  Called by config.c
int EnumerateJoysticks()
{
#ifdef _M_ARM
	return(0);
#else
    HRESULT hr;
    JoyStickIndex=0;
    if (FAILED(hr = DirectInput8Create(GetModuleHandle(nullptr),
                    DIRECTINPUT_VERSION,IID_IDirectInput8,(VOID**)&di,nullptr)))
        return 0;

    if (FAILED(hr = di->EnumDevices(DI8DEVCLASS_GAMECTRL,
                    enumCallback,nullptr,DIEDFL_ATTACHEDONLY)))
        return 0;

    return JoyStickIndex;
#endif
}

/*****************************************************************************/
#ifndef _M_ARM
BOOL CALLBACK enumCallback(const DIDEVICEINSTANCE* instance, VOID* /*context*/)
{
    HRESULT hr;
    hr = di->CreateDevice(instance->guidInstance, &Joysticks[JoyStickIndex],nullptr);
    strncpy(StickName[JoyStickIndex],instance->tszProductName,STRLEN);
    JoyStickIndex++;
    return(JoyStickIndex<MAXSTICKS);
}
#endif

/*****************************************************************************/
// Init joystick called by config.c
bool InitJoyStick (unsigned char StickNumber)
{
#ifndef _M_ARM
    HRESULT hr;
    CurrentStick=StickNumber;
    if (Joysticks[StickNumber]==nullptr)
        return false;

    if (FAILED(hr= Joysticks[StickNumber]->SetDataFormat(&c_dfDIJoystick2)))
        return false;

    if (FAILED(hr= Joysticks[StickNumber]->EnumObjects(enumAxesCallback,nullptr,DIDFT_AXIS)))
        return false;

	return true; //return true on success
#else
	return(1);
#endif
}

/*****************************************************************************/
#ifndef _M_ARM
BOOL CALLBACK enumAxesCallback(const DIDEVICEOBJECTINSTANCE* instance, VOID* /*context*/)
{
    DIPROPRANGE propRange;
    propRange.diph.dwSize = sizeof(DIPROPRANGE);
    propRange.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    propRange.diph.dwHow = DIPH_BYID;
    propRange.diph.dwObj = instance->dwType;
    propRange.lMin=0;
    propRange.lMax=0xFFFF;

    if (FAILED(Joysticks[CurrentStick]->SetProperty(DIPROP_RANGE, &propRange.diph)))
        return DIENUM_STOP;

    return DIENUM_CONTINUE;
}
#endif

/*****************************************************************************/
// Called by get_pot_value to read data from physical joystick
#ifndef _M_ARM
HRESULT
JoyStickPoll(DIJOYSTATE2 *js,unsigned char StickNumber)
{
    HRESULT hr;
    if (Joysticks[StickNumber] ==nullptr)
        return (S_OK);

    hr=Joysticks[StickNumber]->Poll();
    if (FAILED(hr)) {
        hr=Joysticks[StickNumber]->Acquire();
        while (hr == DIERR_INPUTLOST)
            hr = Joysticks[StickNumber]->Acquire();

        if (hr == DIERR_INVALIDPARAM)  //|| (hr == DIERR_NOTINITALIZED
            return(E_FAIL);

        if (hr == DIERR_OTHERAPPHASPRIO)
            return(S_OK);
    }

    if (FAILED(hr= Joysticks[StickNumber]->GetDeviceState(sizeof(DIJOYSTATE2), js)))
        return hr;
    return(S_OK);
}
#endif

/*****************************************************************************/
// inline function returns joystick emulation type
inline int vccJoystickType() {
    return (GetMuxState() & 2) ? LeftJS.HiRes : RightJS.HiRes;
}

/*****************************************************************************/
// Called by mc6821 when $FF20 is written
void vccJoystickStartTandy(unsigned char next)
{
	if (vccJoystickType() == 2) {
        if ( next == 2 ) {
            JS_Ramp_On = 1;
            sticktarg = 0;
            JS_Ramp_Clock = 0;
        }
    }
}

/*****************************************************************************/
// Called by mc6821 when zero is written to $FF00
void
vccJoystickStartCCMax()
{
    if (vccJoystickType() != 3) return;
    JS_Ramp_On = 1;
    sticktarg = 0;
    JS_Ramp_Clock = 0;
}

/*****************************************************************************/
// Called by keyboard.c when FF00 is read to add joystick bits to scancode
unsigned char
vccJoystickGetScan(unsigned char code)
{
    unsigned int StickValue;
    unsigned char axis = GetMuxState(); // 0 rx, 1 ry, 2 lx, 3 ly

    int JS_Type = vccJoystickType();
    StickValue = get_pot_value(axis);

    // If hires hardware joystick
    if (JS_Type > 1) {
        if (JS_Ramp_On) {
            // if target is zero set it based on current stick value
            if (sticktarg == 0) {
                if (JS_Type == 2) {
                    sticktarg = TANDYRAMPMIN + ((StickValue*TANDYRAMPMUL)>>6);
                    if(sticktarg>TANDYRAMPMAX) sticktarg=TANDYRAMPMAX;
                    // cut target in half if double speed
                    if (!EmuState.DoubleSpeedFlag) sticktarg = sticktarg/2;

                } else if (JS_Type == 3) {  //ccmax
                    sticktarg = CCMAXRAMPMIN + ((StickValue*CCMAXRAMPMUL)>>6);
                    if(sticktarg>CCMAXRAMPMAX) sticktarg=CCMAXRAMPMAX;
                }
            }
        }

#if USE_DEBUG_RAMP
        int x = JS_Ramp_Clock;
        float xx = 600.0f * x / (TANDYRAMPMAX-TANDYRAMPMIN);
        float yy = 20.0f * axis;
        DebugDrawLine(xx, yy, xx, yy+18, VCC::ColorBlue);
#endif // USE_DEBUG_RAMP

        // If clock exceeds target set compare bit and stop ramp
        if (JS_Ramp_Clock > sticktarg) {
            code |= 0x80;
            JS_Ramp_On = 0;
        }

    // else standard or software hires
    } else if (StickValue != 0) {  // OS9 joyin needs this for koronis rift
        unsigned int val = DACState();
        if (StickValue >= val) {
            code |= 0x80;
        }
    }

    if (LeftButton1Status == 1) {
        //Left Joystick Button 1 Down?
        code = code & 0xFD;
    }

    if (RightButton1Status == 1) {
        //Right Joystick Button 1 Down?
        code = code & 0xFE;
    }

    if (LeftButton2Status == 1) {
        //Left Joystick Button 2 Down?
        code = code & 0xF7;
    }

    if (RightButton2Status == 1) {
        //Right Joystick Button 2 Down?
        code = code & 0xFB;
    }
    return code;
}

/*****************************************************************************/
// Called by vcc.c on mouse moves. x and y range 0-3fff

void joystick(unsigned int x,unsigned int y)
{
    if (x>STICKMAX) x=STICKMAX;
    if (y>STICKMAX) y=STICKMAX;

    if (LeftJS.UseMouse==1) {
        LeftStickX=x;
        LeftStickY=y;
    }
    if (RightJS.UseMouse==1) {
        RightStickX=x;
        RightStickY=y;
    }
    return;
}

/*****************************************************************************/
void
SetStickNumbers(unsigned char Temp1,unsigned char Temp2)
{
    LeftStickNumber=Temp1;
    RightStickNumber=Temp2;
    return;
}

/*****************************************************************************/
// called by pia0_read->vccKeyboardGetScan->vccJoystickGetScan
unsigned int
get_pot_value(unsigned char pot)
{
#ifndef _M_ARM
	DIJOYSTATE2 Stick1 = { 0 };

    // Poll left joystick if attached
    if (LeftJS.UseMouse==3) {
        JoyStickPoll(&Stick1,LeftStickNumber);
        LeftStickX = (unsigned short)Stick1.lX>>2;    // Not tested
        LeftStickY = (unsigned short)Stick1.lY>>2;    // Not tested
        LeftButton1Status= Stick1.rgbButtons[0]>>7;
        LeftButton2Status= Stick1.rgbButtons[1]>>7;
    }

    // Poll right joystick if attached
    if (RightJS.UseMouse ==3) {
        JoyStickPoll(&Stick1,RightStickNumber);
        RightStickX = (unsigned short)Stick1.lX>>2;   // Not tested
        RightStickY = (unsigned short)Stick1.lY>>2;   // Not tested
        RightButton1Status= Stick1.rgbButtons[0]>>7;
        RightButton2Status= Stick1.rgbButtons[1]>>7;
    }
#endif

    switch (pot) {
    case 0:
        return RightStickX;
        break;

    case 1:
        return RightStickY;
        break;

    case 2:
        return LeftStickX;
        break;

    case 3:
        return LeftStickY;
        break;
    }
    return 0;
}

/*****************************************************************************/
// Called by keyboard.h to handle keyboard joysticks

unsigned char
SetMouseStatus(unsigned char ScanCode,unsigned char Phase)
{
    unsigned char ReturnValue=ScanCode;

    // Mask scan code high bit to accept extended keys.
    unsigned char TempCode = ScanCode & 0x7F;

    switch (Phase) {
    case 0:
        if (LeftJS.UseMouse==0) {

            if (TempCode==LeftJS.Left) {
                LeftStickX=STICKMID;
                ReturnValue=0;
            }
            if (TempCode==LeftJS.Right) {
                LeftStickX=STICKMID;
                ReturnValue=0;
            }
            if (TempCode==LeftJS.Up) {
                LeftStickY=STICKMID;
                ReturnValue=0;
            }
            if (TempCode==LeftJS.Down) {
                LeftStickY=STICKMID;
                ReturnValue=0;
            }
            if (TempCode==LeftJS.Fire1) {
                LeftButton1Status=0;
                ReturnValue=0;
            }
            if (TempCode==LeftJS.Fire2) {
                LeftButton2Status=0;
                ReturnValue=0;
            }
        }

        if (RightJS.UseMouse==0) {
            if (TempCode==RightJS.Left) {
                RightStickX=STICKMID;
                ReturnValue=0;
            }
            if (TempCode==RightJS.Right) {
                RightStickX=STICKMID;
                ReturnValue=0;
            }
            if (TempCode==RightJS.Up) {
                RightStickY=STICKMID;
                ReturnValue=0;
            }
            if (TempCode==RightJS.Down) {
                RightStickY=STICKMID;
                ReturnValue=0;
            }
            if (TempCode==RightJS.Fire1) {
                RightButton1Status=0;
                ReturnValue=0;
            }
            if (TempCode==RightJS.Fire2) {
                RightButton2Status=0;
                ReturnValue=0;
            }
        }
        break;

    case 1:
        if (LeftJS.UseMouse==0) {
            if (TempCode==LeftJS.Left) {
                LeftStickX=0;
                ReturnValue=0;
            }
            if (TempCode==LeftJS.Right) {
                LeftStickX=STICKMAX;
                ReturnValue=0;
            }
            if (TempCode==LeftJS.Up) {
                LeftStickY=0;
                ReturnValue=0;
            }
            if (TempCode==LeftJS.Down) {
                LeftStickY=STICKMAX;
                ReturnValue=0;
            }
            if (TempCode==LeftJS.Fire1) {
                LeftButton1Status=1;
                ReturnValue=0;
            }
            if (TempCode==LeftJS.Fire2) {
                LeftButton2Status=1;
                ReturnValue=0;
            }
        }

        if (RightJS.UseMouse==0) {
            if (TempCode==RightJS.Left) {
                ReturnValue=0;
                RightStickX=0;
            }
            if (TempCode==RightJS.Right) {
                RightStickX=STICKMAX;
                ReturnValue=0;
            }
            if (TempCode==RightJS.Up) {
                RightStickY=0;
                ReturnValue=0;
            }
            if (TempCode==RightJS.Down) {
                RightStickY=STICKMAX;
                ReturnValue=0;
            }
            if (TempCode==RightJS.Fire1) {
                RightButton1Status=1;
                ReturnValue=0;
            }
            if (TempCode==RightJS.Fire2) {
                RightButton2Status=1;
                ReturnValue=0;
            }
        }
        break;
    }
    return ReturnValue;
}

/****************************************************************************/
// Called by vcc.c to set mouse joystick buttons
// Side=0 Left Button Side=1 Right Button State 1=Down

void
SetButtonStatus(unsigned char Side,unsigned char State)
{
    unsigned char Btemp;
    Btemp = (Side<<1) | State;
    if (LeftJS.UseMouse==1)
        switch (Btemp) {
        case 0:
            LeftButton1Status=0;
            break;

        case 1:
            LeftButton1Status=1;
            break;

        case 2:
            LeftButton2Status=0;
            break;

        case 3:
            LeftButton2Status=1;
            break;
        }

    if (RightJS.UseMouse==1)
        switch (Btemp) {
        case 0:
            RightButton1Status=0;
            break;

        case 1:
            RightButton1Status=1;
            break;

        case 2:
            RightButton2Status=0;
            break;

        case 3:
            RightButton2Status=1;
            break;
        }
}


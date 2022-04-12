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

#define DIRECTINPUT_VERSION 0x0800

#include <windows.h>
#include "defines.h"
#include "joystickinput.h"
#include "logger.h"

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

// Clock cycles since DAC written
extern int DAC_Clock=0;

// DAC change is used for software high resolution joystick.
// It is used to simulate the normal DAC comparator time delay.
extern int DAC_Change;

// Rising and falling values scaled from "deep scan" figure
// in "HI-RES INTERFACE" by Kowalski, Gault, and Marentes.
static int DAC_Rising[10] ={256,256,181, 81, 49,26,11, 4, 0,0};
static int DAC_Falling[10]={256,256,256,154,128,82,51,26,13,0};
static int JS_Hires=0;  // 0=lowres,1=software,2=tandy,3=ccmax
static int JS_Ramp_On;

// Hires ramp constants. Determined durring testing
#define TANDYRAMPMIN   1200
#define TANDYRAMPMAX  10950
#define TANDYRAMPMUL     37
#define CCMAXRAMPMIN    800
#define CCMAXRAMPMAX  14000
#define CCMAXRAMPMUL     21 

// Joystick values  (0-16383)
#define STICKMAX 16383
#define STICKMID 8191
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

static LPDIRECTINPUTDEVICE8 Joysticks[MAXSTICKS];
char StickName[MAXSTICKS][STRLEN];
static unsigned char JoyStickIndex=0;
static LPDIRECTINPUT8 di;
BOOL CALLBACK enumCallback(const DIDEVICEINSTANCE* , VOID* );
BOOL CALLBACK enumAxesCallback(const DIDEVICEOBJECTINSTANCE* , VOID* );
static unsigned char CurrentStick;

unsigned int get_pot_value(unsigned char);

/*****************************************************************************/
// Locate connected joysticks.  Called by config.c
int EnumerateJoysticks(void)
{
    HRESULT hr;
    JoyStickIndex=0;
    if (FAILED(hr = DirectInput8Create(GetModuleHandle(NULL),
                    DIRECTINPUT_VERSION,IID_IDirectInput8,(VOID**)&di,NULL)))
    return(0);

    if (FAILED(hr = di->EnumDevices(DI8DEVCLASS_GAMECTRL,
                    enumCallback,NULL,DIEDFL_ATTACHEDONLY)))
    return(0);
    return(JoyStickIndex);
}

/*****************************************************************************/
BOOL CALLBACK enumCallback(const DIDEVICEINSTANCE* instance, VOID* context)
{
    HRESULT hr;
    hr = di->CreateDevice(instance->guidInstance, &Joysticks[JoyStickIndex],NULL);
    strncpy(StickName[JoyStickIndex],instance->tszProductName,STRLEN);
    JoyStickIndex++;
    return(JoyStickIndex<MAXSTICKS);
}

/*****************************************************************************/
// Init joystick called by config.c
bool InitJoyStick (unsigned char StickNumber)
{
//  DIDEVCAPS capabilities;
    HRESULT hr;
    CurrentStick=StickNumber;
    if (Joysticks[StickNumber]==NULL)
        return(0);

    if (FAILED(hr= Joysticks[StickNumber]->SetDataFormat(&c_dfDIJoystick2)))
        return(0);

//  if (FAILED(hr= Joysticks[StickNumber]->SetCooperativeLevel(NULL, DISCL_EXCLUSIVE )))
//      return(0);

    //Fails for some reason Investigate this
//  if (FAILED(hr= Joysticks[StickNumber]->GetCapabilities(&capabilities)))
//      return(0);

    if (FAILED(hr= Joysticks[StickNumber]->EnumObjects(enumAxesCallback,NULL,DIDFT_AXIS)))
        return(0);
    return(1); //return true on success
}

/*****************************************************************************/
BOOL CALLBACK enumAxesCallback(const DIDEVICEOBJECTINSTANCE* instance, VOID* context)
{
    HWND hDlg= (HWND)context;
    DIPROPRANGE propRange;
    propRange.diph.dwSize = sizeof(DIPROPRANGE);
    propRange.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    propRange.diph.dwHow = DIPH_BYID;
    propRange.diph.dwObj = instance->dwType;
    propRange.lMin=0;
    propRange.lMax=0xFFFF;

    if (FAILED(Joysticks[CurrentStick]->SetProperty(DIPROP_RANGE, &propRange.diph)))
        return(DIENUM_STOP);

    return(DIENUM_CONTINUE);
}

/*****************************************************************************/
// Called by get_pot_value to read data from physical joystick
HRESULT
JoyStickPoll(DIJOYSTATE2 *js,unsigned char StickNumber)
{
    HRESULT hr;
    if (Joysticks[StickNumber] ==NULL)
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
        return(hr);

    return(S_OK);
}

extern SystemState EmuState; // for DoubleSpeedFlag
static int sticktarg = 0;    // Target stick cycle count


/*****************************************************************************/
// Called by mc6821 when zero is written to $FF00 to start the CCMAX joystick ramp
void
vccJoystickStartCCMax()
{
//    unsigned char axis;
//    axis = GetMuxState();       // 0 rx, 1 ry, 2 lx, 3 ly
//    if (GetMuxState() < 2) {
//        JS_Hires = RightJS.HiRes;
//    } else {
//        JS_Hires = LeftJS.HiRes;              // 0=lowres,1=software,2=tandy,3=ccmax
//    }

    // Determine if selected joystick is CCMAX high res
    JS_Hires = (GetMuxState()<2) ? RightJS.HiRes : LeftJS.HiRes;
    if (JS_Hires != 3) return;
    JS_Ramp_On = 1; // Set ramp on flag
    sticktarg = 0;  // Reset the ramp target
    DAC_Clock = 0;  // Reset the DAC timer
}

/*****************************************************************************/
// Called by mc6821 when 0x02 is written to $FF20 to start the Tandy joystick ramp
void
vccJoystickStartTandy()
{
//    unsigned char axis;
//    axis = GetMuxState();       // 0 rx, 1 ry, 2 lx, 3 ly
//    if (GetMuxState() < 2) {
//        JS_Hires = RightJS.HiRes;
//    } else {
//        JS_Hires = LeftJS.HiRes;              // 0=lowres,1=software,2=tandy,3=ccmax
//    }
    // A few pains to reduce spurious ramping

    // Determine if selected joystick is Tandy high res
    JS_Hires = (GetMuxState()<2) ? RightJS.HiRes : LeftJS.HiRes;
    if (JS_Hires != 2) return;
    JS_Ramp_On = 1;
    sticktarg = 0;  // Reset the ramp target
    DAC_Clock = 0;  // Reset the DAC timer
}

/*****************************************************************************/
// Called by keyboard.c to add joystick bits to scancode
unsigned char
vccJoystickGetScan(unsigned char code)
{
    unsigned int StickValue;
    unsigned int val;
    unsigned char axis = GetMuxState(); // 0 rx, 1 ry, 2 lx, 3 ly

    StickValue = get_pot_value(axis);

    // If hires hardware joystick
    if (JS_Hires > 1) {
        if (JS_Ramp_On) {
            // if target is zero set it based on current stick value
            if (sticktarg == 0) {
                if (JS_Hires == 2) {
                    sticktarg = TANDYRAMPMIN + ((StickValue*TANDYRAMPMUL)>>6);
                    if(sticktarg>TANDYRAMPMAX) sticktarg=TANDYRAMPMAX;
                    // cut target in half if double speed
                    if (!EmuState.DoubleSpeedFlag) sticktarg = sticktarg/2;

                } else if (JS_Hires == 3) {  //ccmax
                    sticktarg = CCMAXRAMPMIN + ((StickValue*CCMAXRAMPMUL)>>6);
                    if(sticktarg>CCMAXRAMPMAX) sticktarg=CCMAXRAMPMAX;
                }
            }
        }
        // If clock exceeds target set compare bit and stop ramp
        if (DAC_Clock > sticktarg) {
            code |= 0x80;
            JS_Ramp_On = 0;
        }

    // else standard or software hires
    } else if (StickValue != 0) {  // OS9 joyin needs this for koronis rift
        val = DACState();
        if ((JS_Hires==1) && (DAC_Clock < 10)) {
            if (DAC_Change == 1) {
                val -= DAC_Rising[DAC_Clock]*DAC_Change;
            } else if (DAC_Change == -1) {
                val -= DAC_Falling[DAC_Clock]*DAC_Change;
            }
        }
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
    DIJOYSTATE2 Stick1;

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

    switch (pot) {
    case 0:
        return(RightStickX);
        break;

    case 1:
        return(RightStickY);
        break;

    case 2:
        return(LeftStickX);
        break;

    case 3:
        return(LeftStickY);
        break;
    }
    return (0);
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
    return(ReturnValue);
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


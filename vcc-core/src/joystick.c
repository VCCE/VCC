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

//
// joystick device
//
// TODO: Mac joystick 'driver'
// TODO: Win joystick 'driver'
// TODO: Conf - keyboard, mouse, or joystick input
// TODO: Hi-res joystick interface support
//

#include "joystick.h"

#include "xDebug.h"

#include <stdio.h>

/*****************************************************************/

void SetButtonStatus(joystick_t * pJoystick, int Side, int State);
void joystick(joystick_t * pJoystick, short unsigned, short unsigned);

/****************************************************************************/

result_t jsEmuDevInit(emudevice_t * pEmuDevice)
{
    joystick_t *    pJoystick = (joystick_t *)pEmuDevice;
    
    ASSERT_JOYSTICK(pJoystick);
    
    if ( pJoystick != NULL )
    {
        emuRootDevAddListener(NULL, pEmuDevice, eEventMouse);
    }
    
    return XERROR_NONE;
}

/****************************************************************************/
/**
    Handle events from the system like mouse movement, joystick, etc
 */
result_t jsEmuDevEventHandler(emudevice_t * pEmuDevice, event_t * event)
{
    result_t        result      = XERROR_GENERIC;
    joystick_t *    pJoystick   = (joystick_t *)pEmuDevice;
    
    ASSERT_JOYSTICK(pJoystick);
    
    if ( pJoystick != NULL )
    {
        switch ( event->type )
        {
            // TODO: only if mouse as joystick is enabled
            case eEventMouse:
            {
                ASSERT_MOUSEEVENT(event);
                mouseevent_t * mouseEvent = CAST_MOUSEEVENT(event);
                
                switch(mouseEvent->subType)
                {
                    case eEventMouseMove:
                    {
                        int width   = mouseEvent->width;
                        int height  = mouseEvent->height;
                        int x       = mouseEvent->x;
                        int y       = mouseEvent->y;
                        
                        // clamp x
                        if ( x < 0 ) x = 0;
                        if ( x >= width ) x = width-1;

                        // clamp y
                        if ( y < 0 ) y = 0;
                        if ( y >= height ) y = height-1;
                        
                        // scale X position to 0-63
                        x = (int)(64.0 * x/width);
                        assert(x >= 0 && x < 64);
                        
                        // scale Y position to 0-63
                        y = (int)(64.0 * y/height);
                        // invert for OS X
                        y = 63 - y;
                        assert(y >= 0 && y < 64);
                        
                        joystick(pJoystick,x,y);
                        
                        result = XERROR_NONE;
                    }
                    break;
                        
                    case eEventMouseButton:
                    {
                        // TODO: left/right joysticks
                        
                        if ( mouseEvent->button1 != -1 )
                        {
                            SetButtonStatus(pJoystick,0,mouseEvent->button1);
                        }
                        if ( mouseEvent->button2 != -1 )
                        {
                            SetButtonStatus(pJoystick,0,mouseEvent->button2);
                        }
                        if ( mouseEvent->button3 != -1 )
                        {
                            SetButtonStatus(pJoystick,0,mouseEvent->button3);
                        }
                    }
                    break;
                        
                    default:
                        break;
                }
            }
            break;
                
            case eEventKey:
            {
                ASSERT_KEYEVENT(event);
                keyevent_t * keyEvent = CAST_KEYEVENT(event);
                
                char temp[128];
                sprintf(temp,"Joystick: key = %d (0x%04X) %c",keyEvent->ccvKey,keyEvent->ccvKey,keyEvent->ccvKey);
                
                emuDevLog(&pJoystick->device, "%s", temp);
            }
            break;
                
            default:
                break;
        }
    }
    
    return result;
}

/****************************************************************************/

result_t jsEmuDevDestroy(emudevice_t * pEmuDevice)
{
    joystick_t *    pJoystick = (joystick_t *)pEmuDevice;
    
    ASSERT_JOYSTICK(pJoystick);
    
    if ( pJoystick != NULL )
    {
        free(pJoystick);
    }
    
    return XERROR_NONE;
}

/****************************************************************************/
// TODO: update joystick conf
result_t jsEmuDevConfSave(emudevice_t * pEmuDevice, config_t * config)
{
    result_t    errResult    = XERROR_INVALID_PARAMETER;
    joystick_t *    pJoystick;

    pJoystick = (joystick_t *)pEmuDevice;
    
    ASSERT_JOYSTICK(pJoystick);
    
    if ( pJoystick != NULL )
    {
#if false
        /*
         Joysticks
         TODO: move to joystick device/peripheral
         */
        confSetInt(config,"LeftJoyStick","UseMouse",pJoystick->Left.UseMouse);
        confSetInt(config,"LeftJoyStick","Left",pJoystick->Left.Left);
        confSetInt(config,"LeftJoyStick","Right",pJoystick->Left.Right);
        confSetInt(config,"LeftJoyStick","Up",pJoystick->Left.Up);
        confSetInt(config,"LeftJoyStick","Down",pJoystick->Left.Down);
        confSetInt(config,"LeftJoyStick","Fire1",pJoystick->Left.Fire1);
        confSetInt(config,"LeftJoyStick","Fire2",pJoystick->Left.Fire2);
        confSetInt(config,"LeftJoyStick","DiDevice",pJoystick->Left.DiDevice);
        
        confSetInt(config,"RightJoyStick","UseMouse",pJoystick->Right.UseMouse);
        confSetInt(config,"RightJoyStick","Left",pJoystick->Right.Left);
        confSetInt(config,"RightJoyStick","Right",pJoystick->Right.Right);
        confSetInt(config,"RightJoyStick","Up",pJoystick->Right.Up);
        confSetInt(config,"RightJoyStick","Down",pJoystick->Right.Down);
        confSetInt(config,"RightJoyStick","Fire1",pJoystick->Right.Fire1);
        confSetInt(config,"RightJoyStick","Fire2",pJoystick->Right.Fire2);
        confSetInt(config,"RightJoyStick","DiDevice",pJoystick->Right.DiDevice);
#endif

        errResult = XERROR_NONE;
    }
    
    return errResult;
}

/****************************************************************************/
// TODO: update joystick conf
result_t jsEmuDevConfLoad(emudevice_t * pEmuDevice, config_t * config)
{
    result_t    errResult    = XERROR_INVALID_PARAMETER;
    joystick_t *    pJoystick;

    pJoystick = (joystick_t *)pEmuDevice;
    
    ASSERT_JOYSTICK(pJoystick);

    if ( pJoystick != NULL )
    {
#if false
        int             iValue;

        if ( confGetInt(config,"LeftJoyStick","UseMouse",&iValue) == XERROR_NONE )
        {
            pJoystick->Left.UseMouse    = (unsigned char)iValue;
        }
        if ( confGetInt(config,"LeftJoyStick","Left",&iValue) == XERROR_NONE )
        {
            pJoystick->Left.Left        = (unsigned char)iValue;
        }
        if ( confGetInt(config,"LeftJoyStick","Right",&iValue) == XERROR_NONE )
        {
            pJoystick->Left.Right        = (unsigned char)iValue;
        }
        if ( confGetInt(config,"LeftJoyStick","Up",&iValue)  == XERROR_NONE )
        {
            pJoystick->Left.Up            = (unsigned char)iValue;
        }
        if ( confGetInt(config,"LeftJoyStick","Down",&iValue) == XERROR_NONE )
        {
            pJoystick->Left.Down        = (unsigned char)iValue;
        }
        if ( confGetInt(config,"LeftJoyStick","Fire1",&iValue) == XERROR_NONE )
        {
            pJoystick->Left.Fire1        = (unsigned char)iValue;
        }
        if ( confGetInt(config,"LeftJoyStick","Fire2",&iValue) == XERROR_NONE )
        {
            pJoystick->Left.Fire2        = (unsigned char)iValue;
        }
        if ( confGetInt(config,"LeftJoyStick","DiDevice",&iValue) == XERROR_NONE )
        {
            pJoystick->Left.DiDevice    = (unsigned char)iValue;
        }
        
        if ( confGetInt(config,"RightJoyStick","UseMouse",&iValue) == XERROR_NONE )
        {
            pJoystick->Right.UseMouse    = (unsigned char)iValue;
        }
        if ( confGetInt(config,"RightJoyStick","Left",&iValue) == XERROR_NONE )
        {
            pJoystick->Right.Left        = (unsigned char)iValue;
        }
        if ( confGetInt(config,"RightJoyStick","Right",&iValue) == XERROR_NONE )
        {
            pJoystick->Right.Right        = (unsigned char)iValue;
        }
        if ( confGetInt(config,"RightJoyStick","Up",&iValue) == XERROR_NONE )
        {
            pJoystick->Right.Up        = (unsigned char)iValue;
        }
        if ( confGetInt(config,"RightJoyStick","Down",&iValue) == XERROR_NONE )
        {
            pJoystick->Right.Down        = (unsigned char)iValue;
        }
        if ( confGetInt(config,"RightJoyStick","Fire1",&iValue) == XERROR_NONE )
        {
            pJoystick->Right.Fire1        = (unsigned char)iValue;
        }
        if ( confGetInt(config,"RightJoyStick","Fire2",&iValue) == XERROR_NONE )
        {
            pJoystick->Right.Fire2        = (unsigned char)iValue;
        }
        if ( confGetInt(config,"RightJoyStick","DiDevice",&iValue) == XERROR_NONE )
        {
            pJoystick->Right.DiDevice    = (unsigned char)iValue;
        }
#endif
        
        errResult = XERROR_NONE;
    }
    
    return errResult;
}

/****************************************************************************/

#pragma mark -
#pragma mark --- implementation ---

/****************************************************************************/
/**
 */
joystick_t * joystickCreate()
{
    joystick_t *    pJoystick;
    
    pJoystick = calloc(1,sizeof(joystick_t));
    if ( pJoystick != NULL )
    {
        pJoystick->device.id          = EMU_DEVICE_ID;
        pJoystick->device.idModule    = VCC_JOYSTICK_ID;
        strcpy(pJoystick->device.Name,"Joystick");
        
        pJoystick->device.pfnInit           = jsEmuDevInit;
        pJoystick->device.pfnEventHandler   = jsEmuDevEventHandler;
        
        pJoystick->device.pfnDestroy    = jsEmuDevDestroy;
        pJoystick->device.pfnSave       = jsEmuDevConfSave;
        pJoystick->device.pfnLoad       = jsEmuDevConfLoad;
        
        emuDevRegisterDevice(&pJoystick->device);
        
        pJoystick->LeftStickX  = 32;
        pJoystick->LeftStickY  = 32;
        pJoystick->RightStickX = 32;
        pJoystick->RightStickY = 32;
        
        pJoystick->Left.UseMouse        = 1;
        pJoystick->Left.Left            = 75;
        pJoystick->Left.Right            = 77;
        pJoystick->Left.Up                = 72;
        pJoystick->Left.Down            = 80;
        pJoystick->Left.Fire1            = 59;
        pJoystick->Left.Fire2            = 60;
        pJoystick->Left.DiDevice        = 0;
        
        pJoystick->Right.UseMouse        = 1;
        pJoystick->Right.Left            = 75;
        pJoystick->Right.Right          = 77;
        pJoystick->Right.Up             = 72;
        pJoystick->Right.Down            = 80;
        pJoystick->Right.Fire1          = 59;
        pJoystick->Right.Fire2          = 60;
        pJoystick->Right.DiDevice        = 0;
        
    }
    
    return pJoystick;
}

/*****************************************************************/

//Side=0 Left Button Side=1 Right Button State 1=Down
void SetButtonStatus(joystick_t * pJoystick, int Side, int State)
{
    unsigned char Btemp=0;
    //vccinstance_t *    pInstance;
    
    assert(pJoystick != NULL);
    //pInstance = (vccinstance_t *)emuDevGetParentModuleByID(&pJoystick->device,VCC_CORE_ID);
    //assert(pInstance != NULL);
    
    Btemp= (Side<<1) | State;
    if ( pJoystick->Left.UseMouse == 1 )
        switch (Btemp)
    {
        case 0:
            pJoystick->LeftButton1Status=0;
            break;
            
        case 1:
            pJoystick->LeftButton1Status=1;
            break;
            
        case 2:
            pJoystick->LeftButton2Status=0;
            break;
            
        case 3:
            pJoystick->LeftButton2Status=1;
            break;
    }
    
    if ( pJoystick->Right.UseMouse == 1 )
        switch (Btemp)
    {
        case 0:
            pJoystick->RightButton1Status=0;
            break;
            
        case 1:
            pJoystick->RightButton1Status=1;
            break;
            
        case 2:
            pJoystick->RightButton2Status=0;
            break;
            
        case 3:
            pJoystick->RightButton2Status=1;
            break;
    }
    return;
}

/*****************************************************************/
//
//
//
// TODO: this was previously used to set the mouse status from the keyboard (not called now)
char SetMouseStatus(joystick_t * pJoystick, char ScanCode,unsigned char Phase)
{
    char ReturnValue=ScanCode;
    //vccinstance_t *    pInstance;
    
    assert(pJoystick != NULL);
    //pInstance = (vccinstance_t *)emuDevGetParentModuleByID(&pJoystick->device,VCC_CORE_ID);
    //assert(pInstance != NULL);
    
    switch (Phase)
    {
        case 0:
            if (pJoystick->Left.UseMouse==0)
            {
                if (ScanCode==pJoystick->Left.Left)
                {
                    pJoystick->LeftStickX=32;
                    ReturnValue=0;
                }
                if (ScanCode==pJoystick->Left.Right)
                {
                    pJoystick->LeftStickX=32;
                    ReturnValue=0;
                }
                if (ScanCode==pJoystick->Left.Up)
                {
                    pJoystick->LeftStickY=32;
                    ReturnValue=0;
                }
                if (ScanCode==pJoystick->Left.Down)
                {
                    pJoystick->LeftStickY=32;
                    ReturnValue=0;
                }
                if (ScanCode==pJoystick->Left.Fire1)
                {
                    pJoystick->LeftButton1Status=0;
                    ReturnValue=0;
                }
                if (ScanCode==pJoystick->Left.Fire2)
                {
                    pJoystick->LeftButton2Status=0;
                    ReturnValue=0;
                }
            }
            
            if (pJoystick->Right.UseMouse==0)
            {
                if (ScanCode==pJoystick->Right.Left)
                {
                    pJoystick->RightStickX=32;
                    ReturnValue=0;
                }
                if (ScanCode==pJoystick->Right.Right)
                {
                    pJoystick->RightStickX=32;
                    ReturnValue=0;
                }
                if (ScanCode==pJoystick->Right.Up)
                {
                    pJoystick->RightStickY=32;
                    ReturnValue=0;
                }
                if (ScanCode==pJoystick->Right.Down)
                {
                    pJoystick->RightStickY=32;
                    ReturnValue=0;
                }
                if (ScanCode==pJoystick->Right.Fire1)
                {
                    pJoystick->RightButton1Status=0;
                    ReturnValue=0;
                }
                if (ScanCode==pJoystick->Right.Fire2)
                {
                    pJoystick->RightButton2Status=0;
                    ReturnValue=0;
                }
            }
            break;
            
        case 1:
            if (pJoystick->Left.UseMouse==0)
            {
                if (ScanCode==pJoystick->Left.Left)
                {
                    pJoystick->LeftStickX=0;
                    ReturnValue=0;
                }
                if (ScanCode==pJoystick->Left.Right)
                {
                    pJoystick->LeftStickX=63;
                    ReturnValue=0;
                }
                if (ScanCode==pJoystick->Left.Up)
                {
                    pJoystick->LeftStickY=0;
                    ReturnValue=0;
                }
                if (ScanCode==pJoystick->Left.Down)
                {
                    pJoystick->LeftStickY=63;
                    ReturnValue=0;
                }
                if (ScanCode==pJoystick->Left.Fire1)
                {
                    pJoystick->LeftButton1Status=1;
                    ReturnValue=0;
                }
                if (ScanCode==pJoystick->Left.Fire2)
                {
                    pJoystick->LeftButton2Status=1;
                    ReturnValue=0;
                }
            }
            
            if (pJoystick->Right.UseMouse==0)
            {
                if (ScanCode==pJoystick->Right.Left)
                {
                    ReturnValue=0;
                    pJoystick->RightStickX=0;
                }
                if (ScanCode==pJoystick->Right.Right)
                {
                    pJoystick->RightStickX=63;
                    ReturnValue=0;
                }
                if (ScanCode==pJoystick->Right.Up)
                {
                    pJoystick->RightStickY=0;
                    ReturnValue=0;
                }
                if (ScanCode==pJoystick->Right.Down)
                {
                    pJoystick->RightStickY=63;
                    ReturnValue=0;
                }
                if (ScanCode==pJoystick->Right.Fire1)
                {
                    pJoystick->RightButton1Status=1;
                    ReturnValue=0;
                }
                if (ScanCode==pJoystick->Right.Fire2)
                {
                    pJoystick->RightButton2Status=1;
                    ReturnValue=0;
                }
            }
            break;
    }
    return(ReturnValue);
}

/*****************************************************************/

void joystick(joystick_t * pJoystick, unsigned short x,unsigned short y)
{
    //vccinstance_t *    pInstance;
    
    assert(pJoystick != NULL);
    //pInstance = (vccinstance_t *)emuDevGetParentModuleByID(&pJoystick->device,VCC_CORE_ID);
    //assert(pInstance != NULL);

    if ( x > 63 )
    {
        x = 63;
    }
    if ( y > 63 )
    {
        y = 63;
    }
    if ( pJoystick->Left.UseMouse == 1 )
    {
        pJoystick->LeftStickX = x;
        pJoystick->LeftStickY = y;
    }
    if ( pJoystick->Right.UseMouse == 1 )
    {
        pJoystick->RightStickX = x;
        pJoystick->RightStickY = y;
    }
}

/*****************************************************************/

void SetStickNumbers(joystick_t * pJoystick, unsigned char Temp1,unsigned char Temp2)
{
    pJoystick->LeftStickNumber = Temp1;
    pJoystick->RightStickNumber = Temp2;
}

/*****************************************************************/

// TODO: fix get_pot_value

unsigned short get_pot_value(joystick_t * pJoystick, unsigned char pot)
{
#if 0
    DIJOYSTATE2 Stick1;
#endif
    //vccinstance_t *    pInstance;
    
    assert(pJoystick != NULL);
    //pInstance = (vccinstance_t *)pJoystick->device.pParent;
    //assert(pInstance != NULL);
    
    if ( pJoystick->Left.UseMouse == 3 )
    {
#if 0
        JoyStickPoll(&Stick1,LeftStickNumber);
        LeftStickX =(unsigned short)Stick1.lX>>10;
        LeftStickY= (unsigned short)Stick1.lY>>10;
        LeftButton1Status= Stick1.rgbButtons[0]>>7;
        LeftButton2Status= Stick1.rgbButtons[1]>>7;
#else
        pJoystick->LeftStickX    = 0;
        pJoystick->LeftStickY    = 0;
        pJoystick->LeftButton1Status= 0;
        pJoystick->LeftButton2Status= 0;
#endif
    }
    
    if ( pJoystick->Right.UseMouse == 3 )
    {
#if 0
        JoyStickPoll(&Stick1,RightStickNumber);
        RightStickX= (unsigned short)Stick1.lX>>10;
        RightStickY= (unsigned short)Stick1.lY>>10;
        RightButton1Status= Stick1.rgbButtons[0]>>7;
        RightButton2Status= Stick1.rgbButtons[1]>>7;
#else
        pJoystick->RightStickX    = 0;
        pJoystick->RightStickY    = 0;
        pJoystick->RightButton1Status= 0;
        pJoystick->RightButton2Status= 0;
#endif
    }
    
    switch ( pot )
    {
        case 0:
            return(pJoystick->RightStickX);
            break;
            
        case 1:
            return(pJoystick->RightStickY);
            break;
            
        case 2:
            return(pJoystick->LeftStickX);
            break;
            
        case 3:
            return(pJoystick->LeftStickY);
            break;
    }
    
    return(0);
}

/*****************************************************************/

#if 0 // WIN32 specific functions (Direct Input)
//#include <dinput.h>

#define MAXSTICKS 10
#define STRLEN 64

//HRESULT    JoyStickPoll(DIJOYSTATE2 * ,unsigned char);
int            EnumerateJoysticks(void);
bool        InitJoyStick(unsigned char);

#endif


/********************************************************************/

void RefreshJoystickStatus()
{
	/// TODO: RefreshJoystickStatus
	XTRACE("TODO: RefreshJoystickStatus\n");
	
#if 0
	unsigned char Index=0;
	bool Temp=false;
	
	NumberofJoysticks = EnumerateJoysticks();
	for (Index=0;Index<NumberofJoysticks;Index++)
		Temp=InitJoyStick (Index);
	
	if (Right.DiDevice>(NumberofJoysticks-1))
		Right.DiDevice=0;
	if (Left.DiDevice>(NumberofJoysticks-1))
		Left.DiDevice=0;
	SetStickNumbers(Left.DiDevice,Right.DiDevice);
	if (NumberofJoysticks==0)	//Use Mouse input if no Joysticks present
	{
		if (Left.UseMouse==3)
			Left.UseMouse=1;
		if (Right.UseMouse==3)
			Right.UseMouse=1;
	}
#endif
	
	return;
}

/********************************************************************/

#if 0
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

LPDIRECTINPUTDEVICE8 Joysticks[MAXSTICKS];
char StickName[MAXSTICKS][STRLEN];
unsigned char JoyStickIndex=0;
LPDIRECTINPUT8 di;
BOOL CALLBACK enumCallback(const DIDEVICEINSTANCE* , VOID* );
BOOL CALLBACK enumAxesCallback(const DIDEVICEOBJECTINSTANCE* , VOID* );
unsigned char CurrentStick;

int EnumerateJoysticks(void)
{
	HRESULT hr;
	JoyStickIndex=0;
	if (FAILED(hr = DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION,IID_IDirectInput8,(VOID**)&di,NULL)))
		return(0);

	if (FAILED(hr = di->EnumDevices(DI8DEVCLASS_GAMECTRL, enumCallback,NULL,DIEDFL_ATTACHEDONLY)))
		return(0);
	return(JoyStickIndex);
}

BOOL CALLBACK enumCallback(const DIDEVICEINSTANCE* instance, VOID* context)
{
	HRESULT hr;
	hr = di->CreateDevice(instance->guidInstance, &Joysticks[JoyStickIndex],NULL);
	strncpy(StickName[JoyStickIndex],instance->tszProductName,STRLEN);
	JoyStickIndex++;
	return(JoyStickIndex<MAXSTICKS);
}


bool InitJoyStick (unsigned char StickNumber)
{
//	DIDEVCAPS capabilities;
	HRESULT hr;
	CurrentStick=StickNumber;
	if (Joysticks[StickNumber]==NULL)
		return(0);

	if (FAILED(hr= Joysticks[StickNumber]->SetDataFormat(&c_dfDIJoystick2)))
		return(0);

//	if (FAILED(hr= Joysticks[StickNumber]->SetCooperativeLevel(NULL, DISCL_EXCLUSIVE ))) 
//		return(0);

	//Fails for some reason Investigate this
//	if (FAILED(hr= Joysticks[StickNumber]->GetCapabilities(&capabilities)))
//		return(0);

	if (FAILED(hr= Joysticks[StickNumber]->EnumObjects(enumAxesCallback,NULL,DIDFT_AXIS)))
		return(0);
	return(1); //return true on success
}



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


HRESULT JoyStickPoll(DIJOYSTATE2 *js,unsigned char StickNumber)
{
	HRESULT hr;
	if (Joysticks[StickNumber] ==NULL)
		return (S_OK);

	hr=Joysticks[StickNumber]->Poll();
	if (FAILED(hr))
	{
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


#endif


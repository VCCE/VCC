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

/****************************************************************************/
/*
	MC6821 PIA module
 
	This handles the PIAs in the Coco3
 
	TODO: add reference to external documentation
	TODO: document PIA0 
	TODO: add bitbanger read/write
	TODO: should be more plug-in like for peripherals, i.e. cassette, joystick, bitbanger
	TODO: sound enable flag handling
	TODO: RGB monitor sensing flag
	TODO: RAM size input flag (needed?)
*/
/****************************************************************************/
/*
 
 PIA1 - $FF20-$FF23
 
 | +----------+-----------------------------------------------------------------+
 |	FF20 (65312) PIA 1 side A data register - PIA1AD CoCo 1/2/3	
 | +----------+-----------------------------------------------------------------+
 |Bit7        | 6 bit DAC MSB
 |Bit6        |
 |Bit5        |
 |Bit4        |
 |Bit3        |
 |Bit2		  | 6 BIT DAC LSB
 |Bit1        | RS-232C data output
 |Bit0        | CASSETTE DATA INPUT
 | +----------+-----------------------------------------------------------------+

 | +----------+-----------------------------------------------------------------+
 |	FF21 (65313) PIA 1 side A control register - PIA1AC CoCo 1/2/3	
 | +----------+-----------------------------------------------------------------+
 |Bit7        | CD FIRQ FLAG
 |Bit6        | n/a
 |Bit5        | 1
 |Bit4        | 1
 |Bit3        | Cassette motor control 0 = off 1 = on
 |Bit2		  | Data direction control 0=$FF20 data direction, 1=normal
 |Bit1        | FIRQ polarity 0=falling 1=rising
 |Bit0        | CD FIRQ (RS-232C) 0=FIRQ disabled 1=enabled
 | +----------+-----------------------------------------------------------------+
 
 | +----------+-----------------------------------------------------------------+
 |	FF22 (65314) PIA 1 side B data register - PIA1BD CoCo 1/2/3	
 | +----------+-----------------------------------------------------------------+
 |Bit7        | VDG Control a/g:            Alphanum=0, graphics=1
 |Bit6        |                             GM2
 |Bit5        |                             GM1 & invert
 |Bit4        | VDG Control                 GM0 & shift toggle
 |Bit3        | RGB Monitor sensing (INPUT) CSS = Solor Set Select 0,1
 |Bit2		  | RAM Size input
 |Bit1        | Single bit sound output
 |Bit0        | RS-232C Data input
 | +----------+-----------------------------------------------------------------+
 
 | +----------+-----------------------------------------------------------------+
 |	FF23 (65315) PIA 1 side B control register - PIA1BC CoCo 1/2/3	
 | +----------+-----------------------------------------------------------------+
 |Bit7        | CART FIRQ FLAG
 |Bit6        | n/a
 |Bit5        | 1
 |Bit4        | 1
 |Bit3        | Sound enable
 |Bit2		  | Data direction control 0=$FF22 data direction, 1=normal
 |Bit1        | FIRQ polarity 0=falling 1=rising
 |Bit0        | CART FIRQ 0=FIRQ disabled 1=enabled
 | +----------+-----------------------------------------------------------------+
 */
/****************************************************************************/

#include "mc6821.h"

#include "keyboard.h"
#include "pakinterface.h"
#include "cassette.h"
#include "tcc1014graphics.h"
#include "cpu.h"
#include "coco3.h"

#include "xDebug.h"

/****************************************************************************/

//
// Side A
//
#define PIA1A_DAT_DAC               0xFC
#define PIA1A_DAT_SER_OUT           0x02
#define PIA1A_DAT_CASS_DATA         0x01

#define PIA1A_CTL_CD_FIRQ           0x80
// unused 0x40
// 1 0x20
// 1 0x10
#define PIA1A_CTL_CASS_MOTOR        0x08
#define PIA1A_CTL_DATA_DIRECTION    0x04
#define PIA1A_CTL_FIRQ_POLARITY     0x02
#define PIA1A_CTL_CD_FIRQ_ENABLE    0x01

//
// Side B
//
#define PIA1B_DAT_VDG_CTL           0x80
#define PIA1B_DAT_GM2               0x40
#define PIA1B_DAT_GM1               0x20
#define PIA1B_DAT_GM0               0x10
#define PIA1B_DAT_RGB_SENSE         0x08
#define PIA1B_DAT_RAM_SIZE          0x04
#define PIA1B_DAT_SOUND_OUT         0x02
#define PIA1B_DAT_SER_IN            0x01

#define PIA1B_CTL_CART_FIRQ         0x80
// unused 0x40
// 1 0x20
// 1 0x10
#define PIA1B_CTL_SOUND_ENABLED     0x08
#define PIA1B_CTL_DATA_DIRECTION    0x04
#define PIA1B_CTL_FIRQ_POLARITY     0x02
#define PIA1B_CTL_CART_FIRQ_ENABLE  0x01

/****************************************************************************/

#pragma mark -
#pragma mark --- emulator device callbacks ---

/****************************************************************************/

result_t mc6821EmuDevDestroy(emudevice_t * pEmuDevice)
{
	mc6821_t *	pMC6821;
	
	pMC6821 = (mc6821_t *)pEmuDevice;
	
	ASSERT_MC6821(pMC6821);
	
	if ( pMC6821 != NULL )
	{
		free(pMC6821);
	}
	
	return XERROR_NONE;
}

/****************************************************************************/

result_t mc6821EmuDevConfSave(emudevice_t * pEmuDevice, config_t * config)
{
	result_t	errResult	= XERROR_INVALID_PARAMETER;
	mc6821_t *	pMC6821;
	
	pMC6821 = (mc6821_t *)pEmuDevice;
	
	ASSERT_MC6821(pMC6821);
	
	if ( pMC6821 != NULL )
	{
		// do nothing for now
		
		errResult = XERROR_NONE;
	}
	
	return errResult;
}

/****************************************************************************/

result_t mc6821EmuDevConfLoad(emudevice_t * pEmuDevice, config_t * config)
{
	result_t	errResult	= XERROR_INVALID_PARAMETER;
	mc6821_t *	pMC6821;
	
	pMC6821 = (mc6821_t *)pEmuDevice;
	
	ASSERT_MC6821(pMC6821);
	
	if ( pMC6821 != NULL )
	{
		// do nothing for now
		
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
mc6821_t * mc6821Create()
{
	mc6821_t *	pMC6821;
	
	pMC6821 = calloc(1,sizeof(mc6821_t));
	if ( pMC6821 != NULL )
	{
		pMC6821->device.id			= EMU_DEVICE_ID;
		pMC6821->device.idModule	= VCC_MC6821_ID;
        pMC6821->device.idDevice    = VCC_MC6821_ID;
		strcpy(pMC6821->device.Name,"MC6821");
		
		pMC6821->device.pfnDestroy	= mc6821EmuDevDestroy;
		pMC6821->device.pfnSave		= mc6821EmuDevConfSave;
		pMC6821->device.pfnLoad		= mc6821EmuDevConfLoad;
		
		emuDevRegisterDevice(&pMC6821->device);
	}
	
	return pMC6821;
}

/****************************************************************************/
/****************************************************************************/

/****************************************************************************/
/**
 */
void mc6821_Reset(mc6821_t * pMC6821)
{
	unsigned char Index=0;
	
	for (Index=0; Index<4; Index++)
	{
		pMC6821->rega[Index] = 0;
		pMC6821->regb[Index] = 0;
		pMC6821->rega_dd[Index] = 0;
		pMC6821->regb_dd[Index] = 0;
	}
	
	pMC6821->VDG_gmode = 0;
}

/****************************************************************************/
/**
 */
unsigned char mc6821VDGMode(mc6821_t * pMC6821)
{
	return ((pMC6821->regb[2] & 248) >> 3);
}

/****************************************************************************/
/**
	63.5 uS
 */
void mc6821_IRQ_HS(mc6821_t * pMC6821, cpu_t * pCPU, hsphase_e phase)
{
	assert(pCPU != NULL);

	switch (phase)
	{
        case FALLING:	// HS went High to low
            if ( (pMC6821->rega[1] & 2) ) //IRQ on low to High transition
            {
                return;
            }
            
            pMC6821->rega[1]=(pMC6821->rega[1] | 128);
            if (pMC6821->rega[1] & 1)
            {
                cpuAssertInterrupt(pCPU,IRQ,1);
            }
        break;

        case RISING:	// HS went Low to High
            if ( !(pMC6821->rega[1] & 2) ) //IRQ  High to low transition
            {
                return;
            }
            
            pMC6821->rega[1] = (pMC6821->rega[1] | 128);
            if ( pMC6821->rega[1] & 1 )
            {
                cpuAssertInterrupt(pCPU,IRQ,1);
            }
        break;

        case ANY:
            pMC6821->rega[1] = (pMC6821->rega[1] | 128);
            if ( pMC6821->rega[1] & 1 )
            {
                cpuAssertInterrupt(pCPU,IRQ,1);
            }
        break;
            
        default:
            // invalid
        break;
	}
}

/****************************************************************************/
/**
 */
void mc6821_SetCART(mc6821_t * pMC6821, signal_e signal)
{
    pMC6821->CART = signal;
}

/****************************************************************************/

void mc6821_IRQ_CART(mc6821_t * pMC6821, cpu_t * pCPU)
{
    if ( pMC6821->CART == Low )
    {
        // set CART interrupt flag
        pMC6821->regb[3] = (pMC6821->regb[3] | PIA1B_CTL_CART_FIRQ);
        
        if ( (pMC6821->regb[3] & PIA1B_CTL_CART_FIRQ_ENABLE) )
        {
            cpuAssertInterrupt(pCPU,FIRQ,0);
        }
        else
        {
            // [original comment: "Kludge but working"]
            // removing this breaks MegaBug (at least)
            cpuDeassertInterrupt(pCPU,FIRQ);
        }
    }
}

/****************************************************************************/
/**
	60HZ Vertical sync pulse 16.667 mS
 */
void mc6821_IRQ_FS(mc6821_t * pMC6821, cpu_t * pCPU, fsphase_e phase)
{
	assert(pCPU != NULL);
	
	switch (phase)
	{
        case HighToLow:	//FS went High to low
            if ( (pMC6821->rega[3] & PIA1A_CTL_CD_FIRQ) == 0 ) // IRQ on High to low transition
            {
                pMC6821->rega[3] = (pMC6821->rega[3] | 128);
            }
            
            if ( pMC6821->rega[3] & PIA1A_CTL_CD_FIRQ_ENABLE )
            {
                cpuAssertInterrupt(pCPU,IRQ,1);
            }
        break;

        case LowToHigh:	//FS went Low to High

            if ( (pMC6821->rega[3] & 2) ) // IRQ Low to High transition
            {
                pMC6821->rega[3] = (pMC6821->rega[3] | 128);
                
                if (pMC6821->rega[3] & 2)
                {
                    cpuAssertInterrupt(pCPU,IRQ,1);
                }
            }
        break;
            
        default:
            // invalid
        break;
	}
}

/****************************************************************************/
/**
 */
unsigned char mc6821_GetMuxState(mc6821_t * pMC6821)
{
	return ( ((pMC6821->rega[1] & 8) >> 3) + ((pMC6821->rega[3] & 8) >> 2));
}

/****************************************************************************/
/**
 */
unsigned char mc6821_DACState(mc6821_t * pMC6821)
{
	return (pMC6821->regb[0]>>2);
}

/****************************************************************************/
/**
 */
unsigned int mc6821_GetDACSample(mc6821_t * pMC6821)
{
	unsigned int RetVal=0;
	unsigned short SampleLeft=0,SampleRight=0,PakSample=0;
	unsigned short OutLeft=0,OutRight=0;
	coco3_t *	pCoco3;
	
	assert(pMC6821 != NULL);
	pCoco3 = (coco3_t *)pMC6821->device.pParent;
	
	PakSample = pakAudioSample(pCoco3->pPak);
	
	SampleLeft = (PakSample>>8) + pMC6821->Asample + pMC6821->Ssample;
	SampleRight = (PakSample & 0xFF) + pMC6821->Asample + pMC6821->Ssample; //9 Bits each
	SampleLeft = SampleLeft<<6;	//Conver to 16 bit values
	SampleRight = SampleRight<<6;	//For Max volume
	
    if ( SampleLeft == pMC6821->LastLeft )	//Simulate a slow high pass filter
	{
		if (OutLeft)
			OutLeft--;
	}
	else
	{
		OutLeft=SampleLeft;
		pMC6821->LastLeft=SampleLeft;
	}

	if (SampleRight==pMC6821->LastRight)
	{
		if (OutRight)
			OutRight--;
	}
	else
	{
		OutRight=SampleRight;
		pMC6821->LastRight=SampleRight;
	}

	RetVal=(OutLeft<<16)+(OutRight);
	
	return(RetVal);
}

/****************************************************************************/
/**
 */
unsigned char mc6821_GetCasSample(mc6821_t * pMC6821)
{
	assert(pMC6821);
	
	return (pMC6821->Csample);
}

/****************************************************************************/
/**
 */
void mc6821_SetCasSample(mc6821_t * pMC6821, unsigned char Sample)
{
	assert(pMC6821);
	
	pMC6821->regb[0] = pMC6821->regb[0] & ~PIA1A_DAT_CASS_DATA;
	if ( Sample > 0x7F)
	{
		pMC6821->regb[0] = pMC6821->regb[0] | PIA1A_DAT_CASS_DATA;
	}
}

/****************************************************************************/
/****************************************************************************/
/*
    PIA read/write callbacks
 */

/****************************************************************************/
/**
	Shift Row Col
 */
unsigned char mc6821_pia0Read(emudevice_t * pEmuDevice, unsigned char port)
{
	unsigned char	dda;
	unsigned char	ddb;
	mc6821_t *		pMC6821 = (mc6821_t *)pEmuDevice;
	coco3_t *	    pCoco3;
	
	assert(pMC6821 != NULL);
	pCoco3 = (coco3_t *)pMC6821->device.pMachine;
    assert(pCoco3 != NULL);
    
	dda = (pMC6821->rega[1] & 4); /* data direction A */
	ddb = (pMC6821->rega[3] & 4); /* data direction B */
	
	switch (port)
	{
        case 0:
            if ( dda )
            {
                pMC6821->rega[1] = (pMC6821->rega[1] & 63);
                return (keyboardScan(pCoco3->pKeyboard,pMC6821->rega[2])); // Read
            }
            else
            {
                return (pMC6821->rega_dd[port]);
            }
            break;
            
		case 1:
			return (pMC6821->rega[port]);
			break;
			
		case 3:
			return (pMC6821->rega[port]);
			break;
			
		case 2: //Write
			if (ddb)
			{
				pMC6821->rega[3] = (pMC6821->rega[3] & 63);
				return (pMC6821->rega[port] & pMC6821->rega_dd[port]);
			}
			else
			{
				return (pMC6821->rega_dd[port]);
			}
			break;
	}
	
	return (0);
}

/****************************************************************************/
/**
 */
void mc6821_pia0Write(emudevice_t * pEmuDevice, unsigned char port, unsigned char data)
{
	unsigned char dda,ddb;
	mc6821_t *		pMC6821 = (mc6821_t *)pEmuDevice;
	coco3_t *       pCoco3;
	
	assert(pMC6821 != NULL);
	pCoco3 = (coco3_t *)pMC6821->device.pParent;
    assert(pCoco3 != NULL);
    
	dda = (pMC6821->rega[1] & 4); /* data direction A */
	ddb = (pMC6821->rega[3] & 4); /* data direction B */
	
	switch ( port )
	{
		case 0:
			if ( dda )
			{
				pMC6821->rega[port] = data;
			}
			else
			{
				pMC6821->rega_dd[port] = data;
			}
		break;
			
		case 2:
			if ( ddb )
			{
				pMC6821->rega[port] = data;
			}
			else
			{
				pMC6821->rega_dd[port] = data;
			}
		break;
			
		case 1:
			pMC6821->rega[port] = (data & 0x3F);
		break;
			
		case 3:
			pMC6821->rega[port] = (data & 0x3F);
		break;	
	}
}

/****************************************************************************/
/**
	PIA1 port read - $FF20-$FF23
  */
unsigned char mc6821_pia1Read(emudevice_t * pEmuDevice, unsigned char port)
{
	unsigned int	Flag = 0;
	unsigned char	dda;
	unsigned char	ddb;
	mc6821_t *		pMC6821 = (mc6821_t *)pEmuDevice;
	coco3_t *	pCoco3;
	
	assert(pMC6821 != NULL);
	pCoco3 = (coco3_t *)pMC6821->device.pParent;
    assert(pCoco3 != NULL);
    
	port -= 0x20;
	
	dda = (pMC6821->regb[1] & 4); /* FF20 data direction control */
	ddb = (pMC6821->regb[3] & 4); /* FF22 data direction control */
	
	switch ( port )
	{
		case 0:
			if ( dda )
			{
				pMC6821->regb[1] = (pMC6821->regb[1] & 63); // Cass In
				
				// TODO: shouldn't this apply the data direction mask?
				Flag = pMC6821->regb[port] ;//& regb_dd[port];
				
				return (Flag);
			}
			else
			{
				return (pMC6821->regb_dd[port]);
			}
		break;

		case 1:
			return (pMC6821->regb[port]);
		break;
			
		case 2:
			if ( ddb )
			{
#if 0
				// FF22 Bit0 RS-232C Data input - set it here?
				pMC6821->regb[3] |= (bb_read_bit(&pMC6821->bb.device) != 0);
#endif
				
				// TODO: why the mask?
				pMC6821->regb[3] = (pMC6821->regb[3] & 63);
				
				return (pMC6821->regb[port] & pMC6821->regb_dd[port]);
			}
			else
			{
				return (pMC6821->regb_dd[port]);
			}
		break;
			
		case 3:
			return (pMC6821->regb[port]);
		break;
	}
	
	assert(0 && "incorrect port read");
	
	return (0);
}

/****************************************************************************/
/**
	PIA1 port write - $FF20-$FF23
 */
void mc6821_pia1Write(emudevice_t * pEmuDevice, unsigned char port, unsigned char data)
{
	unsigned char dda,ddb;
	mc6821_t *		pMC6821 = (mc6821_t *)pEmuDevice;
	coco3_t *	pCoco3;
	
	assert(pMC6821 != NULL);
	pCoco3 = (coco3_t *)pMC6821->device.pParent;
    assert(pCoco3 != NULL);
	
	port -= 0x20;
	
	dda = (pMC6821->regb[1] & 4);	/* FF20 data direction control */
	ddb = (pMC6821->regb[3] & 4);	/* FF22 data direction control */
	
	switch ( port )
	{
		case 0:
			if ( dda )
			{
				pMC6821->regb[port] = data;
				
				if ( mc6821_GetMuxState(pMC6821) == 0 ) 
				{
					if ( (pMC6821->regb[3] & 8) != 0 )	// check sound enable					// == 0 for cassette writes
					{
                        // audio sample
						pMC6821->Asample = (pMC6821->regb[0] & 0xFC) >> 1;	// 0 to 127
					}
					else
					{
                        // cassette sample
						pMC6821->Csample = (pMC6821->regb[0] & 0xFC);
					}
				}
				
#if 0
				// FF20 bit 1 - RS232 output = (data&0x02)
				bb_output(&pMC6821->bb.device,(pMC6821->regb[port] & 0x02) >> 1);
#endif
			}
			else
			{
				pMC6821->regb_dd[port] = data;
			}
		break;
			
		case 2:
			if ( ddb )
			{
				pMC6821->regb[port] = (data & pMC6821->regb_dd[port]); 
				
				SetGimeVdgMode2(pCoco3->pGIME,(pMC6821->regb[2] & 248) >> 3);
				
				pMC6821->Ssample = (pMC6821->regb[port] & 2)<<6;
			}
			else
			{
				pMC6821->regb_dd[port] = data;
			}
		break;
			
		case 1:
			pMC6821->regb[port] = (data & 0x3F);	// TODO: why the mask to lose upper two bits?

			// Bit 3 - Cassette motor flag
			casMotor(pCoco3->pCassette,(data & 8)>>3);
		break;
			
		case 3:
			pMC6821->regb[port] = (data & 0x3F);	// TODO: why the mask to lose upper two bits?
		break;
	}
}

/****************************************************************************/
/****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

#if 0
// TODO: move to its own file

typedef struct _bitbanger_token bitbanger_token;
struct _bitbanger_token
{
    double		last_pulse_time;
    double *	pulses;
    int *		factored_pulses;
    int			recorded_pulses;
    int			value;
    emu_timer *	timeout_timer;
    int			over_threshhold;
};

typedef struct _bitbanger_t bitbanger_t;
struct _bitbanger_t
{
    emudevice_t			device;
    
    /* filter function; returns non-zero if input accepted */
    int (*filter)(emudevice_t * pEmuDevice, const int * pulses, int total_pulses, int total_duration);
    
    double	pulse_threshhold;				/* the maximum duration pulse that we will consider */
    double	pulse_tolerance;				/* deviation tolerance for pulses */
    int		minimum_pulses;					/* the minimum amount of pulses before we start analyzing */
    int		maximum_pulses;					/* the maximum amount of pulses that we will track */
    int		begin_pulse_value;				/* the begining value of a pulse */
    int		initial_value;					/* the initial value of the bitbanger line */
};

#if 0
bitbanger_t			bb;
#endif

bb_read_bit();
bb_write_bit();

static const bitbanger_config coco_bitbanger_config =
{
	coco_bitbanger_filter,
	1.0 / 10.0,
	0.2,
	2,
	10,
	0,
	0
};

static int coco_bitbanger_filter(const device_config *img, const int *pulses, int total_pulses, int total_duration)
{
	int i;
	int result = 0;
	int word;
	int pos;
	int pulse_type;
	int c;
	
	if (total_duration >= 11)
	{
		word = 0;
		pos = 0;
		pulse_type = 0;
		result = 1;
		
		for (i = 0; i < total_pulses; i++)
		{
			if (pulse_type)
				word |= ((1 << pulses[i]) - 1) << pos;
			pulse_type ^= 1;
			pos += pulses[i];
		}
		
		c = (word >> 1) & 0xff;
		printer_output(img, c);
	}
	return result;
}

INLINE bitbanger_token *get_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == BITBANGER);
	return (bitbanger_token *) device->token;
}

INLINE const bitbanger_config *get_config(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == BITBANGER);
	return (const bitbanger_config *) device->static_config;
}


static DEVICE_START(bitbanger)
{
	bitbanger_token *bi;
	const bitbanger_config *config = get_config(device);
	
	bi = get_token(device);
	bi->pulses = auto_alloc_array(device->machine, double, config->maximum_pulses);
	bi->factored_pulses = auto_alloc_array(device->machine, int, config->maximum_pulses);
	bi->last_pulse_time = 0.0;
	bi->recorded_pulses = 0;
	bi->value = config->initial_value;
	bi->timeout_timer = timer_alloc(device->machine, bitbanger_overthreshhold, (void *) device);
	bi->over_threshhold = 1;
}

static void bitbanger_analyze(const device_config *device)
{
	int i, factor, total_duration;
	double smallest_pulse;
	double d;
	bitbanger_token *bi = get_token(device);
	const bitbanger_config *config = get_config(device);
	
	/* compute the smallest pulse */
	smallest_pulse = config->pulse_threshhold;
	for (i = 0; i < bi->recorded_pulses; i++)
		if (smallest_pulse > bi->pulses[i])
			smallest_pulse = bi->pulses[i];
	
	/* figure out what factor the smallest pulse was */
	factor = 0;
	do
	{
		factor++;
		total_duration = 0;
		
		for (i = 0; i < bi->recorded_pulses; i++)
		{
			d = bi->pulses[i] / smallest_pulse * factor + config->pulse_tolerance;
			if ((i < (bi->recorded_pulses-1)) && (d - floor(d)) >= (config->pulse_tolerance * 2))
			{
				i = -1;
				break;
			}
			bi->factored_pulses[i] = (int) d;
			total_duration += (int) d;
		}
	}
	while((i == -1) && (factor < config->maximum_pulses));
	if (i == -1)
		return;
	
	/* filter the output */
	if (config->filter(device, bi->factored_pulses, bi->recorded_pulses, total_duration))
		bi->recorded_pulses = 0;
}

static void bitbanger_addpulse(const device_config *device, double pulse_width)
{
	bitbanger_token *bi = get_token(device);
	const bitbanger_config *config = get_config(device);
	
	/* exceeded total countable pulses? */
	if (bi->recorded_pulses == config->maximum_pulses)
		memmove(bi->pulses, bi->pulses + 1, (--bi->recorded_pulses) * sizeof(double));
	
	/* record the pulse */
	bi->pulses[bi->recorded_pulses++] = pulse_width;
	
	/* analyze, if necessary */
	if (bi->recorded_pulses >= config->minimum_pulses)
		bitbanger_analyze(device);
}

static TIMER_CALLBACK(bitbanger_overthreshhold)
{
	const device_config *device = (const device_config *) ptr;
	bitbanger_token *bi = get_token(device);
	
	bitbanger_addpulse(device, attotime_to_double(timer_get_time(machine)) - bi->last_pulse_time);
	bi->over_threshhold = 1;
	bi->recorded_pulses = 0;
}


/**
	bitbanger_output - outputs data to a bitbanger port
*/
void bb_output(emudevice_t * pEmuDevice, int value)
{
	double				current_time;
	double				pulse_width;
	
	/* normalize input */
	value = value ? 1 : 0;
	
	bi = get_token(pBB);
	
	/* only meaningful if we change */
	if ( bi->value != value )
	{
		current_time = attotime_to_double(timer_get_time(device->machine));
		pulse_width = current_time - bi->last_pulse_time;
		
		assert(pulse_width >= 0);
		
		if (    ! bi->over_threshhold 
			 && (   (bi->recorded_pulses > 0) 
				 || (bi->value == config->initial_value))
			)
		{
			bitbanger_addpulse(device, pulse_width);
		}
		
		/* update state */
		bi->value			= value;
		bi->last_pulse_time = current_time;
		bi->over_threshhold = 0;
		
		/* update timeout timer */
		timer_reset(bi->timeout_timer, double_to_attotime(config->pulse_threshhold));
	}
}
#endif

/****************************************************************************/
/****************************************************************************/

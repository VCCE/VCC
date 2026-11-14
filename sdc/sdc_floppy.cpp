//#define USE_LOGGING
//#define USE_STAMPEDLOG
//#define USE_LOGFILE
//======================================================================
// SDC simulator.  EJ Jaquay 2025
//
// This file is part of VCC (Virtual Color Computer).
// Vcc is Copyright 2015 by Joseph Forgione
//
// VCC (Virtual Color Computer) is free software, you can redistribute
// and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, either version 3 of
// the License, or (at your option) any later version.
//
// VCC (Virtual Color Computer) is distributed in the hope that it will
// be useful, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with VCC (Virtual Color Computer).  If not, see
// <http://www.gnu.org/licenses/>.
//
//======================================================================

//----------------------------------------------------------------------
//  $FF40 ; Control register (write)
//      Bit 7 halt flag 0 = disabled 1 = enabled
//      Bit 6 drive select 3 or side select
//      Bit 5 density flag 0 = single 1 = double
//      Bit 4 write precompensation 0 = no precomp 1 = precomp
//      Bit 3 drive motor enable 0 = motors off 1 = motors on
//      Bit 2 drive select 2
//      Bit 1 drive select 1
//      Bit 0 drive select 0
//
//  $FF48 ; Command register (write)
//      high order nibble; low order nibble type; command
//      0x0 ;   I ; Restore
//      0x1 ;   I ; Seek
//      0x2 ;   I ; Step
//      0x4 ;   I ; Step in
//      0x5 ;   I ; Step out
//      0x8 ;  II ; Read sector
//      0x9 ;  II ; Read sector mfm
//      0xA ;  II ; write sector
//      0xB ;  II ; write sector mfm
//      0xC ; III ; read address
//      0xD ; III ; force interrupt
//      0xE ; III ; read track
//      0xF ;  IV ; write track
//      low order nibble
//      type ; meanings
//        I ; b3 head loaded, b2 track verify , b1-b0 step rate
//       II ; b3 side compare enable, b2 delay, b1 side, b0 0
//      III ; b2 delay others 0
//       IV ; interrupt control b3 immediate, b2 index pulse, b1 notready, b0 ready
//
//  The becker port (drivewire) uses port $FF41 for status and port $FF42
//  for data so these must be always allowed for becker.dll to work.
//
//  NOTE: Stock SDCDOS does not support the becker ports, it expects
//  drivewire to be on the bitbanger ports. RGBDOS does, however, and will
//  still work with sdc.dll installed.
//
//----------------------------------------------------------------------

#include <vcc/util/logger.h>
#include "sdc.h"

//----------------------------------------------------------------------
// Floppy globals
//----------------------------------------------------------------------

// Floppy Status bits.
#define FLP_READY     0      // No status bits set
#define FLP_BUSY      0x01   // b0 Busy, command is executing
#define FLP_NOTREADY  0x80   // b7 Not ready (no disk inserted)

#define FLP_READERR   0x08   // b3 Read error (CRC) types I & II
#define FLP_WRITEERR  0x20   // b5 Write error (Fault)
#define FLP_READONLY  0x40   // b6 Write protected

// Note: bits 1,2,and 4 depend on command type
// Type I commands: RESTORE,SEEK,STEP 
#define FLP_INDEX     0x02   // b1 INDEX
#define FLP_TRKZERO   0x04   // b2 Track 0
#define FLP_SEEKERR   0x10   // b4 Seek track error
// Type II/III comands: READ, WRITE
#define FLP_DATAREQ   0x02   // b1 DRQ, data is ready
#define FLP_DATALOST  0x04   // b2 Lost data, CPU failed to service DRQ
#define FLP_RNF       0x10   // b4 Record (sector) Not Found

#define WAITTIME      5      // Command timeout wait (steps)
#define STEPTIME      15     // 15 pings = 0.95 milliseconds

//Control register masks
#define CTRL_DRIVE0     0x01
#define CTRL_DRIVE1     0x02
#define CTRL_DRIVE2     0x04
#define CTRL_MOTORON    0x08
#define CTRL_WRTPRECMP  0x10
#define CTRL_INTENABLE  0x20
#define CTRL_SIDESELECT 0x40
#define CTRL_HALTFLAG   0x80

// Floppy controller commands
enum CommandType {
    RESTORE,
    SEEK,
    STEP,
    STEPUPD,
    STEPIN,
    STEPINUPD,
    STEPOUT,
    STEPOUTUPD,
    READSECTOR,
    READSECTORM,
    WRITESECTOR,
    WRITESECTORM,
    READADDRESS,
    FORCEINTERUPT,
    READTRACK,
    WRITETRACK,
    IDLE = 255
};

void FloppyCommand(unsigned char);
void FloppyRestore();
void FloppySeek();
void FloppyReadSector();
void FloppyWriteSector();
void FloppyWriteByte(unsigned char);
void CommandDone();
void DecodeControlRegister(unsigned char); 
unsigned int FloppyLSN(unsigned int,unsigned int,unsigned int);
static unsigned char FloppyReadByte();

typedef void (*DelayedFunc)(void);
static struct {
    bool active;
    int remaining_hsyncs;
    DelayedFunc func;
} DelayedExec;

void delay_execution(int count, DelayedFunc func);

//================================================================
// Floppy state variables
//================================================================
//
static unsigned char DataReg = 0;
static unsigned char FlopDrive = 0;
static unsigned char FlopData = 0;
static unsigned char FlopTrack = 0;
static unsigned char FlopSide = 0;
static unsigned char FlopSector = 0;
static unsigned char FlopStatus = FLP_READY; // All bits clear
static DWORD FlopWrCnt = 0;
static DWORD FlopRdCnt = 0;
static unsigned char FlopWrBuf[256];
static unsigned char FlopRdBuf[256];
static unsigned char FloppyPortData=0;
static unsigned char FlopHaltEnable=0;
static unsigned char FlopNMIenable=0;
static unsigned char FlopMotorOn=0;
static unsigned int IOWaiter=0;        // Command timeout counter
static unsigned int ExecTimeWaiter=0;  // Command wait timer
static CommandType  CurrentCmd=IDLE;   // Current command
static unsigned int CmdQualifier;      // Command qualifier bits

// Debuging metrics
static uint32_t hsync_clock=0;   // Increment on hsync (Ping)
static uint32_t data_sum = 0;    // Total DataReg values sent this command

//================================================================

unsigned char FloppyPortRead(unsigned char port)
{
    // Any floppy port read disables halt
    FlopHaltEnable = 0;

    unsigned int temp=0;
    switch (port) {
   // Floppy status 
    case 0x48:
        temp = FlopStatus;
        break;
    // Floppy track 
    case 0x49:
        temp = FlopTrack;
        break;
    // Floppy sector 
    case 0x4A:
        temp = FlopSector;
        break;
    // Floppy data
    case 0x4B:
        if (CurrentCmd == IDLE) {
            // Nitros rb1773 driver uses DataReg echo to verify drive exists 
            temp = DataReg;
        } else {
            temp = FloppyReadByte();
        }
        break;
    default:
        break;
    }

#ifdef USE_STAMPEDLOG
    if (port == 0x4B) {
        if (CurrentCmd==IDLE) {
            LOG_STAMPED(hsync_clock,"floppy idle read data %02x",temp);
        } else {
            LOG_STAMPED(hsync_clock,"floppy read data"); //gives count
            data_sum += temp;
        }
    } else {
        LOG_STAMPED(hsync_clock,"floppy read port %02x %02x",port,temp);
    }
#endif

    return temp;
}

void FloppyPortWrite(unsigned char data, unsigned char port)
{
    switch (port) {
    // Floppy control
    case 0x40:
        DecodeControlRegister(data);
        break;
    // Becker data (future)
    case 0x42:
        break;
    // Floppy command
    case 0x48:
        FloppyCommand(data);
        break;
    // set track
    case 0x49:
        LOG_STAMPED(hsync_clock,"floppy set track %d",data);
        FlopTrack = data;
        break;
    // set sector
    case 0x4A:
        LOG_STAMPED(hsync_clock,"floppy set sector %d",data);
        FlopSector = data;
        break;
    // write data byte
    case 0x4B:
        if (CurrentCmd==IDLE) {
            LOG_STAMPED(hsync_clock,"floppy idle write data %02x",data);
            DataReg=data;
        } else {
            LOG_STAMPED(hsync_clock,"floppy write data"); //logs count
            FloppyWriteByte(data);
        }
        break;
    }
    return;
}

//------------------------------------------------
// Disk control 
//  b7 halt enable flag
//  b6 select drive 3, select side
//  b5 double density, NMI enable flag
//  b4 write precompensation
//  b3 drive motor
//  b2 select drive 2
//  b1 select drive 1
//  b0 select drive 0
//------------------------------------------------
void DecodeControlRegister(unsigned char data)  {

    FlopHaltEnable = (data & CTRL_HALTFLAG)   ? 1 : 0;
    FlopSide       = (data & CTRL_SIDESELECT) ? 1 : 0;
    FlopNMIenable  = (data & CTRL_INTENABLE)  ? 1 : 0;
    FlopMotorOn    = (data & CTRL_MOTORON)    ? 1 : 0;

    // Detect changes to drive or side?
    FlopDrive = 4;
    if (data & CTRL_DRIVE0)
        FlopDrive = 0;
    if (data & CTRL_DRIVE1)
        FlopDrive = 1;
    if (data & CTRL_DRIVE2)
        FlopDrive = 2;
    if (FlopSide && FlopDrive == 4) {
        FlopSide = 0;
        FlopDrive = 3;
    }

    LOG_STAMPED(hsync_clock,
        "floppy decode control dr:%d si:%d halt:%d ienab:%d Mot:%d",
         FlopDrive, FlopSide, FlopHaltEnable, FlopNMIenable, FlopMotorOn);
}

//------------------------------------------------
// floppy command.
//------------------------------------------------
void FloppyCommand(unsigned char data)
{
    // BUSY remains true while controller is doing command
    FlopStatus |= FLP_BUSY;

    CurrentCmd = static_cast<CommandType>(data >> 4);
    CmdQualifier = data & 0xF;

    switch (CurrentCmd) {
    case RESTORE:
        LOG_STAMPED(hsync_clock,"floppy RESTORE");
        FloppyRestore();
        break;
    case SEEK:
        LOG_STAMPED(hsync_clock,"floppy SEEK");
        FloppySeek();
        break;
    case STEP:
    case STEPUPD:
        LOG_STAMPED(hsync_clock,"floppy STEP");
        break;
    case STEPIN:
    case STEPINUPD:
        LOG_STAMPED(hsync_clock,"floppy STEPIN");
        break;
    case STEPOUT:
    case STEPOUTUPD:
        LOG_STAMPED(hsync_clock,"floppy STEPOUT");
        break;
    case READSECTOR:
    case READSECTORM:
        LOG_STAMPED(hsync_clock,"floppy READSECTOR");
        FloppyReadSector();
        break;
    case WRITESECTOR:
    case WRITESECTORM:
        LOG_STAMPED(hsync_clock,"floppy WRITESECTOR");
        FloppyWriteSector();
        break;
    case READADDRESS:
        LOG_STAMPED(hsync_clock,"floppy READADDRESS");
        break;
    case FORCEINTERUPT:
        LOG_STAMPED(hsync_clock,"floppy FORCEINTERRUPT");
        CommandDone();
        FlopStatus = FLP_READY;
        break;
    case READTRACK:
        LOG_STAMPED(hsync_clock,"floppy READTRACK");
        break;
    case WRITETRACK:
        LOG_STAMPED(hsync_clock,"floppy WRITETRACK");
        break;
    }
}

//----------------------------------------------------------------------
// Command complete
//----------------------------------------------------------------------
void CommandDone()
{
    CurrentCmd = IDLE;

    IOWaiter = 0;
    FlopRdCnt = 0;
    FlopWrCnt = 0;
    FlopStatus &= ~FLP_BUSY;
    FlopStatus &= ~FLP_DATAREQ;

#ifdef USE_STAMPEDLOG
{
    LOG_STAMPED(hsync_clock,"floppy command DONE sum %d NMI:%d",data_sum,FlopNMIenable);
//    LOG_STAMPED(hsync_clock,"floppy debug 100 ms CPU pause");
//    Sleep(100);
    data_sum = 0;
}
#endif

    if (FlopNMIenable)
        AssertNMI();
//  loops  delay_execution(1, AssertNMI);
}

//----------------------------------------------------------------------
// floppy restore
//----------------------------------------------------------------------
void FloppyRestore()
{
    DLOG("FloppyRestore\n");
    FlopTrack = 0;
    FlopSide = 0;
    FlopSector = 0;
    FlopStatus |= FLP_TRKZERO;
    delay_execution(100,CommandDone);
}

//----------------------------------------------------------------------
// floppy seek.
//----------------------------------------------------------------------
void FloppySeek()
{

    DLOG("FloppySeek drive %d side %d track %d data %d\n",
        FlopDrive,FlopSide,FlopTrack,DataReg);

    FlopTrack = FlopData;  //FIXME! What does set track do then?

    if (FlopTrack == 0) FlopStatus |= FLP_TRKZERO;
    delay_execution(10,CommandDone);
    //CommandDone();
}

//----------------------------------------------------------------------
// convert side (drive), track, sector, to LSN.  The floppy controller
// uses CHS addressing while hard drives, and the SDC, use LBA addressing.
//----------------------------------------------------------------------
unsigned int FloppyLSN(
        unsigned int drive,   // 0 or 1
        unsigned int track,   // 0 to num tracks
        unsigned int side,    // 0 or 1
        unsigned int sector   // 1 to 18
    )
{
    if (gCocoDisk[drive].doublesided)
        return (track * 2 + side) * 18 + (sector - 1);
    else
        return track * 18 + sector - 1;
}

//----------------------------------------------------------------------
// floppy read sector
//----------------------------------------------------------------------
void FloppyReadSector()
{
    auto lsn = FloppyLSN(FlopDrive,FlopTrack,FlopSide,FlopSector);
    
    LOG_STAMPED(hsync_clock,"floppy Read dr:%d si:%d t:%d s:%d lsn:%u",
        FlopDrive,FlopSide,FlopTrack,FlopSector,lsn);

    // Update status line FIXME move to SDC
    // UpdateStatusLine(gLoadedBank,FlopDrive,lsn);

    // Buffer should be empty, but make sure
    // FIXME Seperate buffer per drive????
    FlopRdCnt = 0;

    // Seek the sector
    if (!SDCSeekSector(FlopDrive,lsn)) {
        FlopStatus = FLP_SEEKERR;
        DLOG("FloppyReadSector seek error %d %d\n",FlopDrive,lsn);
        return;
    }

    // Load buffer into the sector
    auto hf = gCocoDisk[FlopDrive].hFile;
    while (FlopRdCnt < 256) {
        DWORD cnt = 0;
        if (!ReadFile(hf,FlopRdBuf+FlopRdCnt,256-FlopRdCnt,&cnt,NULL)) {
            DLOG("FloppyReadSector read error %d\n",FlopDrive);
            FlopStatus = FLP_READERR;
            return;
        }
        if (cnt < 1) {
            FlopStatus = FLP_READERR;
            DLOG("FloppyReadSector short read %d\n",FlopDrive);
            return;
        }
        FlopRdCnt += cnt;
    }

    FlopStatus = FLP_BUSY | FLP_DATAREQ;
}

//----------------------------------------------------------------------
// floppy write sector
//----------------------------------------------------------------------
void FloppyWriteSector()
{
    // write not implemented
    int lsn = FlopTrack * 18 + FlopSector - 1;
    DLOG("FloppyWriteSector %d %d not implmented\n",FlopDrive,lsn);
    FlopStatus = FLP_READONLY;
}

//----------------------------------------------------------------------
// floppy write data
//----------------------------------------------------------------------
void FloppyWriteByte(unsigned char data)
{
    if (FlopWrCnt<256)  {
        FlopWrCnt++;
        FlopWrBuf[FlopWrCnt] = data;
    } else {
        if ((FlopStatus &= FLP_DATAREQ) != 0) {
            CommandDone();
        }
        FlopStatus = FLP_READY;
    }
    FlopData = data;
}

//----------------------------------------------------------------------
// floppy read data
//----------------------------------------------------------------------

unsigned char FloppyReadByte()
{
    unsigned char rpy=0;

    if (FlopRdCnt>0)  {
        rpy = FlopRdBuf[256-FlopRdCnt];
        FlopRdCnt--;
        if (FlopRdCnt == 0) FlopSector++;
        FlopStatus = FLP_BUSY | FLP_DATAREQ;
        return rpy;
    } else {
        CommandDone();
        return 0;
    }
}

//------------------------------------------------------------
// Code to delay function execution for count hsyncs follows
//------------------------------------------------------------
void delay_execution(int count, DelayedFunc func)
{
    if (DelayedExec.active) {
        DLOG("delay_execution active - request denied\n");
        return;
    }
    if (count == 0) {
        func();
        return;
    }

    DelayedExec.active = true;
    DelayedExec.remaining_hsyncs = count;
    DelayedExec.func = func;
}

//------------------------------------------------------------
// Ping occurs on hsync 
//------------------------------------------------------------
void PingFloppy(void)
{
    hsync_clock++;

    // time out command
    if (CurrentCmd != IDLE) {
        if (IOWaiter++ > 2000) { 
            LOG_STAMPED(hsync_clock,"floppy TIMEOUT");
            CommandDone();
        }
    }

    if (!DelayedExec.active) return;
    if (--DelayedExec.remaining_hsyncs > 0) return;
    DelayedFunc func = DelayedExec.func;
    DelayedExec.active = false;
    DelayedExec.func = NULL;
    func();
}


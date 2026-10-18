#define USE_LOGGING
#define USE_STAMPEDLOG
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
void DoFloppyCommand();
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
static CommandType Command=IDLE;       // Current command

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
    case 0x48:
        temp = FlopStatus;
        break;
    case 0x49:
        temp = FlopTrack;
        break;
    case 0x4A:
        temp = FlopSector;
        break;
    case 0x4B:
        if (Command == IDLE) {
            temp = 0;
        } else {
            temp = FloppyReadByte();
        }
        data_sum += temp;
        break;
    default:
        break;
    }
    LOG_TRAFFIC("floppy read port ",port,temp);
    return temp;
}

void FloppyPortWrite(unsigned char data, unsigned char port)
{
    LOG_TRAFFIC("floppy write port ",port,data);

    switch (port) {
    // Floppy control
    case 0x40: //DSKREG
        DecodeControlRegister(data);
        break;
    // Floppy command
    case 0x48:
        FloppyCommand(data);
        break;
    // set track
    case 0x49:
        FlopTrack = data;
        break;
    // set sector
    case 0x4A:
        FlopSector = data;
        break;
    // write data byte
    case 0x4B:
        if (Command==IDLE) {
            DataReg=data;
        } else {
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

    FlopHaltEnable = (data & 0b10000000) ? 1 : 0;
    FlopNMIenable  = (data & 0b00100000) ? 1 : 0;
    FlopMotorOn    = (data & 0b00001000) ? 1 : 0;

    // Drive and side select. Detect changes?
    switch (data & 0b0100111) {
    case 0b0000001: // drive 0 side 0
            FlopDrive = 0;
            FlopSide = 0;
            break;
    case 0b0100001: // drive 0 side 1
            FlopDrive = 0;
            FlopSide = 1;
            break;
    case 0b0000010: // drive 1 side 0
            FlopDrive = 1;
            FlopSide = 0;
            break;
    case 0b0100010: // drive 1 side 1
            FlopDrive = 1;
            FlopSide = 1;
            break;
    case 0b0000100: // drive 2 side 0
            FlopDrive = 2;
            FlopSide = 0;
            break;
    case 0b0100100: // drive 2 side 1
            FlopDrive = 2;
            FlopSide = 1;
            break;
    case 0b0100000: // drive 3 side 0
            FlopDrive = 3;
            FlopSide = 0;
            break;
    }
#ifdef USE_STAMPEDLOG
    {
        char log[80];
        snprintf(log,sizeof(log),
            "floppy control dr:%d si:%d halt:%d ienab:%d Mot:%d",
            FlopDrive, FlopSide, FlopHaltEnable, FlopNMIenable, FlopMotorOn);
        LOG_STAMPED(hsync_clock,log);
    }
#endif
}

//#define CMD_DELAY 

static bool CommandPending={};
static unsigned int CommandDelay = 0;

//------------------------------------------------
// floppy command.
//------------------------------------------------
void FloppyCommand(unsigned char data)
{

    // BUSY remains true while controller is doing command
    FlopStatus |= FLP_BUSY;

    unsigned char cmd = data >> 4;
    switch (cmd) {
    case 0:
        LOG_STAMPED(hsync_clock,"floppy RESTORE");
        Command = RESTORE;
        break;
    case 1:
        Command = SEEK;
        break;
    case 2:  //STEP
    case 3:  //STEPUPD
        break;
    case 4:  //STEPIN
    case 5:  //STEPINUPD
        break;
    case 6:  //STEPOUT
    case 7:  //STEPOUTUPD
        break;
    case 8:
    case 9:
        Command = READSECTOR;
        break;
    case 10:
    case 11:
        Command = WRITESECTOR;
        break;
    case 12:
        Command = READADDRESS;
        break;
    case 13: // FORCEINTERRUPT
LOG_STAMPED(hsync_clock,"floppy FORCEINTERRUPT");
        CommandDone();
        FlopStatus = FLP_READY;
        break;
    case 14: 
        break;
    case 15:
        break;
    default:
        DLOG_C("FloppyCommand unknown %02x\n",data);
        Command = IDLE;
        break;
    }

// TODO:  Remove this CMD_DELAY logic

    if (Command != IDLE) {
#ifdef CMD_DELAY
        if (CommandDelay > 0) {
            DLOG_C("FloppyCommand pending %02x\n",Command);
            ExecTimeWaiter = 0;
            CommandPending = true;
        } else {
            DLOG_C("FloppyCommand immediate %02x\n",Command);
            DoFloppyCommand();
        }
#else
//        DLOG_C("FloppyCommand nowait %02x\n",Command);
        DoFloppyCommand();
#endif
    }
}

//----------------------------------------------------------------------
// Command complete
//----------------------------------------------------------------------
void CommandDone()
{
    // Command == IDLE and CommandPending == FALSE are not the same thing
    // IDLE controls DATREG reads/writes
    // CommandPending is a command state - waiting for command to execute
    // it is cleared when command starts execution.
    Command = IDLE;
    CommandPending = false;

    IOWaiter = 0;
    FlopRdCnt = 0;
    FlopWrCnt = 0;
    FlopStatus &= ~FLP_BUSY;
    FlopStatus &= ~FLP_DATAREQ;

    char log[64];
    if (FlopNMIenable) {
        snprintf(log,sizeof(log),"floppy command DONE sum %d assert NMI",data_sum);
	} else {
        snprintf(log,sizeof(log),"floppy command DONE sum %d",data_sum);
	}
	LOG_STAMPED(hsync_clock,log);
    data_sum = 0;

    if (FlopNMIenable) AssertNMI();
}

//------------------------------------------------
// Do command This redirect is probably unneccesary for FDC
//------------------------------------------------
void DoFloppyCommand() {

    switch (Command) {
//        case IDLE:
//            break;
        case RESTORE:
            DLOG_C("DoFloppyCommand RESTORE\n");
            FloppyRestore();
            break;
        case SEEK:
            DLOG_C("DoFloppyCommand SEEK\n");
            FloppySeek();
            break;
        case STEP:
            break;
        case STEPUPD:
            break;
        case STEPIN:
        case STEPINUPD:
            break;
        case STEPOUT:
        case STEPOUTUPD:
            break;
        case READSECTOR:
        case READSECTORM:
            FloppyReadSector();
            break;
        case WRITESECTOR:
        case WRITESECTORM:
            DLOG_C("DoFloppyCommand WRITESECTOR\n");
            FloppyWriteSector();
            break;
        case READADDRESS:
            break;
//        case FORCEINTERUPT:
//            Command = IDLE;
//            FlopRdCnt = 0;
//            FlopStatus = FLP_READY;
//            break;
        case READTRACK:
        case WRITETRACK:
            break;
    }
}

//----------------------------------------------------------------------
// floppy restore
//----------------------------------------------------------------------
void FloppyRestore()
{
    DLOG_C("FloppyRestore\n");
    FlopTrack = 0;
    FlopSide = 0;
    FlopSector = 0;
    CommandDone();
}

//----------------------------------------------------------------------
// floppy seek.
//----------------------------------------------------------------------
void FloppySeek()
{

    DLOG_C("FloppySeek drive %d side %d track %d data %d\n",
        FlopDrive,FlopSide,FlopTrack,DataReg);

    FlopTrack = FlopData;  //FIXME! What does set track do then?

    if (FlopTrack == 0) FlopStatus |= FLP_TRKZERO;
    CommandDone();
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
    
#ifdef USE_STAMPEDLOG
{
    char log[64];
    snprintf(log,sizeof(log),"Floppy Read dr:%d si:%d t:%d s:%d lsn:%d",
            FlopDrive,FlopSide,FlopTrack,FlopSector,lsn);
    LOG_STAMPED(hsync_clock,log);
}
#endif

    // Update status line FIXME move to SDC
    // UpdateStatusLine(gLoadedBank,FlopDrive,lsn);

    // Buffer should be empty, but make sure
    // FIXME Seperate buffer per drive????
    FlopRdCnt = 0;

    // Seek the sector
    if (!SDCSeekSector(FlopDrive,lsn)) {
        FlopStatus = FLP_SEEKERR;
        DLOG_C("FloppyReadSector seek error %d %d\n",FlopDrive,lsn);
        return;
    }

    // Load buffer into the sector
    auto hf = gCocoDisk[FlopDrive].hFile;
    while (FlopRdCnt < 256) {
        DWORD cnt = 0;
        if (!ReadFile(hf,FlopRdBuf+FlopRdCnt,256-FlopRdCnt,&cnt,NULL)) {
            DLOG_C("FloppyReadSector read error %d\n",FlopDrive);
            FlopStatus = FLP_READERR;
            return;
        }
        if (cnt < 1) {
            FlopStatus = FLP_READERR;
            DLOG_C("FloppyReadSector short read %d\n",FlopDrive);
            return;
        }
        FlopRdCnt += cnt;
    }

    FlopStatus = FLP_BUSY | FLP_DATAREQ;
//    DLOG_C("FloppyReadSector %d %d %d bytes\n",FlopDrive,lsn,FlopRdCnt);
}

//----------------------------------------------------------------------
// floppy write sector
//----------------------------------------------------------------------
void FloppyWriteSector()
{
    // write not implemented
    int lsn = FlopTrack * 18 + FlopSector - 1;
    DLOG_C("FloppyWriteSector %d %d not implmented\n",FlopDrive,lsn);
    FlopStatus = FLP_READONLY;
}

//----------------------------------------------------------------------
// floppy write data
//----------------------------------------------------------------------
void FloppyWriteByte(unsigned char data)
{
    //DLOG_C("FloppyWriteByte %d\n",data);
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
/*
    if (FlopRdCnt == 0) {
        DLOG_C("FloppyReadByte starting sec %d\n",FlopSector);
Sleep(1000);
        FloppyReadSector();
        DLOG_C("FloppyReadByte read %d bytes\n",FlopRdCnt);
//        CommandDone();
//        FlopStatus = FLP_RNF;
//        delay_execution(1, CommandDone);
    }
*/
    if (FlopRdCnt == 0) {
        CommandDone();
//        FlopStatus = FLP_RNF;
        return 0;
    }

    if (FlopRdCnt>0)  {
//        if (FlopRdCnt > 253)
//            DLOG_C("FloppyReadByte buffer read byte %d sec %d\n",256-FlopRdCnt,FlopSector);
        rpy = FlopRdBuf[256-FlopRdCnt];
        FlopRdCnt--;
        if (FlopRdCnt == 0) FlopSector++;
        FlopStatus = FLP_BUSY | FLP_DATAREQ;
    }// else {
     //   DLOG_C("FloppyReadByte buffer read complete\n");
     //   FlopStatus = FLP_READY;
     //   CommandDone();
     //   // Check for lost data here
     //   FlopSector++;
     //}

//    if (FlopRdCnt == 0) {
//        DLOG_C("FloppyReadByte buffer read complete\n");
//        CommandDone();
//    }
    return rpy;
}

//------------------------------------------------------------
// Code to delay function execution for count hsyncs follows
//------------------------------------------------------------

void delay_execution(int count, DelayedFunc func)
{
//    assert(!DelayedExec.active);
    if (DelayedExec.active) {
        DLOG_C("delay_execution active - request denied\n");
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

void PingFloppy(void)
//void process_delayed_execution(void)
{
hsync_clock++;

    if (Command != IDLE) {
        if (IOWaiter++ > 200) 
            CommandDone();
    }
    if (!DelayedExec.active) return;
    if (--DelayedExec.remaining_hsyncs > 0) return;
    DelayedFunc func = DelayedExec.func;
    DelayedExec.active = false;
    DelayedExec.func = NULL;
DLOG_C("PingFloppy calling function %p\n",func);
    func();
}

//        if (IOWaiter++ > 100) {
// usage:
//void CommandDone()
//{
//    Command = IDLE;
//    CommandPending = false;
//
//    FlopStatus &= ~FLP_BUSY;
//    FlopStatus &= ~FLP_DATAREQ;
//
//    if (FlopNMIenable)
//        delay_execution(1, AssertNMI);
//}


#include "acia.h"

//------------------------------------------------------------------------
// sc6551
//------------------------------------------------------------------------

// sc6551 private functions
DWORD WINAPI sc6551_input_thread(LPVOID);
DWORD WINAPI sc6551_output_thread(LPVOID);
void sc6551_output(char);

// Comunications hooks
void com_open();
void com_close();
int  com_write(char*,int);
int  com_read(char*,int);

// Stat Reg is protected by a critical section
// to prevent race conditions
unsigned char StatReg = 0;

// Status register bits.
// b0-B2  parity, frame, overrun error
// b3 Rcv Data register full if tru
// b4 Snd Data register empty if true
// b5 DCD Data carrier detect
// b6 DSR Data set ready
// b7 IRQ Interrupt generated
// Only RxF, TxE, and IRQ are used here
#define StatOvr  0x04
#define StatRxF  0x08
#define StatTxE  0x10
#define StatIRQ  0x80

unsigned char CmdReg  = 0;
// Command register bits.
// b0 DTR 1 = enable receive and interupts
// b1 Receiver IRQ enable.
// b3 b2  Transmit IRQ control
// B4 Echo mode. 0 = normal 1 = Echo (B2&B3 zero)
// B7 B6 B4 parity control
// Only DTR is used here
#define CmdDTR   0x01

// CtrReg is not really used, it is just echoed back to CoCo
unsigned char CtlReg  = 0;

HANDLE hEventRead;       // Event handle for read thread
HANDLE hInputThread;
HANDLE hOutputThread;

#define INBSIZ 256
#define OUTBSIZ 1024
char InBuf[INBSIZ];
int  InBufCnt=0;
int  InReadCnt=0;
char OutBuf[OUTBSIZ];
int  OutBufCnt=0;
HANDLE  WriteLock;

//------------------------------------------------------------------------
//  Initiallize sc6551. This gets called when DTR is set on
//------------------------------------------------------------------------

void sc6551_init()
{
    DWORD id;

    // Close any previous instance then open communications
    sc6551_close();
    com_open();

    // Create input thread and event to wake it
    hInputThread=CreateThread(NULL,0,sc6551_input_thread,NULL,0,&id);
    hEventRead = CreateEvent (NULL,FALSE,FALSE,NULL);
    hOutputThread=CreateThread(NULL,0,sc6551_output_thread,NULL,0,&id);
    WriteLock = CreateMutex(NULL,FALSE,NULL);
    sc6551_initialized = 1;
}

//------------------------------------------------------------------------
//  Close sc6551.  This gets called when DTR is set off
//------------------------------------------------------------------------
void sc6551_close()
{
    if (hInputThread) {
        TerminateThread(hInputThread,1);
        WaitForSingleObject(hInputThread,2000);
    }
    if (hOutputThread) {
        TerminateThread(hOutputThread,1);
        WaitForSingleObject(hOutputThread,2000);
    }
    hInputThread = NULL;
    hOutputThread = NULL;
    com_close();
    sc6551_initialized = 0;
}

//------------------------------------------------------------------------
// Data input
// Reads hang. They are done in a seperate thread.
//------------------------------------------------------------------------

DWORD WINAPI sc6551_input_thread(LPVOID param)
{
    DWORD rc;
    DWORD ms=100;

    while(TRUE) {
        rc = WaitForSingleObject(hEventRead,ms);
        if (CmdReg & CmdDTR) {
            if ( InReadCnt >= InBufCnt ) {
                InBufCnt = com_read(InBuf,INBSIZ);
                InReadCnt = 0;
                StatReg = StatReg | StatRxF;
                ms = 2;
            } else if (rc == WAIT_TIMEOUT) {
                if ( InReadCnt < InBufCnt ) StatReg = StatReg | StatRxF;
                StatReg = StatReg | StatIRQ;
                AssertInt(IRQ,0);
                ms = 100;
            }
        }
    }
}

//------------------------------------------------------------------------
// Output is buffered for 50 ms
//------------------------------------------------------------------------

DWORD WINAPI sc6551_output_thread(LPVOID param)
{
    while(TRUE) {
        if (OutBufCnt) {
            WaitForSingleObject(WriteLock,INFINITE);
            com_write(OutBuf,OutBufCnt);
            OutBufCnt = 0;
            StatReg = StatReg | StatTxE;
            ReleaseMutex(WriteLock);
        }
        Sleep(50);
    }
}

//------------------------------------------------------------------------
// Port I/O
//------------------------------------------------------------------------

unsigned char sc6551_read(unsigned char port)
{
    unsigned char data;
    switch (port) {
        case 0x68:
            data = InBuf[InReadCnt];
            InReadCnt++;
            if (InReadCnt >= InBufCnt) StatReg = StatReg &~ StatRxF;
            SetEvent(hEventRead);       // data was read
            break;
        case 0x69:
            data = StatReg;
            StatReg = StatReg & ~StatIRQ;   // Stat read clears IRQ flag
            break;
        case 0x6A:
            data = CmdReg;
            break;
        case 0x6B:
            data = CtlReg;
            break;
    }
    return data;
}

void sc6551_write(unsigned char data,unsigned short port)
{
    switch (port) {
        case 0x68:
            //Wait for output thread to finish
            WaitForSingleObject(WriteLock,2000);
            OutBuf[OutBufCnt++] = data;
            if (OutBufCnt < OUTBSIZ) {
                StatReg = StatReg | StatTxE;
            } else {
                StatReg = StatReg & ~StatTxE;
            }
            ReleaseMutex(WriteLock);
            break;
        case 0x69:
            StatReg = 0;
            break;
        case 0x6A:
            CmdReg = data;
            // If DTR set enable sc6551
            if (CmdReg & CmdDTR) {
                if (sc6551_initialized == 0) sc6551_init();
                StatReg = StatTxE;
                SetEvent(hEventRead); // tell input worker
            } else {
            // Else disable sc6551
                sc6551_close();
                StatReg = 0;
            }
            break;
        case 0x6B:
            CtlReg = data;  // Not used, just returned on read
            break;
    }
}

//----------------------------------------------------------------
// Dispatch I/0 to communication type used.
// Hooks allow sc6551 to do communications to selected media
//----------------------------------------------------------------


// Open com
void com_open() {
    switch (AciaComType) {
    case 0: // Legacy Console
        console_open();
        break;
    case 1: 
        wincmd_open();
        break;
    }
}

void com_close() {
    switch (AciaComType) {
    case 0: // Console
        console_close();
        break;
    case 1: 
        wincmd_close();
        break;
    }
}

int com_write(char * buf,int len) { // returns bytes written
    switch (AciaComType) {
    case 0: // Legacy Console
        return console_write(buf,len);
    case 1: 
        return wincmd_write(buf,len);
    }
    return 0;
}

int com_read(char * buf,int len) {  // returns bytes read
    switch (AciaComType) {
    case 0: // Legacy Console
        return console_read(buf,len);
    case 1: 
        return wincmd_read(buf,len);
    }
    return 0;
}


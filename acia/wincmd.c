
//------------------------------------------------------------------
// Com with windows cmd line process
//------------------------------------------------------------------

#include "acia.h"

HANDLE hChildInR = NULL;
HANDLE hChildInW = NULL;
HANDLE hChildOutR = NULL;
HANDLE hChildOutW = NULL;
PROCESS_INFORMATION pi = {0};

int CmdIsOpen=0;
int ReadingCmdOut=0;

void wincmd_open()
{
    if (CmdIsOpen) return;
//  Create pipes for child
    STARTUPINFO si = {0};
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

//  Create pipe hRead,hWrite,attr,default buffer size
    CreatePipe(&hChildOutR, &hChildOutW, &saAttr, 0);
    SetHandleInformation(hChildOutR, HANDLE_FLAG_INHERIT, 0);
    CreatePipe(&hChildInR, &hChildInW, &saAttr, 0);
    SetHandleInformation(hChildInW, HANDLE_FLAG_INHERIT, 0);

//  Create the child process.
    si.cb = sizeof(STARTUPINFO);
    si.hStdError = hChildOutW;
    si.hStdOutput = hChildOutW;
    si.hStdInput = hChildInR;
    si.dwFlags |= STARTF_USESTDHANDLES;
    int rc=CreateProcess( NULL,"cmd /q /d /a",NULL,NULL,
                          TRUE,CREATE_NO_WINDOW,NULL,NULL, &si, &pi);
WaitForInputIdle(pi.hProcess,5000);
    CloseHandle(hChildOutW);
    CloseHandle(hChildInR);
    ReadingCmdOut=0;
    CmdIsOpen=1;
    sprintf(AciaStat,"WinCmd%d",rc);
}

void wincmd_close()
{
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hChildInW);
    CloseHandle(hChildOutR);
    CmdIsOpen=0;
    sprintf(AciaStat,"ACIA");
}

int wincmd_eof(char* buf, int siz) {
    int cnt = 0;
    if (siz > 1) { *buf++ = 0x0D; cnt++; }
    if (siz > 0) { *buf++ = 0x1B; cnt++; }
    ReadingCmdOut=0;
    return cnt;
}

int  wincmd_read(char* buf,int siz)
{
    int cnt;
    if (!CmdIsOpen) wincmd_open();

    //  kludgy loop to get child talking
    if (ReadingCmdOut) {
        int avail;
        int ctr = 2;
        while (1) {
            PeekNamedPipe(hChildOutR,NULL,siz,NULL,&avail,NULL);
            if (avail) break;
            if (ctr == 0) return wincmd_eof(buf,siz);
            Sleep(100);
            ctr--;
        }
    }
    ReadFile(hChildOutR, buf, siz, &cnt, NULL);
    if (!cnt) return wincmd_eof(buf,siz);

    // Null out linefeeds (assumes ascii)
    int i = 0;
    while (i<cnt) { 
        if (buf[i]==0x0A) buf[i]=0x00; 
        i++;
    }
    ReadingCmdOut=1;
    return cnt;
}

int  wincmd_write(char* buf,int siz)
{
    int cnt;
    if (ReadingCmdOut) return siz;  // Toss os9 shell echo
    if (!CmdIsOpen) wincmd_open();
    WriteFile(hChildInW, buf, siz, &cnt, NULL);
    return cnt;
}

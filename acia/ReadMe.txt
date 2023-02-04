========================================================================
    DYNAMIC LINK LIBRARY : acia Project Overview
========================================================================

Issues
------

1) Baud rate (for recv) delay table contains wild guesses.
2) Using file write mode to send text to a windows file
   causes an extra blank line to be appended to the file.
3) The sc6551 driver used by /t2 does not seem to use xmode baud, 
   xon, or xoff values.
4) No hardware flow control other than RxF and Txe signals
5) /t2 connect failure sometimes requires reset to recover
6) Pak rom protocols for dealing with dial ups and file transfers
   are not supported in the DLL.  It should be possible to write
   a server program on the PC to listen for a connection and 
   support these functions.

Todo
----

1) Improve simulation accuracy of baud rates
2) Research pak rom protocols and adjust as required/possible 
3) Use text 'COM20' to spec serial port instead of number '20'
4) Proper detection of serial port "modem" events when reading:
   Break, ring, CTS, DSR, RX and DX changes, etc.
5) Implement DCD DSR flow control. 
6) Dtermine how to tell /t2 driver (and pak) when EOF has occured
   on binary file reads. 
7) Much testing

Would be nice
-------------

1) Support for second sc6551 device

These notes are based on using /t2 on (Nitr)Os9 and the ROM from
the Radio Shack RS232 program pack. 

                General notes

Acia.dll is a Vcc add on that attempts to emulate the SC6551 ACIA.

The Color Computer RS232 program pack contains a SC6551
Asynchronous Communication Adapter (ACIA) which is a specialized
UART for dealing with 6500/6800 series CPUs.  

Acia.dll does not deal with many of the details of communicating
with RS232 devices.  Instead it establishes a byte stream connection
with a the console, TCP socket, or file and leaves the
details of how to communicate to the PC operating system.  This
allows flexibility but also means it does not behave exactly like
a real RS232 interface.

                RS232.ROM

If the file "rs232.rom" is in the Vcc execution directory it will 
be automatically loaded when acia.dll is selected. To use it 
do "EXEC &HE010" from RSDOS command prompt.  The rom is a copy
of the 4K rom from the Radio Shack Deluxe RS232 program Pak.
If a different ROM is used it must be 4096 bytes long.

                    Settings

Connection details are set using the Acia Interface config dialog
which can be reached from the Vcc Cartridge menu when the DLL is 
loaded.  The dialog has radio buttons for console, file read,
file write, tcpip, and comx modes of operation.  When activated 
Console mode brings up the Vcc console window.  File write mode causes
all data output to go to a file.  File read mode does the reverse,
data is input from a file.  Tcpip mode establishes a network connection
with a network server and Comx mode (no done yet) is intended to 
establishes a connection with a windows COM port, which may be 
connected to a RS232 device such as a modem or Arduino using a 
USB to UART converter.  Details of the various mode follows.

				    Console mode

Console mode is most useful when using (Nitr)Os9 and the t2 device 
and associated sc6551 driver. t2 and sc6551 must both be loaded at 
boot to work properly.  It can also be used for testing purposes,
which is why it was originally created.

Caution: There can only be one console associated with Vcc. Conflicts
will occur is the console is used for other purposes, such as for
debug logging.

/t2 settings for Windows console should be as follows:

xmode /t2
 -upc -bsb bsl echo lf null=0 pause pag=32 bsp=08
del=18 eor=0D eof=1B reprint=09 dup=19 psc=17 abort=03
quit=05 bse=08 bell=07 type=80 baud=06 xon=11 xoff=13

Basically the idea is to match CoCo window settings.  Acia.dll
translates the control codes to the proper console functions.

Colors default to white on black.  Only colors 0-7 are supported.

To launch a shell simply do: shell i=/t2 and the console
will come up. Typing "ex" in the console window causes the
console to close.

The console handles scrolling fairly well. The windows system
automatically moves the console window within the screen buffer
to keep the cursor visible. This causes text to appear to scroll
as long as the cursor is within the boundaries of the buffer.

By default the windows screen buffer is large but eventually it
becomes full. To handle this acia.dll deletes lines from the top of
the buffer when the cursor tries to move to a line below the bottom.
This affects full screen editors.  They use cursor move sequences
to control positioning within the screen boundaries but must scroll
text when the cursor tries to move beyond the screen boundaries.

Vim (fixed tsedit) works pretty well. It takes care of deleting lines
from the top of the screen when the cursor nears the bottom and
replaces the lines when scrolling up. Vim seems to be hard coded to
24 lines so the screen must be at least 24 lines long. (changing
tspars or typing v to set the screen type does not work for me)

Scred assumes the terminal will delete lines from the top of the
screen when the cursor is at the bottom of the screen. (emphasis on
screen) This means screen length must match what scred thinks for
it to work properly.  Since acia.dll console does the delete based
on buffer size it actually is that size that scred must be aware of.
Fix it to set the screen buffer to a reasonable size and use scred's
 -l=<screen buffer len> argument. (500 lines still seems to work
but I don't use scred much). The size of the buffer can be changed
from the console properties menu.

I did a brief check of uemacs. It seems to work okay except I find
the lack of support of arrow keys annoying and I am too used to
vi's use of hjkl for cursor position to adapt to emacs.

		      File Read and File Write Modes

File modes allow reading or writing to/from a windows file. After
selecting the appropriate radio button in the Acia Interface config
dialog the file name should be entered in the Name field. The
file name will be saved in the Vcc ini file.

Important.  You must turn off local echo to use File Mode with
the os9 /t2 device before using it to move data to/from
windows files. 'xmode /t2 -echo'  Failure to do so may result
in I/O errors and possible hangs.  Also make sure pause is off
of overflows will occur.

There are two file modes,  File Read and File Write.  When
file read is set writes to acia are ignored.  When file write
is set reads from acia always return null chars in binary mode
and EOF characters in text mode. It advised to not use binary
mode with the os9 /t2.

Text mode.  When text mode is checked end of line translations 
(CR -> CRLF) are done and EOF characters are appended at the
end of files.  This is recommended when using /t2 in os9 unless
attempting to write binary files to windows.  Reading binary files
is difficult under os9 because the sc6551 driver does not have a
means other than <CR><ESC> sequence to detect the end of file.  

Sending command output to a file can be done with something like
'dir -e > /t2' If doing a long listing it is better to place the 
output in a os9 file and copy that to /t2, for example:

	dir -e -x > cmds.list
	copy cmds.list /t2

Conversely when reading text from a file on Windows one first 
must copy the file to NitrOs9 and then list it.  The command 
'list /t2' will often cause a buffer overflow because there
is no flow control to tell the driver to stop reading.
   
					Tcpip Mode

After selecting TCPIP radio button on the Acia config dialog the 
server hostname or IP address should be entered in the Name field
(default is localhost) and the server port in the Port field.

I have been testing tcpip mode by using netcat on Linux as a server.
On Linux ' nc -l -p 48000'   48000 is the port number I am using.
After launching a shell connected to /t2 on (Nitr)Os9 the Linux
session becomes a terminal connected to Os9. 

				  COM port mode (COMx)

After selecting COMx radio button the COM port number should be
entered in the Port field. (Future this may be changed to using
the Name field to all specifying ports other than COMx) 

I have been testing the COM port mode using a com0com port emulator
in windows along with putty.  com0com was used to 'wire' two psuedo
port COM20 and COM21 together. I used acia.dll to connect to COM20
and PuTTy to connect to COM21.  This allowed me to simulate connecting
with a terminal.



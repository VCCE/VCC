========================================================================
    DYNAMIC LINK LIBRARY : acia Project Overview
========================================================================

Acia.dll is a Vcc add-on that attempts to emulate the Deluxe RS232
program pack. It allows connection to windows serial ports and a
tcpip server. It also allows reading/writing windows files and
interaction with the Vcc console.

Issues
------
1) Baud rates are not very accurate.
2) No hardware flow control other than RxF and TxE signals
3) DLL does not exit if Vcc crashes. (Issue with all Vcc Dll's)
4) No detection of serial port "modem" events when reading:
   Break, ring, CTS, DSR, RX and DX changes, etc.

Would be nice
-------------
1) Support for second sc6551 device

Acia Quick Start 
----------------
To use acia.dll insert it in the cartridge slot or a MPI slot.
This will cause "Acia Config" to be added to the Cartridge menu.
Clicking on this menu item allows selection of one of the acia 
modes: Console, File read, File write, TCPIP (client), and COMx.
The Name and Port fields are used to set parameters for modes
that require them.  The text mode check box when checked causes
acia.dll to do CR <-> CRLF translations when using the file 
read and write modes. The default mode is CONSOLE. 

For a quick test leave the ACIA config mode set to console
and enter and run the following basic program.  (You can cut
and paste using Vcc Edit -> Paste Basic Code)

10 POKE &HFF6A,1
20 IF (PEEK(&HFF69) AND 8)=0 THEN 20
30 PRINT CHR$(PEEK(&HFF68));
40 GOTO20

The Vcc console should come up. The console is a CMD type window 
that is connected to the ACIA. Anything typed in the console 
will be read and printed by the basic program.  To close the
console POKE &FF6A,0 or press F5 from the Vcc window.

The following program does the opposite, characters typed in the 
Vcc window are written to the console and CR writes CRLF:

10 POKE &HFF6A,1
20 K$=INKEY$: IF K$="" THEN 20
30 C=ASC(K$)
40 IF (PEEK(&HFF69) AND 16)=0 THEN 40
50 POKE &HFF68,C
60 IF C<>13 THEN 20
70 C=10: GOTO 40

Console mode is most useful when using Os9 which is described later.

Radio Shack Deluxe RS232 program Pak
-------------------------------------
 
If the file "rs232.rom" is in the Vcc execution directory it will
be automatically loaded when acia.dll is selected but not started.
To start it do "EXEC &HE010" from RSDOS command prompt. The rom
is a copy of the 4K rom from the Radio Shack Deluxe RS232 program
Pack.  If a different ROM is used it must be 4096 bytes long. Note
that if acia.dll is in a MPI slot but not selected it can still be
used but the pack rom will not be accessible.  To exec the rom
type EXEC&HE010 at the basic prompt. Note that the pak currently
has trouble with the "goto basic" function on Vcc which limits its
usefulness. Suggestions for a solution would be appreciated.

SC6551 Operation
----------------

The sc6551 emulation is controlled by four port addresses, 0xFF68
thru 0xFF6B.  The CPU controls the sc6551 by writing to these
addresses and gets status and data from the sc6551 by reading them.
A sc6551 datasheet will explain these in detail - what follows is
a brief explaination of the most important bits to acia.dll.

0xFF68 is the data register. A write to the address will transmit
a byte of data and a read will receive a byte of data.

0xFF69 is the status byte.  The CPU reads this port to determine
the status of the sc6551.  Bits 0, 1, and 2 of the status byte are
error indicators.  Bit 3 indicates that a data byte is ready to read. 
Bit 4 indicates that the sc6551 is ready to transmit a byte. Bits 
5 and 6 are modem and data set ready bits, and Bit 7 indicates the
sc6551 has asserted and IRQ.  Acia.dll does not set the error bits and
bits 5 and 6 are always clear to indicate mode and data ready states.

Data flow is controlled by status bit 3 (RxF) and bit 4 (TxE). When
the CPU wants to send data it first must verify bit 4 (TxE) is set before
writing to the data register, and when it wants to recieve data it must
first check that bit 3 (RxF) is set before reading the data register.

Acia.dll will assert an IRQ and set status bit 7 when data becomes ready
to read if Bit 1 of the command register (0xFF6A) is clear. The coco can
use the interrupt to reduce input polling.

0xFF6A is the command register.  Bit 0 of the command register is the
DTR bit.  This bit enables or disables the sc6551.  If this bit is set
by the coco CPU then communications are active (open) and if it is
cleared communications are inactive (closed).  Other command register
bits set IRQ behavior and parity. Refer to data sheet for details.

0xFF6B is the control register. Bits 0-3 of the control register set
the baud rate according to the following table.  Datasheet rates that
are not supported by acia.dll have replaced by the next higher value.
Acia.dll uses screen sync signal to time the input speeds.  These
are only approximate and if F8 is toggled (Screen refresh throttle)
the rates are invalid.

       val   rate     val   rate
        0     MAX      8    1200
        1      75      9    2400
        2      75     10    2400
        3     110     11    4800
        4     300     12    4899
        5     300     13    9600
        6     300     14    9600
        7     600     15   19200

Bits 5 and 6 of 0xFF6B set the data length and Bit 7 sets the number
of stop bits. B5-6: 00=8 01=7 10=6 11=5 len.  B7 0=1, 1=2 stops.

Basic programs can enable acia by poking 1 to &HFF6A and disable it by
poking 0.  They can then peek &HFF69 and test the result for RxF by
anding 8 or test for TxE by anding 16. Then peek &HFF68 to read data
and poke &HFF68 to write it.

<<<<<<< Updated upstream
=======
To demonstrate here is a simple Basic program that will read data
from the emulated acia:

10 POKE &HFF6A,1
20 S=PEEK(&HFF69) AND 8
30 IF S=0 THEN 20
40 PRINT CHR$(PEEK(&HFF68));
50 GOTO20

To test it set the Acia Config dialog to read a text file from the PC.
(Filename is relative to user directory).  When the program is run
Basic will print it's contents.  If you press break and then continue
the program will resume printing where it left off. If you poke 0 to
&HFF6A acia will close the file and when the program is run again it
will reopen and start from the file's beginning.

Since the baud rate has not been set acia defaults to 0 which is max
speed. In this case the program's output is throttled by the PRINT
rate. If F8 is toggled it runs much faster.

This simple program does not check for end of file so you have to use
break to end it because acia.dll will endlessly send EOF and CR
sequences as basic attempts to read past the end of file.

More detail on file read mode can be found later in this document.

RS232.ROM
---------

If the file "rs232.rom" is in the Vcc execution directory it will
be automatically loaded when acia.dll is selected but not started.
(make sure Autostart Cart in Misc Config dialog is unchecked)
To start it do "EXEC &HE010" from RSDOS command prompt. The rom
is a copy of the 4K rom from the Radio Shack Deluxe RS232 program
Pack.  If a different ROM is used it must be 4096 bytes long. Note
that if acia.dll is in a MPI slot but not selected it can still be
used but the pack rom will not be accessible.

>>>>>>> Stashed changes
Communications Modes
--------------------

Communications modes are set using the Acia Interface config dialog
which can be reached from the Vcc Cartridge menu when the DLL is
loaded.  The dialog has radio buttons for Console, File read,
File write, TCPIP, and COMx modes of operation.  When activated
Console mode brings up the Vcc console window.  File write mode
causes all data output to go to a file.  File read mode does the
reverse; data is input from a file.  TCPIP mode establishes a
client connection with a network server and Comx mode establishes
a connection with a windows serial port.  When File read or Write
mode is selected the filename relative to user home directory
must be inserted into the Name field.  When COMx mode is selected
the port name (eg: COM3) must be placed in the Name field with no
leading blanks.  When TCPIP mode is selected the server address
must be in the Name field and the port number in the Port field.
Additional detail of these modes follows.

Console mode
------------

Console mode can be useful when using (Nitr)Os9 and the t2 device
and associated sc6551 driver. t2 and sc6551 must both be loaded at
boot (in the boot track) to work properly. The t2 device can also
be used for testing purposes, which is why it was originally created.

Caution: There can only be one console associated with Vcc. Conflicts
will occur is the console is used for other purposes, such as for
debug logging.

/t2 settings for Windows conso1le should be as follows:

xmode /t2
 -upc -bsb bsl echo lf null=0 pause pag=32 bsp=08
del=18 eor=0D eof=1B reprint=09 dup=19 psc=17 abort=03
quit=05 bse=08 bell=07 type=80 baud=06 xon=11 xoff=13

Some of these settings are not honored by the sc6551 driver.
Xon, Xoff, and baud xmode settings seem to have no effect.

The console uses standard OS9 text window settings. Acia.dll
will translate many OS9 text screen control codes to do the 
proper console functions.  Colors default to white on black.
Only colors 0-7 are supported.

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
from the windows properties menu in the console title bar.

I did a brief check of uemacs. It seems to work okay except I find
the lack of support of arrow keys annoying and I am too used to
vi's use of hjkl for cursor position to adapt to emacs.


File Read and File Write Modes
------------------------------

File modes allow reading or writing to/from a windows file. After
selecting the appropriate radio button in the Acia Interface config
dialog the file name relative to user home directory should be
entered in the Name field. For example: "test\myfile.txt" in the
name field refers to %USERPROFILE%\test\myfile.txt

There are two file modes,  File Read and File Write. Read mode
causes acia to receive characters from the file specified and
write mode sends charactes.  Writes in read mode are ignored
and reads in write mode returns nulls.

It is sometimes best to turn off local echo and pause on the
os9 /t2 device to avoid I/O errors, eg: xmode /t2 -echo -pause

Text mode.  When text mode is checked end of line translations
(CR <-> CRLF).  This is recommended when using /t2 in os9 unless
attempting to write binary files to windows.  Reading binary files
is difficult under os9 because the sc6551 driver does not report
end of file conditions.  To avoid hangs acia.dll returns a <CR><ESC>
sequence when an attempt is made to read past the end of a file
regardless of text mode settings.

Sending command output to a file can be done with something like
'dir -e > /t2' If doing a long listing it is better to place the
output in a os9 file and copy that to /t2, for example:

    dir -e -x > cmds.list
    copy cmds.list /t2

When using other programs to read or write files it is important
to note that setting DTR opens the file and clearing DTR closes
it. The NitrOS9 driver takes care of this but many terminal
programs do not. DTR is controlled by bit 0 in 0xFF6A.

TCPIP Mode
----------

TCPIP mode causes acia to do a client connection to a tcpip server.
After selecting TCPIP radio button on the Acia config dialog the
server hostname or IP address should be entered in the Name field
(default is localhost) and the server port in the Port field.  Setting
bit 0 in 0xFF6A opens the connection and clearing it closes it.

Testing tcpip mode was done using netcat on Linux as a server.
On Linux ' nc -l -p 48000'   48000 is the port number I am using.
After launching a shell connected to /t2 on (Nitr)Os9 the Linux
session becomes a terminal connected to Os9.

Also Twilight Term (TWI-TERM.BIN) was used to test connection with
a telnet BBS on the internet.  (Set the address on the BBS in the
acia interface config name field and 23 in the port field)

COMx Serial Port Mode
----------------------

After selecting COMx radio button the port name should be entered
in the Name field, for example "COM3".  Leading blanks will not
work, port name must be left justified.

Valid baud rates are 110,300,600,1200,2400,4800,9600,19200

Testing of the COM port mode was done using a com0com port emulator
in windows along with putty.  com0com is used to 'wire' two psuedo
port COM20 and COM21 together. I used acia.dll to connect to COM20
and PuTTy to connect to COM21.  This allowed me to use putty as
an os9 /t2 terminal  

It was also tested with a USRobotics faxmodem via a Radio
Shack USB to serial adapter and the windows legacy USBSER driver and
was able to communicate successfully the a modem.



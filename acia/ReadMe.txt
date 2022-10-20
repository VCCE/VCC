========================================================================
    DYNAMIC LINK LIBRARY : acia Project Overview
========================================================================

Using /t2 on (Nitr)Os9

Terminal settings for Windows console should be as follows:

/t2
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
replaces the lines when scrolling up. Vim seems to be hardcoded to
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
the lack of support of arrow keys too annoying and I am too used 
to vi's idea use of hjkl for cursor position to adapt to emacs.

<h2 align="center">Vcc Change Log</h2>

- VCC v2.1.9.0 Jul/2024
    - Disassembler window breakpoints and step code when paused - EJJ.
    - Support for lwasm "BREAK" instruction (0x113E) - EJJ
    - Disassembler documentation

- VCC v2.1.8.3 Jan/2024
    - Add web link in help menu to User Guide - EJJaquay 
    - Correct hdblba.rom and instructions for SuperIDE - Bill Pierce
    - Add Hard Disk config for disabling RTC - EJJaquay
    - Add instruction decode to Debugger Processor Window - EJJ
    - Add Disassembler Window - EJJ
    - Add command line auto paste option - EJJ
    - Fix blinking text in 40 and 80 col text mode - EJJ
    - User Guide updates - EJJ

- VCC v2.1.8.2 Nov/2023
    - Fixed 6309 TFM to inc/dec source register before write - James Rye
    - Convert ReleaseNotes (this) to markdown - EJJ
    - Corrected Debugger DP register display - EJJ
    - Added "Set PC" button to Debugger Processor Window - EJJ
    - Fix File Exit bug (acia.dll i/o threads hang) - EJJ
    - Convert user guide master (AKA "Welcome to VCC") to markdown - EJJ

- VCC v2.1.8.1 Oct/2023
    - Fix version numbering - EJJ
    - Make dialogs stay on top and all DLL dialogs close when Vcc does. - EJJ
    - Fix command line quick load bug - EJJ
    - Fix special character fonts for OS9/NitrOS9

- VCC v2.1.0.8 August/2023
    - Increase VSYNC IRQ delay so MAX-10 sees keystrokes.
    - Corrected some 6309 register instruction issues found by Wally Z.
    - Added RS232 Pak emulation (acia.dll) - EJ Jaquay
    - Added pause function - James Rye
    - Added execution trace feature to Vcc Debugger - Mike Rojas
    - Fixed PMODE 4 color - James Rye, Mike Rojas
    - Improved windows mouse cursor position when shown - Chet Simpson
    - Minor Bug fixes
    - Updated Manual - Bill Pierce

- VCC v2.1.0.7 Oct/2022
    - VCC Debugger - Mike Rajas
    - Updated manual - Bill Pierce & Mike Rojas

- VCC v2.1.0.6 Apr/2022
    - Added "Show Windows Mouse Pointer In VCC Screen" checkbox to the
     "Joystick" menu. Left unchecked, this will hide the Windows mouse
      pointer while the focus is in the VCC main screen. - EJ Jaquay
    - Several bug fixes in keymap editor - EJ Jaquay
    - Fixed a bug that could error copies from VHD 0 to VHD 1 - EJ Jaquay
    - Add RS & CocoMax3 Hirez interfaces (FINALLY!) - EJ Jaquay
    - Added feature to Hard Drive Insert mode to create a new VHD when
      specified name is not found - EJ Jaquay
    - Fixed a keyboard bug which was affecting the play of Tetris - EJ Jaquay
    - A few cosmetic changes to the \"Config\" menu - Bill Pierce & EJ Jaquay
    - Updated Manual - Bill Pierce

- VCC v2.1.0.5 Jan/2022
    - Changes made to the KeyMap Editor - EJ Jaquay
    - Minor bug fixes - VCC Development Team
    - Updated manual with many typos fixed - David Johnson, Bill Pierce & EJ Jaquay

- VCC v2.1.0d ???/2021
    - Fixed bug which cause certain RSDOS word processors (EliteWord,
      VIP Writer, etc.) to skip every other line of text. - James Rye
    - Eliminated the "Allow Resize" checkbox. You should always be able to
      resize. - Bill Pierce
    - Reset a few "default" valuses that had gotten changed. - Bill Pierce
    - Add a "Custom KeyMap Editor" for making custom keymaps. - EJ Jaquay
    - Mapped "BREAK" to "F12" (as well as "ESCAPE) to faciliate usin
      CTRL-BRK used by some games/applications. - Bill Pierce
    - Updated the Manual - Bill Pierce,EJ Jaquay

- VCC v2.1.0c Jan/2021
    - Added 2nd Hard Drive - EJ Jaquay
    - Fixed the ALT key problem - EJ Jaquay
    - Added F3 & F4 for decreasing/increasing CPU speed - Trey Tomes
    - Added "Remember Screen Size" to remember your custom screen size
      on each run - James Rye & Bill Pierce
    - Added the "Game Master Cart" (GMC) by John Linville - Chet Simpson
    - Fixed the border color bug - James Rye
    - Updated Manual - Bill Pierce

- VCC v2.1.0b Nov/2020
    - Added "Copy/Paste" Edit menu items for copying text from VCC screen
      and pasting text into VCC. - James Rye.
    - Improved Composite Palettes (not fixed, WIP). - James Rye.
    - Fixed "file paths" so that each type of file; vhd, dsk, cas, rom,
      dll, etc. has it's own pathlist" - James Rye.
    - Added "Flip Artifact Colors" to the config menu. - James Rye.
    - Added "Save Config" and "Load Config" to the "File" menu (previously
      disabled).- This allows custom .ini files for different
      configurations. - EJ Jaquay.
    - Also added the ability to load custom ".ini" files when running
      VCC frrom the command line - EJ Jaquay.
    - Updated manual - Bill Pierce

- VCC v2.1.0a Oct/2020
    - Fixed VCC "ini" file to save in "user/home/appdata" to avoid permissions
      problem in Win7-10. - James Rye
    - Fixed "Force Aspect" to actually work in all modes but full screen
      mode. - James Ross
    - Updated manual to reflect changes.- Bill Pierce

- VCC v2.0.1f Aug/2020
    - Improved PMODE artifact color scheme, - Peter Westburg

- VCC v2.0.1e Dec/2019
    - Reverted "exit bug" fix as it caused VCC to hang in full screen
      mode - Bill Pierce.

- VCC v2.0.1d Dec/2019
    - Added "\*.ccc" to the program pak extensions. "Exit bug" fixed.
      Several cosmetic changes - Bill Pierce.

- VCC v.2.0.1c Dec/2019
    - Completed 6309 opcodes that were missing or incorrect - Walter Zambotti.
    - Updated manual - Bill Pierce

- VCC v2.0.1b June/2016
    - Several "Edited" releases under same versions.
    - Some cosmetic changes to dialogs.
    - Minor syntax changes to achieve working build for VS2015 - Wes Gayle

- VCC v2.0.1a June/2016
    - Moved code to "MS Visual Studio 2015 Community" (No Release) - Gary Coulborne

- VCC v1.42c ??/2016
    - VCC source code released to "Open Source" by Joseph Forgeone.

- VCC v1.43c ??/2015
    - Move the "Boisy/Becker" port to a cartridge format to eliminate crashes
      (never released) - Aaron Wolfe, David Ladd

- VCC v1.43b ??/2012
    - Added support for the "Boisy/Becker" interface for DriveWire4
      (Binary Release Only) - Aaron Wolfe, David Ladd

- VCC v1.42 ??/2012
    - Source code acquired for VCC by Aaron Wolfe, David Ladd, & Bill Pierce
     (No Release)

- VCC v1.42 ??/????
    - Last known release from original author Joseph Forgeone


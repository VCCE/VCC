Readme for reroc (re-return to coco) 
----------------------------------------------------------------------------

01/12/2005 Began porting PC-Dragon to windows
01/15/2005 Gave up on PC-Dragon , It uses to many "tricks" to work 
01/17/2005 Began writting 6809 core (Plan09)
01/18/2005 Did switch case framework

Monday, January 24, 2005, 10:21:44 PM EST Emulation Core is able to start Extened basic (barely)
-------------------------------------------------------------------------------------------------
95C3: STA D: D30 X: FFC8 Y: A00E U: 80E8 S: 3F2B PC: 95C4 DP: 0 CC: 0
95C5: LDA D: D30 X: FFC8 Y: A00E U: 80E8 S: 3F2B PC: 95C6 DP: 0 CC: 0
95C8: ANDA D: 30 X: FFC8 Y: A00E U: 80E8 S: 3F2B PC: 95C9 DP: 0 CC: 4
95CA: STA D: 30 X: FFC8 Y: A00E U: 80E8 S: 3F2B PC: 95CB DP: 0 CC: 4
95CD: PULS D: 30 X: FFC8 Y: A00E U: 80E8 S: 3F2B PC: 95CE DP: 0 CC: 4
A285: PSHS D: 300D X: 8101 Y: A00E U: 80E8 S: 3F31 PC: A286 DP: 0 CC: 4
A287: LDB D: 300D X: 8101 Y: A00E U: 80E8 S: 3F30 PC: A288 DP: 0 CC: 4
A289: INCB D: 3000 X: 8101 Y: A00E U: 80E8 S: 3F30 PC: A28A DP: 0 CC: 4
A28A: PULS D: 3001 X: 8101 Y: A00E U: 80E8 S: 3F30 PC: A28B DP: 0 CC: 0
A28C: BMI D: D01 X: 8101 Y: A00E U: 80E8 S: 3F31 PC: A28D DP: 0 CC: 0
A28E: BNE D: D01 X: 8101 Y: A00E U: 80E8 S: 3F31 PC: A28F DP: 0 CC: 0
A30A: PSHS D: D01 X: 8101 Y: A00E U: 80E8 S: 3F31 PC: A30B DP: 0 CC: 0
A30C: LDX D: D01 X: 8101 Y: A00E U: 80E8 S: 3F2D PC: A30D DP: 0 CC: 0
A30E: CMPA D: D01 X: 418 Y: A00E U: 80E8 S: 3F2D PC: A30F DP: 0 CC: 0
temp16 is 5
A310: BNE D: D01 X: 418 Y: A00E U: 80E8 S: 3F2D PC: A311 DP: 0 CC: 0
A31D: CMPA D: D01 X: 418 Y: A00E U: 80E8 S: 3F2D PC: A31E DP: 0 CC: 0
temp16 is 0
A31F: BNE D: D01 X: 418 Y: A00E U: 80E8 S: 3F2D PC: A320 DP: 0 CC: 4
A321: LDX D: D01 X: 418 Y: A00E U: 80E8 S: 3F2D PC: A322 DP: 0 CC: 4
A323: LDA D: D01 X: 418 Y: A00E U: 80E8 S: 3F2D PC: A324 DP: 0 CC: 0
A325: STA D: 6001 X: 418 Y: A00E U: 80E8 S: 3F2D PC: A326 DP: 0 CC: 0
A327: TFR D: 6001 X: 419 Y: A00E U: 80E8 S: 3F2D PC: A328 DP: 0 CC: 0
A329: BITB D: 419 X: 419 Y: A00E U: 80E8 S: 3F2D PC: A32A DP: 0 CC: 0
Dumping Vidio Ram...
x""        ?¬?ρ          ?????????6?¦  ?¦          ?
  ?¬  S ΗF        ΗFό;              ?¦                  ??     ??
 Η? X ??pδ ¦J?Ί&??ͺ¦? ~¬?     ?>?    ? ?? ?           Η `                   ?¦B?
?   ,?                                    ~λL~αχ  JΗO¦RY ?^~δλ5¬f½gΆ½?¬)?όβό<?ι?
όh   ¦w   ¦w¦J¦J¦J¦J¦J¦J¦J¦J¦J¦J            999999999~ιs~ξ±999999999~ιε~ΔΙ999999
999999999~κF999~κ=~ι£~ηs~ι¦999999~β?999S ΗF



                       9


                                   αΗ                                         EX
TENDED`COLOR`BASIC`qnq``````````````````````````````````````````````````````````
````````````````````````````````````````````````````````````````````````````````
````````````````````````````````````````````````````````````````````````````````
````````````````````````````````````````````````````````````````````````````````
````````````````````````````````````````````````````````````````````````````````
````````````````````````````````````````````````````````````````````````````````
`````````````````````````````Press any key to continue
-------------------------------------------------------------------------------------------------



01/30/2005 Got enough CPU working to write the keyboard routines and rough 6821 emulation

02/03/2005 SAM Emulation 

02/06/2005 faking the 6847 but it works sort of

02/08/2005 Moved from VS6 to EVC3 CPU deved on a PC for ease of debugging

02/10/2005 Moved from GDI to Gapi

02/14/2005 6847 mostly working with gapi now.



02/20/2005 stellar life line acts funny fixed SEX was using an unsigned B register

02/22/2005 Mega-Bug weird lines fixed leax	a,x is signed!

02/27/2005 Emulated RAM/ROM Maps arcanoid works now.

03/02/2005 CPU core mostly "works" now.

03/05/2005 Joystick works

03/06/2005 Began emulating wd1793 FDC

03/06/2005 Started playing with sound output code

03/10/2005 Fixed problem with Keyboard icon showing while emulator is running.

03/11/2005 ROMS are now internal 

03/13/2005 SEX and DAA seem ok now. all CC are getting set. 

03/14/2005 Fixed ASR ops

03/15/2005 OS9 Booted!!! But crashes if a dir takes more than a screen of files
	BUG ADCB_D was changing the A register! DOD works better

03/16/2005 OS9 fixed (hope) OS9 sends an FDC command before it selects a drive I considered this an error
	added Drive active "light" to UI
	??? canyon climber doesn't work with HS interupt on. ??? timing? 

3/25/2005 Canyon climber image was bad. works fine

3/27/2005 

3/31/2005 Joystick works in all 3 modes now full/corner/hardkey

4/01/2005 reorged code a bit




6/24/2005 Moved source to x86 platform from Poco and changed name to reroc

6/25/2005 Reworked for DirectX and broke the joysticks
          required massive rewrite of mc6847.c 

6/28/2005 Moved Memory access code from mc6809.c to mmu.c
          renamed io_glue.x to iobus.x
          general code cleanup where possible.


7/10/2005 Got the CC3 graphics rendering engine working
		Tandy demos now work (sort of)
		removed mc6847 c & h replaced with GimeGraphics c/h


7/14/2005 Hres text now works except blink.

7/15/2005 Added support for the new interupt model.
		Gate Crasher demo works.

7/18/2005
		Moved the Roms images internal with option to load
		from .rom files if needed.

		Fixed the Tandy demo bouncing ball problem.
		vpitch wasn't calling SetupScreen()

		Found the problem with Dungeons of Daggorath!!!
		lea calculations weren't getting added to the cycle counter
		this was extending the interupt times as all timing is
		based on CPU time. 
7/27/2005	Rewrote wd1793 code from scratch. Nitro09 "works" now.










6/04/2007	Fixed Fullscreen and 32bit composite color problem


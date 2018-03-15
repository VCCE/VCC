
# Installing

Downloaded VS2018 at <https://www.visualstudio.com/>. Made sure to select MFC and Windows XP support. WIN32 wasn't an option. 

Needed to add the following to becker.h:

```
#define bool int
#define true 1
#define false 0
```

# Planned updates


# 6309
(D) Direct (I) Inherent (R) Relative (M) Immediate (X) Indexed (E) extened

hd6309.c, hd6309defs.h

See The_6309_Book.pdf for instructions information. 
See [Socks 6309][sock6309] page for instructions, timings, and byte value.

## Unemulated

- ADCD_D UNEMULATED
- ADCD_E UNEMULATED
- ADCR   UNEMULATED
- BAND   UNEMULATED
- BEOR   UNEMULATED
- BIAND  UNEMULATED
- BIEOR  UNEMULATED
- BIOR   UNEMULATED
- BOR    UNEMULATED
- DIVQ_D DONE
- DIVQ_M DONE
- DIVQ_X DONE
- DIVQ_E DONE
- LDBT   UNEMULATED
- SBCD_D UNEMULATED
- SBCD_E UNEMULATED
- SBCR   UNEMULATED
- STBT   UNEMULATED


## Untested

- ADDE_D: //119B 6309 Untested
- ADDE_E: //11BB 6309 Untested
- ADDE_X: //11AB 6309 Untested
- ADDF_D: //11D8 6309 Untested
- ADDF_E: //11FB 6309 Untested
- ADDF_M: //11CB 6309 Untested
- ADDF_X: //11EB 6309 Untested
- ADDW_E: //10BB 6309 Untested
- ADDW_X: //10AB 6309 Untested TESTED NITRO CHECK no Half carry?
- AIM_E: //72 6309 Untested CHECK NITRO
- ANDD_D: //1094 6309 Untested
- ANDD_E: //10B4 6309 Untested
- ANDR: //1034 6309 Untested wcreate
- ASRD_I: //1047 6309 Untested TESTED NITRO MULTIVUE
- BITD_D: //1095 6309 Untested
- BITD_E: //10B5 6309 Untested CHECK NITRO
- BITD_M: //1085 6309 Untested
- BITD_X: //10A5 6309 Untested
- CLRF_I: //115F 6309 Untested wcreate
- CMPE_D: //1191 6309 Untested
- CMPE_E: //11B1 6309 Untested
- CMPF_D: //11D1 6309 Untested
- CMPF_E: //11F1 6309 Untested
- CMPF_X: //11E1 6309 Untested
- CMPW_D: //1091 6309 Untested
- CMPW_E: //10B1 6309 Untested
- COME_I: //1143 6309 Untested
- COMF_I: //1153 6309 Untested
- COMW_I: //1053 6309 Untested
- DIVD_E: //11BD 6309 02292008 Untested
- EIM_D: //6309 Untested
- EIM_E: //75 6309 Untested CHECK NITRO
- EIM_X: //65 6309 Untested TESTED NITRO
- EORD_D: //1098 6309 Untested
- EORD_E: //10B8 6309 Untested
- EORD_M: //1088 6309 Untested
- EORD_X: //10A8 6309 Untested TESTED NITRO
- INCF_I: //115C 6309 Untested
- LDF_D: //11D6 6309 Untested wcreate
- LSRW_I: //1054 6309 Untested
- ORD_D: //109A 6309 Untested
- ORD_E: //10BA 6309 Untested
- ORD_M: //108A 6309 Untested
- ORD_X: //10AA 6309 Untested wcreate
- PSHUW: //103A 6309 Untested
- PULSW:     //1039 6309 Untested wcreate
- PULUW: //103B 6309 Untested
- ROLD_I: //1049 6309 Untested
- RORD_I: //1046 6309 Untested
- RORW_I: //1056 6309 Untested
- SUBE_D: //1190 6309 Untested HERE
- SUBE_E: //11B0 6309 Untested
- SUBE_M: //1180 6309 Untested
- SUBE_X: //11A0 6309 Untested
- SUBF_D: //11D0 6309 Untested
- SUBF_E: //11F0 6309 Untested
- SUBF_M: //11C0 6309 Untested
- SUBF_X: //11E0 6309 Untested
- SUBW_D: //1090 Untested 6309
- SUBW_E: //10B0 6309 Untested
- TIM_D:     //B 6309 Untested wcreate
- TSTE_I: //114D 6309 Untested TESTED NITRO
- TSTW_I: //105D Untested 6309 wcreate

## Untested nitro
- ADDR: //1030 6309 CC? NITRO 8 bit code
- DIVD_M: //118D 6309 NITRO
- TIM_E: //7B 6309 NITRO

## DIVQ Stack trace
vcc.exe!HD6309Exec 		z:\desktop\vcc\vcc\hd6309.c(2224)
vcc.exe!CPUCycle 		z:\desktop\vcc\vcc\coco3.cpp(234)
vcc.exe!RenderFrame 	z:\desktop\vcc\vcc\coco3.cpp(108)
vcc.exe!EmuLoop 		z:\desktop\vcc\vcc\vcc.c(731)


# New ideas for VCC

The following are new ideas I am thinking about adding to VCC.

### Create new disk
- Ability to create a new formatted disk image for RSDOS
- Ability to create a new formatted disk image for OS9
	
### Unit testing

- Create automated unit tests that will test all or individual  instructions. 
- See if I can make use of cxxtest. I'm familiar with it, use it on my own emulator, and it is dead simple to use.
- List of other unit testing frameworks, for C: <https://stackoverflow.com/questions/65820/unit-testing-c-code>


### Debugger

The debugger will allow you to run the emulator in it's normal window, but a second window (or a command like console would appear under the coco display). You will be able to step through code, inpect registers and memory, and change their values on the fly. Assuming assembler .lst files give enough information, the debugger could allow you to step through the original source of the program, so you can see your commented code, instead of just using the assembled code. I love using edtasm for testing my code, but it's slow, and hard to debug a graphical screen. Having the debugger seperate from the Coco's display will make this much easier.

#### Souce code stepping
New window that will allow user to step through instructions.

#### Registers
New window that will show registers for 6809, and when CPU is in 6309 mode, show the 6309 regs as well.

*Should indicate which regs are 6309.*


#### Memory dump
New window will allow memory dumps in byte, word, mnemonic modes.

#### Stack trace
This may be possible by using a combination of tracking *jsr/rts* combinations, as well as *jsr/puls pc*.

For example, the following would need to be tracked by looking at the *PULS PC* to know that it's returning from a *JSR* call. 

```
CHK309 PSHS  D     ;Save Reg-D
       FDB   $1043 ;6309 COMD instruction (COMA on 6809) 
       CMPB  1,S   ;not equal if 6309 
       PULS  D,PC  ;exit, restoring D
```

# Disk controller issue
Not correctly honoring the m bit at ff48

$ff40 (drive #/motor/nmi control)
$ff48 =status/command
$ff49=track
$ff4a=sector
$ff4b=data

if you FEED the command register ($ff48) a $90 (instead of $80), it should honour the M flag and autoincrement the sector register and continue reading and only raise an irq when it meets an invalid sector #

iobus.c:port_read()
 PackPortRead()
  fd502.c:PackPortRead()
   wd1793.c:disk_io_read() - status reg

wd1793.c:DecodeControlReg() - Reading from $ff48-$ff4f clears bit 7 of DSKREG ($ff40)

pakinterface.c:PakTimer()
 fd502.cpp:HeartBeat()
  wd1793.c:PingFdc()
   wd1793.c:GetBytefromSector()
    wd1793.c:ReadSector()

## Read sector stack trace
fd502.dll!ReadSector()						z:\desktop\vcc\vcc\fd502\wd1793.c(422)
fd502.dll!GetBytefromSector()				z:\desktop\vcc\vcc\fd502\wd1793.c(1028)
fd502.dll!disk_io_read(unsigned char port) 	z:\desktop\vcc\vcc\fd502\wd1793.c(146)
fd502.dll!PackPortRead(unsigned char Port)	z:\desktop\vcc\vcc\fd502\fd502.cpp(186)
mpi.dll!PackPortRead(unsigned char Port) 	z:\desktop\vcc\vcc\mpi\mpi.cpp(274)
vcc.exe!PackPortRead(unsigned char port)	z:\desktop\vcc\vcc\pakinterface.c(132)
vcc.exe!port_read(unsigned short addr) 		z:\desktop\vcc\vcc\iobus.c(164)
vcc.exe!MemRead8(unsigned short address) 	z:\desktop\vcc\vcc\tcc1014mmu.c(216)
vcc.exe!HD6309Exec(int CycleFor) 			z:\desktop\vcc\vcc\hd6309.c(4181)
vcc.exe!CPUCycle()							z:\desktop\vcc\vcc\coco3.cpp(234)
vcc.exe!RenderFrame(SystemState * RFState)	z:\desktop\vcc\vcc\coco3.cpp(89)
vcc.exe!EmuLoop(void * Dummy) 				z:\desktop\vcc\vcc\vcc.c(731)

I'm betting the error is in wd1793.c:GetBytefromSector(), 
[sock6309]: http://users.axess.com/twilight/sock/cocofile/4mzcycle.html

## DecodeControlReg stack trace
fd502.dll!DecodeControlReg(uchar)			z:\desktop\vcc\vcc\fd502\wd1793.c(205)
fd502.dll!disk_io_write(uchar, uchar) 		z:\desktop\vcc\vcc\fd502\wd1793.c(184)
fd502.dll!PackPortWrite(uchar, uchar) 		z:\desktop\vcc\vcc\fd502\fd502.cpp(174)
mpi.dll!PackPortWrite(uchar, uchar)			z:\desktop\vcc\vcc\mpi\mpi.cpp(246)
vcc.exe!PackPortWrite(uchar, unsigned char)	z:\desktop\vcc\vcc\pakinterface.c(141)
vcc.exe!port_write(uchar, ushort) 			z:\desktop\vcc\vcc\iobus.c(285)
vcc.exe!MemWrite8() 						z:\desktop\vcc\vcc\tcc1014mmu.c(240)
vcc.exe!HD6309Exec(int CycleFor) 			z:\desktop\vcc\vcc\hd6309.c(4190)
vcc.exe!CPUCycle() 							z:\desktop\vcc\vcc\coco3.cpp(234)
vcc.exe!RenderFrame(SystemState * RFState) 	z:\desktop\vcc\vcc\coco3.cpp(108)
vcc.exe!EmuLoop(void * Dummy) 				z:\desktop\vcc\vcc\vcc.c(731)

-		PakPortWriteCalls	0x551feb14 {0x00000000, 0x550831b0 {harddisk.dll!PackPortWrite(unsigned char, unsigned char)}, 0x00000000, ...}	void(*)(unsigned char, unsigned char)[0x00000004]
		[0x00000000]	0x00000000	void(*)(unsigned char, unsigned char)
		[0x00000001]	0x550831b0 {harddisk.dll!PackPortWrite(unsigned char, unsigned char)}	void(*)(unsigned char, unsigned char)
		[0x00000002]	0x00000000	void(*)(unsigned char, unsigned char)
		[0x00000003]	0x54f8179e {fd502.dll!_PackPortWrite}	void(*)(unsigned char, unsigned char)

# Crash when screen resolution changes
vcc.exe!LockScreen(SystemState * LSState) 	z:\desktop\vcc\vcc\directdrawinterface.cpp(266)
vcc.exe!RenderFrame(SystemState * RFState) 	z:\desktop\vcc\vcc\coco3.cpp(95)
vcc.exe!EmuLoop(void * Dummy)				z:\desktop\vcc\vcc\vcc.c(731)


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

Several instructions are not working as they are not implemented.


**hd6309.c**

- Divide instructions are broken
	- See hd6309.c:1979. No code implemented.

## Unemulated

- ADCR   UNEMULATED
- SBCR   UNEMULATED
- SBCD_D UNEMULATED
- ADCD_D UNEMULATED
- SBCD_E UNEMULATED
- ADCD_E UNEMULATED
- BAND   UNEMULATED
- BIAND  UNEMULATED
- BOR    UNEMULATED
- BIOR   UNEMULATED
- BEOR   UNEMULATED
- BIEOR  UNEMULATED
- LDBT   UNEMULATED
- STBT   UNEMULATED
- DIVQ_M UNEMULATED
- DIVQ_D UNEMULATED


## Untested

- EIM_D: //6309 Untested
- TIM_D:     //B 6309 Untested wcreate
- ANDR: //1034 6309 Untested wcreate
- PULSW:     //1039 6309 Untested wcreate
- PSHUW: //103A 6309 Untested
- PULUW: //103B 6309 Untested
- RORD_I: //1046 6309 Untested
- ASRD_I: //1047 6309 Untested TESTED NITRO MULTIVUE
- ROLD_I: //1049 6309 Untested
- COMW_I: //1053 6309 Untested
- LSRW_I: //1054 6309 Untested
- RORW_I: //1056 6309 Untested
- TSTW_I: //105D Untested 6309 wcreate
- BITD_M: //1085 6309 Untested
- EORD_M: //1088 6309 Untested
- ORD_M: //108A 6309 Untested
- SUBW_D: //1090 Untested 6309
- CMPW_D: //1091 6309 Untested
- ANDD_D: //1094 6309 Untested
- BITD_D: //1095 6309 Untested
- EORD_D: //1098 6309 Untested
- ORD_D: //109A 6309 Untested
- BITD_X: //10A5 6309 Untested
- EORD_X: //10A8 6309 Untested TESTED NITRO
- ORD_X: //10AA 6309 Untested wcreate
- ADDW_X: //10AB 6309 Untested TESTED NITRO CHECK no Half carry?
- SUBW_E: //10B0 6309 Untested
- CMPW_E: //10B1 6309 Untested
- ANDD_E: //10B4 6309 Untested
- BITD_E: //10B5 6309 Untested CHECK NITRO
- EORD_E: //10B8 6309 Untested
- ORD_E: //10BA 6309 Untested
- ADDW_E: //10BB 6309 Untested
- COME_I: //1143 6309 Untested
- TSTE_I: //114D 6309 Untested TESTED NITRO
- COMF_I: //1153 6309 Untested
- INCF_I: //115C 6309 Untested
- CLRF_I: //115F 6309 Untested wcreate
- SUBE_M: //1180 6309 Untested
- SUBE_D: //1190 6309 Untested HERE
- CMPE_D: //1191 6309 Untested
- ADDE_D: //119B 6309 Untested
- SUBE_X: //11A0 6309 Untested
- ADDE_X: //11AB 6309 Untested
- SUBE_E: //11B0 6309 Untested
- CMPE_E: //11B1 6309 Untested
- ADDE_E: //11BB 6309 Untested
- DIVD_E: //11BD 6309 02292008 Untested
- SUBF_M: //11C0 6309 Untested
- ADDF_M: //11CB 6309 Untested
- SUBF_D: //11D0 6309 Untested
- CMPF_D: //11D1 6309 Untested
- LDF_D: //11D6 6309 Untested wcreate
- ADDF_D: //11D8 6309 Untested
- SUBF_X: //11E0 6309 Untested
- CMPF_X: //11E1 6309 Untested
- ADDF_X: //11EB 6309 Untested
- SUBF_E: //11F0 6309 Untested
- CMPF_E: //11F1 6309 Untested
- ADDF_E: //11FB 6309 Untested
- EIM_X: //65 6309 Untested TESTED NITRO
- AIM_E: //72 6309 Untested CHECK NITRO
- EIM_E: //75 6309 Untested CHECK NITRO

## Untested nitro
- ADDR: //1030 6309 CC? NITRO 8 bit code
- DIVD_M: //118D 6309 NITRO
- TIM_E: //7B 6309 NITRO


## New updates

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

- New window that will allow user to step through instructions.
- New window that will show registers for 6809, and when CPU is in 6309 mode, show the 6309 regs as well.
	- Should indicate which regs are 6309
- New window will allow memory dumps in byte, word, mnemonic modes.


# Lee's notes
- tc1014mmu.c:CopyRom() loads the coco3.rom 
- Getting windows version <https://msdn.microsoft.com/en-us/library/windows/desktop/ms724429(v=vs.85).aspx>
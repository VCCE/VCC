# mkdumpbas.py -- builds psgdump.bas from out/psgdump.bin (lwasm --raw).
import os
here = os.path.dirname(os.path.abspath(__file__))
code = open(os.path.join(here, "out", "psgdump.bin"), "rb").read()[:0x5122 - 0x5000]
PROBE = 0x504C
lines = []
n = 10
def add(t):
    global n
    lines.append("%d %s" % (n, t)); n += 10
add("REM COCO PSG FLASH DUMP - COCO 3, MPI, SDC")
add("CLEAR 300,&H4FFF")
add("C=0:FOR I=0 TO %d:READ A:POKE &H5000+I,A:C=C+A:NEXT" % (len(code) - 1))
add('IF C<>%d THEN PRINT "DATA ERROR":END' % sum(code))
add('INPUT "MPI SLOT WITH PSG (1-4)";P')
add('INPUT "MPI SLOT WITH SDC (1-4)";S')
add("POKE &H5F00,(P-1)*17:POKE &H5F01,(S-1)*17")
add("EXEC &H%04X:R=&H5F10" % PROBE)
add('PRINT "BANK0,1,CTRL:";PEEK(R);PEEK(R+1);PEEK(R+2)')
add('PRINT "$FF5C:";PEEK(R+3)')
add('PRINT "SEL,R1:";PEEK(R+4);PEEK(R+5)')
add('PRINT "PORT A,B:";PEEK(R+6);PEEK(R+7)')
add('PRINT "SRAM 128,192:";PEEK(R+8);PEEK(R+9)')
add('PRINT "SEG0:";:FOR I=0 TO 7:PRINT PEEK(R+12+I);:NEXT:PRINT')
add('PRINT "SEG64:";:FOR I=0 TO 7:PRINT PEEK(R+20+I);:NEXT:PRINT')
add('INPUT "DUMP TO DRIVE 0, FIRST SEGMENT (0-63, -1=NO)";G0:IF G0<0 THEN END')
add("N=0:FOR G=G0 TO 63")
add("POKE &H5F02,G:EXEC &H5000")
add('IF PEEK(&H5F03)*256+PEEK(&H5F04)=0 THEN PRINT G;"BLANK":GOTO %d' % (n + 20))
add('F$="PSG"+RIGHT$(STR$(100+G),2):SAVEM F$,&H6000,&H7FFF,&H6000:N=N+1:PRINT G;"SAVED ";F$')
add("NEXT")
add('PRINT N;"SEGMENTS SAVED"')
for i in range(0, len(code), 16):
    add("DATA " + ",".join(str(b) for b in code[i:i + 16]))
open(os.path.join(here, "psgdump.bas"), "w", newline="\r\n").write("\n".join(lines) + "\n")
print("psgdump.bas: %d bytes of ML, checksum %d" % (len(code), sum(code)))

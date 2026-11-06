********************************************************************
* STRMTEST - CoCo SDC streaming mode test suite
*
* Exercises the SDC Low-Level Stream command ($9x) as described in
* chapter 7 of the CoCo SDC user guide and verifies every byte of
* every block against a known pattern.
*
* Test data files (made by mktest.py) must be in the current SDC
* directory:
*   STRM512.DAT  64 blocks of 512 bytes (32768 bytes)
*   STRMODD.DAT  same 64 blocks plus 3 extra bytes (32771 bytes)
*
* Block b of each file contains:
*   0-2    block number, 24 bit big-endian
*   3      complement of block number low byte
*   4-511  (b + offset) & $FF
*
* The program mounts the data files itself in SDC drive 1.
* Run with:  WIDTH 80 : LOADM"STRMTEST" : EXEC
*
* Assemble:  lwasm --decb -o STRMTEST.BIN strmtest.asm
********************************************************************

        pragma  6809

POLCAT   equ    $A000      Color BASIC keyboard poll vector
CHROUT   equ    $A002      Color BASIC console output vector

* SDC hardware (CoCo scheme)
CTRLATCH equ    $FF40      controller latch
FLASHD   equ    $FF42      flash data register (usable as RAM)
CMDREG   equ    $FF48      command register (write)
STATREG  equ    $FF48      status register (read)
PREG1    equ    $FF49      param 1
PREG2    equ    $FF4A      param 2 / data A
PREG3    equ    $FF4B      param 3 / data B

BUSY     equ    %00000001
READY    equ    %00000010
FAILED   equ    %10000000

CMDMODE  equ    $43
CMDREAD  equ    $81        read logical sector, drive 1
CMDSTRW  equ    $91        stream drive 1, 16 bit transfers
CMDSTRB  equ    $95        stream drive 1, 8 bit transfers
CMDABORT equ    $D0
CMDMOUNT equ    $E1        send data block, drive 1

NBLK     equ    64         full blocks in each data file

* readblk / waitrdy result codes
RC_RDY   equ    0          block ready (or read)
RC_END   equ    1          BUSY cleared - stream ended
RC_TO    equ    2          timeout
RC_BAD   equ    3          data mismatch
RC_RUN   equ    4          stream ran past expected length

        org     $3000

*-------------------------------------------------------------------
start   pshs    cc,a,b,dp,x,y,u
        orcc    #$50            no IRQ/FIRQ: keep DOS motor timer
*                               from writing $FF40 during tests
        ldx     #s_title
        lbsr    prstr

        lbsr    test1
        lbsr    test2
        lbsr    test3
        lbsr    test4
        lbsr    pause
        lbsr    test5
        lbsr    test6
        lbsr    test7
        lbsr    pause
        lbsr    test8
        lbsr    test9
        lbsr    test10

        clr     CTRLATCH        back to floppy mode
        ldx     #s_done
        lbsr    prstr
        puls    cc,a,b,dp,x,y,u,pc

*===================================================================
* T1: full stream, word mode, from block 0 to end of file
*===================================================================
test1   ldx     #s_t1
        lbsr    prstr
        ldx     #f_512
        lbsr    domount
        lbne    tdone
        lda     #CMDSTRW
        ldx     #0
        ldy     #0              read until end
        lbsr    runstream
        lbsr    rescode
        lbne    tabort
        cmpb    #RC_END
        beq     t1a
        ldx     #s_noend        should not happen with Y=0
        lbsr    prstr
        lbra    tabort
t1a     lbsr    prstats
        ldd     CNT
        cmpd    #NBLK
        bne     t1bad
        lda     FSTAT
        bita    #BUSY|FAILED
        bne     t1bad
        lbra    tpass
t1bad   lbra    tfail

*===================================================================
* T2: start mid-file (block 10), read 5 blocks, abort with $D0
*===================================================================
test2   ldx     #s_t2
        lbsr    prstr
        lda     #CMDSTRW
        ldx     #10
        ldy     #5
        lbsr    runstream
        lbsr    rescode
        lbne    tabort
        lbsr    prstats
        lbsr    doabort
        lbcs    tabto
        lbsr    prabort
        lda     ASTAT
        bita    #BUSY|FAILED
        lbne    tfail
        lbra    tpass

*===================================================================
* T3: new stream right after abort must start at its own block (3)
*===================================================================
test3   ldx     #s_t3
        lbsr    prstr
        lda     #CMDSTRW
        ldx     #3
        ldy     #2
        lbsr    runstream
        lbsr    rescode
        lbne    tabort
        lbsr    prstats
        lbsr    doabort
        lbcs    tabto
        lbra    tpass

*===================================================================
* T4: leave command mode mid-stream (no abort), re-enter, and start
*     a new stream at block 7. Stale stream state would show up as
*     a busy timeout or wrong block.
*===================================================================
test4   ldx     #s_t4
        lbsr    prstr
        lda     #CMDSTRW
        ldx     #30
        ldy     #2
        lbsr    runstream
        lbsr    rescode
        lbne    tabort
        clr     CTRLATCH        exit command mode mid-stream
        lda     #CMDSTRW
        ldx     #7
        ldy     #1
        lbsr    runstream       re-enters command mode itself
        lbsr    rescode
        lbne    tabort
        lbsr    prstats
        lbsr    doabort
        lbcs    tabto
        lbra    tpass

*===================================================================
* T5: byte mode ($95) from block 5, 3 blocks via $FF4B, abort
*===================================================================
test5   ldx     #s_t5
        lbsr    prstr
        lda     #CMDSTRB
        ldx     #5
        ldy     #3
        lbsr    runstream
        lbsr    rescode
        lbne    tabort
        lbsr    prstats
        lbsr    doabort
        lbcs    tabto
        lbra    tpass

*===================================================================
* T6: normal 256 byte sector read after streaming. LSN 2 of
*     STRM512.DAT is the first half of block 1.
*===================================================================
test6   ldx     #s_t6
        lbsr    prstr
        lbsr    cmdon
        lbsr    waitnb
        lbcs    tabto
        clr     PREG1
        clr     PREG2
        lda     #2
        sta     PREG3
        lda     #CMDREAD
        sta     CMDREG
        lbsr    waitrdy
        sta     BSTAT
        sta     LSTAT
        tstb
        beq     t6a
        cmpb    #RC_TO
        bne     t6nd
        lbsr    rescode         prints timeout
        lbra    tabort
t6nd    ldx     #s_nodat        BUSY cleared with no data
        lbsr    prstr
        lda     LSTAT
        lbsr    prhex8
        lbsr    crlf
        lbra    tabort
t6a     ldx     #BUF
        ldb     #128
t6b     ldu     PREG2
        stu     ,x++
        decb
        bne     t6b
        clr     EXP
        clr     EXP+1
        lda     #1
        sta     EXP+2
        ldd     #256
        std     CMPLEN
        lbsr    genexp
        lbsr    cmpblk
        beq     t6c
        lbsr    prbad
        lbra    tabort
t6c     lbsr    waitnb          should go idle once data is read
        lbcs    tabto
        sta     FSTAT
        ldx     #s_st1
        lbsr    prstr
        lda     BSTAT
        lbsr    prhex8
        ldx     #s_fin
        lbsr    prstr
        lda     FSTAT
        lbsr    prhex8
        lbsr    crlf
        lbra    tpass

*===================================================================
* T7: file size not a multiple of 256 (3 extra bytes). Stream must
*     use raw file blocks: 64 full blocks + 1 partial block whose
*     first 3 bytes are 00 00 40, then end.
*===================================================================
test7   ldx     #s_t7
        lbsr    prstr
        ldx     #f_odd
        lbsr    domount
        lbne    tdone
        lda     #CMDSTRW
        ldx     #0
        ldy     #NBLK
        lbsr    runstream
        lbsr    rescode
        lbne    tabort
        lbsr    prstats
* partial block
        lbsr    readblk
        cmpb    #RC_RDY
        beq     t7a
        ldx     #s_nopart
        lbsr    prstr
        lbra    tfail
t7a     ldx     #s_part
        lbsr    prstr
        lda     BUF
        lbsr    prhex8
        lda     BUF+1
        lbsr    prhex8
        lda     BUF+2
        lbsr    prhex8
        lbsr    crlf
        ldd     BUF
        cmpd    #$0000
        lbne    tfail
        lda     BUF+2
        cmpa    #NBLK
        lbne    tfail
* now the stream should end
        lbsr    readblk
        cmpb    #RC_END
        beq     t7b
        ldx     #s_noend
        lbsr    prstr
        lbsr    doabort
        lbra    tfail
t7b     sta     FSTAT
        ldx     #s_fin
        lbsr    prstr
        lda     FSTAT
        lbsr    prhex8
        lbsr    crlf
        lda     FSTAT
        bita    #BUSY|FAILED
        lbne    tfail
        lbra    tpass

*===================================================================
* T8: (info) stream starting past end of file. Shows what status the
*     controller gives. Not pass/fail.
*===================================================================
test8   ldx     #s_t8
        lbsr    prstr
        lda     #CMDSTRW
        ldx     #NBLK+5
        ldy     #1
        lbsr    runstream
        cmpb    #RC_BAD
        bne     t8a
        ldb     #RC_RDY         got a block (data is meaningless)
t8a     pshs    b
        ldx     #s_rc
        lbsr    prstr
        lda     ,s
        lbsr    prhex8
        ldx     #s_st
        lbsr    prstr
        puls    b
        lda     FSTAT           status when stream ended
        cmpb    #RC_END
        beq     t8b
        lda     LSTAT           status on timeout
        cmpb    #RC_TO
        beq     t8b
        lda     FBSTAT          status at first block
t8b     lbsr    prhex8
        lbsr    crlf
        lbsr    doabort
        clr     CTRLATCH
        rts

*===================================================================
* T9: $FF42 as scratch RAM during a stream (NTM player pattern):
*     STA $FF42 = 4, then per block: read, poll, DEC $FF42, BNE.
*     Three passes from block 20 must read exactly 12 blocks.
*===================================================================
test9   ldx     #s_t9
        lbsr    prstr
        ldx     #f_512
        lbsr    domount
        lbne    tdone
        lbsr    cmdon
        lbsr    waitnb
        lbcs    tabto
        clr     PREG1
        clr     PREG2
        lda     #20
        sta     PREG3
        sta     EXP+2
        clr     EXP
        clr     EXP+1
        lda     #CMDSTRW
        sta     CMD
        sta     CMDREG
        ldd     #512
        std     CMPLEN
        ldd     #0
        std     CNT
        lda     #3
        sta     T9PASS
t9pass  lda     #4
        sta     FLASHD          STA <$42
t9blk   lbsr    readblk
        tstb
        beq     t9a
        sta     LSTAT
        lbsr    rescode
        ldx     #s_nodat
        lbsr    prstr
        lda     LSTAT
        lbsr    prhex8
        lbsr    crlf
        lbra    tabort
t9a     lbsr    genexp
        lbsr    cmpblk
        beq     t9b
        lbsr    prbad
        lbra    tabort
t9b     lbsr    incexp
        ldd     CNT
        addd    #1
        std     CNT
        cmpd    #40
        bhs     t9x
        dec     FLASHD          DEC <$42
        bne     t9blk
        dec     T9PASS
        bne     t9pass
t9x     lbsr    doabort
        ldx     #s_n2
        lbsr    prstr
        ldd     CNT
        lbsr    prhex16
        lbsr    crlf
        ldd     CNT
        cmpd    #12
        lbne    tfail
* readback in floppy mode too
        clr     CTRLATCH
        lda     #$5A
        sta     FLASHD
        lda     FLASHD
        cmpa    #$5A
        lbne    tfail
        lbra    tpass

*===================================================================
* T10: write to the data port during a stream (CSM player does
*      STD <$4A after each READY poll). The write must be ignored:
*      all 64 blocks must verify and the stream end with status 00.
*===================================================================
test10  ldx     #s_t10
        lbsr    prstr
        ldx     #f_512
        lbsr    domount
        lbne    tdone
        lbsr    cmdon
        lbsr    waitnb
        lbcs    tabto
        clr     PREG1
        clr     PREG2
        clr     PREG3
        clr     EXP
        clr     EXP+1
        clr     EXP+2
        lda     #CMDSTRW
        sta     CMD
        sta     CMDREG
        ldd     #512
        std     CMPLEN
        ldd     #0
        std     CNT
t10blk  lbsr    waitrdy
        sta     FSTAT
        cmpb    #RC_END
        beq     t10end
        tstb
        beq     t10a
        sta     LSTAT
        lbsr    rescode
        lbra    tabort
t10a    ldd     #$1234
        std     PREG2           STD <$4A while streaming
        ldx     #BUF
        clrb
t10r    ldu     PREG2
        stu     ,x++
        decb
        bne     t10r
        lbsr    genexp
        lbsr    cmpblk
        beq     t10b
        lbsr    prbad
        lbra    tabort
t10b    lbsr    incexp
        ldd     CNT
        addd    #1
        std     CNT
        cmpd    #NBLK+4
        blo     t10blk
        ldx     #s_run
        lbsr    prstr
        lbra    tabort
t10end  lbsr    prstats
        ldd     CNT
        cmpd    #NBLK
        lbne    tfail
        lda     FSTAT
        bita    #BUSY|FAILED
        lbne    tfail
        lbra    tpass

*-------------------------------------------------------------------
* Common test exits
*-------------------------------------------------------------------
tabto   ldx     #s_abto
        lbsr    prstr
        bra     tfail
tabort  lbsr    doabort         try to leave controller idle
tfail   ldx     #s_fail
        lbsr    prstr
        bra     tdone
tpass   ldx     #s_pass
        lbsr    prstr
tdone   clr     CTRLATCH
        rts

*===================================================================
* runstream - start a stream and read/verify blocks
*   A = command ($9x), X = start block (16 bit), Y = blocks to read
*   (0 = until the stream ends)
* returns B = RC_RDY  Y blocks read and verified, stream still active
*             RC_END  stream ended (FSTAT = final status)
*             RC_TO   timeout (LSTAT = last status)
*             RC_BAD  data mismatch (FOFF/FEXP/FGOT, BUF)
*             RC_RUN  more than NBLK+4 blocks
*   CNT = blocks verified, FBSTAT = status at first block
*===================================================================
runstream
        sta     CMD
        sty     NREAD
        clr     BLK
        stx     BLK+1
        clr     EXP
        stx     EXP+1
        ldd     #512
        std     CMPLEN
        ldd     #0
        std     CNT
        clr     FBSTAT
        clr     FSTAT
        lbsr    cmdon
        lbsr    waitnb
        bcc     rs1
        sta     LSTAT
        ldb     #RC_TO
        rts
rs1     lda     BLK
        sta     PREG1
        lda     BLK+1
        sta     PREG2
        lda     BLK+2
        sta     PREG3
        lda     CMD
        sta     CMDREG
rsloop  lbsr    readblk
        cmpb    #RC_END
        bne     rs2
        sta     FSTAT
        rts
rs2     cmpb    #RC_TO
        bne     rs3
        sta     LSTAT
        rts
rs3     ldd     CNT
        bne     rs4
        lda     BSTAT
        sta     FBSTAT
rs4     lbsr    genexp
        lbsr    cmpblk
        beq     rs5
        ldb     #RC_BAD
        rts
rs5     lbsr    incexp
        ldd     CNT
        addd    #1
        std     CNT
        cmpd    NREAD
        bne     rs6
        ldb     #RC_RDY
        rts
rs6     cmpd    #NBLK+4
        blo     rsloop
        ldb     #RC_RUN
        rts

*===================================================================
* readblk - wait for READY then read 512 bytes into BUF using the
*   transfer width from CMD bit 2.
*   returns B = RC_RDY / RC_END / RC_TO, A = status, BSTAT = status
*===================================================================
readblk lbsr    waitrdy
        sta     BSTAT
        tstb
        bne     rbx
        ldx     #BUF
        lda     CMD
        bita    #4
        bne     rbbyte
        clrb                    256 words
rbw     ldu     PREG2           reads $FF4A then $FF4B
        stu     ,x++
        decb
        bne     rbw
        lda     BSTAT
        clrb
rbx     rts
rbbyte  ldy     #512
rbb     lda     PREG3
        sta     ,x+
        leay    -1,y
        bne     rbb
        lda     BSTAT
        clrb
        rts

*===================================================================
* Controller helpers
*===================================================================
cmdon   lda     #CMDMODE
        sta     CTRLATCH
        rts

* waitnb - wait while BUSY. A = status, carry set on timeout
waitnb  pshs    x,y
        ldy     #16
wn1     ldx     #0
wn2     lda     STATREG
        bita    #BUSY
        beq     wnok
        leax    -1,x
        bne     wn2
        leay    -1,y
        bne     wn1
        orcc    #1
        puls    x,y,pc
wnok    andcc   #$FE
        puls    x,y,pc

* waitrdy - poll like StreamTest.asm: BUSY clear = done, READY = go
*   A = status, B = RC_RDY / RC_END / RC_TO
waitrdy pshs    x,y
        ldy     #16
wr1     ldx     #0
wr2     lda     STATREG
        bita    #BUSY
        beq     wrend
        bita    #READY
        bne     wrrdy
        leax    -1,x
        bne     wr2
        leay    -1,y
        bne     wr1
        ldb     #RC_TO
        puls    x,y,pc
wrend   ldb     #RC_END
        puls    x,y,pc
wrrdy   ldb     #RC_RDY
        puls    x,y,pc

* doabort - send $D0, wait idle. ASTAT = status, carry set on timeout
doabort lda     #CMDABORT
        sta     CMDREG
        lbsr    waitnb
        sta     ASTAT
        rts

* domount - mount file in drive 1. X -> null terminated name
*   prints error and returns Z clear on failure
domount ldu     #SENDBUF
        ldd     #$4D3A          "M:"
        std     ,u++
dm1     lda     ,x+
        sta     ,u+
        bne     dm1
dm2     cmpu    #SENDBUF+256
        bhs     dm3
        clr     ,u+
        bra     dm2
dm3     lbsr    cmdon
        lbsr    waitnb
        bcs     dmto
        lda     #CMDMOUNT
        sta     CMDREG
        lbsr    waitrdy
        tstb
        bne     dmnr
        ldx     #SENDBUF
        ldb     #128
dm4     ldu     ,x++
        stu     PREG2
        decb
        bne     dm4
        lbsr    waitnb
        bcs     dmto
        bita    #FAILED
        bne     dmerr
        clra                    Z set = ok
        rts
dmnr    sta     LSTAT
        ldx     #s_mnr
        bra     dmprt
dmto    sta     LSTAT
        ldx     #s_mto
        bra     dmprt
dmerr   sta     LSTAT
        ldx     #s_merr
dmprt   lbsr    prstr
        lda     LSTAT
        lbsr    prhex8
        lbsr    crlf
        ldx     #s_fail
        lbsr    prstr
        lda     #1              Z clear = failed
        rts

*===================================================================
* Pattern helpers
*===================================================================
* genexp - fill EXPBUF with the expected 512 bytes for block EXP
genexp  ldx     #EXPBUF
        lda     EXP
        sta     ,x+
        lda     EXP+1
        sta     ,x+
        lda     EXP+2
        sta     ,x+
        coma
        sta     ,x+
        lda     EXP+2
        adda    #4
        ldy     #508
ge1     sta     ,x+
        inca
        leay    -1,y
        bne     ge1
        rts

* cmpblk - compare CMPLEN bytes of BUF to EXPBUF. Z set if equal.
cmpblk  ldx     #BUF
        ldu     #EXPBUF
        ldy     #0
cb1     lda     ,x+
        cmpa    ,u+
        bne     cbf
        leay    1,y
        cmpy    CMPLEN
        bne     cb1
        clra
        rts
cbf     sta     FGOT
        lda     -1,u
        sta     FEXP
        sty     FOFF
        lda     #1
        rts

incexp  inc     EXP+2
        bne     ie1
        inc     EXP+1
        bne     ie1
        inc     EXP
ie1     rts

*===================================================================
* Reporting
*===================================================================
* rescode - print a message for error codes. B = code.
*   Z set if B is RC_RDY or RC_END (no error), B preserved
rescode cmpb    #RC_TO
        beq     rcto
        cmpb    #RC_BAD
        beq     rcbad
        cmpb    #RC_RUN
        beq     rcrun
        clra
        rts
rcto    ldx     #s_to
        lbsr    prstr
        lda     LSTAT
        lbsr    prhex8
        ldx     #s_n
        lbsr    prstr
        ldd     CNT
        lbsr    prhex16
        lbsr    crlf
        lda     #1
        rts
rcbad   lbsr    prbad
        lda     #1
        rts
rcrun   ldx     #s_run
        lbsr    prstr
        lda     #1
        rts

* prbad - print data mismatch details
prbad   ldx     #s_bad
        lbsr    prstr
        lda     EXP
        lbsr    prhex8
        lda     EXP+1
        lbsr    prhex8
        lda     EXP+2
        lbsr    prhex8
        ldx     #s_off
        lbsr    prstr
        ldd     FOFF
        lbsr    prhex16
        lbsr    crlf
        ldx     #s_ex
        lbsr    prstr
        lda     FEXP
        lbsr    prhex8
        ldx     #s_got
        lbsr    prstr
        lda     FGOT
        lbsr    prhex8
        lbsr    crlf
        ldx     #s_data
        lbsr    prstr
        ldx     #BUF
        ldb     #8
pb1     lda     ,x+
        lbsr    prhex8
        lda     #$20
        lbsr    putc
        decb
        bne     pb1
        lbsr    crlf
        rts

* prstats - "  N=nnnn ST1=ss FIN=ss"
prstats ldx     #s_n2
        lbsr    prstr
        ldd     CNT
        lbsr    prhex16
        ldx     #s_st1
        lbsr    prstr
        lda     FBSTAT
        lbsr    prhex8
        ldx     #s_fin
        lbsr    prstr
        lda     FSTAT
        lbsr    prhex8
        lbsr    crlf
        rts

prabort ldx     #s_abst
        lbsr    prstr
        lda     ASTAT
        lbsr    prhex8
        lbsr    crlf
        rts

* pause - wait for a key press (POLCAT scans the keyboard directly so
*   it works with interrupts masked)
pause   ldx     #s_key
        lbsr    prstr
pa1     jsr     [POLCAT]        drain any key already down
        bne     pa1
pa2     jsr     [POLCAT]
        beq     pa2
        lbsr    crlf
        rts

*-------------------------------------------------------------------
* Console output (all registers preserved)
*-------------------------------------------------------------------
putc    pshs    a,b,x,y,u
        jsr     [CHROUT]
        puls    a,b,x,y,u,pc

crlf    pshs    a
        lda     #$0D
        bsr     putc
        puls    a,pc

prstr   pshs    a,x
ps1     lda     ,x+
        beq     ps2
        bsr     putc
        bra     ps1
ps2     puls    a,x,pc

prhex16 bsr     prhex8
        tfr     b,a
prhex8  pshs    a
        lsra
        lsra
        lsra
        lsra
        bsr     prnib
        lda     ,s
        anda    #$0F
        bsr     prnib
        puls    a,pc
prnib   cmpa    #10
        blo     pn1
        adda    #7
pn1     adda    #'0
        bra     putc

*-------------------------------------------------------------------
* Strings
*-------------------------------------------------------------------
s_key   fcc     "PRESS A KEY..."
        fcb     0
s_title fcc     "SDC STREAM TEST V4"
        fcb     $0D,0
s_done  fcc     "DONE"
        fcb     $0D,0
s_t1    fcc     "T1 WORD STREAM 0..END"
        fcb     $0D,0
s_t2    fcc     "T2 START BLK 10, 5 BLKS, ABORT"
        fcb     $0D,0
s_t3    fcc     "T3 NEW STREAM AFTER ABORT"
        fcb     $0D,0
s_t4    fcc     "T4 EXIT LATCH MID-STREAM"
        fcb     $0D,0
s_t5    fcc     "T5 BYTE MODE BLK 5, 3 BLKS"
        fcb     $0D,0
s_t6    fcc     "T6 SECTOR READ AFTER STREAM"
        fcb     $0D,0
s_t7    fcc     "T7 ODD SIZE FILE (+3 BYTES)"
        fcb     $0D,0
s_t8    fcc     "T8 INFO: START PAST END"
        fcb     $0D,0
s_t9    fcc     "T9 $FF42 COUNTER IN STREAM"
        fcb     $0D,0
s_t10   fcc     "T10 WRITE $FF4A IN STREAM"
        fcb     $0D,0
s_pass  fcc     "  PASS"
        fcb     $0D,0
s_fail  fcc     "  ** FAIL **"
        fcb     $0D,0
s_to    fcc     "  TIMEOUT ST="
        fcb     0
s_abto  fcc     "  ABORT TIMEOUT"
        fcb     $0D,0
s_run   fcc     "  STREAM DID NOT END"
        fcb     $0D,0
s_noend fcc     "  NO END SEEN"
        fcb     $0D,0
s_nopart fcc    "  NO PARTIAL BLOCK"
        fcb     $0D,0
s_nodat fcc     "  NO DATA ST="
        fcb     0
s_part  fcc     "  PART BLK HDR="
        fcb     0
s_bad   fcc     "  BAD BLK="
        fcb     0
s_off   fcc     " OFF="
        fcb     0
s_ex    fcc     "  EXP="
        fcb     0
s_got   fcc     " GOT="
        fcb     0
s_data  fcc     "  DATA="
        fcb     0
s_n     fcc     " N="
        fcb     0
s_n2    fcc     "  N="
        fcb     0
s_st1   fcc     " ST1="
        fcb     0
s_fin   fcc     " FIN="
        fcb     0
s_st    fcc     " ST="
        fcb     0
s_rc    fcc     "  RC="
        fcb     0
s_abst  fcc     "  ABORT ST="
        fcb     0
s_mnr   fcc     "  MOUNT NOT READY ST="
        fcb     0
s_mto   fcc     "  MOUNT TIMEOUT ST="
        fcb     0
s_merr  fcc     "  MOUNT FAILED ST="
        fcb     0
f_512   fcc     "STRM512.DAT"
        fcb     0
f_odd   fcc     "STRMODD.DAT"
        fcb     0

*-------------------------------------------------------------------
* Variables
*-------------------------------------------------------------------
CMD     rmb     1
BLK     rmb     3
EXP     rmb     3
NREAD   rmb     2
CNT     rmb     2
CMPLEN  rmb     2
BSTAT   rmb     1
FBSTAT  rmb     1
FSTAT   rmb     1
LSTAT   rmb     1
ASTAT   rmb     1
T9PASS  rmb     1
FOFF    rmb     2
FEXP    rmb     1
FGOT    rmb     1
BUF     rmb     512
EXPBUF  rmb     512
SENDBUF rmb     256

        end     start

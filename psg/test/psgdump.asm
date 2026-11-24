* psgdump.asm -- helpers for PSGDUMP.BAS on a real CoCo 3 + MPI.
*
* The PSG only sees its registers with *SCS and its memory with *CTS, so
* each routine switches the MPI ($FF7F) to the PSG slot, does its work in
* ROM mode with interrupts off, then switches back to the disk slot.
* Nothing here writes the FLASH: the only cart-space write goes to SRAM
* segment 128, and FLASH programming (control bit 5) is never enabled.

MPI     equ     $FF7F
BANK0   equ     $FF5A
BANK1   equ     $FF5B
CTRL    equ     $FF5D
YMSEL   equ     $FF5E
YMDAT   equ     $FF5F
BUF     equ     $6000           ; 8K copy buffer $6000-$7FFF

        org     $5000

* EXEC $5000 -- copy segment SEG into BUF, set NONFF = count of non-$FF
* bytes (0 means the segment is blank).
dump    orcc    #$50
        lda     MPSG
        sta     MPI
        lda     #$10            ; 2MHz, autostart off, no writes
        sta     CTRL
        lda     SEG
        sta     BANK0
        sta     $FFDE           ; ROM mode
        ldx     #$C000
        ldu     #BUF
copy    ldd     ,x++
        std     ,u++
        cmpx    #$E000
        bne     copy
        sta     $FFDF           ; RAM mode
        lda     MSDC
        sta     MPI
        lda     $FF22           ; clear any CART FIRQ latched meanwhile
        andcc   #$AF
* count non-$FF bytes
        ldx     #BUF
        ldy     #0
scan    lda     ,x+
        cmpa    #$FF
        beq     scan2
        leay    1,y
scan2   cmpx    #BUF+$2000
        bne     scan
        sty     NONFF
        rts

* EXEC $5000+probe offset -- register read-back and mirror checks,
* results in RES+0..RES+23.
probe   orcc    #$50
        lda     MPSG
        sta     MPI
        lda     #5
        sta     BANK0
        lda     #130
        sta     BANK1
        lda     #$11            ; 1MHz + autostart off
        sta     CTRL
        lda     BANK0
        sta     RES+0
        lda     BANK1
        sta     RES+1
        lda     CTRL
        sta     RES+2
        lda     $FF5C           ; unused address
        sta     RES+3
        lda     #1              ; R1 <- $FF
        sta     YMSEL
        lda     #$FF
        sta     YMDAT
        lda     YMSEL
        sta     RES+4
        lda     YMDAT
        sta     RES+5
        lda     #7              ; R7 <- $3F (silent, ports input)
        sta     YMSEL
        lda     #$3F
        sta     YMDAT
        lda     #14
        sta     YMSEL
        lda     YMDAT
        sta     RES+6
        lda     #15
        sta     YMSEL
        lda     YMDAT
        sta     RES+7
* SRAM 128 vs 192 mirror: write $A5 to seg 128, read via seg 192
        lda     #$18            ; write enable + autostart off
        sta     CTRL
        sta     $FFDE           ; ROM mode
        lda     #128
        sta     BANK0
        lda     #$A5
        sta     $C000
        lda     $C000
        sta     RES+8           ; read back from 128
        lda     #192
        sta     BANK0
        lda     $C000
        sta     RES+9           ; same byte via 192
        lda     #64
        sta     BANK0
        lda     $C000
        sta     RES+10          ; ROM seg 64 byte 0
        ldx     #RES+12
        lda     #0
        sta     BANK0
        ldy     #$C000
f0      lda     ,y+             ; seg 0 bytes 0-7
        sta     ,x+
        cmpy    #$C008
        bne     f0
        lda     #64
        sta     BANK0
        ldy     #$C000
f64     lda     ,y+             ; seg 64 bytes 0-7
        sta     ,x+
        cmpy    #$C008
        bne     f64
        lda     #$10            ; writes off, autostart off
        sta     CTRL
        clr     BANK0
        clr     BANK1
        sta     $FFDF           ; RAM mode
        lda     MSDC
        sta     MPI
        lda     $FF22
        andcc   #$AF
        rts

* parameters / results (fixed addresses, POKEd/PEEKed by BASIC)
        org     $5F00
MPSG    rmb     1               ; $5F00 MPI value selecting the PSG
MSDC    rmb     1               ; $5F01 MPI value selecting the disk
SEG     rmb     1               ; $5F02 segment to dump
NONFF   rmb     2               ; $5F03 non-$FF byte count
        org     $5F10
RES     rmb     32              ; $5F10 probe results

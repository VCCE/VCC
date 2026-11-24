* autostart.asm -- segment 0 of the PSG test Flash image.
* Runs at $C000 when the cart autostarts: starts a 440Hz tone on YM
* channel A, sets control bit 4 (autostart off, releases CART), then
* cold-starts BASIC so you get the normal OK prompt with the tone on.

        ORG     $C000
start   ORCC    #$50            ; no interrupts
        LDX     #table
loop    LDD     ,X++            ; A = YM register, B = value
        CMPA    #$FF
        BEQ     done
        STA     $FF5E
        STB     $FF5F
        BRA     loop
done    LDA     #$10            ; control: 2MHz, autostart disabled
        STA     $FF5D
        CLR     >$0071          ; force a cold start (extended, DP-safe)
        JMP     [$FFFE]         ; reset vector -> BASIC

table   FCB     0,28            ; tone A period $11C = 440Hz at 2MHz
        FCB     1,1
        FCB     8,12            ; volume 12
        FCB     7,62            ; tone A only
        FCB     $FF,0

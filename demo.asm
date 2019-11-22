; write the "header" (pc, sp, rp)
__start __data_stack __return_stack

>0xfffe __data_stack:       ; start the data-stack at 0xfffe
>0xffde __return_stack:     ; start the return-stack at 0xffde

>0x0100                     ; write palette (4 colors)
    .0x2c .0x21 .0x37       ; #2c2137
    .0x76 .0x44 .0x62       ; #764462
    .0xa9 .0x68 .0x68       ; #a96868
    .0xed .0xb4 .0xa1       ; #edb4a1

>0x4400                     ; start writing code at 0x4400

; xorshift 16-bit random number generator
random_seed: ,0x1234        ; our random seed state

random:                     ; ( -- x ) generate a random number
    @random_seed ldw        ; get the current random number
    dup 7 shl xor
    dup 9 shr xor
    dup 8 shl xor
    dup @random_seed stw    ; store random number
    ret

__start:
    0x0400                  ; push v-ram address
    __start_loop:
        random 4 mod        ; generate a random number 0-1
        over stb            ; place the byte in vram
        1 add               ; next address
        dup 0x4400 lt       ; compare to end address
        @__start_loop jnz   ; do the loop until we filled the whole vram
    drop                    ; drop address
    @__start jmp            ; endless loop :)

# fx16 - a minimalistic fake 16-bit machine

## FX16 Architecture
- 64kb memory
- 3 16-bit registers
    - **pc**: program counter pointing to the next opcode
    - **sp**: stack pointer pointing to the next free data stack cell
    - **rp**: return pointer pointing to the next free return stack cell
- 32 instructions for the CPU (this barely classifies it as a MISC CPU)
- opcodes from **0x0020 - 0xffff** represent subroutine calls to this address
- 65536 cycles before video refreshes

### Memory Layout
| Region | Description |
|--------|-------------|
| 0000 - 0001 | shadow for the **pc** register (read-only) |
| 0002 - 0003 | shadow for the **sp** register (read-only) |
| 0004 - 0005 | shadow for the **rp** register (read-only) |
| 0006 - 0007 | frame counter |
| 0100 - 03ff | 256 color palette (3 bytes per entry RGB) |
| 0400 - 43ff | 128x128 pixel video ram -> 1 byte per pixel |
| 4400 - .... | user ram |

### Opcodes
| Opcode (hex) | Memnonic | Description | Stack Effect |
|--------------|----------|-------------|-------|
| 00 | nop | does nothing | --
| 01 | - | immediate: push next 16-bit value | -- a |
| 02 | drop | drop top stack value | a -- |
| 03 | dup | duplicate top stack value | a -- a a |
| 04 | swap | swap top most stack values | a b -- b a |
| 05 | over | copy second top most stack item to top | a b -- a b a |
| 06 | rot | rotate 3 stack items | a b c -- b c a |
| 07 | push | push top value to return stack | a --  R: -- a |
| 08 | pop | pop top value from return stack | -- a  R: a -- |
| 09 | ldb | load byte | addr -- x |
| 0a | ldw | load word | addr -- x |
| 0b | stb | store byte | x addr -- |
| 0c | stw | store word | x addr -- |
| 0d | jmp | jump to address | addr -- |
| 0e | jz | jump to address when *a* is 0 | a addr -- |
| 0f | jnz | jmp to address when *a* is not 0 | a addr -- |
| 10 | add | add top most values: c = a + b | a b -- c |
| 11 | sub | subtract top most values: c = a - b | a b -- c |
| 12 | mul | multiply top most values: c = a * b | a b -- c |
| 13 | div | divide top most values: c = a / b | a b -- c |
| 14 | mod | modulo top most values: c = a mod b | a b -- c |
| 15 | and | bitwise and top most values: c = a and b | a b -- c |
| 16 | or  | bitwise or top most values: c = a or b | a b -- c |
| 17 | xor | bitwise xor top most values: c = a xor b | a b -- c |
| 18 | not | bitwise invert top most value: b = not a | a -- b |
| 19 | shl | bitwise left shift: c = a shl b | a b -- c |
| 1a | shr | bitwise right shift: c = a shl b | a b -- c |
| 1b | eq  | compare top most values: c = a == b | a b -- c |
| 1c | lt  | compare top most values: c = a < b | a b -- c |
| 1d | le  | compare top most values: c = a <= b | a b -- c |
| 1e | call | call subroutine addr | addr -- R: -- pc |
| 1f | ret | return from subroutine | --  R: pc -- |
| 20 .. ff | - | call subroutine at this address | -- R: -- pc |

## FX16 assembly
Here is a very simple example
```asm
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
random_seed: ,1234          ; our random seed state

random:                     ; ( -- x ) generate a random number
    @random_seed ldw        ; get the current random number
    dup 9  shl
    dup 7  shr
    dup 13 shl
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
```

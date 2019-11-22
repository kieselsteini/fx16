/*
================================================================================

    FX16 - a minimalistic fake 16-bit machine

    This is free and unencumbered software released into the public domain.

    Anyone is free to copy, modify, publish, use, compile, sell, or
    distribute this software, either in source code form or as a compiled
    binary, for any purpose, commercial or non-commercial, and by any
    means.

    In jurisdictions that recognize copyright laws, the author or authors
    of this software dedicate any and all copyright interest in the
    software to the public domain. We make this dedication for the benefit
    of the public at large and to the detriment of our heirs and
    successors. We intend this dedication to be an overt act of
    relinquishment in perpetuity of all present and future rights to this
    software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
    OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
    ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.

    For more information, please refer to <https://unlicense.org>

================================================================================
*/
/*
================================================================================

        INCLUDES

================================================================================
*/
#include <stdio.h>
#include <stdint.h>
#include "SDL.h"


/*
================================================================================

        FX16 CPU

================================================================================
*/
static uint8_t          memory[0x10000];
static uint16_t         pc, sp, rp;

static uint8_t peek8(uint16_t ptr) { return memory[ptr]; }
static uint16_t peek16(uint16_t ptr) { return peek8(ptr) << 8 | peek8(ptr + 1); }
static void poke8(uint16_t ptr, uint8_t val) { memory[ptr] = val; }
static void poke16(uint16_t ptr, uint16_t val) { poke8(ptr, val >> 8); poke8(ptr + 1, val & 255); }
static void push(uint16_t val) { poke16(sp, val); sp -= 2; }
static void pushr(uint16_t val) { poke16(rp, val); rp -= 2; }
static void pushf(int x) { push(x ? 0xffff : 0x0000); }
static uint16_t pop() { sp += 2; return peek16(sp); }
static uint16_t popr() { rp += 2; return peek16(rp); }
static uint16_t nextpc() { uint16_t x = peek16(pc); pc += 2; return x; }

static void execute_cycles(int cycles) {
    uint16_t opcode, a, b, c;
    pc = peek16(0x0000); sp = peek16(0x0002); rp = peek16(0x0004);
    for (; cycles > 0; --cycles) {
        switch ((opcode = nextpc())) {
            case 0x00: break;
            case 0x01: push(nextpc()); break;
            case 0x02: (void)pop(); break;
            case 0x03: a = pop(); push(a); push(a); break;
            case 0x04: b = pop(); a = pop(); push(b); push(a); break;
            case 0x05: b = pop(); a = pop(); push(a); push(b); push(a); break;
            case 0x06: c = pop(); b = pop(); a = pop(); push(b); push(c); push(a); break;
            case 0x07: pushr(pop()); break;
            case 0x08: push(popr()); break;
            case 0x09: a = pop(); push(peek8(a)); break;
            case 0x0a: a = pop(); push(peek16(a)); break;
            case 0x0b: b = pop(); a = pop(); poke8(b, (uint8_t)(a & 255)); break;
            case 0x0c: b = pop(); a = pop(); poke16(b, a); break;
            case 0x0d: pc = pop(); break;
            case 0x0e: a = pop(); if (pop() == 0) pc = a; break;
            case 0x0f: a = pop(); if (pop() != 0) pc = a; break;
            case 0x10: b = pop(); a = pop(); push(a + b); break;
            case 0x11: b = pop(); a = pop(); push(a - b); break;
            case 0x12: b = pop(); a = pop(); push(a * b); break;
            case 0x13: b = pop(); a = pop(); push(a / b); break;
            case 0x14: b = pop(); a = pop(); push(a % b); break;
            case 0x15: b = pop(); a = pop(); push(a & b); break;
            case 0x16: b = pop(); a = pop(); push(a | b); break;
            case 0x17: b = pop(); a = pop(); push(a ^ b); break;
            case 0x18: a = pop(); push(~a); break;
            case 0x19: b = pop(); a = pop(); push(a << b); break;
            case 0x1a: b = pop(); a = pop(); push(a >> b); break;
            case 0x1b: b = pop(); a = pop(); pushf(a == b); break;
            case 0x1c: b = pop(); a = pop(); pushf(a < b); break;
            case 0x1d: b = pop(); a = pop(); pushf(a <= b); break;
            case 0x1e: pushr(pc); pc = pop(); break;
            case 0x1f: pc = popr(); break;
            default: pushr(pc); pc = opcode; break;
        }
    }
    poke16(0x0000, pc); poke16(0x0002, sp); poke16(0x0004, rp);
}

static int load_memory(const char *filename) {
    FILE *fp;
    int bytes;

    if ((fp = fopen(filename, "rb")) == NULL) return 0;
    bytes = fread(memory, 1, 0x10000, fp); fclose(fp);
    return bytes == 0x10000;
}


/*
================================================================================

        FX16 VIDEO

================================================================================
*/
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *texture = NULL;
static SDL_Surface *surface32 = NULL;
static SDL_Surface *surface8 = NULL;
static SDL_Color palette[256];

static int init_screen() {
    Uint32 pixel_format, rmask, gmask, bmask, amask;
    int bpp;

    if ((window = SDL_CreateWindow("FX16", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 256, 256, SDL_WINDOW_RESIZABLE)) == NULL) return 0;
    if ((renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)) == NULL) return 0;
    if (SDL_RenderSetLogicalSize(renderer, 128, 128)) return 0;
    if ((pixel_format = SDL_GetWindowPixelFormat(window)) == SDL_PIXELFORMAT_UNKNOWN) return 0;
    if (SDL_PixelFormatEnumToMasks(pixel_format, &bpp, &rmask, &gmask, &bmask, &amask) == SDL_FALSE) return 0;
    if ((texture = SDL_CreateTexture(renderer, pixel_format, SDL_TEXTUREACCESS_STREAMING, 128, 128)) == NULL) return 0;
    if ((surface32 = SDL_CreateRGBSurface(0, 128, 128, bpp, rmask, gmask, bmask, amask)) == NULL) return 0;
    if ((surface8 = SDL_CreateRGBSurface(0, 128, 128, 8, 0, 0, 0, 0)) == NULL) return 0;

    return 1;
}

static void shutdown_screen() {
    if (surface8 != NULL) SDL_FreeSurface(surface8);
    if (surface32 != NULL) SDL_FreeSurface(surface32);
    if (texture != NULL) SDL_DestroyTexture(texture);
    if (window != NULL) SDL_DestroyWindow(window);
}

static void render_screen() {
    int i, x, y;
    /* copy palette */
    for (i = 0; i < 256; ++i) {
        palette[i].r = peek8(0x0100 + i * 3);
        palette[i].g = peek8(0x0101 + i * 3);
        palette[i].b = peek8(0x0102 + i * 3);
    }
    /* copy vram */
    for (y = 0; y < 128; ++y) {
        for (x = 0; x < 128; ++x) {
            ((Uint8*)surface8->pixels)[surface8->pitch * y + x] = peek8(0x0400 + x + y * 128);
        }
    }
    SDL_SetRenderDrawColor(renderer, palette[0].r, palette[0].g, palette[0].b, 255);
    SDL_RenderClear(renderer);
    SDL_SetPaletteColors(surface8->format->palette, palette, 0, 256);
    SDL_BlitSurface(surface8, NULL, surface32, NULL);
    SDL_UpdateTexture(texture, NULL, surface32->pixels, surface32->pitch);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}


/*
================================================================================

        EVENT LOOP

================================================================================
*/
static void run_loop() {
    SDL_Event ev;
    uint16_t frame = peek16(0x0006);

    for (;;) {
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_QUIT:
                    return;
            }
        }
        poke16(0x0006, frame++);
        execute_cycles(0x10000);
        render_screen();
    }
}


/*
================================================================================

        INIT / SHUTDOWN

================================================================================
*/
static int init_fx16() {
    if (!load_memory("memory.bin")) return 0;
    if (SDL_Init(SDL_INIT_EVERYTHING)) return 0;
    if (!init_screen()) return 0;
    return 1;
}

static void shutdown_fx16() {
    shutdown_screen();
    SDL_Quit();
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    if (init_fx16()) run_loop();
    shutdown_fx16();
    return 0;
}

#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"

#define NUM_ELE_IN_DWORD 4
#define UT 0

int main (int argc, char *argv[])
{
#if UT
    for (int r = 0; r < 16; r++) {
        for (int c = 0; c < 16; c+=4) {
            uint32_t addr = (r * (16 / 4)) + (c/4) - (r / 4);
            printf("r:%d, c:%d\t", r, c);
            if((r/4) > (c/4)) {
            } else {
                int rd = (r / 4);
                addr -= (r - (2 * rd) - 2) * rd;
                printf("addr:%x", addr);
            }
            printf("\n");
        }
    }

#else
    for (int r = 0; r < 16; r++) {
        for (int c = 0; c < 16; c+=4) {
            uint32_t addr = c / 4;
            int rd = (r / 4);
            printf("r:%d, c:%d\t", r, c);
            if((c/4) > (r/4)) {
            } else {
                addr += (r - (2 * rd)) * (rd + 1);
                printf("addr:%x", addr);
            }
            printf("\n");
        }
    }
#endif
}

//
//  scaled_mask.cpp
//  RC3DK
//
//  Created by Eagle Jones on 5/22/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#include "scaled_mask.h"
#include <string.h>
#include <stdio.h>
#include <algorithm>
//#include "./hw/fast_hw.h"
extern void *vio_malloc(size_t size);
extern void vio_free(void *);

scaled_mask::scaled_mask(int _width, int _height, int mask_shift) {
    this->mask_shift = mask_shift;
    //assert((((_width >> mask_shift) << mask_shift) == _width) && (((_height >> mask_shift) << mask_shift) == _height));
    scaled_width = _width >> mask_shift;
    scaled_height = (_height+7) >> mask_shift;
    mask = new uint8_t[scaled_width * scaled_height];
    compressed_width = (((scaled_width + 0x7) >> 3) + 0xf) & ~(0xf);
    compressed = new uint8_t[ compressed_width * scaled_height];
}

scaled_mask::~scaled_mask()
{
    delete[] mask;
    delete[] compressed;
}

void scaled_mask::clear(int fx, int fy)
{
    int x = fx >> mask_shift;
    int y = fy >> mask_shift;
    if(x < 0 || y < 0 || x >= scaled_width || y >= scaled_height) return;
    mask[x + y * scaled_width] = 0;
    if(y > 1) {
        //don't worry about horizontal overdraw as this just is the border on the previous row
        for(int i = 0; i < 3; ++i) mask[x-1+i + (y-1)*scaled_width] = 0;
        mask[x-1 + y*scaled_width] = 0;
    } else {
        //don't draw previous row, but need to check pixel to left
        if(x > 1) mask[x-1 + y * scaled_width] = 0;
    }
    if(y < scaled_height - 2) {
        for(int i = 0; i < 3; ++i) mask[x-1+i + (y+1)*scaled_width] = 0;
        mask[x+1 + y*scaled_width] = 0;
    } else {
        if(x < scaled_width - 1) mask[x+1 + y * scaled_width] = 0;
    }
}

void scaled_mask::initialize()
{
    //set up mask - leave a 1-block strip on border off
    //use 8 byte blocks
    memset(mask, 0, scaled_width);
    memset(mask + scaled_width, 1, (scaled_height - 2) * scaled_width);
    memset(mask + (scaled_height - 1) * scaled_width, 0, scaled_width);
    //vertical border
    for(int y = 1; y < scaled_height - 1; ++y) {
        mask[0 + y * scaled_width] = 0;
        mask[scaled_width-1 + y * scaled_width] = 0;
    }
}

#define AS_DEFINED_IN_MAIL 1
#if AS_DEFINED_IN_MAIL
void scaled_mask::compress()
{
    int max = 0;
    for(int j = 0; j < scaled_height; j++) {
        for(int i = 0; i < scaled_width; i+= 128) {
            for(int p = 0; i+p < scaled_width; p +=8) {
                uint8_t val = 0;
                for(int k = 0; k < 8; k++) {
                    val = val << 1;
                    val = (val | (mask[(j * scaled_width) + i + p + k] != 0));
                }
                max = std::max(max, ((i + 127 - p) >> 3) + (j * compressed_width));
                compressed[((i + 127 - p) >> 3) + (j * compressed_width)] = val;
            }
        }
    }
    printf("Compressed access %d\n", max);
}
#else
void scaled_mask::compress()
{
    int max = 0;
    for(int j = 0; j < scaled_height; j++) {
        for(int i = 0; i < scaled_width; i+= 8) {
            uint8_t val = 0;
            for(int k = 8; k--; ) {
                val = val << 1;
                val = (val | (mask[(j * scaled_width) + i + k] != 0));
            }
            compressed[(i >> 3) + (j * compressed_width)] = val;
        }
    }
    printf("Compressed access %d\n", max);
}
#endif

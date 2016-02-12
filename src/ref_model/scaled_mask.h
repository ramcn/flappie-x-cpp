//
//  scaled_mask.h
//  RC3DK
//
//  Created by Eagle Jones on 5/22/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#ifndef __RC3DK__scaled_mask__
#define __RC3DK__scaled_mask__

#include <assert.h>
#include <stdint.h>

class scaled_mask
{
public:
    void clear(int fx, int fy); //clears the mask at this location, indicating that a feature should not be detected there
    void initialize();
    void compress();
    inline bool test(int x, int y) const { return mask[(x >> mask_shift) + scaled_width * (y >> mask_shift)] != 0; }
    // mask shift of 3 is 8x8 pixel blocks
    scaled_mask(int _width, int _height, int mask_shift = 3);
    ~scaled_mask();
    //protected:
    int scaled_width, scaled_height, mask_shift, compressed_width;
    //TODO: should this be a bitfield instead of bytes?
    uint8_t *mask;
    uint8_t *compressed;
};
#endif /* defined(__RC3DK__scaled_mask__) */

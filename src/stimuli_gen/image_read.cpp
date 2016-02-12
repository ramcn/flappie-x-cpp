#include "stdint.h"
#include "stdlib.h"
#include "stdio.h"
#include "assert.h"


struct __attribute__((__packed__)) Bmp_Header
{
    unsigned short type;
    unsigned int   size; // x
    unsigned short reserved1;
    unsigned short reserved2;
    unsigned int   off_bits; // x
    Bmp_Header(uint32_t _size, uint32_t _off_bits) {
        type      = ((unsigned int)'M' << 8) + 'B' ;
        size      = _size;
        reserved1 = reserved2 = 0;
        off_bits  = _off_bits;
    };
};

struct __attribute__((__packed__)) Bmp_Info
{
    unsigned int   size;
    unsigned int   width; // x
    unsigned int   height; // x
    unsigned short planes; 
    unsigned short bit_count;
    unsigned int   compression;
    unsigned int   size_image; // x
    unsigned int   x_pels_per_meter;
    unsigned int   y_pels_per_meter;
    unsigned int   clr_used;
    unsigned int   clr_important;
    Bmp_Info(uint32_t w, uint32_t h, uint32_t s) {
        size      = sizeof(Bmp_Info);
        width     = w;
        height    = h;
        planes    = 1;
        bit_count = 8;
        compression = 0;
        size_image  = s;
        x_pels_per_meter = 0x0;
        y_pels_per_meter = 0x0;
        clr_used  = 0x100;
        clr_important    = 0;
    };
};

inline void bmp_write(const char* img_file, const unsigned char *im,
                      uint32_t width, uint32_t height, uint32_t step, bool dump = false)
{
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define ALIGN4(x) (((x) + 3) & (~0x3))
    FILE *fp = fopen(img_file, "wb");
    assert(fp != NULL);
    uint32_t size      = height * ALIGN4(width);
    uint32_t palatte   = 0x100 * 4;
    uint32_t hdr_bytes = sizeof(Bmp_Header) + sizeof(Bmp_Info) + palatte;
    Bmp_Header hdr(size + hdr_bytes, hdr_bytes);
    Bmp_Info info(width, height, size);
    fwrite(&hdr, 1, sizeof(Bmp_Header), fp);
    fwrite(&info, 1, sizeof(Bmp_Info), fp);
    for(int i = 0; i < 0x100; i++) {
        unsigned int p = ((((i << 8) + i) << 8) + i);
        fwrite(&p, 1, sizeof(p), fp);
    }
    for(int i = height; i--; ) {
        size_t bytes = fwrite(&im[i*step], 1, width, fp); 
        assert(bytes == width);
        if(dump) {
            for(int j = width; j--;) {
                printf("%2x", im[i*step + j]);
            }
            printf("\n");
        }
        size_t pad   = ALIGN4(width) - width;
        if(pad) {
            bytes = fwrite(&im[i*step], 1, pad, fp);
            assert(bytes == pad);
        }
    }
    fclose(fp);
#undef ALIGN4
#undef MIN
}


inline void
read_mem_fp(FILE *fp, unsigned char *mem, size_t size, const int bpl = 16)
{
    for(size_t off = 0; off < size; off += bpl) {
        for(int j = bpl; j--; ) {
            unsigned int c = 0;
            if(off + j < size) {
                fscanf(fp, "%02X", &c);
            }
            mem[off+j] = c;
        }
    } 
}

int main (int argc, char *argv[])
{
    FILE *fp = fopen(argv[1], "r");
    assert(fp);
    uint32_t width, height;
    fscanf(fp, "// width: %d, height: %d\n", &width, &height);
    size_t size = width * height;

    unsigned char *buf = (unsigned char *)malloc(size);
    assert(buf);

    read_mem_fp(fp, buf, size);

    printf("width: %d, height: %d\n", width, height);
    bmp_write("image.bmp", buf, width, height, width);

    free(buf);
    fclose(fp);
    return 0;
}

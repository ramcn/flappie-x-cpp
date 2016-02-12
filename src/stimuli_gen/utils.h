#pragma once

#include <stdio.h>
#include <assert.h>
#include <string>
#include <iostream>
#include <fstream>
using namespace std;

//#define ASSERT(c, s) if(!c) {std::cout << s << std::endl; assert(c);}
#define ASSERT(cond, ...)  \
    if(!(cond)) {        \
        printf("Assert: %s %d : ", __FILE__, __LINE__);    \
        printf(__VA_ARGS__); printf("\n");                 \
        exit(1); \
    }


inline void dump_mem_fp(FILE *fp, const unsigned char *mem, size_t size, const int bpl = 16)
{
    for(size_t off = 0; off < size; off += bpl) {
        for(int j = bpl; j--; ) {
            unsigned int c = (off + j < size) ? mem[off+j] : 0;
            fprintf(fp, "%02X", c);
        }
        fprintf(fp, "\n");
    }
}


inline void dump_mem(const char *filename, const unsigned char *mem, size_t size,
                     const int bpl = 16)
{
    FILE *fp = fopen(filename, "w+"); assert(fp);
    dump_mem_fp(fp, mem, size, bpl);
    fclose(fp);
}

inline void dump_mem2d_fp(FILE *fp,  const unsigned char *mem, unsigned int width,
                          unsigned int height, unsigned int stride)
{
    fprintf(fp, "// width: %d, height: %d, stride:%d\n", width, height, stride);
    size_t size = stride * height;
    for(size_t line = 0; line < size; line += 16) {
        for(int j = 16; j--; ) {
            unsigned int h = (line + j) / stride;
            unsigned int w = (line + j) - (h * stride);
            unsigned int c = (h < height) && (w < width) ? mem[(stride * h) + w] : 0;
            fprintf(fp, "%02x", c);
        }
        fprintf(fp, "\n");
    } 
}

inline void dump_mem2d(const char *filename, const unsigned char *mem, unsigned int width,
                       unsigned int height, unsigned int stride)
{
    FILE *fp = fopen(filename, "w+"); ASSERT(fp, "%s\n", filename);
    dump_mem2d_fp(fp, mem, width, height, stride);
    fclose(fp);
}


inline void load_file(string file, void *buf, uint32_t size, const char *str,
                      uint32_t &rows, uint32_t &cols, uint32_t &stride, bool flip = false,
                      size_t elm_size=sizeof(float), uint32_t new_stride = 0)
{
    std::ifstream is(file);
    ASSERT(is.is_open(), "Couldn't open file %s\n", file.c_str());
    string line; int32_t line_num = -1; uint32_t rd_size = 0;

    if (flip == 1);
    {
      printf("flip is on\n");
    }
    
    void *tmp = NULL; 
    uint64_t *dst = NULL;
    if(new_stride) {
        tmp = malloc(size);
        dst = (uint64_t *)tmp;
    } else {
        dst = (uint64_t *)buf;
    }
    while(!is.eof()) {
        line_num++;
        std::getline(is, line);
        if(is.eof()) {break;}
        if(line_num == 0) {
            int status = sscanf(line.c_str(), str, &cols, &rows, &stride);
            ASSERT(status ==3, "Couldn't read file header, file:%s, \nstr   :%s, \nheader:%s\n",
                   file.c_str(), str, line.c_str());
            continue;
        }
        if(line.find("//") != std::string::npos) {continue;}
        ASSERT(line.size() == 32, "Incorrect file %s at line:%d \n",
               file.c_str(), line_num);

        int status;
        status = sscanf(line.substr(16,16).c_str(), "%llx", dst++);
        ASSERT(status == 1, "Incorrect file %s at line:%d \n", file.c_str(), line_num);
        status = sscanf(line.substr( 0,16).c_str(), "%llx", dst++);
        ASSERT(status == 1, "Incorrect file %s at line:%d \n", file.c_str(), line_num);
        rd_size += 16;
    }
    ASSERT(rd_size <= size, "Provided buffer size insufficient, file:%s, rd_size:%d, size:%d\n",
           file.c_str(), rd_size, size);
    uint32_t exp_size = elm_size * stride * (flip ? cols : rows);
    printf("elm_size=%d, stride=%d\n", elm_size, stride);
    ASSERT(exp_size == rd_size, "file:%s exp_size: 0x%x, rd_size: 0x%x\n",
	   file.c_str(), exp_size, rd_size);
    is.close();

    if(new_stride) {
        uint32_t _cols = flip ? rows : cols;
        uint32_t _rows = flip ? cols : rows;
        uint8_t *_src  = (uint8_t *)tmp; uint8_t *_dst = (uint8_t*)buf;
        for(uint32_t i = 0; i < _rows; i++) {
            memcpy(_dst, _src, _cols * elm_size);
            _dst += new_stride * elm_size;
            _src += stride * elm_size;
        }
        stride = new_stride;
	printf("new_stride =%d\n", stride);
        free(tmp);
    }

}

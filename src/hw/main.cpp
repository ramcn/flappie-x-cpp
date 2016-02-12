#include <iostream>
#include <string>
#include <vector>
#include <assert.h>
//#include <opencv2/core/core.hpp>
//#include <opencv2/highgui/highgui.hpp>
//#include <opencv2/mat.hpp>
#include "fast_hw.h"
#include "bitmap_image.hpp"

#define SSC_MARK(mark_value)                    \
    {__asm  mov ebx, mark_value}                \
    {__asm  mov ebx, ebx}                       \
    {__asm  _emit 0x64}                         \
    {__asm  _emit 0x67}                         \
    {__asm  _emit 0x90}

//#define SSC_MARK(value)  \
//    __asm__("mov %ebx 0xaa\n" \
//            "mov %ebx %ebx\n" \
//            "_emit 0x64\n" \
//            "_emit 0x67\n" \
//            "_emit 0x90")
extern int g_ncc_compute;
int g_ncc_compute = 0;
string g_det_mode = "";

void print_help()
{
    printf("Usage: <exe> <input image> <num features> <threshold> <run mode:{detect,all-1-by-1,all-together}>\n");
    exit(1);
}

int main (int argc, char *argv[])
{
    if(argc <= 4) {
        print_help();
    }
    string filename = argv[1];
    int num_points  = atoi(argv[2]);
    int threshold   = atoi(argv[3]);
    g_det_mode      = argv[4];
    if(g_det_mode != "detect" && g_det_mode != "all-1-by-1" && g_det_mode != "all-together") {
        printf("Invalid run mode: %s\n", g_det_mode.c_str());
        print_help();
    }
    
    //cv::Mat img = cv::imread(argv[1], CV_LOAD_IMAGE_GRAYSCALE);
    bitmap_image bmp(filename);
    printf("Image file read\n");
    const int IMG_WD = bmp.width();
    const int IMG_HT = bmp.height();
    uint8_t *img_data = bmp.row(0);
    
    fast_detector_9_hw fast;
    fast.init(IMG_WD, IMG_HT, IMG_WD, 7, 3);
    scaled_mask mask(IMG_HT, IMG_WD);
    fast.detect1(img_data, &mask, num_points, threshold, 0, 0, IMG_WD, IMG_HT);

    auto &v   = fast.features;
    auto &p = fast.patches;
    printf("Num Key Points: %ld\n", v.size());
    size_t min = std::min(v.size(), (size_t)10);
    for(int i = 0; i < min; i++) {
        printf("X:%d, Y:%d, Score:%d\n", v[i].x, v[i].y, v[i].score);
    }


    FILE *out_fp = fopen("out.txt", "w+"); assert(out_fp); 
    for(int i = 0; i < v.size(); i++) {
        fprintf(out_fp, "X:%d, Y:%d, Score:%d\n", v[i].x, v[i].y, v[i].score); 
        if(g_det_mode != "detect") {
            for(int j = 0; j < 4; j++) { 
                for(int k = 16; k--; ) {
                    fprintf(out_fp, "%02x", p[i].buf[j+k]);
                }
                fprintf(out_fp, "\n");
            }
        }
    } 
    fclose(out_fp);
    return 0;
}

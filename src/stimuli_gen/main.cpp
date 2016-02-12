#include <iostream>
#include <string>
#include <vector>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
//#include <opencv2/mat.hpp>
#include "../ref_model/fast.h"

#define SSC_MARK(mark_value)         \
        {__asm  mov ebx, mark_value} \
        {__asm  mov ebx, ebx}        \
        {__asm  _emit 0x64}          \
        {__asm  _emit 0x67}          \
        {__asm  _emit 0x90}

//#define SSC_MARK(value)  \
//    __asm__("mov %ebx 0xaa\n" \
//            "mov %ebx %ebx\n" \
//            "_emit 0x64\n" \
//            "_emit 0x67\n" \
//            "_emit 0x90")
extern int g_ncc_compute;
int g_ncc_compute = 0;
int main (int argc, char *argv[])
{
    if(argc <= 3) {
        printf("Usage: <exe> <input image> <num features> <threshold>\n");
        exit(1);
    }
    
    cv::Mat img = cv::imread(argv[1], CV_LOAD_IMAGE_GRAYSCALE);
    if( !img.data ) {
        std::cout << " Couldn't read input image " << argv[1] << std::endl; 
    }

    assert(img.elemSize() == 1);
#ifdef DUMP_IMG    
    FILE *fp_wr = fopen("img_raw.dat", "wb"); assert(fp_wr);
    fwrite(img.data, 1, (img.step * img.rows), fp_wr);
    fclose(fp_wr);
#endif
    
    unsigned char *img_data = (unsigned char*)img.data;
    int num_points = atoi(argv[2]); int threshold = atoi(argv[3]);
    fast_detector_9 fast;
    fast.init(img.cols, img.rows, img.step, 7, 3);
#ifdef LIT_MARKER
    __SSC_MARK(0xaa);
#endif
    fast.detect(img.data, NULL, num_points, threshold, 0, 0, img.cols, img.rows);
#ifdef LIT_MARKER
    __SSC_MARK(0xbb);
#endif

    vector<xy> &v = fast.features;
    printf("Num Key Points: %d\n", v.size());
    /*
    for(int i = 0; i < v.size(); i++) {
        if(v[i].score > threshold) {
            printf("X:%0.8f, Y:%0.8f, Score:%0.8f\n", v[i].x, v[i].y, v[i].score);
        }
    }
    */
    return 0;
}

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <time.h>
#include <cstdio>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgproc/imgproc_c.h>
#include "../ref_model/fast.h"
#include "../hw/fast_hw_h.h"
#include "../ref_model/fast_constants.h"
#include "../stimuli_gen/utils.h"
#include "../stimuli_gen/matrix_generation.h"

template <class T>
bool predy_cmp(const T &a, const T &b)
{
    return (a.y < b.y); 
}

template <class T>
bool patchy_cmp(const T &a, const T &b)
{
    return (a.xy.y < b.xy.y); 
}


extern int g_ncc_compute;
int g_ncc_compute = 0;

#if DETECTOR_INS
FILE *g_fp_dist = NULL;

FILE *g_fp_dist_1 = NULL;
int g_det_count = 0;
#endif

#if TRACKER_INS
FILE *g_fp_patch = NULL;
FILE *g_fp_list = NULL;
FILE *g_fp_result = NULL;
#endif


#if SORT_INS
FILE *g_fp_sort = NULL;
FILE *g_fp_sort_hex = NULL;
FILE *g_fp_sort_patch = NULL;

FILE *g_fp_sort_1 = NULL;
FILE *g_fp_sort_hex_1 = NULL;
FILE *g_fp_sort_patch_1 = NULL;

FILE *fp_mask_file = NULL;
FILE *fp_mask_file_1 = NULL;

FILE *fp_mask_excel = NULL;
FILE *fp_mask_excel_1 = NULL;
#endif

FILE *CONFIG = NULL;
FILE *A = NULL;
FILE *B = NULL;
FILE *C = NULL;
FILE *D = NULL;
FILE *E = NULL;
FILE *F = NULL;
FILE *G = NULL;
FILE *H = NULL;
FILE *I = NULL;
FILE *J = NULL;
FILE *K = NULL;
FILE *L = NULL;
FILE *O = NULL;
FILE *R = NULL;
FILE *S = NULL;
FILE *Q = NULL;
FILE *T = NULL;
FILE *W = NULL;
FILE *MASK1 = NULL;
FILE *MASK2 = NULL;
FILE *SEED = NULL;
using namespace cv;

void MyLine(Mat img, Point start, Point end );


static Scalar randomColor( RNG& rng )
{
  int icolor = (unsigned) rng;
  return Scalar( icolor&255, (icolor>>8)&255, (icolor>>16)&255 );
}

void get_patch(const uint8_t *im, int x, int y, uint32_t stride, uint8_t *buf)
{
    uint32_t wd = full_patch_width; uint32_t ht = full_patch_width;
    im = im + ((y - ht/2) * stride) + (x - wd/2);
    for(uint32_t i = 0; i < ht; i++) {
        memcpy(buf, im, wd);
        buf += wd; im += stride;
    }
}



int main (int argc, char *argv[])
{
    //if(argc <= 8) {
    //   printf("Usage: <exe> <input image> <num features> <threshold> <width> <height> <mask_en> <tthr> <test_image> <instr_dir>\n");
    //   exit(1);
    //}
    time_t t;
    unsigned seed = (unsigned) time (&t);
    srand(seed);
    string dir, instr_dir;
    int num_points, threshold, width, height, mask_en, tthreshold, test_image, imageSize, inner_start, sample, new_width, new_height;
    Mat copyimg;
    unsigned char *copyimg_data;
    unsigned char *finalimg_data;



    int buffassign[3] = {0, 1, 2};
    int buffassign_1[2] = {1, 2};
    int randomArrayind_1 = rand() % 3;
    int randomArrayind_2 = rand() % 2;
    int trkcbbuffassignment = buffassign[randomArrayind_1];
    int cbdet1buffassignment;
    int cbdet2buffassignment;


    if (trkcbbuffassignment == 0) {
        cbdet1buffassignment = 0;
        cbdet2buffassignment = 0;
    } else if (trkcbbuffassignment == 1 || trkcbbuffassignment == 2){
        cbdet1buffassignment = buffassign_1[randomArrayind_2];
        if (cbdet1buffassignment == 1){
            cbdet2buffassignment = 2;
        } else {
            cbdet2buffassignment = 1;
        }
    }
    
    int det1_points;
    int det2_points;
    
    if (std::string (argv[1]) == "rc_code") {
        string dir_d = argv[2];
        int sample = atoi(argv[3]);
        int num_points_d = atoi(argv[4]);
        int threshold_d  = atoi(argv[5]);
        int width_d = atoi(argv[6]);
        int height_d = atoi(argv[7]);
        int new_width_d = atoi(argv[8]);
        int new_height_d = atoi(argv[9]);
        int mask_en_d = atoi(argv[10]);
        int tthreshold_d = atoi(argv[11]);
        string instr_dir_d = argc > 12 ? argv[12] : ".";
        dir = dir_d; num_points = num_points_d; threshold = threshold_d; width = width_d; height = height_d;
        mask_en = mask_en_d; tthreshold = tthreshold_d; instr_dir = instr_dir_d; new_width = new_width_d; new_height = new_height_d;
        enum {stimuli};
        if (sample == stimuli){
            printf("input_image provided\n");
            string img_file  = dir + "/image.bmp";
            cv::Mat img = cv::imread(img_file.c_str(), CV_LOAD_IMAGE_GRAYSCALE);
            if( !img.data ) {
                std::cout << " Couldn't read input image " << argv[1] << std::endl; 
            }
     
            float img_cols = img.cols;
            float img_rows  = img.rows;
            float x = width/img_cols;
            float y = height/img_rows;
     
            Mat copyimg_d(img.clone());

            imwrite( "copyimg.pgm", copyimg_d);

            copyimg = copyimg_d.clone();
            
        }
        
    }              
    

    A = fopen((instr_dir + "__CHL_Out_Matrix.txt").c_str(), "w+");
    D = fopen((instr_dir + "__CHL_Inv_Out_Matrix.txt").c_str(), "w+");
    B = fopen((instr_dir + "__CHLT_Out_Matrix.txt").c_str(), "w+");   
    C = fopen((instr_dir + "__CHL_In_Matrix.txt").c_str(), "w+");
    E = fopen((instr_dir + "__LCt_inn_out.txt").c_str(), "w+");
    F = fopen((instr_dir + "__LCt_inn_in.txt").c_str(), "w+");
    G = fopen((instr_dir + "__cov_state_in.txt").c_str(), "w+");
    H = fopen((instr_dir + "__cov_state_out.txt").c_str(), "w");
    I = fopen((instr_dir + "__cholesky_sw_out.txt").c_str(), "w+");
    W = fopen((instr_dir + "__Kt_out.txt").c_str(),"w+");
    SEED= fopen((instr_dir + "__SEED.txt").c_str(),"w+");
    
#if TRACKER_INS
    g_fp_patch = fopen((instr_dir + "__Trk_FPPatchList.txt").c_str(), "w+"); assert(g_fp_patch);
    g_fp_list = fopen((instr_dir + "__Trk_FPPredList.txt").c_str(), "w+"); assert(g_fp_list);
    g_fp_result = fopen((instr_dir + "__Trk_FPTrackList.txt").c_str(), "w+");
#endif
    
   
    copyimg_data = (unsigned char*)copyimg.data;
    if (trkcbbuffassignment == 0 || trkcbbuffassignment == 1){
        dump_mem2d((instr_dir + "__Image2.txt").c_str(), copyimg_data, copyimg.cols, copyimg.rows, copyimg.step);
    } else {
        dump_mem2d((instr_dir + "__Image1.txt").c_str(), copyimg_data, copyimg.cols, copyimg.rows, copyimg.step);

    }
    fast_detector_9 fast;
    fast.init(copyimg.cols, copyimg.rows, copyimg.step, 7, 3);
    if (trkcbbuffassignment == 0 || trkcbbuffassignment == 1){
#if DETECTOR_INS
    g_fp_dist = fopen((instr_dir + "__Det2_FPList.txt").c_str(), "w+"); assert(g_fp_dist);
    CONFIG = fopen((instr_dir + "__Config.txt").c_str(), "w+"); assert(CONFIG);
#endif

#if SORT_INS
    g_fp_sort = fopen((instr_dir + "__Sorts2.txt").c_str(), "w+"); assert(g_fp_sort);
    g_fp_sort_hex = fopen((instr_dir + "__Det2_SortedFPList.txt").c_str(), "w+"); assert(g_fp_sort_hex);
    g_fp_sort_patch = fopen((instr_dir + "__Det2_SortedFPPatchList.txt").c_str(), "w+"); assert(g_fp_sort_patch);
#endif
    } else {

#if DETECTOR_INS
    g_fp_dist = fopen((instr_dir + "__Det1_FPList.txt").c_str(), "w+"); assert(g_fp_dist);
    CONFIG = fopen((instr_dir + "__Config.txt").c_str(), "w+"); assert(CONFIG);
#endif

#if SORT_INS
    g_fp_sort = fopen((instr_dir + "__Sorts1.txt").c_str(), "w+"); assert(g_fp_sort);
    g_fp_sort_hex = fopen((instr_dir + "__Det1_SortedFPList.txt").c_str(), "w+"); assert(g_fp_sort_hex);
    g_fp_sort_patch = fopen((instr_dir + "__Det1_SortedFPPatchList.txt").c_str(), "w+"); assert(g_fp_sort_patch);
#endif

    }
   
#if MASK_EN 
// Read mask points binary file
    if (trkcbbuffassignment == 0 || trkcbbuffassignment == 1){
        fp_mask_file = fopen((instr_dir + "__Masked2_Coordinates.txt").c_str(), "w+");
    } else {
        fp_mask_file = fopen((instr_dir + "__Masked1_Coordinates.txt").c_str(), "w+");
    }
        
    assert(fp_mask_file);
    
    scaled_mask mask(copyimg.cols, copyimg.rows);
    mask.initialize();
    if(mask_en) {
        time_t t;
        srand((unsigned) time (&t));
        //srand(1);//srand(time(NULL)); TODO to introduce true random
        for(uint32_t fy = 0; fy < copyimg.rows; fy += 8) {
            for(uint32_t fx = 0; fx < copyimg.cols; fx += 8) {
                int frac = (rand() % 100);
                if(frac < 10) {
                    mask.clear(fx, fy);
                }
            }
        }
    }
    if (trkcbbuffassignment == 0 || trkcbbuffassignment == 1){
        fp_mask_excel = fopen((instr_dir + "__Masked2_Bits.txt").c_str(), "w+");
    } else {
         fp_mask_excel = fopen((instr_dir + "__Masked1_Bits.txt").c_str(), "w+");
    }
    assert(fp_mask_excel);
    for(uint32_t fy = 0; fy < copyimg.rows; fy += 1) {
        for(uint32_t fx = 0; fx < copyimg.cols; fx += 8) {
            bool status = mask.test(fx, fy);
            for(int i = 8; i--;) {
                fprintf(fp_mask_excel, "%d\t", (int)status);
            }
            if((!status) && !(fy & 0x7)) {
                fprintf(fp_mask_file, "x:%d, y:%d\n", fx, fy); 
            }
        }
        fprintf(fp_mask_excel, "\n");
    }
    fclose(fp_mask_excel);
    fclose(fp_mask_file);
    
    mask.compress();
    if (trkcbbuffassignment == 0 || trkcbbuffassignment == 1){
        dump_mem2d((instr_dir + "__Det2_Mask.txt").c_str(), mask.compressed,
                   mask.compressed_width, mask.scaled_height, mask.compressed_width);
    } else {
        dump_mem2d((instr_dir + "__Det1_Mask.txt").c_str(), mask.compressed,
                   mask.compressed_width, mask.scaled_height, mask.compressed_width);  
    }
#endif

    
#if MASK_EN
    fast.detect(copyimg_data, &mask, num_points, threshold, 0, 0, copyimg.cols, copyimg.rows);
#else
    fast.detect(copyimg_data, NULL, num_points, threshold, 0, 0, copyimg.cols, copyimg.rows);
#endif



    vector<xy> v = fast.features;
    if (trkcbbuffassignment == 0 || trkcbbuffassignment == 1){
        det2_points = v.size();
    } else {
        det1_points = v.size(); 
    }
    printf("Num Key Points: %d\n", v.size());
#if SORT_INS
    fclose(g_fp_sort_patch);
#endif

    print_config(CONFIG, width, height, threshold, tthreshold, num_points, v.size(), v.size(), 1, 1, num_points, num_points, 8, 8, 8, 8, 8, 8, 0, 20, 50, 1, 1, 2);

    fclose(CONFIG);

    g_det_count = g_det_count + 1;
    
#if TRACKER_INS
    float img_cols = copyimg.cols;
    float img_rows  = copyimg.rows;
    float x1 = new_width/img_cols;
    float y1 = new_height/img_rows;
    
    Mat copyimg_d1(copyimg.clone());
    
    cv::resize(copyimg, copyimg_d1, Size(), x1, y1, INTER_LINEAR); //resize(const Mat& src, Mat& dst, Size dsize, double fx=0, double fy=0, int interpolation=INTER_LINEAR)

    imwrite( "copyimg.jpg", copyimg);

    Mat finalimg = copyimg_d1;

    finalimg_data =  (unsigned char*)copyimg.data;

    imwrite( "finalimg.jpg", finalimg);
   
    vector<xy_hw> pred_list; vector<Patch_t> patch_list; 
    for(int fid = 0; fid < num_points; fid++){
        uint32_t idx = patch_list.size();
        patch_list.resize(idx + 1);
        uint8_t *buf_sw = patch_list[fid].buf;
        xy_hw pred;
        pred.LastPredInGroup = false;

        
#if 0
                pred.ImageId         = (rand() & 1);
#else
                if (trkcbbuffassignment == 0 || trkcbbuffassignment == 1){
                    pred.ImageId         = 0;
                } else {
                    pred.ImageId         = 1;
                }
#endif
   

                float curr_x = v[fid].x * x1;
                pred.x = (int)(curr_x + 0.5f);
                
                float curr_y = v[fid].y * y1;
                pred.y = (int)(curr_y + 0.5f);

                if(pred.x < 8 || pred.x > (copyimg.cols - 9)
                   || pred.y < 8 || pred.y > (copyimg.rows - 9)) {
                    printf("I: Droped, x:%03d, y:%03d\n", pred.x, pred.y);
                    continue;
                }
                pred_list.push_back(pred);
                get_patch(copyimg_data, v[fid].x, v[fid].y, width, buf_sw);
                pred_list.back().LastPredInGroup = true;
                patch_list[idx].xy = pred;
    }
    
    std::sort(pred_list.begin(), pred_list.end(), predy_cmp<xy_hw>);
    std::sort(patch_list.begin(), patch_list.end(), patchy_cmp<Patch_t>);
    if (trkcbbuffassignment == 0 || trkcbbuffassignment == 1){
        dump_mem2d((instr_dir + "__Image1.txt").c_str(), finalimg_data, finalimg.cols, finalimg.rows, finalimg.step);
    } else {
        dump_mem2d((instr_dir + "__Image2.txt").c_str(), finalimg_data, finalimg.cols, finalimg.rows, finalimg.step);
    }
    const int num_batch = 4;
    int lid = 0; int patch_idx = 0;
    for(lid; lid < pred_list.size(); lid += num_batch) {
        int bid;
        xy_hw my_pred[num_batch];
        xy_hw result[num_batch];
        int patch_length = 0 ;
        for(bid = 0; bid < num_batch; bid++) {
            int fid  = (lid + bid);
            if(pred_list.size() <= fid) {
                continue;
            }
            xy_hw &pred    = pred_list[fid];
            my_pred[bid] = pred; 
            xy o = fast.track(patch_list[patch_idx].buf, finalimg_data, half_patch_width, half_patch_width, pred.x, pred.y,
                              fast_track_radius, tthreshold, fast_min_match);
            result[bid].x = o.x;
            result[bid].y = o.y;
            result[bid].score = (o.score * 1024);
            if(pred.LastPredInGroup == true) {
                dump_mem_fp(g_fp_patch, patch_list[patch_idx].patch, (full_patch_width * full_patch_width));
                patch_idx += 1;
            }
        }
        dump_mem_fp(g_fp_list, (uint8_t*)my_pred, bid * sizeof(xy_hw));
        dump_mem_fp(g_fp_result,(uint8_t*)result, bid * sizeof(xy_hw)); 
    }
 

#endif

#if 1
    if (trkcbbuffassignment == 0 || trkcbbuffassignment == 1){
#if DETECTOR_INS
    g_fp_dist_1 = fopen((instr_dir + "__Det1_FPList.txt").c_str(), "w+"); assert(g_fp_dist_1);
    CONFIG = fopen((instr_dir + "__Config.txt").c_str(), "w+"); assert(CONFIG);
#endif

#if SORT_INS
    g_fp_sort_1 = fopen((instr_dir + "__Sorts1.txt").c_str(), "w+"); assert(g_fp_sort_1);
    g_fp_sort_hex_1 = fopen((instr_dir + "__Det1_SortedFPList.txt").c_str(), "w+"); assert(g_fp_sort_hex_1);
    g_fp_sort_patch_1 = fopen((instr_dir + "__Det1_SortedFPPatchList.txt").c_str(), "w+"); assert(g_fp_sort_patch_1);
#endif 
    } else {
#if DETECTOR_INS
    g_fp_dist_1 = fopen((instr_dir + "__Det2_FPList.txt").c_str(), "w+"); assert(g_fp_dist_1);
    CONFIG = fopen((instr_dir + "__Config.txt").c_str(), "w+"); assert(CONFIG);
#endif

#if SORT_INS
    g_fp_sort_1 = fopen((instr_dir + "__Sorts2.txt").c_str(), "w+"); assert(g_fp_sort_1);
    g_fp_sort_hex_1 = fopen((instr_dir + "__Det2_SortedFPList.txt").c_str(), "w+"); assert(g_fp_sort_hex_1);
    g_fp_sort_patch_1 = fopen((instr_dir + "__Det2_SortedFPPatchList.txt").c_str(), "w+"); assert(g_fp_sort_patch_1);
#endif    

    }
#if MASK_EN 
// Read mask points binary file
    if (trkcbbuffassignment == 0 || trkcbbuffassignment == 1){
        fp_mask_file_1 = fopen((instr_dir + "__Masked1_Coordinates.txt").c_str(), "w+");
    } else {
        fp_mask_file_1 = fopen((instr_dir + "__Masked2_Coordinates.txt").c_str(), "w+"); 
    }
    assert(fp_mask_file_1);
    scaled_mask mask_1(finalimg.cols, finalimg.rows);
    mask_1.initialize();
    if(mask_en) {
        time_t t;
        srand((unsigned) time (&t));
        //srand(1);//srand(time(NULL)); TODO to introduce true random
        for(uint32_t fy = 0; fy < finalimg.rows; fy += 8) {
            for(uint32_t fx = 0; fx < finalimg.cols; fx += 8) {
                int frac = (rand() % 100);
                if(frac < 10) {
                    mask_1.clear(fx, fy);
                }
            }
        }
    }
    if (trkcbbuffassignment == 0 || trkcbbuffassignment == 1){
        fp_mask_excel_1 = fopen((instr_dir + "__Masked1_Bits.txt").c_str(), "w+");
    } else {
        fp_mask_excel_1 = fopen((instr_dir + "__Masked2_Bits.txt").c_str(), "w+");
    }
    assert(fp_mask_excel_1);
    for(uint32_t fy = 0; fy < finalimg.rows; fy += 1) {
        for(uint32_t fx = 0; fx < finalimg.cols; fx += 8) {
            bool status = mask_1.test(fx, fy);
            for(int i = 8; i--;) {
                fprintf(fp_mask_excel_1, "%d\t", (int)status);
            }
            if((!status) && !(fy & 0x7)) {
                fprintf(fp_mask_file_1, "x:%d, y:%d\n", fx, fy); 
            }
        }
        fprintf(fp_mask_excel_1, "\n");
    }
    fclose(fp_mask_excel_1);
    fclose(fp_mask_file_1);
    
    mask_1.compress();
    if (trkcbbuffassignment == 0 || trkcbbuffassignment == 1){
        dump_mem2d((instr_dir + "__Det1_Mask.txt").c_str(), mask.compressed,
                   mask.compressed_width, mask.scaled_height, mask.compressed_width);
    } else {
        dump_mem2d((instr_dir + "__Det2_Mask.txt").c_str(), mask.compressed,
                   mask.compressed_width, mask.scaled_height, mask.compressed_width);
    }

#else
    MASK1 = fopen((instr_dir + "__Det1_Mask.txt").c_str(),"w+");
    MASK2 = fopen((instr_dir + "__Det2_Mask.txt").c_str(),"w+");
#endif
    
    fprintf(SEED, "SEED:%d", seed);
    fclose(SEED);
    
#if MASK_EN
    fast.detect(finalimg_data, &mask_1, num_points, threshold, 0, 0, finalimg.cols, finalimg.rows);
#else
    fast.detect(finalimg_data, NULL, num_points, threshold, 0, 0, finalimg.cols, finalimg.rows);
#endif



    vector<xy> x = fast.features;

    if (trkcbbuffassignment == 0 || trkcbbuffassignment == 1){
        det1_points = x.size();
    } else {
        det2_points = x.size();
    }
    printf("Num Key Points: %d\n", x.size());
#if SORT_INS
    fclose(g_fp_sort_patch_1);
#endif

    int rateArray[3] = {10, 15, 20};
    int randomArrayind = rand() % 3;
    int mipirate = rateArray[randomArrayind];

    int detworksetmax = 256;
    int detworksetmin = 32;
    int detworkset = ((rand() % (detworksetmax - detworksetmin)) + detworksetmin);

    print_config(CONFIG, new_width, new_height, threshold, tthreshold, num_points, det1_points, det2_points, 1, 1, pred_list.size(), pred_list.size(), 8, 8, 8, 8, 8, 8, 0, mipirate, detworkset, trkcbbuffassignment, cbdet1buffassignment, cbdet2buffassignment);

    fclose(CONFIG);
#endif 
    
    return 0;
}
 
void MyLine(Mat img, Point start, Point end )
{
    int thickness = 2;
    int lineType = 8;
    line( img,
          start,
          end,
          Scalar( 0, 0, 0 ),
          thickness,
          lineType );
}

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cstdio>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
//#include <opencv2/mat.hpp>
#include "../ref_model/fast.h"
#include "../hw/fast_hw.h"
#include "../ref_model/fast_constants.h"
#include "../stimuli_gen/utils.h"

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


#if TRACKER_INS
FILE *g_fp_patch = NULL;
FILE *g_fp_list = NULL;
FILE *g_fp_result = NULL;
#endif

#if DETECTOR_INS
FILE *g_fp_dist = NULL;
FILE *g_fp_dist_1 = NULL;
int g_det_count = 2;
#endif


#if SORT_INS
FILE *g_fp_sort = NULL;
FILE *g_fp_sort_hex = NULL;
FILE *g_fp_sort_patch = NULL;

FILE *g_fp_sort_1 = NULL;
FILE *g_fp_sort_hex_1 = NULL;
FILE *g_fp_sort_patch_1 = NULL;
#endif

//#define ASSERT(cond, str)  if(!(cond)) {std::cout << str << std::endl; abort(0);}
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

#if 0
struct Fast_Score {
    union {
        struct {
            uint32_t y : 11;
            uint32_t x : 11;
            uint32_t score : 10;
        };
        struct {
            uint32_t resvd : 31;
            uint32_t first : 1;
        };
    };
};
#endif

#if 0
struct feature
{
    uint8_t patch[full_patch_width*full_patch_width];
    uint32_t num_pred;
    /*
      struct Point {
        float x, y;
        };*/
    Fast_Score pred[3];
    //float dx, dy;
};
#endif


int main (int argc, char *argv[])
{
    if(argc <= 7) {
        printf("Usage: <exe> <tracker dir> <num features> <threshold> <width> <height> <track_threshold> <mask_enable{0:1}> [<dump dir>]\n");
        exit(1);
    }
    string dir = argv[1];
    string instr_dir = argc > 8 ? argv[8] : ".";
    string img_file  = dir + "/image.bmp";
    string conf_file = dir + "/conf.txt";
    string mask_file = dir + "/mask_file.bin";
    string module_file = dir + "/module.txt";
    string MAT_size = dir + "/MAT_size.txt";
    string ch_in_file_bin = dir + "/CHL_In_file.bin";
    string ch_out_file_bin = dir + "/CHL_Out_file.bin";
    string ch_inv_out_file_bin = dir + "/CHL_Out_Inv_file.bin";
    string LCt_inn_in_file_bin = dir + "/LCt_inn_in_file_top.bin";
    string LCt_inn_out_file_bin = dir + "/LCt_inn_out_file_top.bin";
  	string LCt_inn_out_2_file_bin = dir + "/LCt_inn_out_2_file_top.bin";
    string Kt_out_file_bin = dir + "/Kt_out_file_top.bin";
    string cov_state_file_bin = dir + "/cov_state_file_top.bin";
    string cov_state_out_file_bin = dir + "/cov_out_file_top.bin";
        
    int num_points = atoi(argv[2]);
    int threshold  = atoi(argv[3]);
    int mask_en    = atoi(argv[7]);
    int num_feat;
    int m;
    int s;
    int stride_1;
    int stride_2;
    
    printf ("number_points:%d\n", num_points);

#undef READ_FILE
    // Read image
    cv::Mat img = cv::imread(img_file.c_str(), CV_LOAD_IMAGE_GRAYSCALE);
    //assert(img.data); 
    //assert(img.elemSize() == 1);
    unsigned char *img_data = (unsigned char*)img.data;

    
#if INSTRUMENT // Stimulus for RTL
    system((string("mkdir -p ") + instr_dir).c_str());
    img.cols = atoi(argv[4]); // 640 or 480 or 320 or 240
    img.rows = atoi(argv[5]); // 480 or 320 or 240 or 160
    int fast_track_threshold = atoi(argv[6]);//track_threshold
    dump_mem2d((instr_dir + "__Image1.txt").c_str(), img_data, img.cols, img.rows, img.step);
    dump_mem2d((instr_dir + "__Image2.txt").c_str(), img_data, img.cols, img.rows, img.step);
    fast_detector_9 fast;
    fast.init(img.cols, img.rows, img.step, full_patch_width, half_patch_width);

#if MODULE_EN
#define READ_FILE_1(n, str, ...) fscanf(module_file_1, str, __VA_ARGS__) 
// Read module enable file
    FILE *module_file_1 = fopen(module_file.c_str(), "r");
    char mod1[80];
    char mod2[80];
	char mod3[80];
    char mod_1[80] = "Tracker";
    char mod_2[80] = "Detector";
    char mod_3[80] = "NoImage";
    if (module_file_1){   
    assert(module_file_1);
    READ_FILE_1(1, "%s", &mod1);
    READ_FILE_1(1, "%s", &mod2);
	}
    else{
        char mod3[80] = "NoImage";
    }    
#endif
   
#define READ_FILE_2(n, str, ...) fscanf(MAT_size_1, str, __VA_ARGS__) 
    // Read matrix size file
    FILE *MAT_size_1 = fopen(MAT_size.c_str(), "r");
    assert(MAT_size_1);
    READ_FILE_2(4, "m=%d, s=%d, st_1=%d, st_2=%d", &m, &s, &stride_1, &stride_2);
    printf("m=%d, s=%d, stride1=%d, stride2=%d\n", m, s, stride_1, stride_2);

    
    vector<xy_hw> pred_list; vector<Patch_t> patch_list;    
    if ((strcmp (mod1, mod_1) == 0 && strcmp (mod2, mod_2) == 0)||(strcmp (mod1, mod_1) == 0))
    {
#define READ_FILE(n, str, ...)  assert(n == fscanf(fp, str, __VA_ARGS__)) 
        // Read feature points and prediction
        FILE *fp = fopen(conf_file.c_str(), "r"); assert(fp);
        READ_FILE(1, "Num Prediction:%d", &num_feat);
        for(int fid = 0; fid < num_feat; fid++) {
            string patch_file = dir + "/patch_" + to_string(fid) + ".bmp";
            cv::Mat patch_img = cv::imread(patch_file.c_str(), CV_LOAD_IMAGE_GRAYSCALE);
            assert(patch_img.elemSize() == 1);
            assert(patch_img.cols == full_patch_width);
            assert(patch_img.rows == full_patch_width);
            assert(patch_img.step == full_patch_width);
            //feature &f        = feature_map[fid];
            uint32_t num_pred = 0;
            for(int i=0; i < 3; i++) {
                float x, y;
                READ_FILE(2, "%f %f", &x, &y);
                
                xy_hw pred;
                pred.LastPredInGroup = false; //(i != 0);
#if IMAGE_ID_RANDOM
                pred.ImageId         = (rand() & 1);
#else                
                pred.ImageId         = 0;
#endif
                pred.x = (x + 0.5);
                pred.y = (y + 0.5);
                if(pred.x < 8 || pred.x > (img.cols - 9)
                   || pred.y < 8 || pred.y > (img.rows - 9)) {
                    printf("I: Droped, x:%03d, y:%03d\n", pred.x, pred.y);
                    continue;
                }
                pred_list.push_back(pred);
                num_pred++;

                uint32_t idx = patch_list.size();
                patch_list.resize(idx + 1); 
                //printf("idx:%d\n", idx);
                memcpy(patch_list[idx].patch, patch_img.data, sizeof(Patch_t));
                pred_list.back().LastPredInGroup = true;
                patch_list[idx].xy = pred;
            }
       }
    }

	std::sort(pred_list.begin(), pred_list.end(), predy_cmp<xy_hw>);
    std::sort(patch_list.begin(), patch_list.end(), patchy_cmp<Patch_t>);

#if DETECTOR_INS
    g_fp_dist = fopen((instr_dir + "__Det1_FPList.txt").c_str(), "w+"); assert(g_fp_dist);
    g_fp_dist_1 = fopen((instr_dir + "__Det2_FPList.txt").c_str(), "w+"); assert(g_fp_dist_1);
#endif

#if SORT_INS
    g_fp_sort = fopen((instr_dir + "__Sorts_1.txt").c_str(), "w+"); assert(g_fp_sort);
    g_fp_sort_hex = fopen((instr_dir + "__Det1_SortedFPList.txt").c_str(), "w+"); assert(g_fp_sort_hex);
    g_fp_sort_patch = fopen((instr_dir + "__Det1_SortedFPPatchList.txt").c_str(), "w+"); assert(g_fp_sort_patch);


    g_fp_sort_1 = fopen((instr_dir + "__Sorts_2.txt").c_str(), "w+"); assert(g_fp_sort_1);
    g_fp_sort_hex_1 = fopen((instr_dir + "__Det2_SortedFPList.txt").c_str(), "w+"); assert(g_fp_sort_hex_1);
    g_fp_sort_patch_1 = fopen((instr_dir + "__Det2_SortedFPPatchList.txt").c_str(), "w+"); assert(g_fp_sort_patch_1);
#endif 

#if TRACKER_INS
    g_fp_patch = fopen((instr_dir + "__Trk_FPPatchList.txt").c_str(), "w+"); assert(g_fp_patch);
#endif
    
    g_fp_list = fopen((instr_dir + "__Trk_FPPredList.txt").c_str(), "w+"); assert(g_fp_list);
    g_fp_result = fopen((instr_dir + "__Trk_FPTrackList.txt").c_str(), "w+");
    FILE *fp_config = fopen((instr_dir + "__Config.txt").c_str(), "w+"); assert(fp_config);
    FILE *fp_config_256 = fopen((instr_dir + "__Config_256.txt").c_str(), "w+"); assert(fp_config_256);
    FILE *fp_config_matsol_0 = fopen((instr_dir + "__Config_matsol_0.txt").c_str(), "w+"); assert(fp_config_matsol_0);//run both stages
    FILE *fp_config_matsol_1 = fopen((instr_dir + "__Config_matsol_1.txt").c_str(), "w+"); assert(fp_config_matsol_1);//run only stage 2
  

#if MODULE_EN    
    if (strcmp (mod1, mod_1) == 0 && strcmp (mod2, mod_2) == 0)
    {
        fprintf(fp_config, "0000007F //ModuleEn = 01111111(MS, MC, CD, T, D, S, P)\n");
        fprintf(fp_config_256, "0000007F //ModuleEn = 01111111(MS, MC, CD, T, D, S, P)\n");
        fprintf(fp_config_matsol_0, "0000007F //ModuleEn = 01111111(MS, MC, CD, T, D, S, P)\n");
        fprintf(fp_config_matsol_1, "0000007F //ModuleEn = 01111111(MS, MC, CD, T, D, S, P)\n");
    }
    else if (strcmp (mod1, mod_1) == 0)
    {   
        fprintf(fp_config, "00000078 //ModuleEn = 01111000(MS, MC, CD, T, D, S, P)\n");
        fprintf(fp_config_256, "00000078 //ModuleEn = 01111000(MS, MC, CD, T, D, S, P)\n");
        fprintf(fp_config_matsol_0, "00000078 //ModuleEn = 01111000(MS, MC, CD, T, D, S, P)\n");
        fprintf(fp_config_matsol_1, "00000078 //ModuleEn = 01111000(MS, MC, CD, T, D, S, P)\n");
    }
    else if (strcmp (mod1, mod_1) != 0 && strcmp (mod3, mod_3) != 0)
    {        
        fprintf(fp_config, "00000077 //ModuleEn = 01110111(MS, MC, CD, T, D, S, P)\n");
        fprintf(fp_config_256, "00000077 //ModuleEn = 01110111(MS, MC, CD, T, D, S, P)\n"); 
        fprintf(fp_config_matsol_0, "00000077 //ModuleEn = 01110111(MS, MC, CD, T, D, S, P)\n");
        fprintf(fp_config_matsol_1, "00000077 //ModuleEn = 01110111(MS, MC, CD, T, D, S, P)\n"); 
    }
	else if (strcmp (mod3, mod_3) == 0)
    {        
        fprintf(fp_config, "00000070 //ModuleEn = 01110000(MS, MC, CD, T, D, S, P)\n");
        fprintf(fp_config_256, "00000070 //ModuleEn = 01110000(MS, MC, CD, T, D, S, P)\n"); 
        fprintf(fp_config_matsol_0, "00000070 //ModuleEn = 01110000(MS, MC, CD, T, D, S, P)\n");
        fprintf(fp_config_matsol_1, "00000070 //ModuleEn = 01110000(MS, MC, CD, T, D, S, P)\n"); 
    }
    
#endif

    fprintf(fp_config, "%08X //CholDecompMatSize: %d\n", m, m);
    fprintf(fp_config, "%08X //CholDecompMatStride: %d\n", stride_1/4, stride_1/4);
    fprintf(fp_config, "BF800000 //MatMulCov Alpha\n");
    fprintf(fp_config, "3F800000 //MatMulCov Beta\n");
    fprintf(fp_config_256, "%08X //CholDecompMatSize: %d\n", m, m);
    fprintf(fp_config_256, "%08X //CholDecompMatStride: %d\n", stride_1/8, stride_1/8);
    fprintf(fp_config_256, "BF800000 //MatMulCov Alpha\n");
    fprintf(fp_config_256, "3F800000 //MatMulCov Beta\n"); 

    fprintf(fp_config_matsol_0, "%08X //CholDecompMatSize: %d\n", m, m);
    fprintf(fp_config_matsol_0, "%08X //CholDecompMatStride: %d\n", stride_1/4, stride_1/4);
    fprintf(fp_config_matsol_0, "BF800000 //MatMulCov Alpha\n");
    fprintf(fp_config_matsol_0, "3F800000 //MatMulCov Beta\n");
    fprintf(fp_config_matsol_1, "%08X //CholDecompMatSize: %d\n", m, m);
    fprintf(fp_config_matsol_1, "%08X //CholDecompMatStride: %d\n", stride_1/4, stride_1/4);
    fprintf(fp_config_matsol_1, "BF800000 //MatMulCov Alpha\n");
    fprintf(fp_config_matsol_1, "3F800000 //MatMulCov Beta\n"); 




   
    
    
#if MASK_EN 
// Read mask points binary file
    FILE *fp_mask_file = fopen((instr_dir + "__Masked_Coordinates.txt").c_str(), "w+");
    assert(fp_mask_file);
    scaled_mask mask(img.cols, img.rows);
    if (strcmp (mod2, mod_2) == 0) {
        FILE *mask_file_1 = fopen(mask_file.c_str(), "rb"); assert(mask_file_1);
        int bytes_read = fread (mask.mask, 1, mask.scaled_width*mask.scaled_height, mask_file_1);
        assert (bytes_read == mask.scaled_width*mask.scaled_height);
        printf("elements=%d\n", bytes_read);
        printf("scaled_width=%d, scaled_height=%d\n", mask.scaled_width, mask.scaled_height);
        fclose(mask_file_1);
    }
    else {
        mask.initialize();
        if(mask_en) {
            srand(1);//srand(time(NULL)); TODO to introduce true random
            for(uint32_t fy = 0; fy < img.rows; fy += 8) {
                for(uint32_t fx = 0; fx < img.cols; fx += 8) {
                    int frac = (rand() % 100);
                    if(frac < 10) {
                        mask.clear(fx, fy);
                    }
                }
            }
        }
    }
    FILE *fp_mask_excel = fopen((instr_dir + "__Masked_Bits.txt").c_str(), "w+");
    assert(fp_mask_excel);
    for(uint32_t fy = 0; fy < img.rows; fy += 1) {
        for(uint32_t fx = 0; fx < img.cols; fx += 8) {
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
    dump_mem2d((instr_dir + "__Det1_Mask.txt").c_str(), mask.compressed,
               mask.compressed_width, mask.scaled_height, mask.compressed_width);
    dump_mem2d((instr_dir + "__Det2_Mask.txt").c_str(), mask.compressed,
               mask.compressed_width, mask.scaled_height, mask.compressed_width);



    fprintf(fp_config, "%08X //MatMulCov Matrix A rows: %d\n", s+1, s+1);
    fprintf(fp_config, "%08X //MatMulCov Matrix A cols: %d\n", m, m);
    fprintf(fp_config, "%08X //MatMulCov Matrix A stride: %d\n", stride_1/4, stride_1/4);
    fprintf(fp_config_256, "%08X //MatMulCov Matrix A rows: %d\n", s+1, s+1);
    fprintf(fp_config_256, "%08X //MatMulCov Matrix A cols: %d\n", m, m);
    fprintf(fp_config_256, "%08X //MatMulCov Matrix A stride: %d\n", stride_1/8, stride_1/8);
    fprintf(fp_config_matsol_0, "%08X //MatMulCov Matrix A rows: %d\n", s+1, s+1);
    fprintf(fp_config_matsol_0, "%08X //MatMulCov Matrix A cols: %d\n", m, m);
    fprintf(fp_config_matsol_0, "%08X //MatMulCov Matrix A stride: %d\n", stride_1/4, stride_1/4);
    fprintf(fp_config_matsol_1, "%08X //MatMulCov Matrix A rows: %d\n", s+1, s+1);
    fprintf(fp_config_matsol_1, "%08X //MatMulCov Matrix A cols: %d\n", m, m);
    fprintf(fp_config_matsol_1, "%08X //MatMulCov Matrix A stride: %d\n", stride_1/4, stride_1/4);
    
    fprintf(fp_config, "%08X //MatMulCov Matrix B rows: %d\n", m, m);
    fprintf(fp_config, "%08X //MatMulCov Matrix B cols: %d\n", s, s);
    fprintf(fp_config, "%08X //MatMulCov Matrix B stride: %d\n", stride_1/4, stride_1/4);    
    fprintf(fp_config_256, "%08X //MatMulCov Matrix B rows: %d\n", m, m);
    fprintf(fp_config_256, "%08X //MatMulCov Matrix B cols: %d\n", s, s);
    fprintf(fp_config_256, "%08X //MatMulCov Matrix B stride: %d\n", stride_1/8, stride_1/8);
    fprintf(fp_config_matsol_0, "%08X //MatMulCov Matrix B rows: %d\n", m, m);
    fprintf(fp_config_matsol_0, "%08X //MatMulCov Matrix B cols: %d\n", s, s);
    fprintf(fp_config_matsol_0, "%08X //MatMulCov Matrix B stride: %d\n", stride_1/4, stride_1/4);    
    fprintf(fp_config_matsol_1, "%08X //MatMulCov Matrix B rows: %d\n", m, m);
    fprintf(fp_config_matsol_1, "%08X //MatMulCov Matrix B cols: %d\n", s, s);
    fprintf(fp_config_matsol_1, "%08X //MatMulCov Matrix B stride: %d\n", stride_1/4, stride_1/4);

    fprintf(fp_config, "%08X //MatMulCov Matrix C rows: %d\n", s+1, s+1);
    fprintf(fp_config, "%08X //MatMulCov Matrix C cols: %d\n", s, s);
    fprintf(fp_config, "%08X //MatMulCov Matrix C stride: %d\n", stride_2/4, stride_2/4);   
    fprintf(fp_config_256, "%08X //MatMulCov Matrix C rows: %d\n", s+1, s+1);
    fprintf(fp_config_256, "%08X //MatMulCov Matrix C cols: %d\n", s, s);
    fprintf(fp_config_256, "%08X //MatMulCov Matrix C stride: %d\n", stride_2/8, stride_2/8); 

    fprintf(fp_config_matsol_0, "%08X //MatMulCov Matrix C rows: %d\n", s+1, s+1);
    fprintf(fp_config_matsol_0, "%08X //MatMulCov Matrix C cols: %d\n", s, s);
    fprintf(fp_config_matsol_0, "%08X //MatMulCov Matrix C stride: %d\n", stride_2/4, stride_2/4);   
    fprintf(fp_config_matsol_1, "%08X //MatMulCov Matrix C rows: %d\n", s+1, s+1);
    fprintf(fp_config_matsol_1, "%08X //MatMulCov Matrix C cols: %d\n", s, s);
    fprintf(fp_config_matsol_1, "%08X //MatMulCov Matrix C stride: %d\n", stride_2/4, stride_2/4); 


    fprintf(fp_config, "00000002 //MatSolve skip-stage (0-skip none, 1-skip stage-1, 2-skip stage-2)\n");
    fprintf(fp_config_matsol_0, "00000000 //MatSolve skip-stage (0-skip none, 1-skip stage-1, 2-skip stage-2)\n");
    fprintf(fp_config_matsol_1, "00000001 //MatSolve skip-stage (0-skip none, 1-skip stage-1, 2-skip stage-2)\n");   
    fprintf(fp_config_256, "00000002 //MatSolve skip-stage (0-skip none, 1-skip stage-1, 2-skip stage-2)\n");

    fprintf(fp_config, "%08X //Image1Width:%d\n", img.cols, img.cols);
    fprintf(fp_config, "%08X //Image1Height:%d\n", img.rows, img.rows);
    fprintf(fp_config, "%08X //Image2Width:%d\n", img.cols, img.cols);
    fprintf(fp_config, "%08X //Image2Height:%d\n", img.rows, img.rows);
    fprintf(fp_config_256, "%08X //Image1Width:%d\n", img.cols, img.cols);
    fprintf(fp_config_256, "%08X //Image1Height:%d\n", img.rows, img.rows);
    fprintf(fp_config_256, "%08X //Image2Width:%d\n", img.cols, img.cols);
    fprintf(fp_config_256, "%08X //Image2Height:%d\n", img.rows, img.rows);
    fprintf(fp_config_matsol_0, "%08X //Image1Width:%d\n", img.cols, img.cols);
    fprintf(fp_config_matsol_0, "%08X //Image1Height:%d\n", img.rows, img.rows);
    fprintf(fp_config_matsol_0, "%08X //Image2Width:%d\n", img.cols, img.cols);
    fprintf(fp_config_matsol_0, "%08X //Image2Height:%d\n", img.rows, img.rows);
    fprintf(fp_config_matsol_1, "%08X //Image1Width:%d\n", img.cols, img.cols);
    fprintf(fp_config_matsol_1, "%08X //Image1Height:%d\n", img.rows, img.rows);
    fprintf(fp_config_matsol_1, "%08X //Image2Width:%d\n", img.cols, img.cols);
    fprintf(fp_config_matsol_1, "%08X //Image2Height:%d\n", img.rows, img.rows);

    int rateArray[3] = {10, 15, 20};
    int randomArrayind = rand() % 3;
    int mipirate = rateArray[randomArrayind];

    int detworksetmax = 256;
    int detworksetmin = 32;
    int detworkset = ((rand() % (detworksetmax - detworksetmin)) + detworksetmin);
    

    fprintf(fp_config, "00000000 //CircularBuffer1Depth: 0\n");
    fprintf(fp_config, "00000000 //CircularBuffer2Depth: 0\n");
    fprintf(fp_config_256, "00000000 //CircularBuffer1Depth: 0\n");
    fprintf(fp_config_256, "00000000 //CircularBuffer2Depth: 0\n");
    fprintf(fp_config_matsol_0, "00000000 //CircularBuffer1Depth: 0\n");
    fprintf(fp_config_matsol_0, "00000000 //CircularBuffer2Depth: 0\n");
    fprintf(fp_config_matsol_1, "00000000 //CircularBuffer1Depth: 0\n");
    fprintf(fp_config_matsol_1, "00000000 //CircularBuffer2Depth: 0\n");


    

    fprintf(fp_config,"%08X //MIPI1Rate: %d\n", mipirate, mipirate);
    fprintf(fp_config, "%08X //MIPI2Rate: %d\n", mipirate, mipirate);
    fprintf(fp_config_256, "%08X //MIPI1Rate: %d\n", mipirate, mipirate);
    fprintf(fp_config_256, "%08X //MIPI2Rate: %d\n", mipirate, mipirate);
    fprintf(fp_config_matsol_0, "%08X //MIPI1Rate: %d\n", mipirate, mipirate);
    fprintf(fp_config_matsol_0, "%08X //MIPI2Rate: %d\n", mipirate, mipirate);
    fprintf(fp_config_matsol_1, "%08X //MIPI1Rate: %d\n", mipirate, mipirate);
    fprintf(fp_config_matsol_1, "%08X //MIPI2Rate: %d\n", mipirate, mipirate);
    
    
    
#if CW_EN
	if (strcmp (mod3, mod_3) != 0){
    fprintf(fp_config, "00000001 //TrkCWTEn: 1\n");
    fprintf(fp_config_256, "00000001 //TrkCWTEn: 1\n");
    fprintf(fp_config_matsol_0, "00000001 //TrkCWTEn: 1\n");
    fprintf(fp_config_matsol_1, "00000001 //TrkCWTEn: 1\n");
	}
	else{
		fprintf(fp_config, "00000000 //TrkCWTdis\n");
    	fprintf(fp_config_256, "00000000 //CWTdis\n");
        fprintf(fp_config_matsol_0, "00000000 //TrkCWTdis\n");
    	fprintf(fp_config_matsol_1, "00000000 //CWTdis\n");
    }
#else
    fprintf(fp_config, "00000000 //TrkCWTdis\n");
    fprintf(fp_config_256, "00000000 //TrkCWTdis\n");
    fprintf(fp_config_matsol_0, "00000000 //TrkCWTdis\n");
    fprintf(fp_config_matsol_1, "00000000 //TrkCWTdis\n");
#endif
    // int buffassign[2] = {1,2};
    //int randomArrayind1 = rand() % 2;
    //int cbbuffassignmenta = buffassign[randomArrayind1];
    int cbbuffassignmenta = 1;
    int cbbuffassignmentb = 2;
    printf("cbassigna: %d, cbassignb: %d\n", cbbuffassignmenta, cbbuffassignmentb);
    
    /* if (cbbuffassignmenta == 1){
        cbbuffassignmentb = 2; 
    } else {
        cbbuffassignmentb = 1;
        }*/
    
    if ((strcmp (mod1, mod_1) == 0 && strcmp (mod2, mod_2) == 0)||(strcmp (mod1, mod_1) == 0)){
        fprintf(fp_config, "%08X //TrkThreshold:%d\n", (uint32_t)fast_track_threshold, (uint32_t)fast_track_threshold);
        fprintf(fp_config, "%08X //TrkFPListLen:%d\n", pred_list.size(), pred_list.size());
        fprintf(fp_config, "%08X //TrkFPPatchListLen:%d\n", patch_list.size(), patch_list.size());
        fprintf(fp_config, "00000001 //TrkMode: 1\n");
#if 1
        fprintf(fp_config, "%08X //TrkCBAssignment: %d\n", cbbuffassignmenta, cbbuffassignmenta);
#else
        fprintf(fp_config, "00000003 //TrkCBAssignment: 3\n");
#endif 
        fprintf(fp_config_256, "%08X //TrkThreshold:%d\n", (uint32_t)fast_track_threshold, (uint32_t)fast_track_threshold);
        fprintf(fp_config_256, "%08X //TrkFPListLen:%d\n", pred_list.size(), pred_list.size());
        fprintf(fp_config_256, "%08X //TrkFPPatchListLen:%d\n", patch_list.size(), patch_list.size());
        fprintf(fp_config_256, "00000001 //TrkMode: 1\n");
#if 1
        fprintf(fp_config_256, "%08X //TrkCBAssignment: %d\n", cbbuffassignmenta, cbbuffassignmenta);
#else
        fprintf(fp_config_256, "00000003 //TrkCBAssignment: 3\n");
#endif        
        fprintf(fp_config_matsol_0, "%08X //TrkThreshold:%d\n", (uint32_t)fast_track_threshold, (uint32_t)fast_track_threshold);
        fprintf(fp_config_matsol_0, "%08X //TrkFPListLen:%d\n", pred_list.size(), pred_list.size());
        fprintf(fp_config_matsol_0, "%08X //TrkFPPatchListLen:%d\n", patch_list.size(), patch_list.size());
        fprintf(fp_config_matsol_0, "00000001 //TrkMode: 1\n");
#if 1
        fprintf(fp_config_matsol_0, "%08X //TrkCBAssignment: %d\n", cbbuffassignmenta, cbbuffassignmenta);
#else
        fprintf(fp_config_matsol_0, "00000003 //TrkCBAssignment: 3\n");
#endif       
        fprintf(fp_config_matsol_1, "%08X //TrkThreshold:%d\n", (uint32_t)fast_track_threshold, (uint32_t)fast_track_threshold);
        fprintf(fp_config_matsol_1, "%08X //TrkFPListLen:%d\n", pred_list.size(), pred_list.size());
        fprintf(fp_config_matsol_1, "%08X //TrkFPPatchListLen:%d\n", patch_list.size(), patch_list.size());
        fprintf(fp_config_matsol_1, "00000001 //TrkMode: 1\n");
#if 1
        fprintf(fp_config_matsol_1, "%08X //TrkCBAssignment: %d\n", cbbuffassignmenta, cbbuffassignmenta);
#else
        fprintf(fp_config_matsol_1, "00000003 //TrkCBAssignment: 3\n");
#endif  
    } else if (strcmp (mod3, mod_3) == 0){
        fprintf(fp_config, "00000000 //Tracker Disabled\n");
        fprintf(fp_config, "00000000 //Tracker Disabled\n");
		fprintf(fp_config, "00000000 //Tracker Disabled\n");
        fprintf(fp_config, "00000000 //Tracker Disabled\n");
		fprintf(fp_config, "00000000 //Tracker Disabled\n");
        
        fprintf(fp_config_256, "00000000 //Tracker Disabled\n");
        fprintf(fp_config_256, "00000000 //Tracker Disabled\n");
		fprintf(fp_config_256, "00000000 //Tracker Disabled\n");
        fprintf(fp_config_256, "00000000 //Tracker Disabled\n");
		fprintf(fp_config_256, "00000000 //Tracker Disabled\n");

        fprintf(fp_config_matsol_0, "00000000 //Tracker Disabled\n");
        fprintf(fp_config_matsol_0, "00000000 //Tracker Disabled\n");
		fprintf(fp_config_matsol_0, "00000000 //Tracker Disabled\n");
        fprintf(fp_config_matsol_0, "00000000 //Tracker Disabled\n");
		fprintf(fp_config_matsol_0, "00000000 //Tracker Disabled\n");
        
        fprintf(fp_config_matsol_1, "00000000 //Tracker Disabled\n");
        fprintf(fp_config_matsol_1, "00000000 //Tracker Disabled\n");
		fprintf(fp_config_matsol_1, "00000000 //Tracker Disabled\n");
        fprintf(fp_config_matsol_1, "00000000 //Tracker Disabled\n");
		fprintf(fp_config_matsol_1, "00000000 //Tracker Disabled\n");
    }
	else {
        fprintf(fp_config, "00000000 //Tracker Disabled\n");
        fprintf(fp_config, "00000000 //Tracker Disabled\n");
        fprintf(fp_config, "00000000 //Tracker Disabled\n");
        fprintf(fp_config, "00000000 //Tracker Disabled\n");
        fprintf(fp_config, "00000000 //Tracker Disabled\n");
        fprintf(fp_config_256, "00000000 //Tracker Disabled\n");
        fprintf(fp_config_256, "00000000 //Tracker Disabled\n");
        fprintf(fp_config_256, "00000000 //Tracker Disabled\n");
        fprintf(fp_config_256, "00000000 //Tracker Disabled\n");
        fprintf(fp_config_256, "00000000 //Tracker Disabled\n");
        fprintf(fp_config_matsol_0, "00000000 //Tracker Disabled\n");
        fprintf(fp_config_matsol_0, "00000000 //Tracker Disabled\n");
        fprintf(fp_config_matsol_0, "00000000 //Tracker Disabled\n");
        fprintf(fp_config_matsol_0, "00000000 //Tracker Disabled\n");
        fprintf(fp_config_matsol_0, "00000000 //Tracker Disabled\n");
        fprintf(fp_config_matsol_1, "00000000 //Tracker Disabled\n");
        fprintf(fp_config_matsol_1, "00000000 //Tracker Disabled\n");
        fprintf(fp_config_matsol_1, "00000000 //Tracker Disabled\n");
        fprintf(fp_config_matsol_1, "00000000 //Tracker Disabled\n");
        fprintf(fp_config_matsol_1, "00000000 //Tracker Disabled\n");
    }

    
#if CW_EN
	if (strcmp (mod3, mod_3) != 0){
    fprintf(fp_config, "00000001 //Det1CWTEn: 1\n");
    fprintf(fp_config_256, "00000001 //Det1CWTEn: 1\n");
    fprintf(fp_config_matsol_0, "00000001 //Det1CWTEn: 1\n");
    fprintf(fp_config_matsol_1, "00000001 //Det1CWTEn: 1\n");
	}
	else{
		fprintf(fp_config, "00000000 //Det1CWTDis\n");
    	fprintf(fp_config_256, "00000000 //Det1CWTDis\n");
        fprintf(fp_config_matsol_0, "00000000 //Det1CWTDis\n");
    	fprintf(fp_config_matsol_1, "00000000 //Det1CWTDis\n");
    }
#else
    fprintf(fp_config, "00000000 //Det1CWTdis\n");
    fprintf(fp_config_256, "00000000 //Det1CWTdis\n");
    fprintf(fp_config_matsol_0, "00000000 //Det1CWTdis\n");
    fprintf(fp_config_matsol_1, "00000000 //Det1CWTdis\n");
#endif

    if (strcmp (mod3, mod_3) != 0 ){
   			 fprintf(fp_config, "00000001 //Det1MaskEn\n");
 		     fprintf(fp_config_256, "00000001 //Det1MaskEn\n");
             fprintf(fp_config_matsol_0, "00000001 //Det1MaskEn\n");
 		     fprintf(fp_config_matsol_1, "00000001 //Det1MaskEn\n");
		}
		else{
            fprintf(fp_config, "00000000 //Det1MaskDis\n");
            fprintf(fp_config_256, "00000000 //Det1MaskDis\n");
            fprintf(fp_config_matsol_0, "00000000 //Det1MaskDis\n");
            fprintf(fp_config_matsol_1, "00000000 //Det1MaskDis\n");
        }
#else
  	    fprintf(fp_config, "00000000 //Det1MaskDis\n");
        fprintf(fp_config_256, "00000000 //Det1MaskDis\n");
        fprintf(fp_config_matsol_0, "00000000 //Det1MaskDis\n");
        fprintf(fp_config_matsol_1, "00000000 //Det1MaskDis\n");
#endif


        //if (strcmp (mod2, mod_2) == 0){
        fprintf(fp_config, "%08X //Det1Threshold:%d\n", threshold, threshold);
        fprintf(fp_config_256, "%08X //Det1Threshold:%d\n", threshold, threshold);
        fprintf(fp_config_matsol_0, "%08X //Det1Threshold:%d\n", threshold, threshold);
        fprintf(fp_config_matsol_1, "%08X //Det1Threshold:%d\n", threshold, threshold);
        /*} else {
    fprintf(fp_config, "%08X //Det1Threshold:%d\n", 0, 0);
    fprintf(fp_config_256, "%08X //Det1Threshold:%d\n", 0, 0);
    }*/
    
#endif

    //Cholesky input file 
    FILE *CHL_in_file_bin_1 = fopen(ch_in_file_bin.c_str(), "rb"); assert(CHL_in_file_bin_1);
    int chl_in;
    FILE *chl_in_bin_file = fopen((instr_dir + "__CHL_In_Matrix.txt").c_str(), "w+");
    assert(chl_in_bin_file);

    for (int i = 0; i < m; ++i)
    {
        for(int k=0; k < m; k += 4)
        {
            int x= (4 - (m%4));
            if(k/4 == m/4){
                int y;
                for (y=0;y<x; y++){
                    fprintf(chl_in_bin_file, "BAADF00D");
                }
            }
            int j = k+3;
            if (j > (m-1))
                j = m-1;
            
            for(; j >= k; j--) {
                fread (&chl_in, sizeof(int), 1, CHL_in_file_bin_1);
                //printf("cholesky_input = %d\n", chl_in);
                fprintf(chl_in_bin_file, "%08X", chl_in);
            }
            fprintf(chl_in_bin_file, "\n");
        }
        if ((m%8 != 0) && (m%8 <= 4)){  
            fprintf(chl_in_bin_file, "BAADF00DBAADF00DBAADF00DBAADF00D\n");
        }
    }
//cholesky output file       
    FILE *CHL_out_file_bin_1 = fopen(ch_out_file_bin.c_str(), "rb"); assert(CHL_out_file_bin_1);
    int chl_out;
    FILE *chl_out_bin_file = fopen((instr_dir + "__CHL_Out_Matrix.txt").c_str(), "w+");
    assert(chl_out_bin_file);

    for (int i = 0; i < m; ++i)
    {
        for(int k=0; k < m; k += 4)
        {
            int x= (4 - (m%4));
            if(k/4 == m/4){
                int y;
                for (y=0;y<x; y++){
                    fprintf(chl_out_bin_file, "BAADF00D");
                }
            }
            int j = k+3;
            if (j > (m-1))
                j = m-1;
            
            for(; j >= k; j--) {
                fread (&chl_out, sizeof(int), 1, CHL_out_file_bin_1);
                if (i>j){
                    fprintf(chl_out_bin_file, "DEADBEEF"); 
                }
                else{
                    //printf("cholesky_output = %d\n", chl_out);
                    fprintf(chl_out_bin_file, "%08X", chl_out);
                }      
            }
            fprintf(chl_out_bin_file, "\n"); 
        }
        if ((m%8 != 0) && (m%8 <= 4)){  
            fprintf(chl_out_bin_file, "BAADF00DBAADF00DBAADF00DBAADF00D\n");
        }
    }

//cholesky output inverse file & matrix solve input  
    FILE *CHL_inv_out_file_bin_1 = fopen(ch_inv_out_file_bin.c_str(), "rb"); assert(CHL_inv_out_file_bin_1);
    int chl_inv;
    FILE *chl_inv_out_bin_file = fopen((instr_dir + "__CHL_Inv_Out_Matrix.txt").c_str(), "w+");
    assert(chl_inv_out_bin_file);

    for (int i = 0; i < m; ++i)
    {
        for(int k=0; k < m; k += 4)
        {
            int x= (4 - (m%4));
            if(k/4 == m/4){
                int y;
                for (y=0;y<x; y++){
                    fprintf(chl_inv_out_bin_file, "BAADF00D");
                }
            }
            int j = k+3;
            if (j > (m-1))
                j = m-1;
            
            for(; j >= k; j--) {
                fread (&chl_inv, sizeof(int), 1, CHL_inv_out_file_bin_1);
                if (i>j){
                    fprintf(chl_inv_out_bin_file, "DEADBEEF"); 
                }
                else{
                    //printf("cholesky_inv = %d\n", chl_inv);
                    fprintf(chl_inv_out_bin_file, "%08X", chl_inv);
                }      
            }
            fprintf(chl_inv_out_bin_file, "\n"); 
        }
        if ((m%8 != 0) && (m%8 <= 4)){  
            fprintf(chl_inv_out_bin_file, "BAADF00DBAADF00DBAADF00DBAADF00D\n");
        }
    }


//MAT_SOL_In_LCt_Inn

    FILE *LCt_inn_in_file_bin_1 = fopen(LCt_inn_in_file_bin.c_str(), "rb"); assert(LCt_inn_in_file_bin_1);
    int LCt_inn_in_hex;
    FILE *LCt_inn_in_bin_file = fopen((instr_dir + "__LCt_inn_in.txt").c_str(), "w+");
    assert(LCt_inn_in_bin_file);
    for (int i = 0; i < s+1; ++i)
    {
        for(int k=0; k < m; k += 4)
        {
            if(k/4 == m/4){
                int x= (4 - (m%4));
                int y;
                for (y=0;y<x; y++){
                    fprintf(LCt_inn_in_bin_file, "BAADF00D");

                }

            }
            int j = k+3;
            if (j > (m-1))
                j = m - 1;
            
            for(; j >= k; j--) {
                fread (&LCt_inn_in_hex, sizeof(int), 1, LCt_inn_in_file_bin_1);
                fprintf(LCt_inn_in_bin_file, "%08x", LCt_inn_in_hex);
            }
            
            fprintf(LCt_inn_in_bin_file, "\n");
        }
        if ((m%8 != 0) && (m%8 <= 4)){  
            fprintf(LCt_inn_in_bin_file, "BAADF00DBAADF00DBAADF00DBAADF00D\n");
        }   
    } 

    FILE *LCt_inn_out_file_bin_1 = fopen(LCt_inn_out_file_bin.c_str(), "rb"); assert(LCt_inn_out_file_bin_1);
    int LCt_inn_out_hex;
    FILE *LCt_inn_out_bin_file = fopen((instr_dir + "__LCt_inn_out.txt").c_str(), "w+");
    assert(LCt_inn_out_bin_file);
    for (int i = 0; i < s+1; ++i)
    {
        for(int k=0; k < m; k += 4)
        {
            if(k/4 == m/4){
                int x= (4 - (m%4));
                int y;
                for (y=0;y<x; y++){
                    fprintf(LCt_inn_out_bin_file, "BAADF00D");

                }

            }
            int j = k+3;
            if (j > (m-1))
                j = m - 1;
            
            for(; j >= k; j--) {
                fread (&LCt_inn_out_hex, sizeof(int), 1, LCt_inn_out_file_bin_1);
                fprintf(LCt_inn_out_bin_file, "%08x", LCt_inn_out_hex);
            }
            
            fprintf(LCt_inn_out_bin_file, "\n");
        }
        if ((m%8 != 0) && (m%8 <= 4)){  
            fprintf(LCt_inn_out_bin_file, "BAADF00DBAADF00DBAADF00DBAADF00D\n");
        }   
    }


    FILE *Kt_out_file_bin_1 = fopen(Kt_out_file_bin.c_str(), "rb"); assert(Kt_out_file_bin_1);
    int Kt_out_hex;
    FILE *Kt_out_bin_file = fopen((instr_dir + "__Kt_out.txt").c_str(), "w+");
    assert(Kt_out_bin_file);
    for (int i = 0; i < m; ++i)
    {
        for(int k=0; k < s+1; k += 4)
        {
            if(k/4 == (s+1)/4){
                int x= (4 - ((s+1)%4));
                int y;
                for (y=0;y<x; y++){
                    fprintf(Kt_out_bin_file, "BAADF00D");

                }

            }
            int j = k+3;
            if (j > ((s+1) - 1))
                j = (s+1) - 1;
            
            for(; j >= k; j--) {
                fread (&Kt_out_hex, sizeof(int), 1, Kt_out_file_bin_1);
                fprintf(Kt_out_bin_file, "%08x", Kt_out_hex);
            }
            
            fprintf(Kt_out_bin_file, "\n");
        }
        if (((s+1)%8 != 0) && ((s+1)%8 <= 4)){  
            fprintf(Kt_out_bin_file, "BAADF00DBAADF00DBAADF00DBAADF00D\n");
        }   
    }

	FILE *LCt_inn_out_2_file_bin_1 = fopen(LCt_inn_out_2_file_bin.c_str(), "rb"); assert(LCt_inn_out_2_file_bin_1);
    int LCt_inn_out_2_hex;
    FILE *LCt_inn_out_2_bin_file = fopen((instr_dir + "__LCt_inn_out_2.txt").c_str(), "w+");
    assert(LCt_inn_out_2_bin_file);
    for (int i = 0; i < s+1; ++i)
    {
        for(int k=0; k < m; k += 4)
        {
            if(k/4 == m/4){
                int x= (4 - (m%4));
                int y;
                for (y=0;y<x; y++){
                    fprintf(LCt_inn_out_2_bin_file, "BAADF00D");

                }

            }
            int j = k+3;
            if (j > (m-1))
                j = m - 1;
            
            for(; j >= k; j--) {
                fread (&LCt_inn_out_2_hex, sizeof(int), 1, LCt_inn_out_2_file_bin_1);
                fprintf(LCt_inn_out_2_bin_file, "%08x", LCt_inn_out_2_hex);
            }
            
            fprintf(LCt_inn_out_2_bin_file, "\n");
        }
        if ((m%8 != 0) && (m%8 <= 4)){  
            fprintf(LCt_inn_out_2_bin_file, "BAADF00DBAADF00DBAADF00DBAADF00D\n");
        }   
    }


    FILE *cov_state_file_bin_1 = fopen(cov_state_file_bin.c_str(), "rb"); assert(cov_state_file_bin_1);
    int cov_state_hex;
    FILE *cov_state_bin_file = fopen((instr_dir + "__cov_state_in.txt").c_str(), "w+");
    assert(cov_state_bin_file);
    for (int i = 0; i < s+1; ++i)
    {
        for(int k=0; k < s; k += 4)
        {
            if(k/4 == s/4){
                int x= (4 - (s%4));
                int y;
                for (y=0;y<x; y++){
                    fprintf(cov_state_bin_file, "BAADF00D");

                }

            }
            int j = k+3;
            if (j > (s-1))
                j = s - 1;
            
            for(; j >= k; j--) {
                fread (&cov_state_hex, sizeof(int), 1, cov_state_file_bin_1);
                fprintf(cov_state_bin_file, "%08x", cov_state_hex);
            }
            
            fprintf(cov_state_bin_file, "\n");
        }
        if ((s%8 != 0) && (s%8 <= 4)){  
            fprintf(cov_state_bin_file, "BAADF00DBAADF00DBAADF00DBAADF00D\n");
        }   
    }

    FILE *cov_state_out_file_bin_1 = fopen(cov_state_out_file_bin.c_str(), "rb"); assert(cov_state_out_file_bin_1);
    int cov_state_out_hex;
    FILE *cov_state_out_bin_file = fopen((instr_dir + "__cov_state_out.txt").c_str(), "w+");
    assert(cov_state_out_bin_file);
    for (int i = 0; i < s+1; ++i)
    {
        for(int k=0; k < s; k += 4)
        {
            if(k/4 == s/4){
                int x= (4 - (s%4));
                int y;
                for (y=0;y<x; y++){
                    fprintf(cov_state_out_bin_file, "BAADF00D");

                }

            }
            int j = k+3;
            if (j > (s-1))
                j = s - 1;
            
            for(; j >= k; j--) {
                fread (&cov_state_out_hex, sizeof(int), 1, cov_state_out_file_bin_1);
                fprintf(cov_state_out_bin_file, "%08x", cov_state_out_hex);
            }
            
            fprintf(cov_state_out_bin_file, "\n");
        }
        if ((s%8 != 0) && (s%8 <= 4)){  
            fprintf(cov_state_out_bin_file, "BAADF00DBAADF00DBAADF00DBAADF00D\n");
        }   
    }
//    if (strcmp (mod2, mod_2) == 0){  
#if MASK_EN
    fast.detect(img_data, &mask, num_points, threshold, 0, 0, img.cols, img.rows);
#else
    fast.detect(img_data, NULL, num_points, threshold, 0, 0, img.cols, img.rows);
#endif
//    }

    const int num_batch = 4;
#ifdef LIT_MARKER
    // Start Marker
    __SSC_MARK(0xaa);
#endif
    if ((strcmp (mod1, mod_1) == 0 && strcmp (mod2, mod_2) == 0)||(strcmp (mod1, mod_1) == 0)) {
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
                assert(patch_idx < patch_list.size() && patch_idx >= 0);
                Patch_t &patch = patch_list[patch_idx];
                my_pred[bid] = pred;

#if FORCE_SECOND_IMAGE_BLACK
                if(pred.ImageId == 0) {
#else
                if(1) {
#endif
                    xy o = fast.track(patch.patch, img_data, half_patch_width, half_patch_width, pred.x, pred.y,
                                      fast_track_radius, fast_track_threshold, fast_min_match); 
                    result[bid].x = o.x;
                    result[bid].y = o.y;
                    result[bid].score = (o.score * 1024);
                } else {
                    result[bid].x = 0;
                    result[bid].y = 0;
                    result[bid].score = 0;
                    fprintf(g_fp_list, "// 00000000 // Score:0, PredX:0 (0.000000), PredY:0 (0.000000)\n");
                }
                if(pred.LastPredInGroup == true) {
                    dump_mem_fp(g_fp_patch, patch.patch, (full_patch_width * full_patch_width));
                    patch_idx += 1;
                }
            }
            dump_mem_fp(g_fp_list, (uint8_t*)my_pred, bid * sizeof(xy_hw));
            dump_mem_fp(g_fp_result,(uint8_t*)result, bid * sizeof(xy_hw)); 
        }
    }
#if LIT_MARKER
// End Marker
    __SSC_MARK(0xbb);
#endif
    vector<xy> &v = fast.features;
	/*
#if INSTRUMENT
    fprintf(fp_config, "%08X //DetNumCorner:%d\n", v.size(), v.size());
    fprintf(fp_config_256, "%08X //DetNumCorner:%d\n", v.size(), v.size());
    if (num_points>v.size()){
        num_points = v.size();
        fprintf(fp_config, "%08X //DetNumSortedCorner:%d\n", num_points, num_points);
        fprintf(fp_config_256, "%08X //DetNumSortedCorner:%d\n", num_points, num_points);
    }
    else {
        fprintf(fp_config, "%08X //DetNumSortedCorner:%d\n", num_points, num_points);
        fprintf(fp_config_256, "%08X //DetNumSortedCorner:%d\n", num_points, num_points);
    }
    fprintf(fp_config, "%08X //SortDelta:%d\n", 0, 0);
    fprintf(fp_config_256, "%08X //SortDelta:%d\n", 0, 0);
    fclose(fp_config);
    fclose(fp_config_256);
#endif  
    */

#if INSTRUMENT
    // if ((strcmp (mod1, mod_1) == 0 && strcmp (mod2, mod_2) == 0) || (strcmp (mod2, mod_2)==0)){
        printf("v.size()=%d\n", v.size());
        fprintf(fp_config, "%08X //Det1NumCorner:%d\n", v.size(), v.size());
        fprintf(fp_config_256, "%08X //Det1NumCorner:%d\n", v.size(), v.size());
        fprintf(fp_config_matsol_0, "%08X //Det1NumCorner:%d\n", v.size(), v.size());
        fprintf(fp_config_matsol_1, "%08X //Det1NumCorner:%d\n", v.size(), v.size());
        if (num_points>v.size()){
            num_points = v.size();
            fprintf(fp_config, "%08X //Det1NumSortedCorner:%d\n", num_points, num_points);
            fprintf(fp_config_256, "%08X //Det1NumSortedCorner:%d\n", num_points, num_points);
            fprintf(fp_config_matsol_0, "%08X //Det1NumSortedCorner:%d\n", num_points, num_points);
            fprintf(fp_config_matsol_1, "%08X //Det1NumSortedCorner:%d\n", num_points, num_points);
        }
        else if(num_points <= v.size()) {
            fprintf(fp_config, "%08X //Det1NumSortedCorner:%d\n", num_points, num_points);
            fprintf(fp_config_256, "%08X //Det1NumSortedCorner:%d\n", num_points, num_points);
            fprintf(fp_config_matsol_0, "%08X //Det1NumSortedCorner:%d\n", num_points, num_points);
            fprintf(fp_config_matsol_1, "%08X //Det1NumSortedCorner:%d\n", num_points, num_points);
        }
        fprintf(fp_config, "%08X //Det1SortDelta:%d\n", 0, 0);
        fprintf(fp_config_256, "%08X //Det1SortDelta:%d\n", 0, 0);
        fprintf(fp_config, "00000001 //Det1Mode: 1\n");
        fprintf(fp_config_256, "00000001 //Det1Mode: 1\n");
#if 1 
        fprintf(fp_config, "%08X //Det1CBAssignment: %d\n", cbbuffassignmenta, cbbuffassignmenta);
#else
        fprintf(fp_config, "00000001 //Det1CBAssignment: 1\n");
#endif

#if 1        
        fprintf(fp_config_256, "%08X //Det1CBAssignment: %d\n", cbbuffassignmenta, cbbuffassignmenta);
#else
        fprintf(fp_config_256, "00000001 //Det1CBAssignment: 1\n");
#endif
        fprintf(fp_config, "%08X //Det1WorkingSetSize: %d\n", detworkset, detworkset);
        fprintf(fp_config_256, "%08X //Det1WorkingSetSize: %d\n", detworkset, detworkset);
        fprintf(fp_config, "00000001 //Det1ExportCorners: 1\n");
        fprintf(fp_config_256, "00000001 //Det1ExportCorners: 1\n");
        fprintf(fp_config, "00000000 //Det1ExportPatches: 0\n");
        fprintf(fp_config_256, "00000000 //Det1ExportPatches: 0\n");
        fprintf(fp_config, "00000000 //Det1DynamicThresholdEnable: 0\n");
        fprintf(fp_config_256, "00000000 //Det1DynamicThresholdEnable: 0\n");
        
        fprintf(fp_config_matsol_0, "%08X //Det1SortDelta:%d\n", 0, 0);
        fprintf(fp_config_matsol_1, "%08X //Det1SortDelta:%d\n", 0, 0);
        fprintf(fp_config_matsol_0, "00000001 //Det1Mode: 1\n");
        fprintf(fp_config_matsol_1, "00000001 //Det1Mode: 1\n");
#if 1
        fprintf(fp_config_matsol_0, "%08X //Det1CBAssignment: %d\n", cbbuffassignmenta, cbbuffassignmenta);
#else
        fprintf(fp_config_matsol_0, "00000001 //Det1CBAssignment: 1\n");
#endif

#if 1
        fprintf(fp_config_matsol_1, "%08X //Det1CBAssignment: %d\n", cbbuffassignmenta, cbbuffassignmenta);
#else
        fprintf(fp_config_matsol_1, "00000001 //Det1CBAssignment: 1\n");
#endif 
        fprintf(fp_config_matsol_0, "%08X //Det1WorkingSetSize: %d\n", detworkset, detworkset);
        fprintf(fp_config_matsol_1, "%08X //Det1WorkingSetSize: %d\n", detworkset, detworkset);
        fprintf(fp_config_matsol_0, "00000001 //Det1ExportCorners: 1\n");
        fprintf(fp_config_matsol_1, "00000001 //Det1ExportCorners: 1\n");
        fprintf(fp_config_matsol_0, "00000000 //Det1ExportPatches: 0\n");
        fprintf(fp_config_matsol_1, "00000000 //Det1ExportPatches: 0\n");
        fprintf(fp_config_matsol_0, "00000000 //Det1DynamicThresholdEnable: 0\n");
        fprintf(fp_config_matsol_1, "00000000 //Det1DynamicThresholdEnable: 0\n"); 

#if MASK_EN
#if CW_EN
	if (strcmp (mod3, mod_3) != 0){
    fprintf(fp_config, "00000001 //Det2CWTEn: 1\n");
    fprintf(fp_config_256, "00000001 //Det2CWTEn: 1\n");
    fprintf(fp_config_matsol_0, "00000001 //Det2CWTEn: 1\n");
    fprintf(fp_config_matsol_1, "00000001 //Det2CWTEn: 1\n");
	}
	else{
		fprintf(fp_config, "00000000 //Det2CWTDis\n");
    	fprintf(fp_config_256, "00000000 //Det2CWTDis\n");
        fprintf(fp_config_matsol_0, "00000000 //Det2CWTDis\n");
    	fprintf(fp_config_matsol_1, "00000000 //Det2CWTDis\n");
    }
#else
    fprintf(fp_config, "00000000 //Det2CWTDis\n");
    fprintf(fp_config_256, "00000000 //Det2CWTDis\n");
    fprintf(fp_config_matsol_0, "00000000 //Det2CWTDis\n");
    fprintf(fp_config_matsol_1, "00000000 //Det2CWTDis\n");
#endif

    if (strcmp (mod3, mod_3) != 0 ){
   			 fprintf(fp_config, "00000001 //Det2MaskEn\n");
 		     fprintf(fp_config_256, "00000001 //Det2MaskEn\n");
             fprintf(fp_config_matsol_0, "00000001 //Det2MaskEn\n");
 		     fprintf(fp_config_matsol_1, "00000001 //Det2MaskEn\n");
		}
		else{
            fprintf(fp_config, "00000000 //Det2MaskDis\n");
            fprintf(fp_config_256, "00000000 //Det2MaskDis\n");
            fprintf(fp_config_matsol_0, "00000000 //Det2MaskDis\n");
            fprintf(fp_config_matsol_1, "00000000 //Det2MaskDis\n");
        }
#else
  	    fprintf(fp_config, "00000000 //Det2MaskDis\n");
        fprintf(fp_config_256, "00000000 //Det2MaskDis\n");
        fprintf(fp_config_matsol_0, "00000000 //Det2MaskDis\n");
        fprintf(fp_config_matsol_1, "00000000 //Det2MaskDis\n");
#endif


        //if (strcmp (mod2, mod_2) == 0){
        fprintf(fp_config, "%08X //Det2Threshold:%d\n", threshold, threshold);
        fprintf(fp_config_256, "%08X //Det2Threshold:%d\n", threshold, threshold);
        fprintf(fp_config_matsol_0, "%08X //Det2Threshold:%d\n", threshold, threshold);
        fprintf(fp_config_matsol_1, "%08X //Det2Threshold:%d\n", threshold, threshold);
        /*} else {
    fprintf(fp_config, "%08X //Det1Threshold:%d\n", 0, 0);
    fprintf(fp_config_256, "%08X //Det1Threshold:%d\n", 0, 0);
    }*/

        // if ((strcmp (mod1, mod_1) == 0 && strcmp (mod2, mod_2) == 0) || (strcmp (mod2, mod_2)==0)){
        printf("v.size()=%d\n", v.size());
        fprintf(fp_config, "%08X //Det2NumCorner:%d\n", v.size(), v.size());
        fprintf(fp_config_256, "%08X //Det2NumCorner:%d\n", v.size(), v.size());
        fprintf(fp_config_matsol_0, "%08X //Det2NumCorner:%d\n", v.size(), v.size());
        fprintf(fp_config_matsol_1, "%08X //Det2NumCorner:%d\n", v.size(), v.size());
        if (num_points>v.size()){
            num_points = v.size();
            fprintf(fp_config, "%08X //Det2NumSortedCorner:%d\n", num_points, num_points);
            fprintf(fp_config_256, "%08X //Det2NumSortedCorner:%d\n", num_points, num_points);
            fprintf(fp_config_matsol_0, "%08X //Det2NumSortedCorner:%d\n", num_points, num_points);
            fprintf(fp_config_matsol_1, "%08X //Det2NumSortedCorner:%d\n", num_points, num_points);
        }
        else if(num_points <= v.size()) {
            fprintf(fp_config, "%08X //Det2NumSortedCorner:%d\n", num_points, num_points);
            fprintf(fp_config_256, "%08X //Det2NumSortedCorner:%d\n", num_points, num_points);
            fprintf(fp_config_matsol_0, "%08X //Det2NumSortedCorner:%d\n", num_points, num_points);
            fprintf(fp_config_matsol_1, "%08X //Det2NumSortedCorner:%d\n", num_points, num_points);
        }
        fprintf(fp_config, "%08X //Det2SortDelta:%d\n", 0, 0);
        fprintf(fp_config_256, "%08X //Det2SortDelta:%d\n", 0, 0);
        fprintf(fp_config, "00000001 //Det2Mode: 1\n");
        fprintf(fp_config_256, "00000001 //Det2Mode: 1\n");
#if 1
        fprintf(fp_config, "%08X //Det2CBAssignment: %d\n", cbbuffassignmentb, cbbuffassignmentb);
#else
        fprintf(fp_config, "00000002 //Det2CBAssignment: 2\n");
#endif
#if 1
        fprintf(fp_config_256, "%08X //Det2CBAssignment: %d\n", cbbuffassignmentb, cbbuffassignmentb);
#else
        fprintf(fp_config_256, "00000002 //Det2CBAssignment: 2\n");
#endif
        fprintf(fp_config, "%08X //Det2WorkingSetSize: %d\n", detworkset, detworkset);
        fprintf(fp_config_256, "%08X //Det2WorkingSetSize: %d\n", detworkset, detworkset);
        fprintf(fp_config, "00000001 //Det2ExportCorners: 1\n");
        fprintf(fp_config_256, "00000001 //Det2ExportCorners: 1\n");
        fprintf(fp_config, "00000000 //Det2ExportPatches: 0\n");
        fprintf(fp_config_256, "00000000 //Det2ExportPatches: 0\n");
        fprintf(fp_config, "00000000 //Det2DynamicThresholdEnable: 0\n");
        fprintf(fp_config_256, "00000000 //Det2DynamicThresholdEnable: 0\n");
        
        fprintf(fp_config_matsol_0, "%08X //Det2SortDelta:%d\n", 0, 0);
        fprintf(fp_config_matsol_1, "%08X //Det2SortDelta:%d\n", 0, 0);
        fprintf(fp_config_matsol_0, "00000001 //Det2Mode: 1\n");
        fprintf(fp_config_matsol_1, "00000001 //Det2Mode: 1\n");
#if 1
        fprintf(fp_config_matsol_0, "%08X //Det2CBAssignment: %d\n", cbbuffassignmentb, cbbuffassignmentb);
#else
        fprintf(fp_config_matsol_0, "00000002 //Det2CBAssignment: 2\n");
#endif
#if 1
        fprintf(fp_config_matsol_1, "%08X //Det2CBAssignment: %d\n", cbbuffassignmentb, cbbuffassignmentb);
#else      
        fprintf(fp_config_matsol_1, "00000002 //Det2CBAssignment: 2\n");
#endif
        fprintf(fp_config_matsol_0, "%08X //Det2WorkingSetSize: %d\n", detworkset, detworkset);
        fprintf(fp_config_matsol_1, "%08X //Det2WorkingSetSize: %d\n", detworkset, detworkset);
        fprintf(fp_config_matsol_0, "00000001 //Det2ExportCorners: 1\n");
        fprintf(fp_config_matsol_1, "00000001 //Det2ExportCorners: 1\n");
        fprintf(fp_config_matsol_0, "00000000 //Det2ExportPatches: 0\n");
        fprintf(fp_config_matsol_1, "00000000 //Det2ExportPatches: 0\n");
        fprintf(fp_config_matsol_0, "00000000 //Det2DynamicThresholdEnable: 0\n");
        fprintf(fp_config_matsol_1, "00000000 //Det2DynamicThresholdEnable: 0\n"); 
        

        fprintf(fp_config, "00000000 //ErrorBits: 0\n");
        fprintf(fp_config_256, "00000000 //ErrorBits: 0\n"); 
        fprintf(fp_config_matsol_0, "00000000 //ErrorBits: 0\n");
        fprintf(fp_config_matsol_1, "00000000 //ErrorBits: 0\n"); 
        
        
        fclose(fp_config);
        fclose(fp_config_256);
        fclose(fp_config_matsol_0);
        fclose(fp_config_matsol_1);
#endif 
        //}
        /*else {
    fprintf(fp_config, "%08X //DetNumCorner:%d\n", 0, 0);
    fprintf(fp_config_256, "%08X //DetNumCorner:%d\n", 0, 0);
    fprintf(fp_config, "%08X //DetNumSortedCorner:%d\n", 0, 0);
    fprintf(fp_config_256, "%08X //DetNumSortedCorner:%d\n", 0, 0);
    fprintf(fp_config, "%08X //SortDelta:%d\n", 0, 0);
    fprintf(fp_config_256, "%08X //SortDelta:%d\n", 0, 0);
    fclose(fp_config);
    fclose(fp_config_256);
    }*/

#if SORT_INS
    fclose(g_fp_sort_patch);
    fclose(g_fp_sort_patch_1);
#endif
#if TRACKER_INS 
    fclose(g_fp_list);
    fclose(g_fp_patch);
    fclose(g_fp_result);
#endif
    return 0;
}

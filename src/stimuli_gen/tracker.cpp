#include <iostream>
#include <string>
#include <vector>
#include <map>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
//#include <opencv2/mat.hpp>
#include "../ref_model/fast.h"
#include "../ref_model/fast_constants.h"
#include "../stimuli_gen/utils.h"

//#define ASSERT(cond, str)  if(!(cond)) {std::cout << str << std::endl; abort(0);}
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

struct feature
{
    /*
    feature(int x, int y, const uint8_t * im, int stride) : dx(0), dy(0)
    {
        for(int py = 0; py < full_patch_width; ++py) {
            for(int px = 0; px < full_patch_width; ++px) {
                patch[py * full_patch_width + px] = im[(int)x + px - half_patch_width + ((int)y + py - half_patch_width) * stride];
            }
        }
    }
    */
    uint8_t patch[full_patch_width*full_patch_width];
    struct Point {
        float x, y;
    };
    Point pred[3];
    float dx, dy;
};

struct Fast_Score {
    uint32_t y : 11;
    uint32_t x : 11;
    uint32_t score : 10;
};


extern int g_ncc_compute;
int g_ncc_compute = 0;

int main (int argc, char *argv[])
{
    if(argc <= 1) {
        printf("Usage: <exe> <tracker dir> [<dump dir>]\n");
        exit(1);
    }
    string dir = argv[1];
    string instr_dir = argc > 2 ? argv[2] : ".";
    string img_file  = dir + "/image.bmp";
    string conf_file = dir + "/conf.txt";

    int num_feat;
    
#define READ_FILE(n, str, ...)  assert(n == fscanf(fp, str, __VA_ARGS__)) 
    // Read feature points and prediction
    FILE *fp = fopen(conf_file.c_str(), "r"); assert(fp);
    READ_FILE(1, "Num Prediction:%d", &num_feat);
    std::map<uint64_t, feature> feature_map;
    for(int fid = 0; fid < num_feat; fid++) {
        string patch_file = dir + "/patch_" + to_string(fid) + ".bmp";
        cv::Mat patch_img = cv::imread(patch_file.c_str(), CV_LOAD_IMAGE_GRAYSCALE);
        assert(patch_img.elemSize() == 1);
        assert(patch_img.cols == full_patch_width);
        assert(patch_img.rows == full_patch_width);
        assert(patch_img.step == full_patch_width);
        feature &f        = feature_map[fid];
        memcpy(f.patch, patch_img.data, sizeof(f.patch));
        for(int i=0; i < 3; i++) {
            READ_FILE(2, "%f %f", &f.pred[i].x, &f.pred[i].y);
        } 
    }
#undef READ_FILE
    // Read image
    cv::Mat img = cv::imread(img_file.c_str(), CV_LOAD_IMAGE_GRAYSCALE);
    assert(img.data); 
    assert(img.elemSize() == 1);
    unsigned char *img_data = (unsigned char*)img.data;

#if INSTRUMENT // Stimulus for RTL
    system((string("mkdir -p ") + instr_dir).c_str());
    dump_mem2d((instr_dir + "Image.txt").c_str(), img_data, img.cols, img.rows, img.step);
    FILE *fp_patch = fopen((instr_dir + "FPPatches.txt").c_str(), "w+"); assert(fp_patch);
    vector<Fast_Score> list;
    for(int fid = 0; fid < num_feat; fid++) {
        feature &f = feature_map[fid];
        for(int i = 0; i < 3; i++) {
            Fast_Score pred;
            pred.x = (f.pred[i].x + 0.5);
            pred.y = (f.pred[i].y + 0.5);
            list.push_back(pred);
            dump_mem_fp(fp_patch, f.patch, (full_patch_width * full_patch_width));

            // float to fixed point precision
            f.pred[i].x = pred.x;
            f.pred[i].y = pred.y; 
        }
    }
    dump_mem((instr_dir + "FPList.txt").c_str(), (uint8_t*)list.data(), list.size() * sizeof(Fast_Score));
    fclose(fp_patch);
    FILE *fp_config = fopen((instr_dir + "Config.txt").c_str(), "w+"); assert(fp_patch);
    fprintf(fp_config, "%08x // Width:%d\n", img.cols, img.cols);
    fprintf(fp_config, "%08x // Height:%d\n", img.rows, img.rows);
    fprintf(fp_config, "%08x // Threshold:%d\n", (uint32_t)fast_track_threshold, (uint32_t)fast_track_threshold);
    fprintf(fp_config, "%08x // FPListLen:%d\n", list.size(), list.size());
    fclose(fp_config);
#endif// STIMULUS GENERATION
    
    
    fast_detector_9 fast;
    fast.init(img.cols, img.rows, img.step, full_patch_width, half_patch_width); 
    vector<xy> out; out.resize(num_feat * 3); int out_id = 0;

#ifdef LIT_MARKER    
    // Start Marker
    __SSC_MARK(0xaa);
#endif
    for(int fid = 0; fid < num_feat; fid++) {
        feature &f = feature_map[fid];
        out[out_id++] = fast.track(f.patch, img_data, half_patch_width, half_patch_width, f.pred[0].x, f.pred[0].y,
                                   fast_track_radius, fast_track_threshold, fast_min_match);
        out[out_id++] = fast.track(f.patch, img_data, half_patch_width, half_patch_width, f.pred[1].x, f.pred[1].y,
                                   fast_track_radius, fast_track_threshold, fast_min_match);
        out[out_id++] = fast.track(f.patch, img_data, half_patch_width, half_patch_width, f.pred[2].x, f.pred[2].y,
                                   fast_track_radius, fast_track_threshold, fast_min_match);
    }
#ifdef LIT_MARKER    
    // End Marker 
    __SSC_MARK(0xbb);
#endif
    
    /*
    printf("%s %d %d\n", dir.c_str(), num_feat, g_ncc_compute);
    for(int i = 0; i < out.size(); i += 3) {
        printf("X:%0.8f, Y:%0.8f, Score:%0.8f\n", out[i].x, out[i].y, out[i].score);
    }
    */
#if INSTRUMENT
    vector<Fast_Score> score; score.resize(out_id);
    for(int i = 0; i < out_id; i++) {
        score[i].x = out[i].x; score[i].y = out[i].y;
        score[i].score = (uint32_t)(out[i].score * 1024);
    }
    dump_mem((instr_dir + "Result.txt").c_str(), (uint8_t*)score.data(),
             score.size() * sizeof(Fast_Score), 4);
#endif
    fclose(fp);
    return 0;
}

/*
  for(int i = 0; i < 7; i++) {
  for(int j = 7; j--; ) {
  printf("%2x", f.patch[7*i+j]);
  }
  printf("\n");
  }  
 */

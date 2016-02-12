#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <assert.h>

#include "fast.h"
#include "fast_hw.h"
#include "fast_constants.h"
//#include "utils.h"
#include "bitmap_image.hpp"

extern int g_ncc_compute;
int g_ncc_compute = 0;
string g_det_mode;
#define SW_ONLY 0

template <class T1, class T2>
int fp_cmp(const T1 &a, const T2 &b)
{
    if (a.y == b.y) {
        return (a.x - b.x);
    }
    return (a.y - b.y);    
}

template <class T>
bool fp_cmp(const T &a, const T &b)
{
    int r = fp_cmp<T, T>(a, b);
    return (r < 0);
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

#define CHECK_ARGS(v, s1, s2, s3) \
    if((v != s1) && (v != s2) && (v != s3)) { \
        fprintf(stderr, "Invalid %s, valid values:{%s, %s, %s}\n", #v, #s1, #s2, #s3); \
        exit(1); \
    }
#define MAX_SHOW_COUNT 10
int main (int argc, char *argv[])
{
    // Command Args
    if(argc < 3) {
        printf("Usage: <exe> <input dir> ");
        printf("<module:{track, detect, both-1-by-1, both-together}> ");
        printf("[<detect_mode:{detect, all-1-by-1, all-together}>] ");
        printf("[<sorted_fp>] [<det_threshold>] [<trk_threshold>]\n");
        exit(1);
    }
    string dir        = argv[1];
    string g_mode     = argv[2];
           g_det_mode = argc > 3 ?      argv[3]  : "all-together";
    uint32_t sort_fp  = argc > 4 ? atoi(argv[4]) : (g_det_mode == "detect" ? 8096 : 15);
    uint32_t det_th   = argc > 5 ? atoi(argv[5]) : 15;
    bool test_mem     = argc > 9 ? atoi(argv[9]) : 0;

    CHECK_ARGS(g_mode, "track", "detect", "both-1-by-1");
    CHECK_ARGS(g_det_mode, "detect", "all-1-by-1", "all-together");
    
    string img_file  = dir + "/image.bmp";
    string conf_file = dir + "/conf.txt"; 
    uint32_t num_feat;
    uint32_t num_pred = 0;
    vector<Patch_t> patches;
    vector<xy_hw> list;
    
    // Read image
    bitmap_image img(img_file);
    printf("Read Image !!\n");
    const unsigned int width = argc > 6 ? atoi(argv[6]) : 640;
    const unsigned int height  = argc > 7 ? atoi(argv[7]) : 480;
    img.width() = const unsigned int width;
    img.height()  = const unsigned int height;
    uint32_t trk_th   = argc > 8 ? atoi(argv[8]) : 5;
    
    
#define READ_FILE(n, str, ...)  assert(n == fscanf(fp, str, __VA_ARGS__))
    if(g_mode != "detect") {
        // Read feature points and prediction
        FILE *fp = fopen(conf_file.c_str(), "r"); assert(fp);
        READ_FILE(1, "Num Prediction:%d", &num_feat);
        patches.resize(num_feat * 3);
        for(int fid = 0; fid < num_feat; fid++) {
            string patch_file = dir + "/patch_" + to_string(fid) + ".bmp";
            bitmap_image patch_img(patch_file);
            assert(patch_img.width() == full_patch_width);
            assert(patch_img.height() == full_patch_width);
            for(int i = 0; i < 3; i++) {
                float x, y; xy_hw pred;
                READ_FILE(2, "%f %f", &x, &y);
                pred.x = (x + 0.5); pred.y = (y + 0.5);
                if(pred.x < 8 || pred.x > (img.width() - 9)
                   || pred.y < 8 || pred.y > (img.height() - 9)) {
                    printf("I: Droped, x:%03d, y:%03d\n", pred.x, pred.y);
                    continue;
                }
                list.push_back(pred);
                memcpy(&patches[num_pred], patch_img.row(0), sizeof(Patch_t));
                num_pred++;
            }
        }
        fclose(fp);
    }
#undef READ_FILE

#if (!SW_ONLY)
    fast_detector_9_hw fast_hw;
    if(test_mem) {
        if( ! fast_hw.test_sram()) {
            printf("E: SRAM test failed !!\n");
            exit(1);
        }
    }
    fast_hw.init(img.width(), img.height(), img.width(), full_patch_width, half_patch_width);
    
    vector<xy_hw> out_hw;
    if(g_mode != "detect") {
        out_hw.resize(num_pred);
        fast_hw.track_list(num_pred, patches.data(), list.data(), img.row(0),
                           trk_th, out_hw.data());
    }

    vector<xy_hw> &det_hw = fast_hw.features;
    vector<Patch_t> &patch_hw = fast_hw.patches;
    if(g_mode != "track") {
        fast_hw.detect(img.row(0), sort_fp, det_th, 0, 0, img.width(), img.height()); 
    } 
#endif
    
    fast_detector_9 fast_sw;
    fast_sw.init(img.width(), img.height(), img.width(), full_patch_width, half_patch_width);
    vector<xy> out_sw;
    if(g_mode != "detect") {
        out_sw.resize(num_pred);
        for(int pid = 0; pid < num_pred; pid++) {
            out_sw[pid] = fast_sw.track(patches[pid].buf, img.row(0), half_patch_width,
                                        half_patch_width, list[pid].x, list[pid].y,
                                        fast_track_radius, trk_th, fast_min_match);
        }
    }
    
    vector<xy> &det_sw = fast_sw.features;
    if(g_mode != "track") {
        fast_sw.detect(img.row(0), sort_fp, det_th, 0, 0, img.width(), img.height());
    } 

    printf("I: %s img_wd:%d img_ht:%d\n", dir.c_str(), img.width(), img.height());
    bool match = true; 
#if (!SW_ONLY)
    int trk_diff = 0;
    std::map<int, uint32_t> pix_diff_map, score_diff_map;
    for(int pid = 0; pid < num_pred; pid++) {
        xy_hw &hw = out_hw[pid]; xy &sw = out_sw[pid];
        const float inf = std::numeric_limits<float>::infinity();
        uint32_t sw_score = (sw.score * 1024); bool success = true;
        if(sw.x == inf || sw.y == inf) {
            success = ((hw.x == 0 && hw.y == 0) || (hw.score <= sw_score));
            pix_diff_map[success - 1] += 1;
        } else {
            int x_abs = abs((int)sw.x - (int)hw.x); 
            int y_abs = abs((int)sw.y - (int)hw.y);
            int score_abs = abs((int)hw.score - (int)sw_score);
            pix_diff_map[(x_abs + y_abs)] += 1;
            score_diff_map[score_abs]     += 1;
            success = (   ((x_abs <= 0) && (y_abs <= 0) && (score_abs <= 1))
                          || ((x_abs < 11) && (y_abs < 11) && (score_abs == 0)) ); // compare against search centre
            if(MAX_SHOW_COUNT <= trk_diff && ((x_abs + y_abs > 10) || score_abs > 20)) {
                printf("D[TRK]: Id:%4d (%3d,%3d) ", pid, list[pid].x, list[pid].y);
                printf("sw.x:%3.0f, sw.y:%3.0f, sw.score:%4d", sw.x, sw.y, sw_score);
                printf(" | hw.x:%3d, hw.y:%3d, hw.score:%4d\n", hw.x, hw.y, hw.score);
            }

        }
        if(!success) {
            if(trk_diff < MAX_SHOW_COUNT) {
                printf("E[TRK]: Id:%4d (%3d,%3d) ", pid, list[pid].x, list[pid].y);
                printf("sw.x:%3.0f, sw.y:%3.0f, sw.score:%4d", sw.x, sw.y, sw_score);
                printf(" | hw.x:%3d, hw.y:%3d, hw.score:%4d\n", hw.x, hw.y, hw.score);
                //printf("abs_x:%d abs_y:%d abs_score:%d\n", x_abs, y_abs, score_abs);
                //printf("b_x:%d b_y:%d b_s:%d\n", (int)(x_abs > 1), (int)(y_abs > 1), (int)(score_abs > 4));
            }
            trk_diff++;
            match = false;
        }
    }
    if(g_mode != "track") {
        printf("DET: sw.fp:%4d | hw.fp:%4d\n", det_sw.size(), det_hw.size());
        uint32_t patch_gen_diff = 0;
        if(g_det_mode != "detect") {
            Patch_t patch_sw; uint8_t *buf_sw = patch_sw.buf;
            assert(patch_hw.size() == det_hw.size());
            for(int i = 0; i < det_hw.size(); i++) {
                xy_hw &hw = det_hw[i]; uint8_t *buf_hw = patch_hw[i].buf;
                get_patch(img.row(0), hw.x, hw.y, img.width(), buf_sw);
                if(memcmp(buf_sw, buf_hw, sizeof(patch_sw.patch))) {
                    patch_gen_diff++;
                    if(patch_gen_diff < MAX_SHOW_COUNT) {
                        printf("E[DET]: Patch Gen Failed, x:%3d, y:%3d\n");
                        printf("         SW          |          HW\n");
                        for(int j = 0; j < 7; j++) {
                            bool fail = false;
                            for(int k = 7; k--; ) {
                                printf("%02x ", buf_sw[j+k]);
                            }
                            printf("| ");
                            for(int k = 7; k--; ) {
                                printf("%02x ", buf_hw[j+k]);
                                fail = fail | (buf_hw[j+k] != buf_sw[j+k]);
                            }
                            printf("%s\n", fail ? " <==" : "");
                        }
                    }
                }
            }
        }
        std::sort(det_sw.begin(), det_sw.end(), fp_cmp<xy>);
        std::sort(det_hw.begin(), det_hw.end(), fp_cmp<xy_hw>);
        uint32_t max = std::max(det_sw.size(), det_hw.size());
        vector<int> hw_list, sw_list; int sw_id = 0; int hw_id = 0;
        int score_diff = 0; int score_match = 0;
        while(sw_id < det_sw.size() || hw_id < det_hw.size()) {
            if(det_sw.size() <= sw_id) {hw_list.push_back(hw_id++); continue;}
            if(det_hw.size() <= hw_id) {sw_list.push_back(sw_id++); continue;}
            xy &sw  = det_sw[sw_id]; xy_hw &hw = det_hw[hw_id];
            int cmp = fp_cmp<xy, xy_hw>(sw, hw);
            if(cmp == 0) {
                assert((sw.x == hw.x) && (sw.y == hw.y));
                if(abs(sw.score - hw.score) > 1) {
                    if(score_diff < MAX_SHOW_COUNT) {
                    printf("E[DET]: x:%3.0f, y:%3.0f, sw.score:%3.0f", sw.x, sw.y, sw.score);
                    printf(" | hw.score:%3d\n", hw.score);
                    }
                    score_diff++;
                    match = false;
                } else {
                    score_match++;
                }
                sw_id++; hw_id++;
            } else if (cmp < 0) { // sw is less than hw
                sw_list.push_back(sw_id++);
                match = false;
            } else { // hw is less than sw
                hw_list.push_back(hw_id++);
            }
        }
        uint32_t undetected = sw_list.size();
        if((g_det_mode != "detect") && (!score_diff) && sw_list.size()
           && (sw_list.size() == hw_list.size()))
        {
            int score = det_sw[sw_list[0]].score;
            for(int i = 1; i < sw_list.size(); i++) {
                if(score != det_sw[sw_list[i]].score) {
                    score = 0;
                    break;
                }
            }
            if(score) {
                for(int i = 0; i < hw_list.size(); i++) {
                    if(score != det_hw[hw_list[i]].score) {
                        score = 0;
                        break;
                    }
                }
            }
            undetected = score ? 0 : sw_list.size();
        }
        match = (! score_diff) && (!patch_gen_diff) && (!undetected);
        if (!sw_list.empty()) {printf("%s[DET]: Undetected points\n", match ? "I" : "E");}
        for (int i = 0; i < sw_list.size(); i++) {
            if(i >= MAX_SHOW_COUNT) { break; }
            xy &sw = det_sw[sw_list[i]];
            printf("%s[DET]: \t sw.x:%3.0f, sw.y:%3.0f, sw.score:%3.0f\n",
                   match ? "I" : "E", sw.x, sw.y, sw.score);
        }
        if (!hw_list.empty()) {printf("I[DET]: Additional detect points\n");}
        for (int i = 0; i < hw_list.size(); i++) {
            if (i > MAX_SHOW_COUNT) { break; }
            xy_hw &hw = det_hw[hw_list[i]];
            printf("I[DET]: \t hw.x:%3d, hw.y:%3d, hw.score:%3d\n", hw.x, hw.y, hw.score);
        }
        printf("DET: Match:%d, ScoreDiff:%d, Undetected:%d, Additional:%d, PatchGenDiff:%d, Test:%s\n",
               score_match, score_diff, sw_list.size(), hw_list.size(), patch_gen_diff,
               match ? "Passed" : "Failed");
    }
    if(g_mode != "detect") {
        printf("TRK: pred:%d mismatches:%d\n", num_pred, trk_diff);
        printf("TRK: PixDiffMap   ");
        for(map<int, uint32_t>::iterator it = pix_diff_map.begin(); it != pix_diff_map.end(); it++) {
            printf("[%2d]%3d, ", it->first, it->second);
        }
        printf("\nTRK: ScoreDiffMap ");
        for(map<int, uint32_t>::iterator it = score_diff_map.begin(); it != score_diff_map.end(); it++) {
            printf("[%2d]%3d, ", it->first, it->second);
        }
        printf("\n");
    } 
#else     
    size_t min = std::min(out_sw.size(), (size_t)10);
    for(int i = 0; i < min; i++) {
        printf("I: X %03d, Y %03d, Track=> ", list[i].x, list[i].y);
        printf("X:%0.8f, Y:%0.8f, Score:%0.8f\n", out_sw[i].x, out_sw[i].y, out_sw[i].score);
    }
#endif

    return (!match);
}

/*
  for(int i = 0; i < 7; i++) {
  for(int j = 7; j--; ) {
  printf("%2x", patch_img.row(0)[7*i+j]);
  }
  printf("\n");
  }
  exit(1);
 */

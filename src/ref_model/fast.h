#ifndef FAST_H
#define FAST_H


#include <vector>
#include "scaled_mask.h"
using namespace std;

extern int g_det_count;

typedef struct { float x, y, score, reserved; } xy; 
typedef unsigned char byte;

class fast_detector_9 {
 private:
    int pixel[16];
    int xsize, ysize, stride, patch_stride, patch_win_half_width;
    float inline score_match(const unsigned char *im1, const int x1, const int y1, const unsigned char *im2, const int x2, const int y2, float max_error, float mean1);
    float inline compute_mean1(const unsigned char *im1, const int x1, const int y1);

 public:
    vector<xy> features;
    vector<xy> &detect(const unsigned char *im, const scaled_mask *mask, int number_wanted, int bthresh, int winx, int winy, int winwidth, int winheight);
    vector<xy> &detect(const unsigned char *im, const scaled_mask *mask, int number_wanted, int bthresh) { return detect(im, mask, number_wanted, bthresh, 0, 0, xsize, ysize); }
    xy track(const unsigned char *im1, const unsigned char *im2, int xcurrent, int ycurrent, float predx, float predy, float radius, int b, float min_score);
    void init(const int image_width, const int image_height, const int image_stride, const int tracker_patch_stride, const int tracker_patch_win_half_width);
};
/*
class fast_detector_10 {
 private:
    int pixel[16];
    int xsize, ysize, stride;
    float score_match(const unsigned char *im1, const int x1, const int y1, const unsigned char *im2, const int x2, const int y2, float max_error);

 public:
    vector<xy> features;
    vector<xy> &detect(const unsigned char *im, const scaled_mask *mask, int number_wanted, int bthresh, int winx, int winy, int winwidth, int winheight);
    vector<xy> &detect(const unsigned char *im, const scaled_mask *mask, int number_wanted, int bthresh) { return detect(im, mask, number_wanted, bthresh, 0, 0, xsize, ysize); }
    xy track(const unsigned char *im1, const unsigned char *im2, int xcurrent, int ycurrent, int x1, int y1, int x2, int y2, int b);
    void init(const int xsize, const int ysize, const int stride);
};


class fast_detector_11 {
 private:
    int pixel[16];
    int xsize, ysize, stride;
    float score_match(const unsigned char *im1, const int x1, const int y1, const unsigned char *im2, const int x2, const int y2, float max_error);

 public:
    vector<xy> features;
    vector<xy> &detect(const unsigned char *im, const scaled_mask *mask, int number_wanted, int bthresh, int winx, int winy, int winwidth, int winheight);
    vector<xy> &detect(const unsigned char *im, const scaled_mask *mask, int number_wanted, int bthresh) { return detect(im, mask, number_wanted, bthresh, 0, 0, xsize, ysize); }
    xy track(const unsigned char *im1, const unsigned char *im2, int xcurrent, int ycurrent, int x1, int y1, int x2, int y2, int b);
    void init(const int xsize, const int ysize, const int stride);
};

class fast_detector_12 {
 private:
    int pixel[16];
    int xsize, ysize, stride;
    float score_match(const unsigned char *im1, const int x1, const int y1, const unsigned char *im2, const int x2, const int y2, float max_error);

 public:
    vector<xy> features;
    vector<xy> &detect(const unsigned char *im, const scaled_mask *mask, int number_wanted, int bthresh, int winx, int winy, int winwidth, int winheight);
    vector<xy> &detect(const unsigned char *im, const scaled_mask *mask, int number_wanted, int bthresh) { return detect(im, mask, number_wanted, bthresh, 0, 0, xsize, ysize); }
    xy track(const unsigned char *im1, const unsigned char *im2, int xcurrent, int ycurrent, int x1, int y1, int x2, int y2, int b);
    void init(const int xsize, const int ysize, const int stride);
};
*/
#endif

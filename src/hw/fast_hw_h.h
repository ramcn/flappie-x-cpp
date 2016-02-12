#ifndef FAST_HW_H
#define FAST_HW_H

#include <vector>
#include "../ref_model/scaled_mask.h"
#include "vio.h"
using namespace std;
typedef struct {
    union {
        struct {
            uint32_t y     : 11;
            uint32_t x     : 11;
            uint32_t score : 10;
        };
        struct {
            uint32_t resvd           : 30;
            uint32_t LastPredInGroup : 1;
            uint32_t ImageId         : 1;
        };
    };
} xy_hw;

typedef union {
    struct {
        uint8_t patch[49];
        uint8_t rsvd[11];
		xy_hw xy;
    };
    uint8_t buf[64];
} Patch_t;

typedef unsigned char byte;
struct VIO_TOP_t;
struct VIO_DET_t;
struct VIO_TRK_t;

#define MAX_NUM_FEATURES 120

class fast_detector_9_hw : public VIO_Module {
private:
    int pixel[16];
    int xsize, ysize, stride, patch_stride, patch_win_half_width;
    //float inline score_match(const unsigned char *im1, const int x1, const int y1, const unsigned char *im2, const int x2, const int y2, float max_error, float mean1);
    //float inline compute_mean1(const unsigned char *im1, const int x1, const int y1);    
    VIO_TOP_t    *vio_top;
    VIO_DET_t    *vio_det;
    VIO_TRK_t    *vio_trk;
    Patch_t *m_trk_patchlist;
    xy_hw   *m_trk_predlist;
    xy_hw   *m_trk_tracklist;
    uint8_t *m_det_mask;
    size_t get_frame_size() {return (stride * ysize);};
    size_t get_mask_size() {return (((((xsize/8) / 8) + 15) & (~0xf)) * (ysize / 8));} 

public:
    fast_detector_9_hw();
    ~fast_detector_9_hw();
    vector<xy_hw, Fast_Alloc<xy_hw> > features;
    vector<Patch_t, Fast_Alloc<Patch_t> > patches;
    vector<xy_hw, Fast_Alloc<xy_hw> > &detect(const unsigned char *im, const scaled_mask *mask, int number_wanted, int bthresh, int winx, int winy, int winwidth, int winheight);
    //vector<xy_hw> &detect(const unsigned char *im, /*const scaled_mask *mask,*/ int number_wanted, int bthresh) { return detect(im, /*mask,*/ number_wanted, bthresh, 0, 0, xsize, ysize); }
    /*xy_hw track(const unsigned char *im1, const unsigned char *im2, int xcurrent, int ycurrent, float predx, float predy, float radius, int b, float min_score);*/
    void track_list(uint32_t num_pred, uint32_t num_patch, const Patch_t *patch_list, const xy_hw *pred_list, const uint8_t *image, int threshold, xy_hw *track_list);
    void init(const int image_width, const int image_height, const int image_stride, const int tracker_patch_stride, const int tracker_patch_win_half_width);
    xy_hw *get_trk_predlist(size_t num_fps);
    Patch_t *get_trk_patchlist(size_t num_fps);
    xy_hw *get_trk_tracklist(size_t num_fps);
    xy_hw *get_det_fp(uint32_t &num_fp);
    void move_image_2_hw(const uint8_t *img, bool nb);
    void wait_image_tx();

    //Track Functions
    void track_pre(uint32_t num_pred, const Patch_t *patch_list, const xy_hw *pred_list,
                   const uint8_t *im, int threshold);
    void track_trigger();
    void track_wait();
    uint32_t track_post(xy_hw *track_list);
    
};

#endif

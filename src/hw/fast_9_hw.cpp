#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <iostream>
#include <string>

#include "fast_hw.h"
#include "vio_regs.h"
#include "dma_regs.h"
#include "vio_utils.h"


fast_detector_9_hw::fast_detector_9_hw()
    : m_trk_predlist(NULL),
      m_trk_patchlist(NULL),
      m_trk_tracklist(NULL)
{
    vio_top = new VIO_TOP_t;
    vio_cb1 = new VIO_CB1_t;
    vio_cb2 = new VIO_CB2_t;
    vio_det1 = new VIO_DET1_t;
    vio_det2 = new VIO_DET2_t;
    vio_trk = new VIO_TRK_t;
    memset(vio_top, 0x0, sizeof(VIO_TOP_t));
    memset(vio_det1, 0x0, sizeof(VIO_DET1_t));
    memset(vio_det2, 0x0, sizeof(VIO_DET2_t));
    memset(vio_trk, 0x0, sizeof(VIO_TRK_t));
    memset(vio_cb1, 0x0, sizeof(VIO_CB1_t));
    memset(vio_cb2, 0x0, sizeof(VIO_CB2_t));

    m_trk_predlist  = (xy_hw *)  vio_malloc(VIO_TRK_FPPREDLIST_BUFSIZE);
    m_trk_patchlist = (Patch_t *)vio_malloc(VIO_TRK_FPPATCHLIST_BUFSIZE);
    m_trk_tracklist = (xy_hw *)  vio_malloc(VIO_TRK_FPTRACKLIST_BUFSIZE);
    m_det1_mask     = (uint8_t *)vio_malloc(VIO_DET1_ROIMASK_BUFSIZE);
    m_det2_mask     = (uint8_t *)vio_malloc(VIO_DET2_ROIMASK_BUFSIZE);
    memset(m_det1_mask, 0xff, VIO_DET1_ROIMASK_BUFSIZE);
    memset(m_det2_mask, 0xff, VIO_DET2_ROIMASK_BUFSIZE);
    features.resize(MAX_NUM_FEATURES);
    patches.resize(MAX_NUM_FEATURES);
}

fast_detector_9_hw::~fast_detector_9_hw()
{
    features.clear();
    features.shrink_to_fit();
    patches.clear();
    patches.shrink_to_fit();
    delete vio_top;
    delete vio_cb1;
    delete vio_cb2;
    delete vio_det1;
    delete vio_det2;
    delete vio_trk;
    vio_free(m_trk_predlist);
    vio_free(m_trk_patchlist);
    vio_free(m_trk_tracklist);
    vio_free(m_det1_mask);
    vio_free(m_det2_mask);
}

void
fast_detector_9_hw::init(const int image_width, const int image_height,
                      const int image_stride, const int tracker_patch_stride,
                      const int tracker_patch_win_half_width)
{
    PRINTF("fast_detector_9_hw::init()\n");
    xsize  = image_width;
    ysize  = image_height;
    stride = image_stride;
    patch_stride = tracker_patch_stride;
    patch_win_half_width = tracker_patch_win_half_width;

    // Reset
    VIO_TOP_CONTROL_T &top_ctrl = vio_top->VIO_TOP_CONTROL;
    top_ctrl.field.GlobalIntrDisable  = 1;
    top_ctrl.field.ClearConfigWrError = 1;
    top_ctrl.field.ForceModuleCLKEn   = 0;
    top_ctrl.field.InterruptPolarity  = 0;
    top_ctrl.field.InterruptMode      = 0;
    top_ctrl.field.InterruptPulseWidth= 0;
    WRITE_REG(TOP_CONTROL, top_ctrl.val);
    
    //WRITE_REG(MIPI1, 20);//isrc
    //WRITE_REG(MIPI2, 20);//isrc

    
    //Circular buffer 1
    VIO_CB1_CONTROL_T &cb1_ctrl = vio_cb1->VIO_CB1_CONTROL;
    cb1_ctrl.field.ClearCirBuf1Overflow = 0;
    cb1_ctrl.field.CircularBuffer1Depth = 0;
    vio_cb1->VIO_CB1_IMG_BASE.field.BaseAddress    = VIO_CB1_MEM_BA; 
    vio_cb1->VIO_CB1_IMG_WIDTH.field.ImageWidth    = xsize;
    vio_cb1->VIO_CB1_IMG_HEIGHT.field.ImageHeight  = ysize;
    vio_cb1->VIO_CB1_IMG_STRIDE.field.ImageStride  = stride;
    WRITE_REG(CB1_IMG_BASE, vio_cb1->VIO_CB1_IMG_BASE.val);
    WRITE_REG(CB1_IMG_WIDTH, vio_cb1->VIO_CB1_IMG_WIDTH.val);
    WRITE_REG(CB1_IMG_HEIGHT, vio_cb1->VIO_CB1_IMG_HEIGHT.val);
    WRITE_REG(CB1_IMG_STRIDE, vio_cb1->VIO_CB1_IMG_STRIDE.val);

    //Circular buffer 2
    VIO_CB2_CONTROL_T &cb2_ctrl = vio_cb2->VIO_CB2_CONTROL;
    cb2_ctrl.field.ClearCirBuf2Overflow = 0;
    cb2_ctrl.field.CircularBuffer2Depth = 0;
    vio_cb2->VIO_CB2_IMG_BASE.field.BaseAddress    = VIO_CB2_MEM_BA; 
    vio_cb2->VIO_CB2_IMG_WIDTH.field.ImageWidth    = xsize;
    vio_cb2->VIO_CB2_IMG_HEIGHT.field.ImageHeight  = ysize;
    vio_cb2->VIO_CB2_IMG_STRIDE.field.ImageStride  = stride;
    WRITE_REG(CB2_IMG_BASE, vio_cb2->VIO_CB2_IMG_BASE.val);
    WRITE_REG(CB2_IMG_WIDTH, vio_cb2->VIO_CB2_IMG_WIDTH.val);
    WRITE_REG(CB2_IMG_HEIGHT, vio_cb2->VIO_CB2_IMG_HEIGHT.val);
    WRITE_REG(CB2_IMG_STRIDE, vio_cb2->VIO_CB2_IMG_STRIDE.val);
   
    
    // Detector1 Config
    VIO_DET1_CONTROL_T &det1_ctrl = vio_det1->VIO_DET1_CONTROL;
    det1_ctrl.field.DetectorThreshold = 255;
    det1_ctrl.field.FeatureDetectorStart = 0;
    det1_ctrl.field.SorterStart = 0;
    det1_ctrl.field.AutoStartSorting  = 0;
    det1_ctrl.field.SortingWorkingSetSize = 0;
    det1_ctrl.field.CenterWeightingEn = 0;
    det1_ctrl.field.RegionMaskEn = 0;
    det1_ctrl.field.ExportAllFeatures = 0;
    det1_ctrl.field.ExportAllDescriptors = 0;
    det1_ctrl.field.ImageBufferSpecifier = 0;
    det1_ctrl.field.EnDynamicThreshold = 0;
    WRITE_REG(DET1_CONTROL, det1_ctrl.val);

    vio_det1->VIO_DET1_IMG_BASE.field.BaseAddress    = VIO_IMG1_MEM_BA; 
    vio_det1->VIO_DET1_IMG_WIDTH.field.ImageWidth    = xsize;
    vio_det1->VIO_DET1_IMG_HEIGHT.field.ImageHeight  = ysize;
    vio_det1->VIO_DET1_IMG_STRIDE.field.ImageStride  = stride;
    vio_det1->VIO_DET1_ROIMASK_BASE.field.BaseAddress = VIO_DET1_ROIMASK_MEM_BA;
    vio_det1->VIO_DET1_SORTED_FPLIST_BASE.field.BaseAddress =  VIO_DET1_SORTEDFPLIST_MEM_BA;
    vio_det1->VIO_DET1_EXP_FLIST_BASE.field.BaseAddress =  VIO_DET1_EXP_FLIST_MEM_BA;
    vio_det1->VIO_DET1_WRK_BUFA_BASE.field.BaseAddress =  VIO_DET1_WRK_BUFA_MEM_BA;
    vio_det1->VIO_DET1_WRK_BUFB_BASE.field.BaseAddress =  VIO_DET1_WRK_BUFB_MEM_BA;
    
    WRITE_REG(DET1_IMG_BASE, vio_det1->VIO_DET1_IMG_BASE.val);
    WRITE_REG(DET1_IMG_WIDTH, vio_det1->VIO_DET1_IMG_WIDTH.val);
    WRITE_REG(DET1_IMG_HEIGHT, vio_det1->VIO_DET1_IMG_HEIGHT.val);
    WRITE_REG(DET1_IMG_STRIDE, vio_det1->VIO_DET1_IMG_STRIDE.val);
    WRITE_REG(DET1_ROIMASK_BASE,vio_det1->VIO_DET1_ROIMASK_BASE.val);
    WRITE_REG(DET1_SORTED_FPLIST_BASE,vio_det1->VIO_DET1_SORTED_FPLIST_BASE.val);
    WRITE_REG(DET1_EXP_FLIST_BASE,vio_det1->VIO_DET1_EXP_FLIST_BASE.val);
    WRITE_REG(DET1_WRK_BUFA_BASE,vio_det1->VIO_DET1_WRK_BUFA_BASE.val);
    WRITE_REG(DET1_WRK_BUFB_BASE,vio_det1->VIO_DET1_WRK_BUFB_BASE.val);

    // Detector2 Config
    VIO_DET2_CONTROL_T &det2_ctrl = vio_det2->VIO_DET2_CONTROL;
    det2_ctrl.field.DetectorThreshold = 255;
    det2_ctrl.field.FeatureDetectorStart = 0;
    det2_ctrl.field.SorterStart = 0;
    det2_ctrl.field.AutoStartSorting  = 0;
    det2_ctrl.field.SortingWorkingSetSize = 0;
    det2_ctrl.field.CenterWeightingEn = 0;
    det2_ctrl.field.RegionMaskEn = 0;
    det2_ctrl.field.ExportAllFeatures = 0;
    det2_ctrl.field.ExportAllDescriptors = 0;
    det2_ctrl.field.ImageBufferSpecifier = 0;
    det2_ctrl.field.EnDynamicThreshold = 0;
    WRITE_REG(DET2_CONTROL, det2_ctrl.val);

    vio_det2->VIO_DET2_IMG_BASE.field.BaseAddress    = VIO_IMG2_MEM_BA; 
    vio_det2->VIO_DET2_IMG_WIDTH.field.ImageWidth    = xsize;
    vio_det2->VIO_DET2_IMG_HEIGHT.field.ImageHeight  = ysize;
    vio_det2->VIO_DET2_IMG_STRIDE.field.ImageStride  = stride;
    vio_det2->VIO_DET2_ROIMASK_BASE.field.BaseAddress = VIO_DET2_ROIMASK_MEM_BA;
    vio_det2->VIO_DET2_SORTED_FPLIST_BASE.field.BaseAddress =  VIO_DET2_SORTEDFPLIST_MEM_BA;
    vio_det2->VIO_DET2_EXP_FLIST_BASE.field.BaseAddress =  VIO_DET2_EXP_FLIST_MEM_BA;
    vio_det2->VIO_DET2_WRK_BUFA_BASE.field.BaseAddress =  VIO_DET2_WRK_BUFA_MEM_BA;
    vio_det2->VIO_DET2_WRK_BUFB_BASE.field.BaseAddress =  VIO_DET2_WRK_BUFB_MEM_BA;
    
    WRITE_REG(DET2_IMG_BASE, vio_det2->VIO_DET2_IMG_BASE.val);
    WRITE_REG(DET2_IMG_WIDTH, vio_det2->VIO_DET2_IMG_WIDTH.val);
    WRITE_REG(DET2_IMG_HEIGHT, vio_det2->VIO_DET2_IMG_HEIGHT.val);
    WRITE_REG(DET2_IMG_STRIDE, vio_det2->VIO_DET2_IMG_STRIDE.val);
    WRITE_REG(DET2_ROIMASK_BASE,vio_det2->VIO_DET2_ROIMASK_BASE.val);
    WRITE_REG(DET2_SORTED_FPLIST_BASE,vio_det2->VIO_DET2_SORTED_FPLIST_BASE.val);
    WRITE_REG(DET2_EXP_FLIST_BASE,vio_det2->VIO_DET2_EXP_FLIST_BASE.val);
    WRITE_REG(DET2_WRK_BUFA_BASE,vio_det2->VIO_DET2_WRK_BUFA_BASE.val);
    WRITE_REG(DET2_WRK_BUFB_BASE,vio_det2->VIO_DET2_WRK_BUFB_BASE.val);


    // Tracker Config
    VIO_TRK_CONTROL_T &trk_ctrl = vio_trk->VIO_TRK_CONTROL;
    trk_ctrl.field.FeatureTrackerStart = 0;
    trk_ctrl.field.CenterWeightingEn   = 0;
    trk_ctrl.field.ImageBufferSpecifier = 0;
    trk_ctrl.field.TrackerThreshold    = 255;
    WRITE_REG(TRK_CONTROL, trk_ctrl.val);
    
    vio_trk->VIO_TRK_IMG_BASE.field.BaseAddress            = VIO_IMG1_MEM_BA;
    vio_trk->VIO_TRK_IMG_WIDTH.field.ImageWidth    = xsize;
    vio_trk->VIO_TRK_IMG_HEIGHT.field.ImageHeight  = ysize;
    vio_trk->VIO_TRK_IMG_STRIDE.field.ImageStride  = stride;
    vio_trk->VIO_TRK_FPPREDLIST_BASE.field.BaseAddress     = VIO_TRK_FPPREDLIST_MEM_BA;
    vio_trk->VIO_TRK_FPPATCHLIST_BASE.field.BaseAddress    = VIO_TRK_FPPATCHLIST_MEM_BA;
    vio_trk->VIO_TRK_FPTRACKLIST_BASE.field.BaseAddress    = VIO_TRK_FPTRACKLIST_MEM_BA;    
    WRITE_REG(TRK_IMG_BASE, vio_trk->VIO_TRK_IMG_BASE.val);
    WRITE_REG(TRK_IMG_WIDTH, vio_trk->VIO_TRK_IMG_WIDTH.val);
    WRITE_REG(TRK_IMG_HEIGHT, vio_trk->VIO_TRK_IMG_HEIGHT.val);
    WRITE_REG(TRK_IMG_STRIDE, vio_trk->VIO_TRK_IMG_STRIDE.val);
    WRITE_REG(TRK_FPPREDLIST_BASE, vio_trk->VIO_TRK_FPPREDLIST_BASE.val);
    WRITE_REG(TRK_FPPATCHLIST_BASE, vio_trk->VIO_TRK_FPPATCHLIST_BASE.val);
    WRITE_REG(TRK_FPTRACKLIST_BASE, vio_trk->VIO_TRK_FPTRACKLIST_BASE.val);
    
}


extern string g_det_mode;
vector<xy_hw, Fast_Alloc<xy_hw> > &
fast_detector_9_hw::detect(const unsigned char *im, const scaled_mask *mask,
                           int num_wanted, int bthresh, int winx, int winy,
                           int winwidth, int winheight, bool centerWt,
                           bool otf_en, int det_id)
{
    if(otf_en) {
      if(det_id == 0){
	return detect_otf1( im, mask, num_wanted, bthresh, winx, winy, winwidth, winheight, centerWt);
      } else if(det_id == 1){
	return detect_otf2( im, mask, num_wanted, bthresh, winx, winy, winwidth, winheight, centerWt);	
      } else {
	  ASSERT(false, "Unsupported det_id(%d)", det_id);
      }	
    } else {
        if(det_id == 0) {
            return detect1(im, mask, num_wanted, bthresh, winx, winy, winwidth, winheight, centerWt);
        } else if(det_id == 1) {
            return detect2(im, mask, num_wanted, bthresh, winx, winy, winwidth, winheight, centerWt);
        } else {
            ASSERT(false, "Unsupported det_id(%d)", det_id);
        }
    }
}

vector<xy_hw, Fast_Alloc<xy_hw> > &
fast_detector_9_hw::detect1(const unsigned char *im, const scaled_mask *mask,
                            int num_wanted, int bthresh, int winx, int winy,
                            int winwidth, int winheight, bool centerWt)
{
    PRINTF("fast_detector_9_hw::detect1()\n");
    PRINTF("detect mode: %s\n", g_det_mode.c_str());
    uint8_t *buf = mask ? mask->compressed : m_det1_mask;

    const uint32_t max_supported = VIO_DET1_SORTEDFPLIST_BUFSIZE / sizeof(xy_hw);
    ASSERT(num_wanted <= max_supported, "[FRM] num_wanted (%d) is more than supported (%d)", num_wanted, max_supported);

    ASSERT(winwidth < 1024, "winwidth is more thn 1024");
    ASSERT(winheight < 1024, "winwidth is more thn 1024");

    // Move data to HW
    PROFILE_START(DET_DATA_TX); 
    move_dma_2_hw(im, VIO_IMG1_BUF, get_frame_size());
    move_dma_2_hw(buf, VIO_DET1_ROIMASK_BUF, get_mask_size()); 
    PROFILE_END(DET_DATA_TX);

    PROFILE_START(DET_PROC);
    VIO_TOP_CONTROL_T &top_ctrl = vio_top->VIO_TOP_CONTROL;
    VIO_DET1_STATUS_T &status1    = vio_det1->VIO_DET1_STATUS;
    VIO_DET1_CONTROL_T &control1  = vio_det1->VIO_DET1_CONTROL;
    VIO_CB1_CONTROL_T &cb1_ctrl   = vio_cb1->VIO_CB1_CONTROL;
    VIO_CB2_CONTROL_T &cb2_ctrl   = vio_cb2->VIO_CB2_CONTROL;
    
    control1.field.DetectorThreshold = bthresh;
    control1.field.FeatureDetectorStart = 0;
    control1.field.SorterStart          = 0;
    control1.field.AutoStartSorting     = 0;
    control1.field.SortingWorkingSetSize = 0;
    control1.field.CenterWeightingEn    = centerWt;
    control1.field.RegionMaskEn         = mask != NULL ? 1 : 0;
    control1.field.ExportAllFeatures    = 0;
    control1.field.ExportAllDescriptors = 0;
    control1.field.ImageBufferSpecifier = 0;
    control1.field.EnDynamicThreshold   = 0;
    WRITE_REG(DET1_CONTROL, control1.val);

    // CB_config Write
    cb1_ctrl.field.ClearCirBuf1Overflow = 0;
    cb1_ctrl.field.CircularBuffer1Depth = 0;
    WRITE_REG(CB1_CONTROL, cb1_ctrl.val);    

    cb2_ctrl.field.ClearCirBuf2Overflow = 0;
    cb2_ctrl.field.CircularBuffer2Depth = 0;
    WRITE_REG(CB2_CONTROL, cb2_ctrl.val);    

    
    // Config Write
    top_ctrl.field.GlobalIntrDisable  = 1;
    top_ctrl.field.ClearConfigWrError = 1;
    WRITE_REG(TOP_CONTROL, top_ctrl.val);

    vio_det1->VIO_DET1_SORTED_FPLIST_LENGTH.field.FPListLength = (num_wanted);
    WRITE_REG(DET1_SORTED_FPLIST_LENGTH, vio_det1->VIO_DET1_SORTED_FPLIST_LENGTH.val);

    if(g_det_mode == "detect" || g_det_mode == "all-1-by-1") {
        // Feature Detector
        control1.field.FeatureDetectorStart = 1;
        WRITE_REG(DET1_CONTROL, control1.val);
        status1.field.FeatureDetectorDone = 0;
        POLL_N_WAIT(DET1_STATUS, status1.val, status1.field.FeatureDetectorDone == 0);
    }
    
    if(g_det_mode == "all-1-by-1") {
        PRINTF("detect done !!\nStarting sorter .. \n");
        // Run Sorter
        control1.field.FeatureDetectorStart = 0;
        control1.field.SorterStart = 1;
        WRITE_REG(DET1_CONTROL, control1.val);
        status1.val = 0;
        POLL_N_WAIT(DET1_STATUS, status1.val, status1.field.SorterDone == 0);
    }

    if(g_det_mode == "all-together") {
        control1.field.FeatureDetectorStart = 1;
        control1.field.SorterStart = 0;
        control1.field.AutoStartSorting  = 1;
        WRITE_REG(DET1_CONTROL, control1.val);
        status1.val = 0;
        POLL_N_WAIT(DET1_STATUS, status1.val, status1.val != 0x03);
        usleep(2); // HACK: As sorter/arbiter seems to taking time in write-back to SRAM
    }

    PROFILE_END(DET_PROC);

    // Read back FP
    PROFILE_START(DET_DATA_RX);
    if(g_det_mode == "detect") {
        VIO_DET1_TOT_FNUMBER_T &fp_list1 = vio_det1->VIO_DET1_TOT_FNUMBER;
        fp_list1.val = READ_REG(DET1_TOT_FNUMBER);
        features.resize(fp_list1.field.FPListLength);
        if(features.size()) {
            move_dma_2_cpu(VIO_DET1_EXP_FLIST_BUF, features.data(), sizeof(xy_hw) * fp_list1.field.FPListLength);
        }
    } else {
        VIO_DET1_TOT_FNUMBER_T &fp_list1 = vio_det1->VIO_DET1_TOT_FNUMBER;
        fp_list1.val = READ_REG(DET1_TOT_FNUMBER);
        uint32_t num_fp = min(uint32_t (num_wanted), uint32_t(fp_list1.field.FPListLength));
        features.resize(num_fp);
        move_dma_2_cpu(VIO_DET1_SORTEDFPLIST_BUF, features.data(), sizeof(xy_hw) * num_fp);
    }
    PROFILE_END(DET_DATA_RX);

    PROFILE_REPORT(DET_DATA_TX);
    PROFILE_REPORT(DET_PROC);
    PROFILE_REPORT(DET_DATA_RX);
    return features;
}



vector<xy_hw, Fast_Alloc<xy_hw> > &
fast_detector_9_hw::detect2(const unsigned char *im, const scaled_mask *mask,
                            int num_wanted, int bthresh, int winx, int winy,
                            int winwidth, int winheight, bool centerWt)
{
    PRINTF("fast_detector_9_hw::detect()\n");
    PRINTF("detect mode: %s\n", g_det_mode.c_str());
    uint8_t *buf = mask ? mask->compressed : m_det2_mask;

    if (num_wanted>256) {
      printf("[FRM2] num_wanted (%d) is more than 256\n", num_wanted);
      num_wanted=200;
    }

    assert(winwidth < 1080);
    assert(winheight < 1080);

    // Move data to HW
    PROFILE_START(DET_DATA_TX);
    move_dma_2_hw(im, VIO_IMG2_BUF, get_frame_size());
    move_dma_2_hw(buf, VIO_DET2_ROIMASK_BUF, get_mask_size());
    PROFILE_END(DET_DATA_TX);

    PROFILE_START(DET_PROC);
    VIO_TOP_CONTROL_T &top_ctrl = vio_top->VIO_TOP_CONTROL;
    VIO_DET2_STATUS_T &status2    = vio_det2->VIO_DET2_STATUS;
    VIO_DET2_CONTROL_T &control2  = vio_det2->VIO_DET2_CONTROL;
    control2.field.DetectorThreshold    = bthresh;
    control2.field.FeatureDetectorStart = 0;
    control2.field.SorterStart          = 0;
    control2.field.AutoStartSorting     = 0;
    control2.field.SortingWorkingSetSize = 0;
    control2.field.CenterWeightingEn    = centerWt;
    control2.field.RegionMaskEn         = mask != NULL ? 1 : 0;
    control2.field.ExportAllFeatures    = 0;//check later whether it matters for legacy
    control2.field.ImageBufferSpecifier = 0;//1 or 2 for detector 1 and 2 
    control2.field.EnDynamicThreshold   = 0;
    
    // Config Write
    top_ctrl.field.GlobalIntrDisable  = 1;
    top_ctrl.field.ClearConfigWrError = 1;
    WRITE_REG(TOP_CONTROL, top_ctrl.val);

    vio_det2->VIO_DET2_SORTED_FPLIST_LENGTH.field.FPListLength = (num_wanted);
    WRITE_REG(DET2_SORTED_FPLIST_LENGTH, vio_det2->VIO_DET2_SORTED_FPLIST_LENGTH.val);

    if(g_det_mode == "detect" || g_det_mode == "all-1-by-1") {
        // Feature Detector
        control2.field.FeatureDetectorStart = 1;
        WRITE_REG(DET2_CONTROL, control2.val);
        status2.field.FeatureDetectorDone = 0;
        POLL_N_WAIT(DET2_STATUS, status2.val, status2.field.FeatureDetectorDone == 0);
    }
    
    if(g_det_mode == "all-1-by-1") {
        PRINTF("detect done !!\nStarting sorter .. \n");
        // Run Sorter
        control2.field.FeatureDetectorStart = 0;
        control2.field.SorterStart = 1;
        WRITE_REG(DET2_CONTROL, control2.val);
        status2.val = 0;
        POLL_N_WAIT(DET2_STATUS, status2.val, status2.field.SorterDone == 0);
    }

    if(g_det_mode == "all-together") {
        control2.field.FeatureDetectorStart = 1;
        control2.field.SorterStart = 0;
        control2.field.AutoStartSorting  = 1;
        WRITE_REG(DET2_CONTROL, control2.val);
        status2.val = 0;
        POLL_N_WAIT(DET2_STATUS, status2.val, status2.val != 0x03);
        usleep(2); // HACK: As sorter/arbiter seems to taking time in write-back to SRAM
    }


    // Read back FP
    PROFILE_START(DET_DATA_RX);
    if(g_det_mode == "detect") {
        VIO_DET2_TOT_FNUMBER_T &fp_list2 = vio_det2->VIO_DET2_TOT_FNUMBER;
        fp_list2.val = READ_REG(DET2_TOT_FNUMBER);
        features.resize(fp_list2.field.FPListLength);
        if(features.size()) {
            move_dma_2_cpu(VIO_DET2_EXP_FLIST_BUF, features.data(), sizeof(xy_hw) * fp_list2.field.FPListLength);
        }
    } else {
        VIO_DET2_TOT_FNUMBER_T &fp_list2 = vio_det2->VIO_DET2_TOT_FNUMBER;
        fp_list2.val = READ_REG(DET2_TOT_FNUMBER);
        uint32_t num_fp = min(uint32_t (num_wanted), uint32_t(fp_list2.field.FPListLength));
        features.resize(num_fp);
        move_dma_2_cpu(VIO_DET2_SORTEDFPLIST_BUF, features.data(), sizeof(xy_hw) * num_fp);
    }
    PROFILE_END(DET_DATA_RX);

    PROFILE_REPORT(DET_DATA_TX);
    PROFILE_REPORT(DET_PROC);
    PROFILE_REPORT(DET_DATA_RX);
    return features;
}


void
fast_detector_9_hw::detect_otf_pre1(const unsigned char *im, const scaled_mask *mask,
                                   int num_wanted, int bthresh, int winx, int winy,
                                   int winwidth, int winheight, bool centerWt)
{
    uint8_t *buf = mask ? mask->compressed : m_det1_mask;

    const uint32_t max_supported = VIO_DET1_SORTEDFPLIST_BUFSIZE / sizeof(Patch_t);
    ASSERT(num_wanted <= max_supported, "[OTF] num_wanted (%d) is more than supported (%d)", num_wanted, max_supported);

    ASSERT(winwidth < 1024, "winwidth is more thn 1024");
    ASSERT(winheight < 1024, "winwidth is more thn 1024");

    // Move data to HW
    move_dma_2_hw(buf, VIO_DET1_ROIMASK_BUF, get_mask_size()); 

    VIO_TOP_CONTROL_T &top_ctrl    = vio_top->VIO_TOP_CONTROL;
    VIO_DET1_CONTROL_T &det1_ctrl  = vio_det1->VIO_DET1_CONTROL;
    VIO_CB1_CONTROL_T &cb1_ctrl    = vio_cb1->VIO_CB1_CONTROL;
        
    // Top_Config Write
    top_ctrl.field.GlobalIntrDisable  = 1;
    top_ctrl.field.ClearConfigWrError = 1;
    WRITE_REG(TOP_CONTROL, top_ctrl.val);

    // CB_config Write
    cb1_ctrl.field.ClearCirBuf1Overflow = 1;
    cb1_ctrl.field.CircularBuffer1Depth = 0;
    WRITE_REG(CB1_CONTROL, cb1_ctrl.val); 
    
    // Det_Config Write
    vio_det1->VIO_DET1_SORTED_FPLIST_LENGTH.field.FPListLength = (num_wanted);
    WRITE_REG(DET1_SORTED_FPLIST_LENGTH, vio_det1->VIO_DET1_SORTED_FPLIST_LENGTH.val);
    
    det1_ctrl.field.DetectorThreshold     = bthresh;
    det1_ctrl.field.FeatureDetectorStart  = 0;
    det1_ctrl.field.SorterStart           = 0;
    det1_ctrl.field.AutoStartSorting      = 0;
    det1_ctrl.field.SortingWorkingSetSize = DET_WORK_SIZE;
    det1_ctrl.field.CenterWeightingEn     = centerWt;
    det1_ctrl.field.RegionMaskEn          = mask != NULL ? 1 : 0;
    if (g_det_mode == "otf") { 
      det1_ctrl.field.ExportAllFeatures     = 0;
      det1_ctrl.field.ExportAllDescriptors  = 0;
    } else if (g_det_mode == "otf-detect"){
      printf("\\OTF-DETECT MODE Export features set\\");
      det1_ctrl.field.ExportAllFeatures     = 1;
      det1_ctrl.field.ExportAllDescriptors  = 1;
    }
    det1_ctrl.field.ImageBufferSpecifier  = 1;//1 or 2 for detector 1 and 2 
    det1_ctrl.field.EnDynamicThreshold    = 0;
    det1_ctrl.field.FeatureDetectorStart  = 1;
    WRITE_REG(DET1_CONTROL, det1_ctrl.val);
    
    det1_ctrl.field.FeatureDetectorStart  = 1;
    WRITE_REG(DET1_CONTROL, det1_ctrl.val);

    // ISRC Config
    ISRC_WRITE_REG(ISRC1_CTRL, 0x32);
}

vector<xy_hw, Fast_Alloc<xy_hw> > &
fast_detector_9_hw::detect_otf_post1()
{
    VIO_DET1_STATUS_T &det1_status = vio_det1->VIO_DET1_STATUS;
    VIO_CB1_STATUS_T &cb1_status   = vio_cb1->VIO_CB1_STATUS;
    
    det1_status.val = 0;
    cb1_status.val  = 0;
    //POLL_N_WAIT(CB1_STATUS, cb1_status.val, cb1_status.field.CircularBuffer1Done == 0);
    POLL_N_WAIT(DET1_STATUS, det1_status.val, det1_status.field.FeatureDetectorDone == 0);
    WARN(det1_status.field.CircularBufferOverflow, "Circular Buffer Overflow \n");
    
    // Read back FP
    vio_det1->VIO_DET1_SORTED_FPLIST_LENGTH.val = READ_REG(DET1_SORTED_FPLIST_LENGTH);
    uint32_t num_wanted = vio_det1->VIO_DET1_SORTED_FPLIST_LENGTH.field.FPListLength;
    
    
    VIO_DET1_TOT_FNUMBER_T &fp_list1 = vio_det1->VIO_DET1_TOT_FNUMBER;
    fp_list1.val = READ_REG(DET1_TOT_FNUMBER);
    uint32_t num_fp = min(uint32_t (num_wanted), uint32_t(fp_list1.field.FPListLength));
    uint32_t total_detect =  fp_list1.field.FPListLength;
    printf("\\Total detected points: %d\n", total_detect);
    if (g_det_mode == "otf"){
      patches.resize(num_fp);
      move_dma_2_cpu(VIO_DET1_SORTEDFPLIST_BUF, patches.data(), sizeof(Patch_t) * num_fp);
      features.resize(num_fp);
      for(uint32_t id = 0; id < num_fp; id++) {
        features[id] = patches[id].xy;
      }
    } else if (g_det_mode == "otf-detect"){
      patches.resize(total_detect);
      features.resize(total_detect);
      move_dma_2_cpu(VIO_DET1_EXP_FLIST_BUF, features.data(), sizeof(xy_hw) * total_detect);
      move_dma_2_cpu(VIO_DET1_WRK_BUFA_BUF, patches.data(), sizeof(Patch_t) * total_detect);
      for(uint32_t id = 0; id < total_detect; id++) {
        printf("X:%d, Y:%d, Score:%d\n", features[id].x, features[id].y, features[id].score);
      } 

       /* for(uint32_t id = 0; id < total_detect; id++) {
        patches[id].xy = features[id];
	} */
    }
    return features;
}    


vector<xy_hw, Fast_Alloc<xy_hw> > &
fast_detector_9_hw::detect_otf1(const unsigned char *im, const scaled_mask *mask,
                               int num_wanted, int bthresh, int winx, int winy,
                               int winwidth, int winheight, bool centerWt)
{
    PRINTF("fast_detector_9_hw::detect_otf()\n");
    PRINTF("detect mode: %s\n", g_det_mode.c_str());
    
    PROFILE_START(DET_OTF_PRE);    
    detect_otf_pre1(im, mask, num_wanted, bthresh, winx, winy,
                   winwidth, winheight, centerWt);
    PROFILE_END(DET_OTF_PRE);

    PROFILE_START(DET_IMAGE_TX);
    mipi_dma_2_hw(im, VIO_MIPI1_BUF, get_frame_size(), false/*nb*/);
    PROFILE_END(DET_IMAGE_TX);

    PRINTF("MIPI TX completed\n"); 
    //check_dma_2_hw(im, VIO_CB1_MEM_BA, get_frame_size());

    PROFILE_START(DET_OTF_POST);
    detect_otf_post1();
    PROFILE_END(DET_OTF_POST);
    

    PROFILE_REPORT(DET_OTF_PRE);
    PROFILE_REPORT(DET_IMAGE_TX);
    PROFILE_REPORT(DET_OTF_POST);
    return features;
}


void
fast_detector_9_hw::detect_otf_pre2(const unsigned char *im, const scaled_mask *mask,
                                   int num_wanted, int bthresh, int winx, int winy,
                                   int winwidth, int winheight, bool centerWt)
{
    uint8_t *buf = mask ? mask->compressed : m_det2_mask;

    const uint32_t max_supported = VIO_DET2_SORTEDFPLIST_BUFSIZE / sizeof(Patch_t);
    ASSERT(num_wanted <= max_supported, "[OTF] num_wanted (%d) is more than supported (%d)", num_wanted, max_supported);

    ASSERT(winwidth < 1024, "winwidth is more thn 1024");
    ASSERT(winheight < 1024, "winwidth is more thn 1024");

    // Move data to HW
    move_dma_2_hw(buf, VIO_DET2_ROIMASK_BUF, get_mask_size()); 

    VIO_TOP_CONTROL_T &top_ctrl    = vio_top->VIO_TOP_CONTROL;
    VIO_DET2_CONTROL_T &det2_ctrl  = vio_det2->VIO_DET2_CONTROL;
    VIO_CB2_CONTROL_T &cb2_ctrl    = vio_cb2->VIO_CB2_CONTROL;
        
    // Top_Config Write
    top_ctrl.field.GlobalIntrDisable  = 1;
    top_ctrl.field.ClearConfigWrError = 1;
    WRITE_REG(TOP_CONTROL, top_ctrl.val);

    // CB_config Write
    cb2_ctrl.field.ClearCirBuf2Overflow = 1;
    cb2_ctrl.field.CircularBuffer2Depth = 0;
    WRITE_REG(CB2_CONTROL, cb2_ctrl.val); 
   
    // Det_Config Write
    vio_det2->VIO_DET2_SORTED_FPLIST_LENGTH.field.FPListLength = (num_wanted);
    WRITE_REG(DET2_SORTED_FPLIST_LENGTH, vio_det2->VIO_DET2_SORTED_FPLIST_LENGTH.val);
    
    det2_ctrl.field.DetectorThreshold     = bthresh;
    det2_ctrl.field.FeatureDetectorStart  = 0;
    det2_ctrl.field.SorterStart           = 0;
    det2_ctrl.field.AutoStartSorting      = 0;
    det2_ctrl.field.SortingWorkingSetSize = DET_WORK_SIZE;
    det2_ctrl.field.CenterWeightingEn     = centerWt;
    det2_ctrl.field.RegionMaskEn          = mask != NULL ? 1 : 0;
    if (g_det_mode == "otf") { 
      det2_ctrl.field.ExportAllFeatures     = 0;
      det2_ctrl.field.ExportAllDescriptors  = 0;
    } else if (g_det_mode == "otf-detect"){
      printf("\\OTF-DETECT MODE Export features set\\");
      det2_ctrl.field.ExportAllFeatures     = 1;
      det2_ctrl.field.ExportAllDescriptors  = 1;
    }
    det2_ctrl.field.ImageBufferSpecifier  = 2;//1 or 2 for detector 1 and 2 
    det2_ctrl.field.EnDynamicThreshold    = 0;
    det2_ctrl.field.FeatureDetectorStart  = 1;
    WRITE_REG(DET2_CONTROL, det2_ctrl.val);
    
    // det2_ctrl.field.FeatureDetectorStart  = 1;
    //WRITE_REG(DET2_CONTROL, det2_ctrl.val);

    // ISRC Config
    ISRC_WRITE_REG(ISRC2_CTRL, 0x32);
}

vector<xy_hw, Fast_Alloc<xy_hw> > &
fast_detector_9_hw::detect_otf_post2()
{
    VIO_DET2_STATUS_T &det2_status = vio_det2->VIO_DET2_STATUS;
    VIO_CB2_STATUS_T &cb2_status   = vio_cb2->VIO_CB2_STATUS;
    
    det2_status.val = 0;
    cb2_status.val  = 0;

    //POLL_N_WAIT(CB1_STATUS, cb1_status.val, cb1_status.field.CircularBuffer1Done == 0);
    POLL_N_WAIT(DET2_STATUS, det2_status.val, det2_status.field.FeatureDetectorDone == 0);
    WARN(det2_status.field.CircularBufferOverflow, "Circular Buffer Overflow \n");
    
    // Read back FP
    vio_det2->VIO_DET2_SORTED_FPLIST_LENGTH.val = READ_REG(DET2_SORTED_FPLIST_LENGTH);
    uint32_t num_wanted = vio_det2->VIO_DET2_SORTED_FPLIST_LENGTH.field.FPListLength;
    
    
    VIO_DET2_TOT_FNUMBER_T &fp_list2 = vio_det2->VIO_DET2_TOT_FNUMBER;
    fp_list2.val = READ_REG(DET2_TOT_FNUMBER);
    uint32_t num_fp = min(uint32_t (num_wanted), uint32_t(fp_list2.field.FPListLength));
    uint32_t total_detect =  fp_list2.field.FPListLength;
    printf("\\Total detected points: %d\n", total_detect);
    if (g_det_mode == "otf"){
      patches.resize(num_fp);
      move_dma_2_cpu(VIO_DET2_SORTEDFPLIST_BUF, patches.data(), sizeof(Patch_t) * num_fp);
      features.resize(num_fp);
      for(uint32_t id = 0; id < num_fp; id++) {
        features[id] = patches[id].xy;
      }
    } else if (g_det_mode == "otf-detect"){
      patches.resize(total_detect);
      features.resize(total_detect);
      move_dma_2_cpu(VIO_DET2_EXP_FLIST_BUF, features.data(), sizeof(xy_hw) * total_detect);
      move_dma_2_cpu(VIO_DET2_WRK_BUFA_BUF, patches.data(), sizeof(Patch_t) * total_detect);
      for(uint32_t id = 0; id < total_detect; id++) {
        printf("X:%d, Y:%d, Score:%d\n", features[id].x, features[id].y, features[id].score);
      } 

       /* for(uint32_t id = 0; id < total_detect; id++) {
        patches[id].xy = features[id];
	} */
    }
    return features;
}    


vector<xy_hw, Fast_Alloc<xy_hw> > &
fast_detector_9_hw::detect_otf2(const unsigned char *im, const scaled_mask *mask,
                               int num_wanted, int bthresh, int winx, int winy,
                               int winwidth, int winheight, bool centerWt)
{
    PRINTF("fast_detector_9_hw::detect_otf()\n");
    PRINTF("detect mode: %s\n", g_det_mode.c_str());
    
    PROFILE_START(DET_OTF_PRE);    
    detect_otf_pre2(im, mask, num_wanted, bthresh, winx, winy,
                   winwidth, winheight, centerWt);
    PROFILE_END(DET_OTF_PRE);

    PROFILE_START(DET_IMAGE_TX);
    mipi_dma_2_hw(im, VIO_MIPI2_BUF, get_frame_size(), false/*nb*/);
    PROFILE_END(DET_IMAGE_TX);

    PRINTF("MIPI TX completed\n"); 
    //check_dma_2_hw(im, VIO_CB1_MEM_BA, get_frame_size());

    PROFILE_START(DET_OTF_POST);
    detect_otf_post2();
    PROFILE_END(DET_OTF_POST);
    

    PROFILE_REPORT(DET_OTF_PRE);
    PROFILE_REPORT(DET_IMAGE_TX);
    PROFILE_REPORT(DET_OTF_POST);
    return features;
} 


xy_hw *
fast_detector_9_hw::get_det1_fp(uint32_t &num_fp1)
{
    VIO_DET1_TOT_FNUMBER_T &fp_list1 = vio_det1->VIO_DET1_TOT_FNUMBER;
    fp_list1.val = READ_REG(DET1_TOT_FNUMBER);
#if 0
    num_fp1 = fp_list1.field.FPListLength;
    return (xy_hw *)get_sram_buf(VIO_DET1_FPLIST_BUF);
#else
    VIO_DET1_SORTED_FPLIST_LENGTH_T &sort1 = vio_det1->VIO_DET1_SORTED_FPLIST_LENGTH;
    sort1.val = READ_REG(DET1_SORTED_FPLIST_LENGTH);
    num_fp1 = min(uint32_t(sort1.field.FPListLength), uint32_t(fp_list1.field.FPListLength));
    return (xy_hw *)get_sram_buf(VIO_DET1_SORTEDFPLIST_BUF);
#endif
}

xy_hw *
fast_detector_9_hw::get_det2_fp(uint32_t &num_fp2)
{
    VIO_DET2_TOT_FNUMBER_T &fp_list2 = vio_det2->VIO_DET2_TOT_FNUMBER;
    fp_list2.val = READ_REG(DET2_TOT_FNUMBER);
#if 0
    num_fp2 = fp_list2.field.FPListLength;
    return (xy_hw *)get_sram_buf(VIO_DET2_FPLIST_BUF);
#else
    VIO_DET2_SORTED_FPLIST_LENGTH_T &sort2 = vio_det2->VIO_DET2_SORTED_FPLIST_LENGTH;
    sort2.val = READ_REG(DET2_SORTED_FPLIST_LENGTH);
    num_fp2 = min(uint32_t(sort2.field.FPListLength), uint32_t(fp_list2.field.FPListLength));
    return (xy_hw *)get_sram_buf(VIO_DET2_SORTEDFPLIST_BUF);
#endif
}



void
fast_detector_9_hw::move_image_2_hw(const uint8_t *im, size_t size, bool nb)
{
    move_dma_2_hw(im, VIO_IMG1_BUF, size, nb);
}

void
fast_detector_9_hw::wait_image_tx()
{
    wait_dma_wr();
}

void
fast_detector_9_hw::track_pre(uint32_t num_pred, uint32_t num_patch, const Patch_t *patch_list,
                              const xy_hw *pred_list, const uint8_t *im,
                              int threshold, bool centerWt)
{
    const uint32_t max_patches = VIO_TRK_FPPATCHLIST_BUFSIZE / sizeof(Patch_t);
    const uint32_t max_pred    = VIO_TRK_FPPREDLIST_BUFSIZE / sizeof(xy_hw);
    PRINTF("fast_detector_9_hw::track_pre()\n");
    ASSERT(num_pred > 0, "Number of track point is set to zero\n");
    ASSERT(num_pred < 384, "Number of prediction points are more than 128*3\n");
    ASSERT(num_patch < max_patches, "Number of track patches are more than %d\n", max_patches);

    VIO_TOP_CONTROL_T &top_ctrl  = vio_top->VIO_TOP_CONTROL;
    VIO_TRK_CONTROL_T &ctrl      = vio_trk->VIO_TRK_CONTROL;

    // Move data to HW
    move_dma_2_hw(im, VIO_IMG1_BUF, get_frame_size());
    move_dma_2_hw(patch_list, VIO_TRK_FPPATCHLIST_BUF, num_patch * sizeof(Patch_t));
    move_dma_2_hw(pred_list, VIO_TRK_FPPREDLIST_BUF, num_pred * sizeof(xy_hw));

    // Top Control Config
    top_ctrl.field.GlobalIntrDisable  = 1;
    top_ctrl.field.ClearConfigWrError = 1;
    WRITE_REG(TOP_CONTROL, top_ctrl.val);

    // Tracker Config
    vio_trk->VIO_TRK_FPPREDLIST_LENGTH.field.FPPredListLength = num_pred;
    WRITE_REG(TRK_FPPREDLIST_LENGTH, vio_trk->VIO_TRK_FPPREDLIST_LENGTH.val);

    vio_trk->VIO_TRK_FPPATCHLIST_LENGTH.field.FPPatchListLength = num_patch;
    WRITE_REG(TRK_FPPATCHLIST_LENGTH, vio_trk->VIO_TRK_FPPATCHLIST_LENGTH.val);
    
    ctrl.field.TrackerThreshold     = threshold;
    ctrl.field.CenterWeightingEn    = centerWt;
    ctrl.field.ImageBufferSpecifier = 0;//3 to tracker for same img :: 1 for both buffer tracker with diff images 

    return;
}

void
fast_detector_9_hw::track_trigger()
{
    VIO_TRK_CONTROL_T &ctrl        = vio_trk->VIO_TRK_CONTROL;
    ctrl.field.FeatureTrackerStart = 1;
    WRITE_REG(TRK_CONTROL, ctrl.val);
}


void
fast_detector_9_hw::track_pre_otf(uint32_t num_pred, uint32_t num_patch, const Patch_t *patch_list,
                                  const xy_hw *pred_list, const uint8_t *im,
                                  int threshold, bool centerWt)
{
    const uint32_t max_patches = VIO_TRK_FPPATCHLIST_BUFSIZE / sizeof(Patch_t);
    PRINTF("fast_detector_9_hw::track_pre_otf()\n");
    ASSERT(num_pred > 0, "Number of track point is set to zero\n");
    ASSERT(num_pred < 384, "Number of prediction points are more than 128*3\n");
    ASSERT(num_patch < max_patches, "Number of track patches are more than %d\n", max_patches);

    VIO_TOP_CONTROL_T &top_ctrl  = vio_top->VIO_TOP_CONTROL;
    VIO_TRK_CONTROL_T &ctrl      = vio_trk->VIO_TRK_CONTROL;
    VIO_CB1_CONTROL_T &cb1_ctrl  = vio_cb1->VIO_CB1_CONTROL;

    // Move data to HW
    move_dma_2_hw(patch_list, VIO_TRK_FPPATCHLIST_BUF, num_patch * sizeof(Patch_t));
    move_dma_2_hw(pred_list, VIO_TRK_FPPREDLIST_BUF, num_pred * sizeof(xy_hw));

    // Top Control Config
    top_ctrl.field.GlobalIntrDisable  = 1;
    top_ctrl.field.ClearConfigWrError = 1;
    WRITE_REG(TOP_CONTROL, top_ctrl.val);

    // CB Config
    cb1_ctrl.field.ClearCirBuf1Overflow = 1;
    cb1_ctrl.field.CircularBuffer1Depth = 0;
    WRITE_REG(CB1_CONTROL, cb1_ctrl.val);

    
    // Tracker Config
    vio_trk->VIO_TRK_FPPREDLIST_LENGTH.field.FPPredListLength = num_pred;
    WRITE_REG(TRK_FPPREDLIST_LENGTH, vio_trk->VIO_TRK_FPPREDLIST_LENGTH.val);

    vio_trk->VIO_TRK_FPPATCHLIST_LENGTH.field.FPPatchListLength = num_patch;
    WRITE_REG(TRK_FPPATCHLIST_LENGTH, vio_trk->VIO_TRK_FPPATCHLIST_LENGTH.val);
    
    ctrl.field.TrackerThreshold     = threshold;
    ctrl.field.CenterWeightingEn    = centerWt;
    ctrl.field.ImageBufferSpecifier = 1;
    ctrl.field.FeatureTrackerStart  = 1;
    WRITE_REG(TRK_CONTROL, ctrl.val);
    return;
}

void
fast_detector_9_hw::track_wait()
{
    VIO_TRK_STATUS_T  &status    = vio_trk->VIO_TRK_STATUS; 
    VIO_TRK_CONTROL_T &ctrl      = vio_trk->VIO_TRK_CONTROL;
    
    // Poll
    status.val = 0xfff;
    POLL_N_WAIT(TRK_STATUS, status.val, status.field.FeatureTrackerDone == 0);
}

uint32_t
fast_detector_9_hw::track_post(xy_hw *track_list)
{
    // Read output
    vio_trk->VIO_TRK_FPPREDLIST_LENGTH.val = READ_REG(TRK_FPPREDLIST_LENGTH);
    uint32_t num_pred = vio_trk->VIO_TRK_FPPREDLIST_LENGTH.field.FPPredListLength;
    move_dma_2_cpu(VIO_TRK_FPTRACKLIST_BUF, track_list, sizeof(xy_hw) * num_pred);
    return num_pred;
}

void
fast_detector_9_hw::track_list(uint32_t num_pred, uint32_t num_patch, const Patch_t *patch_list,
                               const xy_hw *pred_list, const uint8_t *im,
                               int threshold, xy_hw *track_list, bool centerWt, bool otf_mode)
{
    if(otf_mode) {
        return track_list_otf(num_pred, num_patch, patch_list, pred_list, im, threshold,
                              track_list, centerWt);
    }
    PRINTF("fast_detector_9_hw::track_list()\n");

    PROFILE_START(TRK_DATA_TX);
    track_pre(num_pred, num_patch, patch_list, pred_list, im, threshold, centerWt);
    PROFILE_END(TRK_DATA_TX); 
    
    PROFILE_START(TRK_PROC);
    track_trigger();
    track_wait(); 
    PROFILE_END(TRK_PROC);

    PROFILE_START(TRK_DATA_RX);
    track_post(track_list);
    PROFILE_END(TRK_DATA_RX);

    PROFILE_REPORT(TRK_DATA_TX);
    PROFILE_REPORT(TRK_PROC);
    PROFILE_REPORT(TRK_DATA_RX);
}

void
fast_detector_9_hw::track_list_otf(uint32_t num_pred, uint32_t num_patch, const Patch_t *patch_list,
                                   const xy_hw *pred_list, const uint8_t *im,
                                   int threshold, xy_hw *track_list, bool centerWt)
{
    PRINTF("fast_detector_9_hw::track_list_otf()\n");
    
    PROFILE_START(TRK_DATA_TX);
    track_pre_otf(num_pred, num_patch, patch_list, pred_list, im, threshold, centerWt);
    PROFILE_END(TRK_DATA_TX);

    ISRC_WRITE_REG(ISRC1_CTRL, 0x32);
    mipi_dma_2_hw(im, VIO_MIPI1_BUF, get_frame_size(), false /*nb*/);
    
    PROFILE_START(TRK_PROC);
    track_wait(); 
    PROFILE_END(TRK_PROC);

    PROFILE_START(TRK_DATA_RX);
    track_post(track_list);
    PROFILE_END(TRK_DATA_RX);

    PROFILE_REPORT(TRK_DATA_TX);
    PROFILE_REPORT(TRK_PROC);
    PROFILE_REPORT(TRK_DATA_RX);    
}

xy_hw *
fast_detector_9_hw::get_trk_predlist(size_t num_fps)
{
    assert(m_trk_predlist); 
    assert(num_fps * sizeof(xy_hw) <= VIO_TRK_FPPREDLIST_BUFSIZE);
    return m_trk_predlist;
}

Patch_t *
fast_detector_9_hw::get_trk_patchlist(size_t num_fps)
{
    assert(m_trk_patchlist); 
    assert(num_fps * sizeof(Patch_t) <= VIO_TRK_FPPATCHLIST_BUFSIZE);
    return m_trk_patchlist;
}

xy_hw *
fast_detector_9_hw::get_trk_tracklist(size_t num_fps)
{
    // if(m_trk_tracklist == NULL) {
    //     m_trk_tracklist = (xy_hw *)malloc(VIO_TRK_FPTRACKLIST_BUFSIZE);
    //    assert(m_trk_tracklist);
    // }
    assert(m_trk_tracklist); 
    assert(num_fps * sizeof(xy_hw) <= VIO_TRK_FPTRACKLIST_BUFSIZE);
    return m_trk_tracklist;
}

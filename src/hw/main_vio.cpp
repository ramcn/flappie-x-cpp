#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <assert.h>
#include "fast_hw.h"
#include "fast_constants.h"
#include <algorithm>
#include "ekf_hw.h"
#define _BSD_SOURCE
#include <sys/time.h>
#include <random>


int main (int argc, char *argv[])
{



    ekf_hw hw; fast_detector_9 fast;
     

    hw.cholesky_decomp(res_cov, res_cov_hw, chk_cols, chk_str);

    hw.mat_sol(chol_out, lc_cov, gainx_hw, lchk_rows, lc_rows, lchk_str, lc_str);

    hw.mat_mul(gain_cov, gain_cov, cov_in, cov_out_hw, gain_cols, gain_rows, gain_cols -1,
               false, true, -1.0f, 1.0f, gain_str, gain_str, cov_in_str);


    fast.detect(copyimg_data, &mask, num_points, threshold, 0, 0, copyimg.cols, copyimg.rows);
    fast.init(copyimg.cols, copyimg.rows, copyimg.step, 7, 3);
    fast.track(patch_list[patch_idx].buf, finalimg_data, half_patch_width, half_patch_width, pred.x, pred.y,
               fast_track_radius, tthreshold, fast_min_match);



}

#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include <vector>
#include <map>
#include <assert.h>
#include <algorithm>
#include <fstream>
using namespace std;

#include "ekf_hw.h"
#include "vio_regs.h"
#include "vio_utils.h"

#define CHECK_ARGS(v, s1, s2, s3, s4, s5)                               \
    if((v != s1) && (v != s2) && (v != s3) && (v != s4) && (v != s5)) { \
    fprintf(stderr, "Invalid %s, valid values:{%s, %s, %s, %s, %s}\n", #v, #s1, #s2, #s3, #s4, #s5); \
    exit(1);                                                            \
    }

#define LOAD 1


inline void load_file(string file, void *buf, uint32_t size, const char *str,
                      uint32_t &rows, uint32_t &cols, uint32_t &stride, bool flip = false,
                      size_t elm_size=sizeof(float), uint32_t new_stride = 0)
{
    std::ifstream is(file);
    ASSERT(is.is_open(), "Couldn't open file %s\n", file.c_str());
    string line; int32_t line_num = -1; uint32_t rd_size = 0;

    if (flip == 1);
    {
      printf("flip is on\n");
    }
    
    void *tmp = NULL; 
    uint64_t *dst = NULL;
    if(new_stride) {
        tmp = malloc(size);
        dst = (uint64_t *)tmp;
    } else {
        dst = (uint64_t *)buf;
    }
    while(!is.eof()) {
        line_num++;
        std::getline(is, line);
        if(is.eof()) {break;}
        if(line_num == 0) {
            int status = sscanf(line.c_str(), str, &cols, &rows, &stride);
            ASSERT(status ==3, "Couldn't read file header, file:%s, \nstr   :%s, \nheader:%s\n",
                   file.c_str(), str, line.c_str());
            continue;
        }
        if(line.find("//") != std::string::npos) {continue;}
        ASSERT(line.size() == 32, "Incorrect file %s at line:%d \n",
               file.c_str(), line_num);

        int status;
        status = sscanf(line.substr(16,16).c_str(), "%llx", dst++);
        ASSERT(status == 1, "Incorrect file %s at line:%d \n", file.c_str(), line_num);
        status = sscanf(line.substr( 0,16).c_str(), "%llx", dst++);
        ASSERT(status == 1, "Incorrect file %s at line:%d \n", file.c_str(), line_num);
        rd_size += 16;
    }
    ASSERT(rd_size <= size, "Provided buffer size insufficient, file:%s, rd_size:%d, size:%d\n",
           file.c_str(), rd_size, size);
    uint32_t exp_size = elm_size * stride * (flip ? cols : rows);
    printf("elm_size=%d, stride=%d\n", elm_size, stride);
    ASSERT(exp_size == rd_size, "file:%s exp_size: 0x%x, rd_size: 0x%x\n",
	   file.c_str(), exp_size, rd_size);
    is.close();

    if(new_stride) {
        uint32_t _cols = flip ? rows : cols;
        uint32_t _rows = flip ? cols : rows;
        uint8_t *_src  = (uint8_t *)tmp; uint8_t *_dst = (uint8_t*)buf;
        for(uint32_t i = 0; i < _rows; i++) {
            memcpy(_dst, _src, _cols * elm_size);
            _dst += new_stride * elm_size;
            _src += stride * elm_size;
        }
        stride = new_stride;
	printf("new_stride =%d\n", stride);
        free(tmp);
    }

}


#define MAX_SHOW_COUNT 10
int main (int argc, char *argv[])
{
    // Command Args
    if(argc < 2) {
        printf("Usage: <exe> <input dir> <mode> <mode_1> <mode_2> <mode_3> [<dim>] \n");
        exit(1);
    }
    string mode = argc > 2 ? argv[2] : "";
    string mode_1 = argc > 3 ? argv[3] : "";
    string mode_2 = argc > 4 ? argv[4] : "";
    string mode_3 = argc > 5 ? argv[5] : "";
    CHECK_ARGS(mode  , "cholesky", "mat_sol", "cov_update", "state_update", "all");
    CHECK_ARGS(mode_1, ""        , "mat_sol", "cov_update", "state_update", ""   );
    CHECK_ARGS(mode_2, ""        , ""       , "cov_update", "state_update", ""   );
    CHECK_ARGS(mode_3, ""        , ""       , "cov_update", "state_update", ""   );

    string dir        = argv[1];
    string chk_file   = dir + "/CHL_In_Matrix.txt";
    string lchk_file   = dir + "/CHL_Out_Matrix.txt";
    string lc_file    = dir + "/MAT_Cov_LC.txt";
    string gain_file  = dir + "/MAT_Cov_Gain.txt";
    string gain_t_file  = dir + "/MAT_Cov_Gain_t.txt";
    string cov_in_file = dir + "/MAT_Cov_In.txt";
    float *res_cov    = (float *)vio_malloc(VIO_EKF_CHOL_IN_BUFSIZE);
    float *result_cov    = (float *)vio_malloc(VIO_EKF_CHOL_IN_BUFSIZE);
    float *lc_cov     = (float *)vio_malloc(VIO_EKF_MMUL_A_BUFSIZE);
    float *gain_cov   = (float *)vio_malloc(VIO_EKF_MMUL_B_BUFSIZE);
    float *cov_in    = (float *)vio_malloc(VIO_EKF_MMUL_C_BUFSIZE);
    uint32_t m, s, stride_1, stride_2;
    uint32_t chk_rows, chk_cols, chk_str;
    uint32_t lchk_rows, lchk_cols, lchk_str;
    uint32_t lc_rows, lc_cols, lc_str;
    uint32_t gain_rows, gain_cols, gain_str;
    uint32_t cov_in_rows, cov_in_cols, cov_in_str;
    chk_str  = (chk_rows + 0x3) & ~0x3;
    
#if CREATE_HISTO
    memset(histo_diff , 0x0, sizeof(histo_diff));
    memset(histo_ratio, 0x0, sizeof(histo_ratio));
#endif
    
#if LOAD
    load_file(chk_file, res_cov, VIO_EKF_CHOL_IN_BUFSIZE,
              "//mat_size col= %d, row=%d, stride = %d", chk_rows, chk_cols, chk_str);
    ASSERT(chk_rows == chk_cols, "Not a squre matrix (cholesky), rows:%d, cols:%d\n",
           "//mat_size col= %d, row=%d, stride = %d", chk_rows, chk_cols, chk_str);

    load_file(lchk_file, result_cov, VIO_EKF_CHOL_IN_BUFSIZE,
              "//mat_size col= %d, row=%d, stride = %d", lchk_rows, lchk_cols, lchk_str);
    ASSERT(lchk_rows == lchk_cols, "Not a squre matrix (cholesky), rows:%d, cols:%d\n", 
           "//mat_size col= %d, row=%d, stride = %d", chk_rows, chk_cols, chk_str);
    
    load_file(lc_file, lc_cov, VIO_EKF_MMUL_A_BUFSIZE,
              "//LC.cols = %d, LC.rows = %d, LC.stride = %d //Row Major", lc_rows, lc_cols, lc_str);
    ASSERT(lc_cols == chk_rows, "LC rows not equal to CHK rows, rows:%d, cols:%d\n",
           "//lc_cols= %d, chk_rows=%d\n", lc_rows, chk_cols);
    
    load_file(gain_file, gain_cov, VIO_EKF_MMUL_B_BUFSIZE,
              "//gain.cols = %d, gain.rows = %d gain.stride = %d //Column Major", gain_rows, gain_cols, gain_str);
    ASSERT(((gain_cols == chk_rows)&&(chk_rows == lc_cols)&&(gain_cols == lc_cols)), "gain.rows: %d, chk.rows: %d,  lc.cols:%d\n",  gain_rows, chk_rows, lc_cols);

    load_file(cov_in_file, cov_in, VIO_EKF_MMUL_C_BUFSIZE,
              "//cov.cols = %d, cov.rows = %d, cov.stride = %d //Column Major", cov_in_rows, cov_in_cols, cov_in_str);
    ASSERT(((lc_rows == gain_rows) && ((cov_in_rows-1) == (cov_in_cols)) && (lc_rows == (cov_in_cols+1))),
	   "//lc_cols = %d, gain_rows = %d, cov_in_cols = %d, cov_in_rows = %d\n",
	   lc_rows, gain_rows, cov_in_rows, cov_in_cols);
    
#else
    for(uint32_t j = 0; j < chk_rows; j++) {
        for(uint32_t i = 0; i < chk_cols; i++) {
            res_cov[(j*chk_str) + i] = (i != j) ? 0 : (j+1)*(j+1);
        }
    }
#endif

    uint32_t m_size, s_size;
    m_size = chk_rows = chk_cols = argc > 6 ? atoi(argv[6]) : chk_rows;
    gain_rows = lc_cols = lchk_rows = lchk_cols = m_size;
    s_size = lc_rows  = argc > 7 ? atoi(argv[7]) : lc_rows;
    gain_cols = s_size;
  
    ekf_hw hw;  bool status = 0;

    if((mode == "cholesky")||(mode_1 == "cholesky")||(mode_2 == "cholesky")||(mode_3 =="cholesky")) {
        float *res_cov_hw = (float *)vio_malloc(VIO_EKF_CHOL_IN_BUFSIZE);
	
        hw.cholesky_decomp(res_cov, res_cov_hw, chk_cols, chk_str);

        vio_free(res_cov_hw);
    }
    
    if((mode == "mat_sol") || (mode_1 == "mat_sol") || (mode_2 == "mat_sol") || (mode_3 == "mat_sol")) {
        float *gainx_hw = (float *)vio_malloc(VIO_EKF_MMUL_B_BUFSIZE);
        float *chol_out = (float *)vio_malloc(m_size * lchk_str * sizeof(float));

        memcpy(chol_out, result_cov, m_size * lchk_str * sizeof(float));
        hw.mat_sol(chol_out, lc_cov, gainx_hw, lchk_rows, lc_rows, lchk_str, lc_str);

        vio_free(gainx_hw);
        vio_free(chol_out);
    }
    
      
    if((mode == "cov_update") || (mode_1 == "cov_update") || (mode_2 == "cov_update") || (mode_3 == "cov_update")) {
        float *cov_out_hw = (float *)vio_malloc(VIO_EKF_MMUL_C_BUFSIZE);

        hw.mat_mul(gain_cov, gain_cov, cov_in, cov_out_hw, gain_cols, gain_rows, gain_cols -1,
                   false, true, -1.0f, 1.0f, gain_str, gain_str, cov_in_str);
        vio_free(cov_out_hw);
    }
    
    if(mode == "all") {
        float *res_cov_out_hw = (float*)vio_malloc(VIO_EKF_CHOL_IN_BUFSIZE);
        float *cov_out_hw     = (float *)vio_malloc(VIO_EKF_MMUL_C_BUFSIZE);
        float *K_out_hw       = (float *)vio_malloc(VIO_EKF_MMUL_B_BUFSIZE);
#if 1
#define SET_STRIDE(x)  (x >> 2)

        float *lc    = lc_cov;
        float *inn   = lc_cov + ((s_size - 1) * (SET_STRIDE(lc_str)) * sizeof(float));
        float *cov_in_1 = cov_in;
        float *state_in = cov_in + ((s_size -1) * (SET_STRIDE(cov_in_str)) * sizeof(float));
        float *cov_out_hw_1 =  cov_out_hw;
        float *state_out_hw =  cov_out_hw + ((s_size -1) * (SET_STRIDE(cov_in_str)) * sizeof(float));

        hw.process(res_cov, lc, inn, cov_in_1, state_in, 
                   chk_str, lc_str, cov_in_str, (s_size - 1), m_size,
                   cov_out_hw_1, state_out_hw, res_cov_out_hw, K_out_hw);

#else
        hw.cholesky_decomp(res_cov, res_cov_out_hw, chk_cols, chk_str);
        hw.mat_sol(NULL, lc_cov, K_out_hw, lchk_rows, lc_rows, lchk_str, lc_str);
        hw.mat_mul(NULL, NULL, cov_in, cov_out_hw, gain_cols, gain_rows, gain_cols - 1,
                   false, true, -1.0f, 1.0f, lc_str, gain_str, cov_in_str);
#endif

        memcpy(gain_cov, lc_cov, s_size * lc_str * sizeof(float));
        vio_free(res_cov_out_hw);
        vio_free(cov_out_hw);
        vio_free(K_out_hw);
    }
    
    vio_free(res_cov);
    vio_free(result_cov);
    vio_free(lc_cov);
    vio_free(gain_cov);
    vio_free(cov_in);
}

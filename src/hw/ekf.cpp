#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <assert.h>
#include <algorithm>
#include "../stimuli_gen/matrix_generation.h"

#include "ekf_hw.h"
#include "../ref_model/ekf_sw.h"
#include "vio_regs.h"
#include "vio_utils.h"
#include "../stimuli_gen/utils.h"

#define CHECK_ARGS(v, s1, s2, s3, s4, s5)                               \
    if((v != s1) && (v != s2) && (v != s3) && (v != s4) && (v != s5)) { \
    fprintf(stderr, "Invalid %s, valid values:{%s, %s, %s, %s, %s}\n", #v, #s1, #s2, #s3, #s4, #s5); \
    exit(1);                                                            \
    }

#define LOG 0
#define LOAD 1
#define DEBUG 0

#if LOG
FILE *log_fp = NULL;
#define LOG_PRINTF(...) fprintf(log_fp, __VA_ARGS__ )
#else
#define LOG_PRINTF
#endif

#define CREATE_HISTO 0
#if CREATE_HISTO
#define NUM_BINS   (1 << 12)
#define HISTO_DIFF_RANGE  (float)1.0e-9
#define HISTO_RATIO_RANGE (float)1.0e-2
uint32_t histo_diff[NUM_BINS];
uint32_t histo_ratio[NUM_BINS];
#endif

bool check_mat_d(float *a, float *b, uint32_t cols, uint32_t rows, uint32_t stride,
               const char *str)
{
    printf("I[%s]: rows:%d, cols:%d, stride:%d, a:%p, b:%p\n",
           str, rows, cols, stride, a, b);
    bool status = 0;
    // a -> ref, b -> dut
    float max_diff = 0, max_ratio = 0;
    for(uint32_t j = 0; j < rows; j++) {
        for(uint32_t i = 0; i < cols; i++) {
            LOG_PRINTF("col:%d row:%d, ref:%e, hw:%e\n", i, j, a[i], b[i]);
            float diff  = std::abs(a[i] - b[i]);
            float a_abs = std::abs(a[i]);
            float b_abs = std::abs(b[i]);
            float ratio = diff / std::max(a_abs, (float)1.0e-03);
            //float ratio = diff / std::max(a_abs, b_abs);
            if((ratio > 0.001f) || isnan(a[i]) || isnan(b[i])) {
                status = status || 1;
                printf("x:%3d y:%3d ref:%e hw:%e diff:%e ratio:%e\n", i, j, a[i], b[i], diff, ratio);
            }
#if CREATE_HISTO
            int sign      = a[i] > b[i] ? 1 : -1;
            int diff_bin  = (NUM_BINS/2) + sign * (int)((diff / (float)HISTO_DIFF_RANGE) * (float)(NUM_BINS/2));
            diff_bin      = std::min(diff_bin, NUM_BINS - 1);
            diff_bin      = std::max(0, diff_bin);
            
            int ratio_bin = (NUM_BINS/2) + sign * (int)((ratio / (float)HISTO_RATIO_RANGE) * (float)(NUM_BINS/2));
            ratio_bin     = std::min(ratio_bin, NUM_BINS - 1);
            ratio_bin     = std::max(0, ratio_bin);

            histo_diff[diff_bin]  += 1;
            histo_ratio[ratio_bin] += 1;
#endif
#if 0
            max_diff    = std::max(diff, max_diff);
            max_ratio   = std::max(ratio, max_ratio);
#else
            if(ratio > max_ratio) {
                max_ratio = ratio;
                max_diff  = diff;
            }
#endif
        }
        a += stride;
        b += stride;
    }
    printf("I[%s]: max_diff: %e, max_ratio: %e\n", str, max_diff, max_ratio);
    return status;
}

void print_mat_d(float *mat, uint32_t cols, uint32_t rows, uint32_t stride,
                 const char *str)
{
    printf("Mat %s\n", str);
    for(uint32_t j = 0; j < rows; j++) {
        for(uint32_t i = 0; i < cols; i++) {
            printf("%11.4e  ", mat[(j * stride) + i]);
        }
        printf("\n");
    }
}

#define DUMP_MEM(filename, mem, wd, ht, stride)                         \
    printf("Dumpping %s\n", filename);                                  \
    dump_mem2d(filename, (uint8_t *)mem, wd*sizeof(float), ht, stride*sizeof(float))

#define PRINT_MAT //print_mat_d

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
#if LOG
    log_fp = fopen("log.txt", "w+"); ASSERT(log_fp, "Couldn't open log file!!");
#endif
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
    printf("cholesky:: rows:%3d cols:%3d stride:%d\n", chk_rows, chk_cols, chk_str);
    printf("res_cov:: rows:%3d cols:%3d stride:%d\n", lchk_rows, lchk_cols, lchk_str);
    printf("LC:: rows:%3d cols:%3d stride:%d\n", lc_rows, lc_cols, lc_str);
    printf("K:: rows:%3d cols:%3d stride:%d\n", gain_rows, gain_cols, gain_str);
    printf("Cov:: rows:%3d cols:%3d stride:%d\n", cov_in_rows, cov_in_cols, cov_in_str);
    ekf_hw hw;  ekf_sw sw; bool status = 0;

    if((mode == "cholesky")||(mode_1 == "cholesky")||(mode_2 == "cholesky")||(mode_3 =="cholesky")) {
        float *res_cov_hw = (float *)vio_malloc(VIO_EKF_CHOL_IN_BUFSIZE);
#if (!DEBUG)
	//print_mat(res_cov, m_size, m_size, chk_str, "res_cov_in");
        hw.cholesky_decomp(res_cov, res_cov_hw, chk_cols, chk_str);
	//print_mat(res_cov_hw, m_size, m_size, chk_str, "res_cov_out");
	
#else
        string chk_out_file = dir + "/CHL_Out_Matrix.txt"; uint32_t tmp1, tmp2, tmp3;
        printf("from the input file\n");
        load_file(chk_out_file, res_cov, VIO_EKF_CHOL_IN_BUFSIZE,
                  "//mat_size col= %d, row=%d, stride = %d", tmp1, tmp2, tmp3);
#endif 
        sw.cholesky_c(res_cov, chk_str, chk_rows);
        //PRINT_MAT(res_cov, chk_rows, chk_cols, chk_str, "SW-Output");
        status = check_mat_d(res_cov, res_cov_hw, chk_cols, chk_rows, chk_str, "CHL");
        printf("status: %d\n", status);
        vio_free(res_cov_hw);
    }
    
    if((mode == "mat_sol") || (mode_1 == "mat_sol") || (mode_2 == "mat_sol") || (mode_3 == "mat_sol")) {
        float *gainx_hw = (float *)vio_malloc(VIO_EKF_MMUL_B_BUFSIZE);
        float *chol_out = (float *)vio_malloc(m_size * lchk_str * sizeof(float));
#if (!DEBUG)
        memcpy(chol_out, result_cov, m_size * lchk_str * sizeof(float));
	//print_mat(chol_out, m_size, m_size, lchk_str, "chol_in");
	//print_mat(lc_cov, m_size, s_size, lc_str, "lc_in");
        hw.mat_sol(chol_out, lc_cov, gainx_hw, lchk_rows, lc_rows, lchk_str, lc_str);
	//print_mat(gainx_hw, m_size, s_size, lc_str, "K_out");
	
#else
        string gain_out_file = dir + "/MAT_Cov_Gain.txt";
        printf("from the input file\n");
        load_file(gain_out_file, gainx_hw, VIO_EKF_MMUL_B_BUFSIZE,
                  "//gain.cols = %d, gain.rows = %d gain.stride = %d //Column Major", lchk_rows, lc_rows, lc_str, 1);
#endif
        sw.mat_sol_1(result_cov, lc_cov, lchk_rows, lc_rows, lchk_str, lc_str); // TO_CHECK: uplo setting (which should be set to 'L'), check in mat_solve_hw() in RC code
        status = check_mat_d(lc_cov, gainx_hw, gain_rows, gain_cols, gain_str, "mat_sol");
        printf("status: %d\n", status);
        //DUMP_MEM("FPGA_MatSol_out.txt", gainx_hw, gain_rows, gain_cols, gain_str);
        vio_free(gainx_hw);
        vio_free(chol_out);
    }
    
      
    if((mode == "cov_update") || (mode_1 == "cov_update") || (mode_2 == "cov_update") || (mode_3 == "cov_update")) {
        float *cov_out_hw = (float *)vio_malloc(VIO_EKF_MMUL_C_BUFSIZE);
#if  (!DEBUG)
        PRINT_MAT(gain_cov, gain_rows, gain_cols, gain_str, "gain_in");
        hw.mat_mul(gain_cov, gain_cov, cov_in, cov_out_hw, gain_cols, gain_rows, gain_cols -1,
                   false, true, -1.0f, 1.0f, gain_str, gain_str, cov_in_str);
        PRINT_MAT(cov_out_hw, gain_cols - 1, gain_cols, cov_in_str, "cov_out_hw");
#else
        string cov_out_file = dir + "/MAT_Cov_Out.txt";
        printf("from the input file\n");
        load_file(cov_out_file, cov_out_hw, VIO_EKF_MMUL_C_BUFSIZE,
                  "//cov.cols = %d, cov.rows = %d, cov_stride = %d //Column Major", cov_in_rows, cov_in_cols, cov_in_str);
#endif
        PRINT_MAT(gain_cov, gain_rows, gain_cols, gain_str, "gain_in_sw");
        mat_mul_c(gain_cov, gain_cov, cov_in, gain_cols, gain_rows, gain_cols - 1, false, true, -1, 1, gain_str, gain_str, cov_in_str, cov_in);
        //sw.mat_mul(gain_cov, gain_cov, cov_in, gain_rows, gain_cols, gain_str, gain_str, cov_in_str);
        PRINT_MAT(cov_in, cov_in_cols, cov_in_rows, cov_in_str, "cov_out_sw");
        status = check_mat_d(cov_in, cov_out_hw, gain_cols - 1, gain_cols, cov_in_str, "mat_mul_cov");
        printf("status: %d\n", status);
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
        PRINT_MAT(cov_in, gain_cols - 1, gain_cols, cov_in_str, "cov_in(1)");
        hw.mat_mul(NULL, NULL, cov_in, cov_out_hw, gain_cols, gain_rows, gain_cols - 1,
                   false, true, -1.0f, 1.0f, lc_str, gain_str, cov_in_str);
        //hw.mat_mul_c(K_out_hw, K_out_hw, cov_in, cov_out_hw, gain_cols, gain_rows, gain_cols - 1,
        //             false, true, -1.0f, 1.0f, lc_str, gain_str, cov_in_str);
        PRINT_MAT(cov_in, gain_cols - 1, gain_cols, cov_in_str, "cov_in(2)");
#endif

#if (!DEBUG)
        memcpy(gain_cov, lc_cov, s_size * lc_str * sizeof(float));
        sw.cholesky_c(res_cov, chk_str, chk_rows);
        check_mat_d(res_cov, res_cov_out_hw, m_size, m_size, chk_str, "res_cov");
        sw.mat_sol_1(res_cov,   gain_cov,   lchk_rows,      lc_rows, lchk_str, lc_str);
        check_mat_d(gain_cov, K_out_hw, m_size, lc_rows, lc_str, "K_mat");
        PRINT_MAT(cov_in, gain_cols - 1, gain_cols, cov_in_str, "cov_in(3)");        
        sw.mat_mul_c(gain_cov,    gain_cov,    cov_in, gain_cols,   gain_rows, gain_cols - 1,
                     false, true, -1.0f, 1.0f, gain_str, gain_str, cov_in_str, cov_in, 0, 0);
        PRINT_MAT(cov_out_hw, gain_cols - 1, gain_cols, cov_in_str, "cov_out_hw");        
        PRINT_MAT(cov_in, gain_cols - 1, gain_cols, cov_in_str, "cov_out_sw");        
        status  = check_mat_d(cov_in,   cov_out_hw,   cov_in_cols,   cov_in_rows,   cov_in_str,   "mat_mul_cov");
        printf("status: %d\n", status);
        fflush(stdout);
#else
        sw.cholesky_decomp(res_cov, chk_str, chk_rows);
        sw.mat_sol(res_cov,   gain_cov,   lchk_rows,      lc_rows, lchk_str, lc_str);
        sw.mat_mul(gain_cov,    gain_cov, cov_in,   1, lc_cols,   gain_cols, lc_str,        gain_str, cov_in_str);          
        status  = check_mat_d(cov_in,   cov_out_hw,   cov_in_cols,   cov_in_rows,  cov_in_str,   "mat_mul_cov"); 
        
#endif
        vio_free(res_cov_out_hw);
        vio_free(cov_out_hw);
        vio_free(K_out_hw);
    }
    
#if CREATE_HISTO
    FILE *fp = fopen("histo.txt", "w+");
    float res_diff  = HISTO_DIFF_RANGE  / (float) (NUM_BINS/2);
    float res_ratio = HISTO_RATIO_RANGE / (float) (NUM_BINS/2);
    fprintf(fp, "Diff-Error Diff-Bin Ratio-Error Ratio-Bin\n");
    for(int i = 0; i < NUM_BINS; i++) {
        fprintf(fp, "%e %d ", ((float)(i - (NUM_BINS/2)) * res_diff), histo_diff[i]);
        fprintf(fp, "%e %d\n", ((float)(i - (NUM_BINS/2)) * res_ratio), histo_ratio[i]);
    }
    fclose(fp);
#endif
    vio_free(res_cov);
    vio_free(result_cov);
    vio_free(lc_cov);
    vio_free(gain_cov);
    vio_free(cov_in);
    return status;
}

#if 0

inline void print_mat(matrix &c, const char *str)
{
    float *mat = &c[0];
    uint32_t cols = c.cols(), rows = c.rows(), stride = c.get_stride();
    printf("Mat %s\n", str);
    for(uint32_t j = 0; j < rows; j++) {
        for(uint32_t i = 0; i < cols; i++) {
            printf("%11.4e  ", mat[(j * stride) + i]);
        }
        printf("\n");
    }
}

inline bool check_mat(matrix &a_mat, matrix &b_mat, const char *str)
{
    float *a = &a_mat[0];
    float *b = &b_mat[0];
    uint32_t cols = a_mat.cols(), rows = a_mat.rows();
    uint32_t a_stride = a_mat.get_stride(), b_stride = b_mat.get_stride();
    //printf("I[%s]: rows:%d, cols:%d, a:%p, b:%p\n", str, rows, cols, a, b);
    bool status = 0;
    // a -> ref, b -> dut
    float max_diff = 0, max_ratio = 0;
    for(uint32_t j = 0; j < rows; j++) {
        for(uint32_t i = 0; i < cols; i++) {
            //LOG_PRINTF("col:%d row:%d, ref:%e, hw:%e\n", i, j, a[i], b[i]);
            float diff  = std::abs(a[i] - b[i]);
            float a_abs = std::abs(a[i]);
            float b_abs = std::abs(b[i]);
            float ratio = diff / std::max(a_abs, (float)1.0e-08);
            //float ratio = diff / std::max(a_abs, b_abs);
            if((ratio > (float)1.0e-03) || isnan(a[i]) || isnan(b[i])) {
                status = status || 1;
                printf("x:%3d y:%3d ref:%e hw:%e diff:%e ratio:%e\n", i, j, a[i], b[i], diff, ratio);
                break;
            }
        }
        a += a_stride;
        b += b_stride;
    }
    return status;
}

#endif

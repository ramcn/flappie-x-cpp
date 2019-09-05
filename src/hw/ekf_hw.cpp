/**********************************************************************************************
   INTEL TOP SECRET
   Copyright 2016 Intel Corporation All Rights Reserved.

   The source code contained or described herein and all documents related to the source code
   ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the
   Material remains with Intel Corporation or its suppliers and licensors. The Material may
   contain trade secrets and proprietary and confidential information of Intel Corporation
   and its suppliers and licensors, and is protected by worldwide copyright and trade secret
   laws and treaty provisions. No part of the Material may be used, copied, reproduced,
   modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way
   without Intel's prior express written permission.
   No license under any patent, copyright, trade secret or other intellectual property right
   is granted to or conferred upon you by disclosure or delivery of the Materials, either
   expressly, by implication, inducement, estoppel or otherwise. Any license under such
   intellectual property rights must be express and approved by Intel in writing.
**********************************************************************************************/

/*
 * VIO HW Accelerators
 * ekf_hw.cpp
 *
 *  Created on: Sep 20, 2016
 *      Author: Om J Omer
 */
#include "string.h"

#include "ekf_hw.h"
#include "vio_utils.h"
#include "vio_regs.h"

#define SET_STRIDE(x)  ((x) >> 2)
#define SET_ADDRESS(x) ((x) >> 4)

ekf_hw::ekf_hw()
{
    vio_top = new VIO_TOP_t;
    vio_ekf = new VIO_EKF_t;
    memset(vio_top, 0x0, sizeof(VIO_TOP_t));
    memset(vio_ekf, 0x0, sizeof(VIO_EKF_t));

    // Reset
    VIO_TOP_CONTROL_T &top_ctrl = vio_top->VIO_TOP_CONTROL;
    top_ctrl.field.GlobalIntrDisable  = 1;
    top_ctrl.field.ClearConfigWrError = 1;
    WRITE_REG(TOP_CONTROL, top_ctrl.val);

    // Base Address
    
    WRITE_REG(EKF_CHOL_IN_BASE,      VIO_EKF_CHOL_IN_MEM_BA);
    WRITE_REG(EKF_CHOL_OUT_BASE,     VIO_EKF_CHOL_OUT_MEM_BA);
    WRITE_REG(EKF_MSLV_A_BASE,       VIO_EKF_MSLV_A_MEM_BA);
    WRITE_REG(EKF_MSLV_X_BASE,       VIO_EKF_MSLV_X_MEM_BA);
    WRITE_REG(EKF_MSLV_B_BASE,       VIO_EKF_MSLV_B_MEM_BA);
    WRITE_REG(EKF_MMUL_A_BASE,       VIO_EKF_MMUL_A_MEM_BA);
    WRITE_REG(EKF_MMUL_B_BASE,       VIO_EKF_MMUL_B_MEM_BA);
    WRITE_REG(EKF_MMUL_C_BASE,       VIO_EKF_MMUL_C_MEM_BA);
    WRITE_REG(EKF_MMUL_Y_BASE,       VIO_EKF_MMUL_Y_MEM_BA);
}

ekf_hw::~ekf_hw()
{
    delete vio_ekf;
    delete vio_top;
}

void
ekf_hw::inverse_diag(float *a, uint32_t dim, uint32_t stride)
{
    for(uint32_t j = 0; j < dim; j++) {
        uint32_t idx = (j * stride) + j;
        a[idx] = 1 / a[idx];
    }
}

void
ekf_hw::zero_mat(float *a, uint32_t dim, uint32_t stride, bool uplo)
{
    // uplo {0 -> zero upper triangle, 1-> zero lower triangle}
    if(uplo) {
        for(uint32_t j = 0; j < dim; j++) {
            for(uint32_t i = 0; i < j; i++) {
                a[i] = 0;
            }
            a += stride;
        }
    } else {
        for(uint32_t j = 0; j < dim; j++) {
            for(uint32_t i = j+1; i < dim; i++) {
                a[i] = 0;
            }
            a += stride;
        }
    }
}

void
ekf_hw::symmetrize_mat(float *a, uint32_t dim, uint32_t stride)
{
    for(uint32_t j = 0; j < dim; j++) {
        for(uint32_t i = 0; i < j; i++) {
            float &p = a[j*stride + i];
            float &q = a[i*stride + j];
            p = q = (p + q)/2;
        }
    }
}

void
ekf_hw::copy_lt_2_ut(float *a, uint32_t dim, uint32_t stride)
{
    for(uint32_t j = 0; j < dim; j++) {
        for(uint32_t i = 0; i < j; i++) {
            float &lt = a[j*stride + i];
            float &ut = a[i*stride + j];
            ut = lt;
        }
    }
}

void
ekf_hw::cholesky_decomp(const float *a, float *ltr, uint32_t dim, uint32_t stride)
{
    PRINTF("ekf_hw::cholesky_decomp()\n");
    ASSERT(!(stride & 0x3), "Cholesky stride should be multiple of 4, str:%d\n", stride);
    // Move data to HW
    PROFILE_START(CHK_DATA_TX);
    move_dma_2_hw(a, VIO_EKF_CHOL_IN_BUF, (dim * stride * sizeof(float)));
    PROFILE_END(CHK_DATA_TX);
    PROFILE_START(CHK_PROC);
    VIO_TOP_CONTROL_T &top_ctrl = vio_top->VIO_TOP_CONTROL;
    VIO_EKF_CONTROL_T &ctrl     = vio_ekf->VIO_EKF_CONTROL;
    VIO_EKF_STATUS_T &status    = vio_ekf->VIO_EKF_STATUS;
    ctrl.field.PredictionCovStart  = 0;
    ctrl.field.CholeskyDecompStart = 0;
    ctrl.field.MatSolveStart       = 0;
    ctrl.field.MatMulStart   = 0;
    // Config Write
    top_ctrl.field.GlobalIntrDisable  = 1;
    top_ctrl.field.ClearConfigWrError = 1;
    WRITE_REG(TOP_CONTROL, top_ctrl.val);

    VIO_EKF_CHOL_DIM_T &chk_dim = vio_ekf->VIO_EKF_CHOL_DIM;
    chk_dim.field.NumRows  = (dim );
    WRITE_REG(EKF_CHOL_DIM, chk_dim.val);
    VIO_EKF_CHOL_FORMAT_T &chk_for = vio_ekf->VIO_EKF_CHOL_FORMAT;
    chk_for.field.Stride = SET_STRIDE(stride);
    chk_for.field.Matrixlayout = 0;
    WRITE_REG(EKF_CHOL_FORMAT, chk_for.val);
    // trigger, poll-n-wait
    ctrl.field.CholeskyDecompStart = 1;
    WRITE_REG(EKF_CONTROL, ctrl.val);
    status.val = 0xfff0;
    POLL_N_WAIT(EKF_STATUS, status.val, status.field.CholeskyDecompDone == 0);
    PROFILE_END(CHK_PROC);

    PROFILE_START(CHK_DATA_RX);
    move_dma_2_cpu(VIO_EKF_CHOL_IN_BUF, ltr, (dim * stride * sizeof(float)));
    if(ltr) {
        zero_mat(ltr, dim, stride, 1);
        inverse_diag(ltr, dim, stride);
    }
    PROFILE_END(CHK_DATA_RX);

    PROFILE_REPORT(CHK_DATA_TX);
    PROFILE_REPORT(CHK_PROC);
    PROFILE_REPORT(CHK_DATA_RX);
}


void
ekf_hw::mat_sol(float *a_rm, float *b_cm, float *x_cm, uint32_t m_size, uint32_t s_size,
		uint32_t a_stride, uint32_t b_stride)
{
    PRINTF("ekf_hw::mat_sol()\n");
    // Move data to HW
    PRINTF("m=%d, a_stride=%d, s=%d, b_stride=%d\n", m_size, a_stride, s_size, b_stride);
    PROFILE_START(MAT_SOL_DATA_TX);
    if(a_rm) {
        inverse_diag(a_rm, m_size, a_stride);
    }
    move_dma_2_hw(a_rm, VIO_EKF_CHOL_IN_BUF, (m_size * a_stride * sizeof(float)));
    move_dma_2_hw(b_cm, VIO_EKF_MMUL_A_BUF, (s_size * b_stride * sizeof(float)));
    PROFILE_END(MAT_SOL_DATA_TX);
    
    PROFILE_START(MAT_SOL_PROC);
    VIO_TOP_CONTROL_T &top_ctrl = vio_top->VIO_TOP_CONTROL;
    VIO_EKF_CONTROL_T &ctrl     = vio_ekf->VIO_EKF_CONTROL;
    VIO_EKF_STATUS_T &status    = vio_ekf->VIO_EKF_STATUS;
    ctrl.val = 0;
    ctrl.field.PredictionCovStart  = 0;
    ctrl.field.CholeskyDecompStart = 0;
    ctrl.field.MatSolveStart       = 0;
    ctrl.field.MatMulStart         = 0;
    ctrl.field.MatSolveSkipStage1  = 0;
    ctrl.field.RDMatSlvPB          = 1;    

    // Config Write
    top_ctrl.field.GlobalIntrDisable  = 1;
    top_ctrl.field.ClearConfigWrError = 1;
    WRITE_REG(TOP_CONTROL, top_ctrl.val);

    VIO_EKF_MSLV_A_DIM_T &lchk_dim = vio_ekf->VIO_EKF_MSLV_A_DIM;
    lchk_dim.field.NumRows  = (m_size);
    WRITE_REG(EKF_MSLV_A_DIM, lchk_dim.val);

    VIO_EKF_MSLV_A_FORMAT_T &lchk_for = vio_ekf->VIO_EKF_MSLV_A_FORMAT;
    lchk_for.field.Stride = SET_STRIDE(a_stride);
    lchk_for.field.Matrixlayout = 0;
    WRITE_REG(EKF_MSLV_A_FORMAT, lchk_for.val);
    
    VIO_EKF_MSLV_X_DIM_T &kmat_dim = vio_ekf->VIO_EKF_MSLV_X_DIM;
    kmat_dim.field.NumRows  = (m_size );
    kmat_dim.field.NumCols  = (s_size );
    WRITE_REG(EKF_MSLV_X_DIM, kmat_dim.val);
    
    VIO_EKF_MSLV_X_FORMAT_T &kmat_str = vio_ekf->VIO_EKF_MSLV_X_FORMAT;
    kmat_str.field.Stride = SET_STRIDE(b_stride);
    WRITE_REG(EKF_MSLV_X_FORMAT, kmat_str.val);

    VIO_EKF_MSLV_B_DIM_T &lcinnmat_dim  = vio_ekf->VIO_EKF_MSLV_B_DIM;
    lcinnmat_dim.field.NumRows  = (m_size);
    lcinnmat_dim.field.NumCols  = (s_size);
    WRITE_REG(EKF_MSLV_B_DIM, lcinnmat_dim.val);

    VIO_EKF_MSLV_B_FORMAT_T &lcinnmat_str = vio_ekf->VIO_EKF_MSLV_B_FORMAT;
    lcinnmat_str.field.Stride =  SET_STRIDE(b_stride);
    WRITE_REG(EKF_MSLV_B_FORMAT, lcinnmat_str.val);    

    // trigger, poll-n-wait
    ctrl.field.MatSolveStart = 1;
    ctrl.field.MatSolveSkipStage2 = 1;
    ctrl.field.MatSolveDualWrEn = 1;
    WRITE_REG(EKF_CONTROL, ctrl.val);
    status.val = 0xfff0;
    POLL_N_WAIT(EKF_STATUS, status.val, status.field.MatSolveDone == 0);
    
    PROFILE_END(MAT_SOL_PROC);

    PROFILE_START(MAT_SOL_DATA_RX);
    move_dma_2_cpu(VIO_EKF_MMUL_B_BUF, x_cm, (s_size * b_stride * sizeof(float)));
    //zero_mat(c_outr, s_size, stride, 1);
    PROFILE_END(MAT_SOL_DATA_RX);

    PROFILE_REPORT(MAT_SOL_DATA_TX);
    PROFILE_REPORT(MAT_SOL_PROC);
    PROFILE_REPORT(MAT_SOL_DATA_RX);
}



void
ekf_hw::mat_mul_c(float *a_rm, float *b_cm, float *c_in, float *c_out,
                  uint32_t M, uint32_t N, uint32_t P,
                  bool a_trans, bool b_trans, float alpha, float beta,
                  uint32_t a_stride, uint32_t b_stride, uint32_t c_stride)
{
    uint32_t a_inner_stride, a_outer_stride, b_inner_stride, b_outer_stride;
    if(a_trans) {
        a_inner_stride = a_stride; a_outer_stride = 1;
    } else {
        a_inner_stride = 1;        a_outer_stride = a_stride;
    }
    if(b_trans) {
        b_inner_stride = b_stride; b_outer_stride = 1;
    } else {
        b_inner_stride = 1;        b_outer_stride = b_stride;
    }
        
    for(uint32_t i = 0; i < M; i++) {
        for(uint32_t j = 0; j < P; j++) {
            float acc = 0;
            const float *a  = a_rm + (a_outer_stride * i); // row selection
            const float *b  = b_cm + (b_inner_stride * j); // col selection
            for(uint32_t k = 0; k < N; k++) {
                acc += a[k * a_inner_stride] * b[k * b_outer_stride];
            }
            uint32_t idx = (c_stride * i) + j;
            c_out[idx] = (beta * c_in[idx]) + (alpha * acc);
        }
    } 
}

void ekf_hw::mat_mul(const float *a_rm, const float *b_cm, const float *c_in, float *c_outr,
		     uint32_t m_size, uint32_t n_size, uint32_t p_size, bool a_trans, bool b_trans,
		     float alpha, float beta, uint32_t a_stride, uint32_t b_stride, uint32_t c_stride,
		     uint32_t a_t, uint32_t b_t, uint32_t c_t)
{
    mat_mul_mod(a_rm, b_cm, c_in, c_outr,
                m_size, n_size, p_size, a_trans, b_trans,
                alpha, beta, a_stride, b_stride, c_stride,
                VIO_EKF_MMUL_A_BUF, VIO_EKF_MMUL_B_BUF, VIO_EKF_MMUL_C_BUF, VIO_EKF_MMUL_Y_BUF,
                a_t, b_t, c_t);
}

#include <sys/time.h>

void ekf_hw::mat_mul_mod(const float *a_rm, const float *b_cm, const float *c_in, float *c_outr,
                         uint32_t m_size, uint32_t n_size, uint32_t p_size, bool a_trans, bool b_trans,
                         float alpha, float beta, uint32_t a_stride, uint32_t b_stride, uint32_t c_stride,
                         const uint32_t a_buf, const uint32_t b_buf, const uint32_t c_buf, const uint32_t y_buf,
                         uint32_t a_t, uint32_t b_t, uint32_t c_t)
{
/*#define PROFILE_START(x) \
    struct timeval pr_##x##_start, pr_##x##_end; \
    gettimeofday(&pr_##x##_start, NULL);

#define PROFILE_END(x) \
    gettimeofday(&pr_##x##_end, NULL);
#define PROFILE_REPORT(x)  \
    printf("%-24s : %5lld\n", #x, (int64_t)(pr_##x##_end.tv_usec - pr_##x##_start.tv_usec) \
           + ((int64_t)(pr_##x##_end.tv_sec - pr_##x##_start.tv_sec) * 1000 * 1000))
*/

    PRINTF("update_cov::m_size = %d, n_size = %d, p_size = %d, a_trans = %d, b_trans = %d, a_stride = %d, b_stride = %d, c_stride = %d\n",
           m_size, n_size, p_size, a_trans, b_trans, a_stride, b_stride, c_stride);
    ASSERT(!(a_stride & 0x3), "a_stride:%d", a_stride);
    ASSERT(!(b_stride & 0x3), "b_stride:%d", b_stride);    
    ASSERT(!(c_stride & 0x3), "c_stride:%d", c_stride);

#define ROW_MAJOR 0
#define COL_MAJOR 1
    PRINTF("ekf_hw::mat_mul()\n");
    PRINTF("m:%d, n:%d, p:%d VIO_EKF_MMUL_A_BUFSIZE:%d\n", m_size, n_size, p_size, VIO_EKF_MMUL_A_BUFSIZE);
    PRINTF("a.stride:%d, b.stride:%d, c.stride:%d\n", a_stride, b_stride, c_stride);
    // Move data to HW
    uint32_t a_rows = a_trans ? n_size : m_size;
    uint32_t b_rows = b_trans ? p_size : n_size;
    PRINTF("a_rows:%d, a_stride:%d sizef=%d\n", a_rows, a_stride, sizeof(float));
    PRINTF("b_rows:%d, b_stride:%d VIO_EKF_MMUL_B_BUFSIZE=%d\n", b_rows, b_stride, VIO_EKF_MMUL_B_BUFSIZE);
    PRINTF("c_rows=%d, c_stride:%d VIO_EKF_MMUL_C_BUFSIZE=%d\n", m_size, c_stride, VIO_EKF_MMUL_C_BUFSIZE);
    ASSERT((a_rows * a_stride * sizeof(float)) < VIO_EKF_MMUL_A_BUFSIZE, "A bufsize exceeds");
    ASSERT((b_rows * b_stride * sizeof(float)) < VIO_EKF_MMUL_B_BUFSIZE, "B bufsize exceeds");
    ASSERT((m_size * c_stride * sizeof(float)) < VIO_EKF_MMUL_C_BUFSIZE, "C bufsize exceeds");

    
    PROFILE_START(MATMUL_FULL);
    PROFILE_START(A_DATA_TX);
    move_dma_2_hw(a_rm, a_buf, (a_rows * a_stride * sizeof(float)));
    PROFILE_END(A_DATA_TX);
    PROFILE_START(B_DATA_TX);
    move_dma_2_hw(b_cm, b_buf, (b_rows * b_stride * sizeof(float)));
    PROFILE_END(B_DATA_TX);
    PROFILE_START(C_DATA_TX);
    move_dma_2_hw(c_in, c_buf, (m_size * c_stride * sizeof(float)));
    PROFILE_END(C_DATA_TX);

    PROFILE_START(SETUP);
    WRITE_REG(EKF_MMUL_A_BASE, SET_ADDRESS(a_buf));
    WRITE_REG(EKF_MMUL_B_BASE, SET_ADDRESS(b_buf));
    WRITE_REG(EKF_MMUL_C_BASE, SET_ADDRESS(c_buf));
    WRITE_REG(EKF_MMUL_Y_BASE, SET_ADDRESS(y_buf));
    
    VIO_TOP_CONTROL_T &top_ctrl = vio_top->VIO_TOP_CONTROL;
    VIO_EKF_CONTROL_T &ctrl     = vio_ekf->VIO_EKF_CONTROL;
    VIO_EKF_STATUS_T &status    = vio_ekf->VIO_EKF_STATUS;
    ctrl.val = 0;
    ctrl.field.PredictionCovStart  = 0;
    ctrl.field.CholeskyDecompStart = 0;
    ctrl.field.MatSolveStart       = 0;
    ctrl.field.MatMulStart   = 0;
    
    // Config Write
    top_ctrl.field.GlobalIntrDisable  = 1;
    top_ctrl.field.ClearConfigWrError = 1;
    WRITE_REG(TOP_CONTROL, top_ctrl.val);

    VIO_EKF_MMUL_A_DIM_T &lcinnmat_dim = vio_ekf->VIO_EKF_MMUL_A_DIM;
    lcinnmat_dim.field.NumRows  = (m_size );
    lcinnmat_dim.field.NumCols  = (n_size );
    WRITE_REG(EKF_MMUL_A_DIM, lcinnmat_dim.val);
    
    VIO_EKF_MMUL_A_FORMAT_T &lcinnmat_str = vio_ekf->VIO_EKF_MMUL_A_FORMAT;
    lcinnmat_str.field.Stride =  SET_STRIDE(a_stride);
    lcinnmat_str.field.Matrixlayout = a_t;
    lcinnmat_str.field.Order = a_trans ? COL_MAJOR : ROW_MAJOR;
    WRITE_REG(EKF_MMUL_A_FORMAT, lcinnmat_str.val);

    VIO_EKF_MMUL_B_DIM_T &kmat_dim  = vio_ekf->VIO_EKF_MMUL_B_DIM;
    kmat_dim.field.NumRows  = (n_size ); 
    kmat_dim.field.NumCols  = (p_size);
    WRITE_REG(EKF_MMUL_B_DIM, kmat_dim.val);

    VIO_EKF_MMUL_B_FORMAT_T &kmat_str = vio_ekf->VIO_EKF_MMUL_B_FORMAT;
    kmat_str.field.Stride =  SET_STRIDE(b_stride);
    kmat_str.field.Matrixlayout = b_t;
    kmat_str.field.Order = b_trans ? COL_MAJOR : ROW_MAJOR;
    WRITE_REG(EKF_MMUL_B_FORMAT, kmat_str.val);

    VIO_EKF_MMUL_C_DIM_T &ststcovmat_dim = vio_ekf->VIO_EKF_MMUL_C_DIM;
    ststcovmat_dim.field.NumRows  = (m_size );
    ststcovmat_dim.field.NumCols  = (p_size);
    WRITE_REG(EKF_MMUL_C_DIM, ststcovmat_dim.val);

    VIO_EKF_MMUL_C_FORMAT_T &ststcovmat_str = vio_ekf->VIO_EKF_MMUL_C_FORMAT;
    ststcovmat_str.field.Stride =  SET_STRIDE(c_stride);
    ststcovmat_str.field.Matrixlayout = c_t;
    ststcovmat_str.field.Order = ROW_MAJOR;
    WRITE_REG(EKF_MMUL_C_FORMAT, ststcovmat_str.val);

    VIO_EKF_MATMUL_COEF_A_T &alpha_1 = vio_ekf->VIO_EKF_MATMUL_COEF_A;
    alpha_1.field.Alpha = alpha;
    WRITE_REG(EKF_MATMUL_COEF_A, alpha_1.val);

    VIO_EKF_MATMUL_COEF_B_T &beta_1 = vio_ekf->VIO_EKF_MATMUL_COEF_B;
    beta_1.field.Beta = beta;
    WRITE_REG(EKF_MATMUL_COEF_B, beta_1.val);
    PROFILE_END(SETUP);
    
    // trigger, poll-n-wait
    PROFILE_START(MATMUL);
    ctrl.field.MatMulStart = 1;
    ctrl.field.MatMulComputeMode = 0;
    WRITE_REG(EKF_CONTROL, ctrl.val);
    status.val = 0xfff0;
    POLL_N_WAIT(EKF_STATUS, status.val, status.field.MatMulDone == 0);
    PROFILE_END(MATMUL);

    PROFILE_START(DATA_RX);
    move_dma_2_cpu(y_buf, c_outr, (m_size * c_stride * sizeof(float)));
    PROFILE_END(DATA_RX);
    PROFILE_END(MATMUL_FULL);

    PROFILE_REPORT(MATMUL_FULL);
    PROFILE_REPORT(A_DATA_TX);
    PROFILE_REPORT(B_DATA_TX);
    PROFILE_REPORT(C_DATA_TX);
    PROFILE_REPORT(SETUP);
    PROFILE_REPORT(MATMUL);
    PROFILE_REPORT(DATA_RX);
}

void
ekf_hw::process(float *res_cov, float *lc, float *inn, float *cov, float *state,
                uint32_t res_cov_stride, uint32_t lc_stride, uint32_t cov_stride,
                uint32_t s_size, uint32_t m_size,
                float *cov_out, float *state_out, float *res_cov_out, float *k_out)
{
    ASSERT(res_cov_stride >= ALIGN_4(m_size), "Result_Cov Stride:%d is less than Observation size %d",
           res_cov_stride, m_size);
    ASSERT(lc_stride  >= ALIGN_4(m_size), "LC Stride:%d is less than Observation size %d", lc_stride, m_size);
    ASSERT(cov_stride >= ALIGN_4(s_size), "Covariance Stride:%d is less than State size %d", cov_stride, s_size);
    ASSERT(res_cov_stride == ALIGN_4(res_cov_stride), "Result_Cov Stride:%d is not aligned to 128b", res_cov_stride);
    ASSERT(lc_stride  == ALIGN_4(lc_stride),  "LC Stride:%d is not aligned to 128b", lc_stride);
    ASSERT(cov_stride == ALIGN_4(cov_stride), "Covariance Stride:%d is not aligned to 128b", cov_stride);
    //ASSERT(lc    != NULL, "LC Matrix pointer is not set");
    ASSERT(inn   != NULL, "Inn Matrix pointer is not set");
    ASSERT(cov   != NULL, "Covariance Matrix Pointer is not set");
    ASSERT(state != NULL, "State Matrix Pointer is not set");
    ASSERT(m_size <= 256, "observatin size is more than 256\n");
    ASSERT(s_size <= 256, "state size is more than 256\n");
    
    
    PRINTF("m_size:%d, s_size:%d, rescov_stride:%d, lc_stride:%d, cov_stride:%d\n",
           m_size, s_size, res_cov_stride, lc_stride, cov_stride); 

    PROFILE_START(EKF_PROC);
    VIO_TOP_CONTROL_T &top_ctrl = vio_top->VIO_TOP_CONTROL;
    VIO_EKF_CONTROL_T &ctrl     = vio_ekf->VIO_EKF_CONTROL;
    VIO_EKF_STATUS_T &status    = vio_ekf->VIO_EKF_STATUS;
    ctrl.val = 0;
    ctrl.field.PredictionCovStart  = 0;
    ctrl.field.CholeskyDecompStart = 0;
    ctrl.field.MatSolveStart       = 0;
    ctrl.field.MatMulStart         = 0;
    ctrl.field.RDMatSlvPB          = 0;

    // Config Write
    top_ctrl.field.GlobalIntrDisable  = 1;
    top_ctrl.field.ClearConfigWrError = 1;
    WRITE_REG(TOP_CONTROL, top_ctrl.val);
    
    VIO_EKF_CHOL_DIM_T &chk_dim = vio_ekf->VIO_EKF_CHOL_DIM;
    chk_dim.field.NumRows  = (m_size );
    WRITE_REG(EKF_CHOL_DIM, chk_dim.val);

    VIO_EKF_CHOL_FORMAT_T &chk_str = vio_ekf->VIO_EKF_CHOL_FORMAT;
    chk_str.field.Stride =  SET_STRIDE(res_cov_stride);
    chk_str.field.Matrixlayout = 0;
    WRITE_REG(EKF_CHOL_FORMAT, chk_str.val);

    VIO_EKF_MSLV_A_DIM_T &lchk_dim = vio_ekf->VIO_EKF_MSLV_A_DIM;
    lchk_dim.field.NumRows  = (m_size);
    WRITE_REG(EKF_MSLV_A_DIM, lchk_dim.val);

    VIO_EKF_MSLV_A_FORMAT_T &lchk_for = vio_ekf->VIO_EKF_MSLV_A_FORMAT;
    lchk_for.field.Stride = SET_STRIDE(res_cov_stride);
    lchk_for.field.Matrixlayout = 0;
    WRITE_REG(EKF_MSLV_A_FORMAT, lchk_for.val);
    
    VIO_EKF_MSLV_X_DIM_T &kmatslv_dim = vio_ekf->VIO_EKF_MSLV_X_DIM;
    kmatslv_dim.field.NumRows  = (m_size );
    kmatslv_dim.field.NumCols  = (s_size + 1);
    WRITE_REG(EKF_MSLV_X_DIM, kmatslv_dim.val);
    
    VIO_EKF_MSLV_X_FORMAT_T &kmatslv_str = vio_ekf->VIO_EKF_MSLV_X_FORMAT;
    kmatslv_str.field.Stride = SET_STRIDE(lc_stride);
    WRITE_REG(EKF_MSLV_X_FORMAT, kmatslv_str.val);

    VIO_EKF_MSLV_B_DIM_T &lcinnmatslv_dim  = vio_ekf->VIO_EKF_MSLV_B_DIM;
    lcinnmatslv_dim.field.NumRows  = (m_size);
    lcinnmatslv_dim.field.NumCols  = (s_size + 1);
    WRITE_REG(EKF_MSLV_B_DIM, lcinnmatslv_dim.val);

    VIO_EKF_MSLV_B_FORMAT_T &lcinnmatslv_str = vio_ekf->VIO_EKF_MSLV_B_FORMAT;
    lcinnmatslv_str.field.Stride =  SET_STRIDE(lc_stride);
    WRITE_REG(EKF_MSLV_B_FORMAT, lcinnmatslv_str.val);    
    

    VIO_EKF_MMUL_A_DIM_T &lcinnmat_dim = vio_ekf->VIO_EKF_MMUL_A_DIM;
    lcinnmat_dim.field.NumRows  = (s_size + 1);
    lcinnmat_dim.field.NumCols  = (m_size );
    WRITE_REG(EKF_MMUL_A_DIM, lcinnmat_dim.val);
    
    VIO_EKF_MMUL_A_FORMAT_T &lcmat_str = vio_ekf->VIO_EKF_MMUL_A_FORMAT;
    lcmat_str.field.Stride =  SET_STRIDE(lc_stride);
    lcmat_str.field.Matrixlayout = 0;
    lcmat_str.field.Order = 0;
    WRITE_REG(EKF_MMUL_A_FORMAT, lcmat_str.val);

    VIO_EKF_MMUL_B_DIM_T &kmat_dim  = vio_ekf->VIO_EKF_MMUL_B_DIM;
    kmat_dim.field.NumRows  = (m_size );
    kmat_dim.field.NumCols  = (s_size );
    WRITE_REG(EKF_MMUL_B_DIM, kmat_dim.val);

    VIO_EKF_MMUL_B_FORMAT_T &kmat_str = vio_ekf->VIO_EKF_MMUL_B_FORMAT;
    kmat_str.field.Stride =  SET_STRIDE(lc_stride);
    kmat_str.field.Matrixlayout = 0;
    kmat_str.field.Order = 1;
    WRITE_REG(EKF_MMUL_B_FORMAT, kmat_str.val); 

    VIO_EKF_MMUL_C_DIM_T &ststcovmat_dim = vio_ekf->VIO_EKF_MMUL_C_DIM;
    ststcovmat_dim.field.NumRows  = (s_size + 1);
    ststcovmat_dim.field.NumCols  = (s_size );
    WRITE_REG(EKF_MMUL_C_DIM, ststcovmat_dim.val);

    VIO_EKF_MMUL_C_FORMAT_T &ststcovmat_str = vio_ekf->VIO_EKF_MMUL_C_FORMAT;
    ststcovmat_str.field.Stride =  SET_STRIDE(cov_stride);
    ststcovmat_str.field.Matrixlayout = 0;
    ststcovmat_str.field.Order = 0;
    WRITE_REG(EKF_MMUL_C_FORMAT, ststcovmat_str.val);

    VIO_EKF_MATMUL_COEF_A_T &alpha = vio_ekf->VIO_EKF_MATMUL_COEF_A;
    alpha.field.Alpha = -1.0f;
    WRITE_REG(EKF_MATMUL_COEF_A, alpha.val);

    VIO_EKF_MATMUL_COEF_B_T &beta = vio_ekf->VIO_EKF_MATMUL_COEF_B;
    beta.field.Beta = 1.0f;
    WRITE_REG(EKF_MATMUL_COEF_B, beta.val);
   
    PROFILE_START(RES_COV_TX);
    move_dma_2_hw(res_cov, VIO_EKF_CHOL_IN_BUF, (m_size * res_cov_stride * sizeof(float)));
    PROFILE_END(RES_COV_TX);
    
    
    PRINTF("Running Cholesky .. \n");
    PROFILE_START(CHOLESKY_PROC);
    ctrl.field.CholeskyDecompStart = 1;
    WRITE_REG(EKF_CONTROL, ctrl.val);
    status.val = 0xfff0;
    POLL_N_WAIT(EKF_STATUS, status.val, status.field.CholeskyDecompDone == 0); 
    ctrl.field.CholeskyDecompStart = 0;
    PROFILE_END(CHOLESKY_PROC);

    move_dma_2_cpu(VIO_EKF_CHOL_IN_BUF, res_cov_out,
                   (m_size * res_cov_stride * sizeof(float)));
    if(res_cov_out) {
        zero_mat(res_cov_out, m_size, res_cov_stride, 1);
        inverse_diag(res_cov_out, m_size, res_cov_stride);
    }
  
    PROFILE_START(LC_TX);
    move_dma_2_hw(lc, VIO_EKF_MSLV_B_BUF, (s_size * lc_stride * sizeof(float)));
    move_dma_2_hw(inn, (VIO_EKF_MSLV_B_BUF + (s_size * lc_stride * sizeof(float))), (m_size * sizeof(float)));
    PROFILE_END(LC_TX);
    
    PRINTF("Running Mat-Solv .. \n");
    PROFILE_START(MAT_SOLV_PROC);
    ctrl.field.MatSolveStart = 1;
    ctrl.field.MatSolveSkipStage2 = 1;
    ctrl.field.MatSolveDualWrEn = 1;
    WRITE_REG(EKF_CONTROL, ctrl.val);
    status.val = 0xfff0;
    POLL_N_WAIT(EKF_STATUS, status.val, status.field.MatSolveDone == 0); 
    ctrl.field.MatSolveStart = 0;
    PROFILE_END(MAT_SOLV_PROC);

    move_dma_2_cpu(VIO_EKF_MMUL_B_BUF, k_out,
                   ((s_size + 1) * lc_stride * sizeof(float)));


    PROFILE_START(COV_TX);
    move_dma_2_hw(cov, VIO_EKF_MMUL_C_BUF, (s_size * cov_stride * sizeof(float)));
    move_dma_2_hw(state, (VIO_EKF_MMUL_C_BUF + (s_size * cov_stride * sizeof(float))), (s_size * sizeof(float)));
    PROFILE_END(COV_TX);
    
    PRINTF("Running Mat-Mul (Cov Update)\n");
    PROFILE_START(COV_UPDATE_PROC);
    ctrl.field.MatMulStart = 1;
    ctrl.field.MatMulComputeMode = 1;
    WRITE_REG(EKF_CONTROL, ctrl.val);
    status.val = 0xfff0;
    POLL_N_WAIT(EKF_STATUS, status.val, status.field.MatMulDone == 0);
    ctrl.field.MatMulStart = 0;
    PROFILE_END(COV_UPDATE_PROC);

    PROFILE_START(EKF_RX);
    move_dma_2_cpu(VIO_EKF_MMUL_C_BUF, cov_out,
                   (s_size * cov_stride * sizeof(float)));

    if(cov_out) {
        copy_lt_2_ut(cov_out, s_size, cov_stride);
    }
    
    move_dma_2_cpu((VIO_EKF_MMUL_C_BUF + (s_size * cov_stride * sizeof(float))),
                   state_out, (s_size * sizeof(float)));
    PROFILE_END(EKF_RX);

    

    PROFILE_REPORT(RES_COV_TX);
    PROFILE_REPORT(LC_TX);
    PROFILE_REPORT(COV_TX);
    
    return;
}



void
ekf_hw::move_y_mat_2_cpu(void *a_rm, uint32_t stride, uint32_t rows, bool nb)
{
    move_dma_2_cpu(VIO_EKF_MMUL_Y_BUF, a_rm, (rows * stride * sizeof(float)));
}

void
ekf_hw::move_c_mat_2_cpu(void *a_rm, uint32_t stride, uint32_t rows, bool nb)
{
    move_dma_2_cpu(VIO_EKF_MMUL_C_BUF, a_rm, (rows * stride * sizeof(float)));
}

void
ekf_hw::move_b_mat_2_cpu(void *a_rm, uint32_t stride, uint32_t rows, bool nb)
{
    move_dma_2_cpu(VIO_EKF_MMUL_B_BUF, a_rm, (rows * stride * sizeof(float)));
}

void
ekf_hw::move_a_mat_2_cpu(void *a_rm, uint32_t stride, uint32_t rows, bool nb)
{
    move_dma_2_cpu(VIO_EKF_MMUL_A_BUF, a_rm, (rows * stride * sizeof(float)));
}


float*
ekf_hw::get_chol_inp_mat()
{
    return (float*)(vio_sram_ptr() + VIO_EKF_CHOL_IN_BUF);
}

float*
ekf_hw::get_matsol_b_mat()
{
    return (float*)(vio_sram_ptr() + VIO_EKF_MSLV_B_BUF);
}

float*
ekf_hw::get_mmul_y_mat()
{
    return (float*)(vio_sram_ptr() + VIO_EKF_MMUL_Y_BUF);
}


float*
ekf_hw::get_mmul_c_mat()
{
    float* ptr =  (float*)(vio_sram_ptr() + VIO_EKF_MMUL_C_BUF);
    return ptr;
}

float*
ekf_hw::get_mmul_b_mat()
{
    return (float*)(vio_sram_ptr() + VIO_EKF_MMUL_B_BUF);
}

float*
ekf_hw::get_mmul_a_mat()
{
    return (float*)(vio_sram_ptr() + VIO_EKF_MMUL_A_BUF);
}

void
ekf_hw::memset_vio_sram()
{
    memset(vio_sram_ptr(), 0x0, vio_sram_size());
}

void
ekf_hw::mask_error_status(uint32_t mask)
{
    WRITE_REG(EKF_ERROR_STATUS, mask);
}

uint32_t
ekf_hw::get_error_status()
{
    uint32_t status = READ_REG(EKF_ERROR_STATUS);
    return status;
}


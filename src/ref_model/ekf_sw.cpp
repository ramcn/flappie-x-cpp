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
 * ekf_sw.cpp
 *
 *  Created on: Sep 20, 2016
 *      Author: Om J Omer
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <assert.h>
#include <math.h>
#include "../stimuli_gen/matrix_generation.h"
#include "ekf_sw.h"

#define BLAS 0
#if BLAS
#ifdef HAVE_MKL
#include "mkl.h"
#else
#include <cblas.h>
#include <lapacke.h>
#endif

#define F_T_TYPE(prefix,func,suffix) prefix ## s ## func ## suffix
#define LAPACK_(func) F_T_TYPE(      ,func, _)
#define cblas_(func)  F_T_TYPE(cblas_,func,  )
#endif

#define ASSERT(cond, ...)  \
    if(!(cond)) {        \
        printf("Assert: %s %d : ", __FILE__, __LINE__);    \
        printf(__VA_ARGS__); \
        exit(1); \
    }

void zero_mat(float *a, uint32_t dim, uint32_t stride, bool uplo)
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

void ekf_sw::cholesky_c(float *a, uint32_t stride, uint32_t dim)
{
    // A = LL' , L is computed as Column major 
    for(int col = 0; col < dim; col++) {
        float *c_buf = a + (col * stride);
        for(int row = col; row < dim; row++) {
            float acc = 0;
            for(int i = 0; i < col; i++) {
                acc += a[(i*stride) + col] * a[i*stride + row];
            }
            float &y    = c_buf[row];
            float &diag = c_buf[col];
            if (row == col) {
                 y = sqrt(y - acc);
            } else {
                y = (y - acc) * (1 / diag);
            }
        }
    }
    zero_mat(a, dim, stride, 1);
}

#define FLOAT_TO_HEX(x) (*(uint32_t*)(&(x)))

void ekf_sw::cholesky_uarch(const float *in, uint32_t stride, uint32_t dim, float *out,
                            int row_stop, int col_stop, const char *file) 
{
    FILE *fp = fopen(file, "w+"); 
    if(!fp) {
        printf("Couldn't open %s for writes\n", file);
        exit(1);
    }
    // A = LL' , L is computed as Column major 
    for(int col = 0; col < dim; col++) {
        const float *in_buf  = in + (col * stride);
        float *out_buf = out + (col * stride);
        for(int row = col; row < dim; row++) {
            fprintf(fp, "%3d %3d \n", col, row);
            float acc   = 0;
            for(int i = 0; i < col; i++) {
                float tmp = out[(i*stride) + col] * out[i*stride + row];
                acc += tmp;
                fprintf(fp, "\t\t%3d  %15.8e 0x%08x %15.8e 0x%08x \n", i, tmp, FLOAT_TO_HEX(tmp),
                        acc, FLOAT_TO_HEX(acc));
            }
            const float &yin  = in_buf[row];
            float &y    = out_buf[row];
            float &diag = out_buf[col];
            float sub   = yin - acc;
            if (row == col) {
                 y = sqrt(sub);
            } else {
                y = (sub) * (1 / diag);
            }
            fprintf(fp, "\t\t sub:%15.8e 0x%08x y:%15.8e 0x%08x \n", sub, FLOAT_TO_HEX(sub), y, FLOAT_TO_HEX(y));
            if((row == row_stop) && (col == col_stop)) {
                break;
            }            
        }
    }
    zero_mat(out, dim, stride, 1);
}


void ekf_sw::cholesky_decomp(float *a, uint32_t stride, uint32_t dim)
{
#if BLAS
    char uplo = 'L';
    lapack_int info;
    lapack_int n   = dim;
    lapack_int lda = stride;
    LAPACK_(potrf)(&uplo, &n, a, &lda, &info);
    printf("uplo:%c n:%d, lda:%d, info:%d\n", uplo, n, lda, info);
    ASSERT(!info, "ekf_sw::cholesky_deomp() failed !!\n");
    zero_mat(a, dim, stride, 1);
#else
    cholesky_c(a, stride, dim);
#endif    
}

static void solve1(int n, const float *x, const float *a, uint32_t a_stride, float &y)
{
    float acc = 0;
    for(int i = 0; i < n; i++) {
        acc += x[i] * a[(a_stride * i) + n];
    }
    float diag = a[(a_stride * n) + n];
    y = (y - acc) * (1 / diag);
}

static void solve2(int n, const float *x, const float *a, uint32_t a_stride, float &y, int N)
{
    float acc = 0;
    for(int i = N-1; i > n; i--) {
        acc += x[i] * a[(a_stride * n) + i];
    }
    float diag = a[(a_stride * n) + n];
    y = (y - acc) * (1 / diag);
}

void
ekf_sw::mat_sol_1(float *a_rm, float *b_cm, uint32_t m_size, uint32_t s_size, uint32_t a_stride, uint32_t b_stride)
{
    //A.X=B, solve for X
    //A -> Column major, B-> Column major, X-> Column major
    for(int col = 0; col < s_size; col++) {
        float* x_buf = b_cm + (col * b_stride);
        for(int row = 0; row < m_size; row++) {
            solve1(row, x_buf, a_rm, a_stride, x_buf[row]);
        }
    }
}

void
ekf_sw::mat_sol_2(float *a_rm, float *b_cm, uint32_t m_size, uint32_t s_size, uint32_t a_stride, uint32_t b_stride)
{
    for(int col = 0; col < s_size; col++) {
        float *x_buf = b_cm + (col * b_stride);
        for(int row = m_size; row--;) {
            solve2(row, x_buf, a_rm, a_stride, x_buf[row], m_size);
        }
    }
}

void ekf_sw::mat_sol_c(float *a_rm, float *b_cm, uint32_t m_size, uint32_t s_size, uint32_t a_stride, uint32_t b_stride)
{
    mat_sol_1(a_rm, b_cm, m_size, s_size, a_stride, b_stride);
    mat_sol_2(a_rm, b_cm, m_size, s_size, a_stride, b_stride);
}
void ekf_sw::mat_sol(float *a_rm, float *b_cm, uint32_t m_size, uint32_t s_size, uint32_t a_stride, uint32_t b_stride)
{
#if BLAS
    char uplo = 'L';
    lapack_int info;
    printf("m=%d, a_stride=%d, s=%d, b_stride=%d\n", m_size, a_stride, s_size, b_stride);
    lapack_int n   = m_size;
    lapack_int lda = a_stride;
    lapack_int nrhs = s_size;
    lapack_int ldb = b_stride;
    LAPACK_(potrs)(&uplo, &n, &nrhs, a_rm, &lda, b_cm, &ldb, &info);
#else
    mat_sol_c(a_rm, b_cm, m_size, s_size, a_stride, b_stride);
#endif 
}

void
ekf_sw::mat_mul_c(const float *a_rm /*mxn|nxm*/, const float *b_cm /*nxp|pxn*/, const float *c_in /*mxp*/,
                  uint32_t m_dim, uint32_t n_dim, uint32_t p_dim, bool a_trans, bool b_trans,
                  float alpha, float beta, uint32_t a_stride, uint32_t b_stride, uint32_t c_stride,
                  float *c_out /*mxp*/, uint32_t mata/*FULL (0) or LT(1) or UT (2)*/,
                  uint32_t matb/*FULL(0) or LT (1) or UT (2)*/)
{
    #if 1
        uint32_t M = m_dim, N = n_dim, P = p_dim;
        uint32_t a_inner_stride, a_outer_stride, b_inner_stride, b_outer_stride;

        float *a_temp;
        float *b_temp;

        if (a_trans){
            a_temp = (float *)malloc(n_dim * a_stride * sizeof(float));
        } else {
            a_temp = (float *)malloc(m_dim * a_stride * sizeof(float));
        }

        if (b_trans){
            b_temp = (float *)malloc(p_dim * b_stride * sizeof(float));
        } else {
            b_temp = (float *)malloc(n_dim * b_stride * sizeof(float));
        }

        if (a_trans){
            matrix_copy(a_rm, a_temp, n_dim, m_dim, a_stride);
            //print_mat_c(a_rm, n_dim, m_dim, a_stride, "A_cm");
            //print_mat_c(a_temp, n_dim, m_dim, a_stride, "A_cm_temp");
        } else {
            matrix_copy(a_rm, a_temp, m_dim, n_dim, a_stride);
            //print_mat_c(a_rm, m_dim, n_dim, a_stride, "A_rm");
            //print_mat_c(a_temp, m_dim, n_dim, a_stride, "A_temp");
        }

        if (b_trans){
            matrix_copy(b_cm, b_temp, p_dim, n_dim, b_stride);
            //print_mat_c(b_cm, p_dim, n_dim, b_stride, "b_cm");
            //print_mat_c(b_temp, p_dim, n_dim, b_stride, "b_cm_temp");
        } else {
            matrix_copy(b_cm, b_temp, n_dim, p_dim, b_stride);
            //print_mat_c(b_cm, n_dim, p_dim, b_stride, "b_rm");
            //print_mat_c(b_temp, n_dim, p_dim, b_stride, "b_rm_temp");
        }

    
        if (mata == 1) {
            if(a_trans) {
                matrix_ut_p(a_temp, n_dim, m_dim, a_stride, 0);
                //  print_mat(a_temp, n_dim, m_dim, a_stride, "A_cm_lt");
            } else {
                matrix_lt_p(a_temp, m_dim, n_dim, a_stride, 0);
                // print_mat(a_temp, m_dim, n_dim, a_stride, "A_rm_lt");
            }
        } else if (mata == 2) {
            if (a_trans) {
                matrix_lt_p(a_temp, n_dim, m_dim, a_stride, 0);
                //print_mat(a_temp, n_dim, m_dim, a_stride, "A_cm_ut");
            } else {
                matrix_ut_p(a_temp, m_dim, n_dim, a_stride, 0);
                // print_mat(a_temp, m_dim, n_dim, a_stride, "A_rm_ut");
            }
        } 



        if (matb == 1) {
            if (b_trans) {
                matrix_ut_p(b_temp, p_dim, n_dim, b_stride, 0);
                //print_mat(b_temp, p_dim, n_dim, b_stride, "b_cm_lt");
            } else {
                matrix_lt_p(b_temp, n_dim, p_dim, b_stride, 0);
                //print_mat(b_temp, n_dim, p_dim, b_stride, "b_rm_ut");
            }
        } else if (matb == 2) {
            if (b_trans) {
                matrix_lt_p(b_temp, p_dim, n_dim, b_stride, 0);
                //print_mat(b_temp, n_dim, p_dim, b_stride, "b_cm_lt");
            } else {
                matrix_ut_p(b_temp, n_dim, p_dim, b_stride, 0);
                //print_mat(b_temp, n_dim, p_dim, b_stride, "b_rm_ut");
            }
        }
        
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
                const float *a  = a_temp + (a_outer_stride * i); // row selection
                const float *b  = b_temp + (b_inner_stride * j); // col selection
                for(uint32_t k = 0; k < N; k++) {
                    acc += a[k * a_inner_stride] * b[k * b_outer_stride];
                }
                uint32_t idx = (c_stride * i) + j;
                c_out[idx] = (beta * c_in[idx]) + (alpha * acc);
            }
        }
        free(a_temp);
        free(b_temp);
    #else
        uint32_t M = m_dim, N = n_dim, P = p_dim;
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
    #endif
}

void
ekf_sw::symmetrize_mat(float *a, uint32_t dim, uint32_t stride)
{
    for(uint32_t j = 0; j < dim; j++) {
        for(uint32_t i = 0; i < j; i++) {
            float &p = a[j*stride + i];
            float &q = a[i*stride + j];
            p = q = (p + q)/2;
        }
    }
}


void ekf_sw::mat_mul(float *a_rm /*sxm*/, float *b_cm /*mxs*/, float *cov_in /*sxs*/,
                     uint32_t m_size, uint32_t s_size,
                     uint32_t a_stride/*m'*/, uint32_t b_stride /*m'*/, uint32_t c_stride/*s'*/)
{
    printf("o.rows:%d, o.cols:%d, a.cols:%d\n", s_size, s_size, m_size);
    printf("a.stride:%d, b.stride:%d, c.stride:%d\n", a_stride, b_stride, c_stride);//gain is transpose have to check
#if BLAS
    cblas_(gemm)(CblasRowMajor, CblasNoTrans, CblasTrans,
                 s_size, s_size, m_size,
                 -1, a_rm, a_stride,
                 b_cm, b_stride,
                 1, cov_in, c_stride);
#else
    mat_mul_c(a_rm, b_cm, cov_in, m_size, s_size, s_size - 1, false/*a_trans*/, true/*b_trans*/,
              -1.0f /*alpha*/, 1.0f /*beta*/, a_stride, b_stride, c_stride, cov_in, 0, 0);
#endif
}
 

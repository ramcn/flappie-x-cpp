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

#pragma once
#include "vio.h"

class VIO_TOP_t;
class VIO_EKF_t;

class ekf_hw : public VIO_Module {    
public:
    ekf_hw();
    ~ekf_hw();
    void cholesky_decomp(const float *a, float *l, uint32_t dim, uint32_t stride);
    void mat_sol(float *l /*sxs*/, float *b /*sxm*/, float *x /*sxm*/, uint32_t s_size, uint32_t m_size,
                 uint32_t l_stride/*m'*/, uint32_t b_stride /*m'*/);
    void mat_mul_c(float *a_rm, float *b_cm, float *c_in, float *c_outr,
                   uint32_t M, uint32_t N, uint32_t P,
                   bool a_trans, bool b_trans, float alpha, float beta,
                   uint32_t a_stride, uint32_t b_stride, uint32_t c_stride);
    void mat_mul(const float *a /*mxn*/, const float *b /*nxp*/, const float *c_in /*mxp*/, float *c_out,
		 uint32_t m_size, uint32_t n_size, uint32_t p_size, bool a_trans, bool b_trans, float alpha,
		 float beta, uint32_t a_stride/*m'*/, uint32_t b_stride /*m'*/, uint32_t c_stride/*s'*/,
		 uint32_t a_t = 0, uint32_t b_t = 0, uint32_t c_t = 0);
    void mat_mul_mod(const float *a_rm, const float *b_cm, const float *c_in, float *c_outr,
                     uint32_t m_size, uint32_t n_size, uint32_t p_size, bool a_trans, bool b_trans,
                     float alpha, float beta, uint32_t a_stride, uint32_t b_stride, uint32_t c_stride,
                     const uint32_t a_buf, const uint32_t b_buf, const uint32_t c_buf, const uint32_t y_buf,
                     uint32_t a_t = 0, uint32_t b_t = 0, uint32_t c_t = 0);
    void process(float *res_cov, float *hp, float *inn, float *cov, float *state_in,
                 uint32_t res_cov_stride, uint32_t hp_stride, uint32_t cov_stride,
                 uint32_t s_size, uint32_t m_size, float *cov_out, float *state_out,
                 float *res_cov_out = NULL, float *k_out = NULL);
    
    void move_y_mat_2_cpu(void *c_out, uint32_t stride, uint32_t rows, bool nb);
    void move_c_mat_2_cpu(void *c_im, uint32_t stride, uint32_t rows, bool nb);
    void move_b_mat_2_cpu(void *b_cm, uint32_t stride, uint32_t rows, bool nb);
    void move_a_mat_2_cpu(void *a_rm, uint32_t stride, uint32_t rows, bool nb);
    float* get_mmul_y_mat();
    float* get_mmul_c_mat();
    float* get_mmul_b_mat();
    float* get_mmul_a_mat();
    float* get_chol_inp_mat();
    float* get_matsol_b_mat();
    void memset_vio_sram();
    void mask_error_status(uint32_t val);
    uint32_t get_error_status();

private:
    VIO_TOP_t *vio_top;
    VIO_EKF_t *vio_ekf;

private:
    void zero_mat(float *a, uint32_t dim, uint32_t stride, bool uplo);
    void symmetrize_mat(float *a, uint32_t dim, uint32_t stride);
    void copy_lt_2_ut(float *a, uint32_t dim, uint32_t stride);
    void inverse_diag(float *a, uint32_t dim, uint32_t stride);
};

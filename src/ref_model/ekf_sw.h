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

#pragma once
#include "stdint.h"

class ekf_sw {
public:
    void cholesky_decomp(float *a, uint32_t stride, uint32_t dim);
    void cholesky_uarch(const float *in, uint32_t stride, uint32_t dim, float *out,
                        int row_stop, int col_stop, const char *file) ;    
    void mat_sol(float *l_cm /*mxm*/, float *lc_rm /*mxs*/, uint32_t m_size, uint32_t s_size,
                 uint32_t l_stride/*m'*/, uint32_t lc_stride /*m'*/);
    void mat_sol_c(float *l_cm /*mxm*/, float *lc_rm /*mxs*/, uint32_t m_size, uint32_t s_size,
                   uint32_t l_stride/*m'*/, uint32_t lc_stride /*m'*/);
    void mat_sol_1(float *l_cm /*mxm*/, float *lc_rm /*mxs*/, uint32_t m_size, uint32_t s_size,
                   uint32_t l_stride/*m'*/, uint32_t lc_stride /*m'*/);
    void mat_sol_2(float *l_cm /*mxm*/, float *lc_rm /*mxs*/, uint32_t m_size, uint32_t s_size,
                   uint32_t l_stride/*m'*/, uint32_t lc_stride /*m'*/);
    void mat_mul(float *a_rm /*sxm*/, float *b_cm /*mxs*/, float *c_in /*sxs*/,
                 uint32_t m_size, uint32_t s_size,
                 uint32_t a_stride/*m'*/, uint32_t b_stride /*m'*/, uint32_t c_stride/*s'*/); 
    void mat_mul_c(const float *a_rm /*mxn*/, const float *b_cm /*nxp*/, const float *c_in /*mxp*/,
                   uint32_t m_dim, uint32_t n_dim, uint32_t p_dim, bool a_trans, bool b_trans,
                   float alpha, float beta, uint32_t a_stride, uint32_t b_stride, uint32_t c_stride,
                   float *c_out, uint32_t mata/*FULL or LT or UT*/,
                   uint32_t matb/*FULL or LT or UT*/);
    void cholesky_c(float *a, uint32_t stride, uint32_t dim);    
    void symmetrize_mat(float *a, uint32_t dim, uint32_t stride);
};
   

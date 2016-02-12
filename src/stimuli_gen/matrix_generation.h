#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"

#pragma once
void matrix_copy(const float *mata, float *matb, uint32_t rows, uint32_t cols, uint32_t stride);
void matrix_gen(float *mat, uint32_t rows, uint32_t cols, uint32_t stride, int seed, int value);
void matrix_transpose(float *mata, float *matb, uint32_t rows, uint32_t cols, uint32_t a_stride, uint32_t b_stride, int seed);
void matrix_diag_inv( float *mata, float *matb, uint32_t rows, uint32_t cols, uint32_t stride, int seed);
void matrix_lt(float *mat, uint32_t rows, uint32_t cols, uint32_t stride, int seed, int diag);
void matrix_ut(float *mat, uint32_t rows, uint32_t cols, uint32_t stride, int seed, int diag);
void matrix_lt_p(float *mat, uint32_t rows, uint32_t cols, uint32_t stride, int seed);
void matrix_ut_p(float *mat, uint32_t rows, uint32_t cols, uint32_t stride, int seed);
void matrix_cm(float *mat, uint32_t rows, uint32_t cols, uint32_t stride, int seed, int num);
void matrix_gen_num_cm(float *mat, uint32_t rows, uint32_t cols, uint32_t stride, int seed, int num);

bool check_mat(float *a, float *b, uint32_t rows, uint32_t cols, uint32_t stride, const char *str);
void matrix_mult(float *mata, float *matb, float *matc, uint32_t rows_a, uint32_t cols_a, uint32_t cols_b,
                 uint32_t stride, int seed);
void matrix_lt_num(float *mat, uint32_t rows, uint32_t cols, uint32_t stride, int seed, int num);
void matrix_gen_sqrt(float *mata, float *matb, float *matc, float *matd, uint32_t rows, uint32_t cols,
                     uint32_t stride, int seed);
void matrix_gen_nan(float *mat, uint32_t rows, uint32_t cols, uint32_t stride, int seed);
void print_matsol_file_cm(FILE *fp, float *mat, uint32_t rows, uint32_t cols, uint32_t stride);

void print_config(FILE *fp, uint32_t width, uint32_t height, uint32_t dthr, uint32_t tthr, uint32_t num_points, uint32_t det1_corners, uint32_t det2_corners, uint32_t CWTen, uint32_t Masken,
                  uint32_t fplistlen, uint32_t fppatlen, uint32_t arows, uint32_t acols, uint32_t bcols, uint32_t x_stride, uint32_t y_stride, uint32_t z_stride, uint32_t test,
                  int mipirate, int detworkset, int cbtrkassignment, int cbdet1assignment, int cbdet2assignment);        
void print_mat_c(const float *mat, uint32_t rows, uint32_t cols, uint32_t stride, const char *str);
void print_mat(float *mat, uint32_t rows, uint32_t cols, uint32_t stride, const char *str);
void print_mat_file_cm(FILE *fp, float *mat, uint32_t rows, uint32_t cols, uint32_t stride, int cr);
void matrix_lower_address(float *mata, float *matb, uint32_t rows, uint32_t cols, uint32_t stride);
void matrix_upper_address(float *mata, float *matb, uint32_t rows, uint32_t cols, uint32_t stride);
void matrix_lower_address_cm(float *mata, float *matb, uint32_t rows, uint32_t cols, uint32_t stride);
void matrix_upper_address_cm(float *mata, float *matb, uint32_t rows, uint32_t cols, uint32_t stride);
bool check_mat(float *a, float *b, uint32_t cols, uint32_t rows, uint32_t stride,
               const char *str);
void mat_mul_c(const float *a_rm /*mxn|nxm*/, const float *b_cm /*nxp|pxn*/, const float *c_in /*mxp*/,
               uint32_t m_dim, uint32_t n_dim, uint32_t p_dim, bool a_trans, bool b_trans,
               float alpha, float beta, uint32_t a_stride, uint32_t b_stride, uint32_t c_stride,
               float *c_out /*mxp*/);
//void cholesky_c(float *a, uint32_t stride, uint32_t dim);
void matrix_gen_num(float *mat, uint32_t rows, uint32_t cols, uint32_t stride, int seed, int num);
//void zero_mat(float *a, uint32_t dim, uint32_t stride, bool uplo);
void matrix_lt_gen(float *mat, float *mata, uint32_t rows, uint32_t cols, uint32_t stride);
void mat_chol_c(float *rmsv_in, float *chl_out, float *chl_in,
                uint32_t rows, uint32_t rmsv_stride, uint32_t chl_out_stride, uint32_t chl_in_stride);
void mat_solve_c(float *a, float *y, uint32_t a_rows, uint32_t y_cols,
                 uint32_t a_stride, uint32_t y_stride,  float *y_out_1, float *b_in, float *a_in);

#include "stdlib.h"
#include "stdint.h"
#include "math.h"
#include "matrix_generation.h"
#include "stdio.h"
#include <string.h>
#include <string>
#include <vector>
#include <iostream>
#include <cmath>
#include <map>
#include <assert.h>
#include <algorithm>
#include "../ref_model/ekf_sw.h"

#define MAX 5


#if LOG
FILE *log_fp = NULL;
#define LOG_PRINTF(...) fprintf(log_fp, __VA_ARGS__ )
#else
#define LOG_PRINTF
#endif


#if LOG
FILE *log_fp = NULL;
#define LOG_PRINTF(...) fprintf(log_fp, __VA_ARGS__ )
#else
#define LOG_PRINTF
#endif

ekf_sw s;

void print_mat_c(const float *mat, uint32_t rows, uint32_t cols, uint32_t stride,
               const char *str)
{
    printf("Mat %s\n", str);
    for(uint32_t i = 0; i < rows; i++) {
        for(uint32_t j = 0; j < cols; j++) {
            printf("%11.4e ", mat[(i * stride) + j]);
        }
        printf("\n");
    }
}

void print_mat(float *mat, uint32_t rows, uint32_t cols, uint32_t stride,
               const char *str)
{
    printf("Mat %s\n", str);
    for(uint32_t i = 0; i < rows; i++) {
        for(uint32_t j = 0; j < cols; j++) {
            printf("%11.4e ", mat[(i * stride) + j]);
        }
        printf("\n");
    }
}


void print_matsol_file_cm(FILE *fp, float *mat, uint32_t rows, uint32_t cols, uint32_t stride)
{
    fprintf(fp, "//mat_size col= %d, row=%d, stride = %d\n", cols, rows, stride);
    for (int i=0; i < cols; i++)
        {
            for(int k=0; k < rows; k += 4)
            {
                if(k/4 == rows/4){
                    int x= (4 - (rows%4));
                    int y;
                    for (y=0;y<x; y++){
                        fprintf(fp, "BAADF00D");
                    }

                }
                int j = k+3;
                if (j>rows-1)
                    j=rows-1;
            
                for(; j>=k; j--) {
                    fprintf(fp, "%08X", *(int*) &mat[(i * stride) + j]);
                }
                fprintf(fp, "\n");
            }
            if ((rows%8 !=0) && (rows%8 <= 4)){
                fprintf(fp, "BAADF00DBAADF00DBAADF00DBAADF00D\n");
            }
        }

}


void print_mat_file_cm(FILE *fp, float *mat, uint32_t rows, uint32_t cols, uint32_t stride, int cr)
{

#if 0
    fprintf(fp, "//Rows: %d, Cols: %d, Stride: %d\n", rows, cols, stride);
#endif

#if 1
    fprintf(fp, "//mat_size col= %d, row=%d, stride = %d\n", cols, rows, stride);

#endif 
    if (cr == 1){
        
        for (int i=0; i < rows; i++)
        {
            for(int k=0; k < cols; k += 4)
            {
                if(k/4 == cols/4){
                    int x= (4 - (cols%4));
                    int y;
                    for (y=0;y<x; y++){
                        fprintf(fp, "BAADF00D");
                    }

                }
                int j = k+3;
                if (j>cols-1)
                    j=cols-1;
            
                for(; j>=k; j--) {
                    fprintf(fp, "%08X", *(int*) &mat[(i * stride) + j]);
                }
                fprintf(fp, "\n");
            }
            if ((cols%8 !=0) && (cols%8 <= 4)){
                fprintf(fp, "BAADF00DBAADF00DBAADF00DBAADF00D\n");
            }
        }

    } else {
        for (int i=0; i < cols; i++)
        {
            for(int k=0; k < rows; k += 4)
            {
                if(k/4 == rows/4){
                    int x= (4 - (rows%4));
                    int y;
                    for (y=0;y<x; y++){
                        fprintf(fp, "BAADF00D");
                    }

                }
                int j = k+3;
                if (j>rows-1)
                    j=rows-1;
            
                for(; j>=k; j--) {
                    fprintf(fp, "%08X", *(int*) &mat[(j * stride) + i]);
                }
                fprintf(fp, "\n");
            }
            if ((rows%8 !=0) && (rows%8 <= 4)){
                fprintf(fp, "BAADF00DBAADF00DBAADF00DBAADF00D\n");
            }
        }
    }
}



bool check_mat(float *a, float *b, uint32_t rows, uint32_t cols, uint32_t stride,
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
	    if((ratio > 0.001f) || std::isnan(a[i]) || std::isnan(b[i])) {
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



void matrix_gen(float *mat, uint32_t rows, uint32_t cols, uint32_t stride, int seed, int value)
{
  srand(seed);
  // 0 for random numbers
  // 1 for all values = 1
  // 2 for all values = 0
  // 3 for identity matrix
  // 4 for lower triangular matrix = 1
  // 5 for upper triangular matrix = 1
  for(uint32_t i = 0; i < rows; i++) {
    for(uint32_t j = 0; j < cols; j++) {
      if (value == 0){
	//mat[(i * stride) + j] = MAX*((float)(rand() % MAX)/(float)MAX -0.5);//-MAX/2 to +MAX/2
	mat[(i * stride) + j] =  ((i * stride) + j + 1) % 50;
      } else if(value == 1){
	mat[(i * stride) + j] = 1;
      } else if (value == 2) {
	mat[(i * stride) + j] = 0;
      } else if (value == 3){
	if (i == j){
	  mat[(i * stride) + j] = 1;
	}  else {
	  mat[(i * stride) + j] = 0;
	}
      }else if (value == 4){
	if ( i < j) {
	  mat[(i * stride) + j] = 0;
	} else {
	  mat[(i * stride) + j] = 1;
	}
      } else if (value == 5){
	if ( i > j) {
	  mat[(i * stride) + j] = 0;
	} else {
	  mat[(i * stride) + j] = 1;
	}
      }
    }
  }	
}

void matrix_lower_address(float *mata, float *matb, uint32_t rows, uint32_t cols, uint32_t stride)
{
 
  uint32_t Rd = 0;
  uint32_t Cd = 0;
  uint32_t count = 0;
 
  for(uint32_t i = 0; i < rows; i++){
    for (uint32_t j = 0; j < cols; j++){
      Rd = i/4;
      Cd = j/4;
      if (Rd < Cd){
	printf("// i=%d, j=%d, count=%d\n", i, j, count);
      } else {
	  matb[count] = mata[(i*stride) + j];
	  printf("//else i=%d, j=%d, count=%d, mat_value:%f\n", i, j, count, mata[(i*stride) +j]);
          count++;
      }
    }
  }
}

void matrix_lower_address_cm(float *mata, float *matb, uint32_t rows, uint32_t cols, uint32_t stride)
{
  uint32_t Rd = 0;
  uint32_t Cd = 0;
  int count = 0;
  for(uint32_t i = 0; i < rows; i++){
    for (uint32_t j = 0; j < cols; j++){
      Rd = i/4;
      Cd = j/4;
      if (Rd < Cd){
	//printf("//if i=%d, j=%d, count=%d\n", i, j, count);

      } else{
          matb[count] = mata[(j*stride) + i];
          //printf("//else i=%d, j=%d, count=%d, mat_value:%f\n", i, j, count, mata[(i*stride) +j]);
          count++;
      }
    }
  }
}



void matrix_upper_address(float *mata, float *matb, uint32_t rows, uint32_t cols, uint32_t stride)
{
  uint32_t Rd = 0;
  uint32_t Cd = 0;
  int count = 0;
  for(uint32_t i = 0; i < rows; i++){
    for (uint32_t j = 0; j < cols; j++){
      Rd = i/4;
      Cd = j/4;
      if (Rd > Cd){
	//printf("//if i=%d, j=%d, count=%d\n", i, j, count);

      } else{
          matb[count] = mata[(i*stride) + j];
          //printf("//else i=%d, j=%d, count=%d, mat_value:%f\n", i, j, count, mata[(i*stride) +j]);
          count++;
      }
    }
  }
}


void matrix_upper_address_cm(float *mata, float *matb, uint32_t rows, uint32_t cols, uint32_t stride)
{
  uint32_t Rd = 0;
  uint32_t Cd = 0;
  int count = 0;
  for(uint32_t i = 0; i < rows; i++){
    for (uint32_t j = 0; j < cols; j++){
      Rd = i/4;
      Cd = j/4;
      if (Rd > Cd){
	//printf("//if i=%d, j=%d, count=%d\n", i, j, count);

      } else{
          matb[count] = mata[(j*stride) + i];
          //printf("//else i=%d, j=%d, count=%d, mat_value:%f\n", i, j, count, mata[(i*stride) +j]);
          count++;
      }
    }
  }
}


void matrix_diag_inv( float *mata, float *matb, uint32_t rows, uint32_t cols, uint32_t stride, int seed)
{
    for(uint32_t i = 0; i < rows; i++) {
        for(uint32_t j = 0; j < cols; j++) {
            if ( i == j){
                float a = 1.0;
                float b = mata[(i * stride) + j];
                matb[(i * stride) + j] = a/b;
            } else {
                matb[(i * stride) + j] = mata[(i * stride) + j];
            }
        }
    }
}

void matrix_transpose(float *mata, float *matb, uint32_t rows, uint32_t cols, uint32_t a_stride, uint32_t b_stride, int seed)
{
    //srand(seed);
    for(uint32_t i = 0; i < rows; i++) {
        for(uint32_t j = 0; j < cols; j++) {
            matb[(j * b_stride) + i] = mata[(i * a_stride) + j];
        }
    }
}


void matrix_copy(const float *mata, float *matb, uint32_t rows, uint32_t cols, uint32_t stride)
{
    //srand(seed);
        for(uint32_t i = 0; i < rows; i++) {
            for(uint32_t j = 0; j < cols; j++) {
                matb[(i * stride) + j] = mata[(i * stride) + j];
            }
        }
}




void matrix_lt_gen(float *mat, float *mata, uint32_t rows, uint32_t cols, uint32_t stride, int diag)
{
    //srand(seed);
    for(uint32_t i = 0; i < rows; i++) {
        for(uint32_t j = 0; j < cols; j++) {
            if (i<j){
                mata[(i * stride) +j] = 0;
            } else if ((diag == 1) && (i == j)){
	      mata[(i * stride) + j] = MAX*((float)(rand() % MAX + 449)/(float)MAX -0.5);
	      //mata[(i * stride) + j] = 1;
            } else {
                mata[(i * stride) + j] = mat[(i * stride) +j];
            }
        }
    }
}

void matrix_lt(float *mat, uint32_t rows, uint32_t cols, uint32_t stride, int seed, int diag)
{
    //srand(seed);
    for(uint32_t i = 0; i < rows; i++) {
        for(uint32_t j = 0; j < cols; j++) {
            if (i<j){
                mat[(i * stride) +j] = 0;
            } else if ((diag == 1) && (i == j)){
                mat[(i * stride) + j] = MAX*((float)(rand() % MAX + 449)/(float)MAX -0.5);// 0 to +MAX/2
            } else {
                mat[(i * stride) + j] = MAX*((float)(rand() % MAX)/(float)MAX -0.5);//-MAX/2 to +MAX/2
            }
        }
    }
}

void matrix_ut(float *mat, uint32_t rows, uint32_t cols, uint32_t stride, int seed, int diag)
{
    //srand(seed);
    for(uint32_t i = 0; i < rows; i++) {
        for(uint32_t j = 0; j < cols; j++) {
            if (i>j){
                mat[(i * stride) +j] = 0;
            } else if ((diag == 1) && (i == j)){
                mat[(i * stride) + j] = MAX*((float)(rand() % MAX + 449)/(float)MAX -0.5);// 0 to +MAX/2
            } else {
                mat[(i * stride) + j] = MAX*((float)(rand() % MAX)/(float)MAX -0.5);//-MAX/2 to +MAX/2
            }
        }
    }
}

void matrix_lt_p(float *mat, uint32_t rows, uint32_t cols, uint32_t stride, int seed)
{
    //srand(seed);
        for(uint32_t i = 0; i < rows; i++) {
            for(uint32_t j = 0; j < cols; j++) {
                if (i<j){
                    mat[(i * stride) +j] = 0;
                } else {
                    mat[(i * stride) + j] = mat[(i * stride) + j];
                }
            }
        }
}

void matrix_ut_p(float *mat, uint32_t rows, uint32_t cols, uint32_t stride, int seed)
{
    //srand(seed);
        for(uint32_t i = 0; i < rows; i++) {
            for(uint32_t j = 0; j < cols; j++) {
                if (i>j){
                    mat[(i * stride) +j] = 0;
                } else {
                    mat[(i * stride) + j] = mat[(i * stride) + j];
                } 
            }
        }
}




void matrix_mult(float *mata, float *matb, float *matc, uint32_t rows_a, uint32_t cols_a, uint32_t cols_b, uint32_t stride, int seed)
{
    for (uint32_t i = 0; i < rows_a; i++)
    {
        for (uint32_t j = 0; j < cols_b; j++)
        {
            float acc = 0;
            const float *a  = mata + (stride * i);
            const float *b  = matb + (j);
            //computing the C(product) matrix
            for (uint32_t k = 0; k < cols_a; k++)
            {
                //printf("a:%f, b:%f, i:%d, j:%d, k:%d\n", a[k], b[(k * stride)], i, j, k);
                acc += a[k] * b[(k * stride)];
                //printf("acc:%f\n", acc);
            }
            matc[(stride * i) + j] = acc; 
         
        }
    }
}

void ms_matrix_mult_cm(float *mata, float *matb, float *matc, uint32_t m, uint32_t n, uint32_t a_stride, uint32_t b_stride, uint32_t c_stride)
{
    for (uint32_t i = 0; i < m; i++)
    {
        for (uint32_t j = 0; j < n; j++)
        {
            float acc = 0;
            const float *a  = mata + i;
            const float *b  = matb + (j*b_stride);
            //computing the C(product) matrix
            for (uint32_t k = 0; k < m; k++)
            {
                //printf("a:%f, b:%f, i:%d, j:%d, k:%d\n", a[k], b[(k * stride)], i, j, k);
                acc += b[k] * a[(k * a_stride)];
                //printf("acc:%f\n", acc);
            }
            matc[(c_stride * j) + i] = acc; 
         
        }
    }
}


void mat_mul_c(const float *a_rm /*mxn|nxm*/, const float *b_cm /*nxp|pxn*/, const float *c_in /*mxp*/,
              uint32_t m_dim, uint32_t n_dim, uint32_t p_dim, bool a_trans, bool b_trans,
              float alpha, float beta, uint32_t a_stride, uint32_t b_stride, uint32_t c_stride,
              float *c_out /*mxp*/)
    {
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
    }





    
void mat_chol_c(float *rmsv_in, float *chl_out, float *chl_in,
                uint32_t rows, uint32_t rmsv_stride, uint32_t chl_out_stride, uint32_t chl_in_stride)//, uint32_t seed)
{
    bool status = 0;
    float *c_zero = (float *)malloc(rows * rmsv_stride * sizeof(float));
    float *chl_in_sw = (float *)malloc(rows * rmsv_stride * sizeof(float));
    float *chl_out_t= (float *)malloc(rows * rmsv_stride * sizeof(float));
    float *chl_out_inv= (float *)malloc(rows * rmsv_stride * sizeof(float));
    
//    matrix_gen(p, rows, rows, rmsv_stride, 2, 0);
//    matrix_lt_gen(p, chl_out_inv, rows, rows, rmsv_stride, 1);
    matrix_lt_gen(rmsv_in, chl_out_inv, rows, rows, rmsv_stride, 1);
    matrix_transpose(chl_out_inv, chl_out_t, rows, rows, rmsv_stride, rmsv_stride, 0);
    matrix_gen(c_zero, rows, rows, rmsv_stride, 0, 2);
    mat_mul_c(chl_out_inv, chl_out_t, c_zero, rows, rows, rows, 0, 0, 1, 1, rmsv_stride, rmsv_stride, rmsv_stride, chl_in);
    matrix_diag_inv(chl_out_t, chl_out, rows, rows, rmsv_stride, 1);
//    print_mat(chl_in, rows, rows, rmsv_stride, "GSK-Chol_in Input");
//    print_mat(chl_out_t, rows, rows, rmsv_stride, "GSK-Chol_out_inv Input");
//    print_mat(chl_out, rows, rows, rmsv_stride, "GSK-Chol_out Output");

    free(c_zero);
    free(chl_in_sw);
    free(chl_out_t);
    free(chl_out_inv);
//    matrix_copy(chl_in, chl_in_sw, rows, rows, rmsv_stride);
//    s.cholesky_c(chl_in_sw, rows, rmsv_stride);
//    status = check_mat(chl_in_sw, chl_out_inv, rows, rows, rmsv_stride, "cholesky_out");
//    printf("Status: %d\n", status);
}




void mat_solve_c(float *a, float *y, uint32_t a_rows, uint32_t y_cols,
                 uint32_t a_stride, uint32_t y_stride, float *y_out_1_t, float *b_in_t, float *a_in)
{
    
    bool status_1 = 0;
    bool status_2 = 0;
    
    float *a_lt= (float *)malloc(a_rows * a_stride * sizeof(float));
    float *a_lt_t= (float *)malloc(a_rows * a_stride * sizeof(float));
    float *a_in_pre= (float *)malloc(a_rows * a_stride * sizeof(float));


    matrix_lt_gen(a, a_lt, a_rows, a_rows, a_stride, 1);//a lower triangle in RM
    matrix_transpose(a_lt, a_lt_t, a_rows, a_rows, a_stride, a_stride, 0); // a_lt_t (a_lt in CM)
    

    ms_matrix_mult_cm(a_lt, y, y_out_1_t, a_rows, y_cols, a_stride, y_stride, y_stride); // Giving a_lt RM (this will be a_lt_t in CM)
    ms_matrix_mult_cm(a_lt_t, y_out_1_t, b_in_t, a_rows, y_cols, a_stride, y_stride, y_stride); // Giving a_lt_t RM (this will be alt_t in CM)


    matrix_diag_inv(a_lt, a_in_pre, a_rows, a_rows, a_stride, 1);// diagonal inverse:: a input    
    matrix_transpose(a_in_pre, a_in, a_rows, a_rows, a_stride, a_stride, 0); 

    /*
    print_mat(a,         a_rows, a_rows, a_stride, "ms_mat_a_SV");
    print_mat(a_lt,      a_rows, a_rows, a_stride, "ms_mat_a_lt");    
    print_mat(a_lt_t,    a_rows, a_rows, a_stride, "ms_mat_a_lt_t");    
    print_mat(b_in_t,    y_cols, a_rows, y_stride, "ms_mat_b");
    print_mat(y_out_1_t, y_cols, a_rows, y_stride, "ms_mat_x1");
    print_mat(y,         y_cols, a_rows, y_stride, "ms_mat_x2");
    */

    
    //verification
#if 0
    float *y_out_1_sw = (float*)malloc(a_rows * y_stride * sizeof(float));
    float *b_in_sw = (float*)malloc(a_rows * y_stride * sizeof(float));

    matrix_copy(y_out_1, y_out_1_sw, a_rows, y_cols, y_stride);//copying b stage 1 input matrix to sw input
    matrix_copy(b_in, b_in_sw, a_rows, y_cols, y_stride);//copying b stage 2 input matrix to sw input

    s.mat_sol_1(a_in, b_in_sw, a_rows, y_cols, a_stride, y_stride);
    print_mat(y_out_1_sw, a_rows, y_cols, y_stride, "y_out_1_sw");
    status_1 = check_mat(b_in_sw, y_out_1_sw, a_rows, y_cols, y_stride, "mat_sol stage 1");

    s.mat_sol_c(a_in, b_in_sw, a_rows, y_cols, a_stride, y_stride);
    print_mat(b_in, a_rows, y_cols, y_stride, "b_in_sw");

    status_2 = check_mat(y, b_in_sw, a_rows, y_cols, y_stride, "mat_sol stage 2");
    printf("Status_1:%d, Status_2:%d\n", status_1, status_2);
#endif
}


void matrix_lt_num(float *mat, uint32_t rows, uint32_t cols, uint32_t stride, int seed, int num)
{
    srand(seed);
    for(uint32_t i = 0; i < rows; i++) {
        for(uint32_t j = 0; j < cols; j++) {
            if (i<j){
                mat[(i * stride) +j] = 0;
            } else if ((i == 3) && ( i > j)){
                if (num == 0){
                    mat[(i * stride) +j] = 0;
                } else if (num == 1) {
                    mat[(i * stride) + j] = (powf(10, 39) - 1);
                } else if (num == 2) {
                    mat[(i * stride) + j] = powf(10, 15);
                } else if (num == 3) {
                    mat[(i * stride) + j] = powf(10, -15);
                }
            } else if ((j == 4) && (i > j)){
                if (num == 0){
                    mat[(i * stride) +j] = 0;
                } else if (num == 1) {
                    mat[(i * stride) + j] = (powf(10, 39) - 1);
                } else if (num ==2) {
                    mat[(i * stride) + j] = powf(10, 15);
                } else if (num ==3) {
                    mat[(i * stride) + j] = powf(10, -15);
                }
            }else if (i == j){
                mat[(i * stride) + j] = MAX*((float)(rand() % MAX + 449)/(float)MAX -0.5);// 0 to +MAX/2
            } else {
                mat[(i * stride) + j] = MAX*((float)(rand() % MAX)/(float)MAX -0.5);//-MAX/2 to +MAX/2
            }
        }
    }
}



void matrix_cm(float *mat, uint32_t rows, uint32_t cols, uint32_t stride, int seed, int num)
{
    srand(seed);
    for(uint32_t i = 0; i < rows; i++) {
        for(uint32_t j = 0; j < cols; j++) {
           mat[(j * stride) + i] = MAX*((float)(rand() % MAX)/(float)MAX -0.5);//-MAX/2 to +MAX/2
        }
    }
}



void matrix_gen_num_cm(float *mat, uint32_t rows, uint32_t cols, uint32_t stride, int seed, int num)
{
    srand(seed);
    for(uint32_t i = 0; i < rows; i++) {
        for(uint32_t j = 0; j < cols; j++) {
            if (i==2){
                if (num == 0){
                    mat[(j * stride) + i] = 0;
                } else if (num == 1) {
                    mat[(j * stride) + i] = (powf(10, 39) - 1);
                } else if (num ==2) {
                    mat[(j * stride) + i] = powf(10, 25);
                } else if (num ==3) {
                    mat[(j * stride) + i] = powf(10, -25);
                }
            } else if (j==3){
                if (num == 0){
                    mat[(j * stride) +i] = 0;
                } else if (num == 1) {
                    mat[(j * stride) + i] = (powf(10, 39) - 1);
                } else if (num ==2) {
                    mat[(j * stride) + i] = powf(10, 25);
                } else if (num ==3) {
                    mat[(j * stride) + i] = powf(10, -25);
                }
            } else {
                mat[(j * stride) + i] = MAX*((float)(rand() % MAX)/(float)MAX -0.5);//-MAX/2 to +MAX/2
            }
        }
    }
}





void matrix_gen_num(float *mat, uint32_t rows, uint32_t cols, uint32_t stride, int seed, int num)
{
    srand(seed);
    for(uint32_t i = 0; i < rows; i++) {
        for(uint32_t j = 0; j < cols; j++) {
            if (i==2){
                if (num == 0){
                    mat[(i * stride) +j] = 0;
                } else if (num == 1) {
                    mat[(i * stride) + j] = (powf(10, 39) - 1);
                } else if (num ==2) {
                    mat[(i * stride) + j] = powf(10, 19);
                } else if (num ==3) {
                    mat[(i * stride) + j] = powf(10, -19);
                }
            } else if (j==3){
                if (num == 0){
                    mat[(i * stride) +j] = 0;
                } else if (num == 1) {
                    mat[(i * stride) + j] = (powf(10, 39) - 1);
                } else if (num ==2) {
                    mat[(i * stride) + j] = powf(10, 19);
                } else if (num ==3) {
                    mat[(i * stride) + j] = powf(10, -19);
                }
            } else {
                mat[(i * stride) + j] = MAX*((float)(rand() % MAX)/(float)MAX -0.5);//-MAX/2 to +MAX/2
            }
        }
    }
}




void matrix_gen_sqrt(float *mata, float *matb, float *matc, float *matd, uint32_t rows,
                     uint32_t cols, uint32_t stride, int seed)
{

    matrix_lt(mata, rows, cols, stride, seed, 1);
    matrix_transpose(mata, matb, rows, cols, stride, stride, 1);
    matrix_gen(matc, rows, cols, stride, 0, 2);
    mat_mul_c(mata, matb, matc, rows, cols, cols, 0, 0, 1, 1, stride, stride, stride, matd);

    int r = 3; int c = 3;

    float beta = mata[(r * stride) + c];
    float alpha = matc[(r * stride) + c];
    float delta = 30;
    printf("beta = %11.e, alpha = %11.e\n", beta, alpha);
    matd[(r * stride) + c] = (alpha - (beta * beta))- delta;      
}

void matrix_gen_nan(float *mat, uint32_t rows, uint32_t cols, uint32_t stride, int seed)
{
    srand(seed);
    for(uint32_t i = 0; i < rows; i++) {
        for(uint32_t j = 0; j < cols; j++) {
            if ((i==2)&&(j==2)){
                mat[(i * stride) + j] =  NAN;
            } else {
            mat[(i * stride) + j] = MAX*((float)(rand() % MAX)/(float)MAX -0.5);//-MAX/2 to +MAX/2
            }
        }
    }
}

void print_config(FILE *fp, uint32_t width, uint32_t height, uint32_t dthr, uint32_t tthr, uint32_t num_points, uint32_t det1_corners, uint32_t det2_corners, uint32_t CWTen, uint32_t Masken,
                  uint32_t fplistlen, uint32_t fppatlen, uint32_t arows, uint32_t acols, uint32_t bcols, uint32_t x_stride, uint32_t y_stride, uint32_t z_stride, uint32_t test, int mipirate, int detworkset,
                  int cbtrkassignment, int cbdet1assignment, int cbdet2assignment)
{

    printf("cbtrkassign: %d, cbdet1assign: %d, cbdet2assign: %d\n", cbtrkassignment, cbdet1assignment, cbdet2assignment);
  
#if MATMUL
    fprintf(fp, "00000020 //ModuleEn = 00100000(MS, MC, CD, T, D, S, P)\n");
#endif
#if MATSOL
    fprintf(fp, "00000040 //ModuleEn = 01000000(MS, MC, CD, T, D, S, P)\n");
#endif
    
#if CHOL
    fprintf(fp, "00000010 //ModuleEn = 00010000(MS, MC, CD, T, D, S, P)\n");
#endif
#if DETECTOR_CONFIG && !TRACKER_CONFIG
    fprintf(fp, "00000007 //ModuleEn = 00000111(MS, MC, CD, T, D, S, P)\n");
#endif
#if TRACKER_CONFIG && !DETECTOR_CONFIG
    fprintf(fp, "00000008 //ModuleEn = 00001000(MS, MC, CD, T, D, S, P)\n");
#endif
#if DETECTOR_CONFIG && TRACKER_CONFIG
    fprintf(fp, "0000000F //ModuleEn = 00001111(MS, MC, CD, T, D, S, P)\n");
#endif

    fprintf(fp, "%08X //CholDecompMatSize: %d\n", arows, arows);
    fprintf(fp, "%08X //CholDecompMatStride: %d\n", x_stride/4, x_stride/4);
    fprintf(fp, "BF800000 //MatMulCov Alpha\n");
    fprintf(fp, "3F800000 //MatMulCov Beta\n");
#if MATSOL
    fprintf(fp, "%08X //MatMulCov Matrix A rows: %d\n", bcols, bcols);
#else
    fprintf(fp, "%08X //MatMulCov Matrix A rows: %d\n", arows, arows);
#endif
    fprintf(fp, "%08X //MatMulCov Matrix A cols: %d\n", acols, acols);
    fprintf(fp, "%08X //MatMulCov Matrix A stride: %d\n", x_stride/4, x_stride/4);
    fprintf(fp, "%08X //MatMulCov Matrix B rows: %d\n", acols, acols);
    fprintf(fp, "%08X //MatMulCov Matrix B cols: %d\n", bcols, bcols);
#if MATSOL    
    fprintf(fp, "%08X //MatMulCov Matrix B stride: %d\n", x_stride/4, x_stride/4);
#else
    fprintf(fp, "%08X //MatMulCov Matrix B stride: %d\n", y_stride/4, y_stride/4);
#endif
    fprintf(fp, "%08X //MatMulCov Matrix C rows: %d\n", arows, arows);
    fprintf(fp, "%08X //MatMulCov Matrix C cols: %d\n", bcols, bcols);
    fprintf(fp, "%08X //MatMulCov Matrix C stride: %d\n", z_stride/4, z_stride/4);
    fprintf(fp, "00000002 //MatSolve skip-stage (0-skip none, 1-skip stage-1, 2-skip stage-2)\n");   


    fprintf(fp, "%08X //Image1Width: %d\n", width, width);
    fprintf(fp, "%08X //Image1Height: %d\n", height, height);
    fprintf(fp, "%08X //Image2Width: %d\n", width, width);
    fprintf(fp, "%08X //Image2Height: %d\n", height, height);
    fprintf(fp, "00000000 //CircularBuffer1Depth: 0\n");// 0 or 1 
    fprintf(fp, "00000000 //CircularBuffer2Depth: 0\n");// 0 
    fprintf(fp, "%08X //MIPI1Rate: %d\n", mipirate, mipirate);
    fprintf(fp, "%08X //MIPI2Rate: %d\n", mipirate, mipirate);
    
#if CW_EN    
    fprintf(fp, "%08X //TrkCWTen:%d\n", CWTen, CWTen);
#else
    fprintf(fp, "%08X //TrkCWTen:%d\n", 0, 0);
#endif

    fprintf(fp, "%08X //TrkThreshold:%d\n", tthr, tthr);
    fprintf(fp, "%08X //TrkFPListLen:%d\n", fplistlen, fplistlen);
    fprintf(fp, "%08X //TrkFPPatchListLen:%d\n", fppatlen, fppatlen);
    fprintf(fp, "00000001 //TrkMode: 1 (0: non otf 1: otf)\n");

#if 1
    fprintf(fp, "%08X //TrkCBAssignment: %d\n", cbtrkassignment, cbtrkassignment);
#else
    fprintf(fp, "00000003 //TrkCBAssignment: 3\n", );
#endif 
    
#if CW_EN    
    fprintf(fp, "%08X //Det1CWTen:%d\n", CWTen, CWTen);
#else
    fprintf(fp, "%08X //Det1CWTen:%d\n", 0, 0);
#endif

#if MASK_EN    
    fprintf(fp, "%08X //Det1MaskEn:%d\n", Masken, Masken);
#else
    fprintf(fp, "%08X //Det1MaskEn:%d\n", 0, 0);
#endif
    fprintf(fp, "%08X //Det1Threshold:%d\n", dthr, dthr);
    fprintf(fp, "%08X //Det1NumCorner:%d\n", det1_corners, det1_corners);
    if (det1_corners > num_points){
        fprintf(fp, "%08X //Det1NumSortedCorner:%d\n", num_points, num_points);
    } else {
        fprintf(fp, "%08X //Det1NumSortedCorner:%d\n", det1_corners, det1_corners); 
    }
    fprintf(fp, "00000000 //Det1SortDelta:0\n");
    fprintf(fp, "00000001 //Det1Mode: 1\n");
#if 1    
    fprintf(fp, "%08X //Det1CBAssignment: %d\n", cbdet1assignment, cbdet1assignment);
#else
    fprintf(fp, "00000001 //Det1CBAssignment: 1\n");
#endif 
    fprintf(fp, "%08X //Det1WorkingSetSize: %d\n", detworkset, detworkset);
    fprintf(fp, "00000001 //Det1ExportCorners: 1\n");
    fprintf(fp, "00000000 //Det1ExportPatches: 0\n");
    fprintf(fp, "00000000 //Det1DynamicThresholdEnable: 0\n");
   
#if CW_EN    
    fprintf(fp, "%08X //Det2CWTen:%d\n", CWTen, CWTen);
#else
    fprintf(fp, "%08X //Det2CWTen:%d\n", 0, 0);
#endif

#if MASK_EN   
    fprintf(fp, "%08X //Det2MaskEn:%d\n", Masken, Masken);
#else
    fprintf(fp, "%08X //Det2MaskEn:%d\n", 0, 0);
#endif 
    fprintf(fp, "%08X //Det2Threshold:%d\n", dthr, dthr);
    fprintf(fp, "%08X //Det2NumCorner:%d\n", det2_corners, det2_corners);
    if (det2_corners > num_points){
        fprintf(fp, "%08X //Det2NumSortedCorner:%d\n", num_points, num_points);
    } else {
        fprintf(fp, "%08X //Det2NumSortedCorner:%d\n", det2_corners, det2_corners); 
    }
    fprintf(fp, "00000000 //Det2SortDelta:0\n");
    fprintf(fp, "00000001 //Det2Mode: 1\n");
#if 1
    fprintf(fp, "%08X //Det2CBAssignment: %d\n", cbdet2assignment, cbdet2assignment);
#else
    fprintf(fp, "00000002 //Det1CBAssignment: 2\n");
#endif 
    fprintf(fp, "%08X //Det2WorkingSetSize: %d\n", detworkset, detworkset);
    fprintf(fp, "00000001 //Det2ExportCorners: 1\n");
    fprintf(fp, "00000000 //Det2ExportPatches: 0\n");
    fprintf(fp, "00000000 //Det2DynamicThresholdEnable: 0\n");

#if CHOL

    if (test == 5){
        int errorbit = 65536;// zero error bit
        fprintf(fp, "%08X //Errorbits: Cholesky Zero", errorbit);
    }
    if (test == 6){
        int errorbit = 131072;// inf error bit
        fprintf(fp, "%08X //Errorbits: Cholesky INF", errorbit);
    }
    if (test == 9){
        int errorbit = 262144;// NAN error bit
        fprintf(fp, "%08X //Errorbits:Cholesky Nan", errorbit);
    }
    if (test == 8){
        int errorbit = 524288;// TINY error bit
        fprintf(fp, "%08X //Errorbits:Cholesky TINY", errorbit);

    }
    if (test == 7){
        int errorbit = 1048576;// HUGH error bit
        fprintf(fp, "%08X //Errorbits:Cholesky HUGH", errorbit, errorbit);

    }
#endif

#if MATSOL
    if (test == 20){
        int errorbit = 0;// zero error bit
        fprintf(fp, "%08X //Errorbits: MATSOL No error", errorbit);
    }
    
    if (test == 21){
        int errorbit = 256;// zero error bit
        fprintf(fp, "%08X //Errorbits: MATSOL Zero", errorbit);
    }
    if (test == 22){
        int errorbit = 512;// inf error bit
        fprintf(fp, "%08X //Errorbits: MATSOL INF", errorbit);
    }
    if (test == 23){
        int errorbit = 2048;// TINY error bit
        fprintf(fp, "%08X //Errorbits:MATSOL TINY", errorbit);

    }
    if (test == 24){
        int errorbit = 4096;// HUGH error bit
        fprintf(fp, "%08X //Errorbits:MATSOL HUGH", errorbit, errorbit);

    }
#endif  
    
#if MATMUL
    if (test == 11){
        int errorbit = 0;// No error bit
        fprintf(fp, "%08X //Errorbits:Matmul No error bit", errorbit);
    }
    
    if (test == 12){
        int errorbit = 1;// zero error bit
        fprintf(fp, "%08X //Errorbits:Matmul Zero", errorbit);
    }
    if (test == 13){ 
        int errorbit = 6;//INF and NAN error bit
        fprintf(fp, "%08X //Errorbits:Matmul INF and NAN", errorbit);
    }
    if (test == 15){
        int errorbit = 8;//TINY error bit
        fprintf(fp, "%08X //Errorbits:Matmul Tiny", errorbit);
    }
    if (test == 14){
        int errorbit = 16;//HUGH error bit
        fprintf(fp, "%08X //Errorbits:Matmul Hugh", errorbit);
    }
    if (test == 16){
        int errorbit = 3;//zero and INF error bit
        fprintf(fp, "%08X //Errorbits:Matmul Zero and INF", errorbit);
    }
    if (test == 17){
        int errorbit = 9;//zero and TINY error bit
        fprintf(fp, "%08X //Errorbits:Matmul Zero and Tiny", errorbit);
    }
    if (test == 18){
        int errorbit = 17;//zero and HUGH error bit
        fprintf(fp, "%08X //Errorbits:Matmul zero and Hugh", errorbit);
    }
    if (test == 19){
        int errorbit = 24;//TINY and HUGH error bit
        fprintf(fp, "%08X //Errorbits:MatMul Tiny and Hugh", errorbit);
    }
#endif

#if DETECTOR_CONFIG
    int errorbit = 0;
    fprintf(fp, "%08X //Errorbits:%d", errorbit, errorbit);

#endif


}

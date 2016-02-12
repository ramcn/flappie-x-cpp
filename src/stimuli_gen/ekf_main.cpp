#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <assert.h>
#include <algorithm>
#include "hw/ekf_hw.h"
#define _BSD_SOURCE
#include <sys/time.h>
#include "matrix_generation.h"
#include <random>




int main (int argc, char *argv[])
{

  struct timeval time; 
  gettimeofday(&time, NULL);
  srand((time.tv_sec * 1000) + (time.tv_usec / 1000));
  int seed = rand();
  //int seed = 530281585;
  printf("************\n");
  printf("seed: %d\n", seed);
  printf("************\n");
 
  if(argc <= 2){
        printf("Usage: <exe> <amat_rows> <amat_cols> <bmat_cols> <a_trans> <b_trans> <a_comp> <b_comp> <c_comp> <a_t> <b_t> <c_t>\n");
        exit(1);
    }
    uint32_t a_mat_rows = atoi(argv[1]);
    uint32_t a_mat_cols = atoi(argv[2]);
    uint32_t b_mat_cols = atoi(argv[3]);
    bool a_trans = atoi(argv[4] != NULL ? argv[4]: "0");
    bool b_trans = atoi(argv[5] != NULL ? argv[5]: "0");
    bool a_comp = atoi(argv[6] != NULL ? argv[6]: "0");
    bool b_comp = atoi(argv[7] != NULL ? argv[7]: "0");
    bool c_comp = atoi(argv[8] != NULL ? argv[8] : "0");
    uint32_t a_t = atoi(argv[9] != NULL ? argv[9] : "0");
    uint32_t b_t = atoi(argv[10] != NULL ? argv[10]: "0");
    uint32_t c_t = atoi(argv[11] != NULL ? argv[11]: "0");
    uint32_t M, N, P, M_hw, N_hw, P_hw;


    printf("a_comp:%d, b_comp:%d, c_comp:%d, a_t:%d, b_t:%d, c_t:%d\n", a_comp, b_comp, c_comp, a_t, b_t, c_t);
    M = a_mat_rows;
    N = a_mat_cols;
    P = b_mat_cols;

    M_hw = a_mat_rows;
    N_hw = a_mat_cols;
    P_hw = b_mat_cols;

    uint32_t x = a_trans ? M : N;
    if (x%8 != 0){
        x= 8 - (x%8) + x;
    }
    uint32_t a_stride = x;

    uint32_t y = b_trans ? N : P;
    if (y%8 != 0){
        y= 8 - (y%8) + y;
    }
    uint32_t b_stride = y;

    uint32_t z = P;
    if (z%8 != 0){
        z= 8 - (z%8) + z;
    }
    uint32_t c_stride = z;
    
    bool status = 0;

    float *a, *b, *a_temp, *b_temp, *a_c, *b_c, *c_c;
    if (a_trans == 1){
    float *a_1 = (float *)vio_malloc(N * a_stride * sizeof(float));
    float *a_temp_1 = (float *)malloc(N * a_stride * sizeof(float));
    a = a_1;
    a_temp = a_temp_1;
    } else {
      float *a_1 = (float *)vio_malloc(M * a_stride * sizeof(float));
      float *a_temp_1 = (float *)malloc(M * a_stride * sizeof(float));
      a = a_1;
      a_temp = a_temp_1;
    }

    if (b_trans == 1){
      float *b_1 = (float *)vio_malloc(P * b_stride * sizeof(float));
      float *b_temp_1 = (float *)vio_malloc(P * b_stride * sizeof(float));
      b = b_1;
      b_temp = b_temp_1;
    }else{
      float *b_1 = (float *)vio_malloc(N * b_stride * sizeof(float));
      float *b_temp_1 = (float *)vio_malloc(N * b_stride * sizeof(float));
      b = b_1;
      b_temp = b_temp_1;
    }
    

    float *c = (float *)vio_malloc(M * c_stride * sizeof(float));
    float *c_temp = (float *)vio_malloc(M * c_stride * sizeof(float));
    
    float *d = (float *)vio_malloc(M * c_stride * sizeof(float));

    float *e = (float *)malloc(M * c_stride * sizeof(float));
    float *e_temp = (float *)malloc(M * c_stride * sizeof(float));
    float *e_c = (float *)malloc(M * c_stride * sizeof(float));
    float *f = (float *)vio_malloc(M * a_stride * sizeof(float));
    float *g = (float *)vio_malloc(N * b_stride * sizeof(float));
    float *h = (float *)vio_malloc(M * c_stride * sizeof(float));
    float *i = (float *)vio_malloc(M * c_stride * sizeof(float));
    ekf_hw hw;


    matrix_gen(a, M, N, a_stride, seed, 0);
    a_c = a;
    matrix_gen(b, N, P, b_stride, seed, 0);
    b_c = b;
    matrix_gen(c, M, P, c_stride, seed, 0);
    c_c = c;


    if (a_comp){
      if (a_t == 1){
	if (a_trans){
	  matrix_ut_p(a, N, M, a_stride, 0);
	  a_c = a;
	} else {
	  matrix_lt_p(a, M, N, a_stride, 0);
	  a_c = a;
	}
      } else if (a_t == 2){
	if (a_trans){
	  matrix_lt_p(a, M, N, a_stride, 0);
	  a_c = a;
	} else {
	  matrix_ut_p(a, N, M, a_stride, 0);
	  a_c = a;
	}
      } else if (a_t == 5){
	if (a_trans){
	  matrix_ut_p(a, N, M, a_stride, 0);
	  matrix_lower_address_cm (a, a_temp, M, N, a_stride);
	  a = a_temp;
	  a_c = a;
	} else {
	  matrix_lt_p(a, M, N, a_stride, 0);
	  matrix_lower_address(a, a_temp, M, N, a_stride);
	  a = a_temp;
	  a_c = a;
	}
      } else if (a_t == 6){
	if (a_trans){
	  matrix_lt_p(a, M, N, a_stride, 0);
	  matrix_upper_address_cm (a, a_temp, M, N, a_stride);
	  a = a_temp;
	  a_c = a;
	} else {
	  matrix_ut_p(a, N, M, a_stride, 0);
	  matrix_upper_address(a, a_temp, M, N, a_stride);
	  a = a_temp;
	  a_c = a;
	}
      }
    }

 
    if (b_comp){
      if (b_t == 1){
	if (b_trans){
	  matrix_ut_p(b, P, N, b_stride, 0);
	  b_c = b;
	} else {
	  matrix_lt_p(b, N, P, b_stride, 0);
	  b_c = b;
	}
      } else if (b_t == 2){
	if (b_trans){
	  matrix_lt_p(b, N, P, b_stride, 0);
	  b_c = b;
	} else {
	  matrix_ut_p(b, P, N, b_stride, 0);
	  b_c = b;
	}
      } else if (b_t == 5){
	if (b_trans){
	  matrix_ut_p(b, P, N, b_stride, 0);
	  matrix_lower_address_cm (b, b_temp, N, P, b_stride);
	  b = b_temp;
	  b_c = b;
	} else {
	  matrix_lt_p(b, N, P, b_stride, 0);
	  matrix_lower_address(b, b_temp, N, P, b_stride);
	  b = b_temp;
	  b_c = b;
	}
      } else if (b_t == 6){
	if (b_trans){
	  matrix_lt_p(b, N, P, b_stride, 0);
	  matrix_upper_address_cm (b, b_temp, N, P, b_stride);
	  b = b_temp;
	  b_c = b;
	} else {
	  matrix_ut_p(b, P, N, b_stride, 0);
	  matrix_upper_address(b, b_temp, N, P, b_stride);
	  b = b_temp;
	  b_c = b;
	}
      }
    }
  
    if (c_comp){
      if (c_t == 1){
	matrix_lt_p(c, M, P, c_stride, 0);
	c_c = c;
      } else if (c_t == 2){
	matrix_ut_p(c, P, N, c_stride, 0);
	c_c = c;
      }  else if (c_t == 5){
	matrix_lt_p(c, M, P, c_stride, 0);
	matrix_lower_address(c, c_temp, N, P, c_stride);
	c = c_temp;
	c_c = c;
	
      } else if (c_t == 6){
	matrix_ut_p(c, P, N, c_stride, 0);
	matrix_upper_address(c, c_temp, N, P, c_stride);
	c = c_temp;
	c_c = c;
      }
    }


    
//matrix multiply sw/hw:
    mat_mul_c(a_c, b_c, c_c, M, N, P, a_trans, b_trans, -1, 1, a_stride, b_stride, c_stride, e_c);
    hw.mat_mul(a, b, c, d, M_hw, N_hw, P_hw, a_trans, b_trans, -1.0f, 1.0f, a_stride, b_stride, c_stride,
	       a_t, b_t, c_t);
    
    if (c_comp){
      if (c_t == 5){
	matrix_lower_address(e_c, e_temp, N, P, c_stride);
	e = e_temp;
      }  else if (c_t == 6){
	matrix_upper_address(e_c, e_temp, N, P, c_stride);
	e = e_temp;
      } else if (c_t == 1){
	e = e_c;
      } else if (c_t == 2){
	e = e_c;
      }
    } else {
      e = e_c;
    } 

 
    status = check_mat(e, d, M, P, c_stride, "MMULT");
#if 0
    hw.move_a_mat_2_cpu(f, a_stride, M, 0);
    hw.move_b_mat_2_cpu(g, b_stride, N, 0);
    hw.move_c_mat_2_cpu(h, c_stride, M, 0);
    hw.move_y_mat_2_cpu(i, c_stride, M, 0);
#endif

#if 0
    print_mat(a_c, M, N, a_stride, "A_C");
    print_mat(b_c, N, P, b_stride, "B_C");
    print_mat(c_c, M, P, c_stride, "C_C");
    print_mat(a, M, N, a_stride, "A_H");
    print_mat(b, N, P, b_stride, "B_H");
    print_mat(c, M, P, c_stride, "C_H");
    print_mat(d, M, P, c_stride, "HW O/P");
    print_mat(e_c, M, P, c_stride, "SW O/P");
    print_mat(e, M, P, c_stride, "SW O/P");
#endif
    
#if 0
    print_mat(f, M, N, a_stride, "F");
    print_mat(g, N, P, b_stride, "G");
    print_mat(h, M, P, c_stride, "H");
    print_mat(i, M, P, c_stride, "I");
#endif


    vio_free(a);
    vio_free(b);
    vio_free(c);
    vio_free(d);
    printf("Status: %d\n", status);
}



#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <cblas.h>
#include "../src/hw/ekf_hw.h"
#include "../src/ref_model/ekf_sw.h"
#include "../src/hw/vio_regs.h"
#include "../src/hw/vio_utils.h"


int max(int x, int y){
   if(x > y) return x;
   else return y;
}
void PrintMatrix(float* pMatrix, const size_t nR, const size_t nC, const CBLAS_ORDER Order) {
    unsigned int i, j;
    if (Order == CblasRowMajor) {
        for (i = 0; i < nR; i++) {
            for (j = 0; j < nC; j++) {
                fprintf(stderr,"%f \t ", pMatrix[i * nC + j]); // !!!
            }
            fprintf(stderr,"\n"); // !!!
        }
    } else {
        for (i = 0; i < nR; i++) {
            for (j = 0; j < nC; j++) {
                fprintf(stderr,"%f \t ", pMatrix[i + j* nR ]); // !!!
            }
            fprintf(stderr,"\n"); // !!!
        }
    }
    fprintf(stderr,"\n"); // !!!
}

int main(void)
{
    const int m = 4;
    const int n = 8;
    const int k = 1;

    float A[] = {1,2,3,4,5,6,7,8,   1,2,3,4,5,6,7,8,   1,2,3,4,5,6,7,8,   1,2,3,4,5,6,7,8};
    float B[] = { 1, 2, 3, 4 };

    float alpha = 1.0, beta = 0.0;

    float *a1 = (float *) vio_malloc(m * n *sizeof(float)); //4x8
    float *b1 = (float *) vio_malloc(m * 1 * sizeof(float)); // 4x1
    float *c1 = (float *) vio_malloc(n * 1 * sizeof(float)); // 8x1
    memcpy(a1, A, m * n * sizeof(float));
    memcpy(b1, B, m * 1 * sizeof(float));
    memset(c1, 0, n * 1 * sizeof(float));
    cblas_sgemv(CblasColMajor, CblasTrans, m, n, alpha, a1, m, b1, k, beta, c1, k);
    fprintf(stderr,"Result after sgemv\n"); 
    PrintMatrix(c1, 8, k, CblasRowMajor);

    memset(c1, 0, n * 1 * sizeof(float));
    cblas_sgemm(CblasColMajor, CblasTrans, CblasNoTrans, n,k,m, alpha, a1, m, b1, n, beta, c1, n);
    fprintf(stderr,"Result after sgemm\n"); 
    PrintMatrix(c1, 8, k, CblasRowMajor); 


    ekf_hw hw;
    float *a = (float *) vio_malloc(m * n *sizeof(float)); //4x8
    float *b = (float *) vio_malloc(m * 1 * sizeof(float)); // 4x1
    float *cin = (float *) vio_malloc(n * 1 * sizeof(float)); // 8x1
    float *cout = (float *) vio_malloc(n * 1 * sizeof(float)); // 8x1
    memcpy(a, A, m * n * sizeof(float));
    memcpy(b, B, m * 1 * sizeof(float));
    memset(cin, 0, n * 1 * sizeof(float));
    memset(cout, 0, n * 1 * sizeof(float));
    hw.mat_mul(a, b, cin, cout , 8, 4, 1, 1,0, alpha, beta, 4, 4, 4, 0,0,0);
    fprintf(stderr, "Result after ekf mm\n"); 
    PrintMatrix(cout, n, k, CblasRowMajor);

    vio_free(a);
    vio_free(b);
    vio_free(cin);
    vio_free(cout);

    return 0;
}

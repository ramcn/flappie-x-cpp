#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <cblas.h>
#include "a.h"
#include "b.h"
#include "cin.h"


int max(int x, int y){
   if(x > y) return x;
   else return y;
}
void PrintMatrix(float* pMatrix, const size_t nR, const size_t nC, const CBLAS_ORDER Order) {
    unsigned int i, j;
    if (Order == CblasRowMajor) {
        for (i = 0; i < nR; i++) {
            for (j = 0; j < nC; j++) {
                printf("%f \t ", pMatrix[i * nC + j]); // !!!
            }
            printf("\n"); // !!!
        }
    } else {
        for (i = 0; i < nR; i++) {
            for (j = 0; j < nC; j++) {
                printf("%f \t ", pMatrix[i + j* nR ]); // !!!
            }
            printf("\n"); // !!!
        }
    }
    printf("\n"); // !!!
}

int main(void)
{
    const int m = 256;
    const int n = 768;
    const int k = 1;

    float alpha = 1.0, beta = 1.0;
    int lda, ldb, ldc;

    float * C = (float*) malloc(n * k * sizeof(float));
    for (int i = 0; i < n*k; i++) C[i] = 0.0;
    memcpy(C, cin, n*k*sizeof(float));
    printf("cin[0]=%f\n",cin[0]); 
    printf("C[0]=%f\n",C[0]); 

    //cblas_sgemv(CblasRowMajor, CblasNoTrans, m, n, alpha, A, n, B, k, beta, C, k);
    //lda = max(1,n); ldb = max(1,k); ldc = max(1,k);  // refer manual and switch n and k
    //cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, m,k,n, alpha, A, lda, B, ldb, beta, C, ldc);
 
    //cblas_sgemv(CblasColMajor, CblasTrans, m, n, alpha, A, m, B, k, beta, C, k);
    lda = max(1,m); ldb = max(1,n); ldc = max(1,n);  // refer manual and switch n and k
    cblas_sgemm(CblasColMajor, CblasTrans, CblasNoTrans, n,k,m, alpha, A, lda, B, ldb, beta, C, ldc);

    PrintMatrix(C, n, k, CblasColMajor);

    free(C);

    return 0;
}

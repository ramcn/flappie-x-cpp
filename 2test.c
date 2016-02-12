#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <cblas.h>

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
    const int m = 4;
    const int n = 5;
    const int k = 1;

    float A[] = { 8, 4, 7, 3, 5, 1, 1, 3, 2, 1, 2, 3, 2, 0, 1, 1, 2, 3, 4, 1};
    float B[5] = { -1, 2, -1, 1, 2 };

    float alpha = 1.0, beta = 0.0;
    int lda, ldb, ldc;

    float * C = (float*) malloc(m * k * sizeof(float));
    for (int i = 0; i < m*k; i++) C[i] = 0.0;


    //working cblas_sgemv(CblasRowMajor, CblasNoTrans, m, n, alpha, A, n, B, k, beta, C, k);
    //lda = max(1,n); ldb = max(1,k); ldc = max(1,k);  // refer manual and switch n and k 
    //cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, m,k,n, alpha, A, lda, B, ldb, beta, C, ldc);

    // working cblas_sgemv(CblasColMajor, CblasNoTrans, m, n, alpha, A, m, B, k, beta, C, k);
    // working cblas_sgemv(CblasColMajor, CblasTrans, m, n, alpha, A, m, B, k, beta, C, k);
    lda = max(1,n); ldb = max(1,n); ldc = max(1,m);  // refer manual and switch n and k 
    cblas_sgemm(CblasColMajor, CblasTrans, CblasNoTrans, m,k,n, alpha, A, lda, B, ldb, beta, C, ldc);


    PrintMatrix(C, m, k, CblasRowMajor);

    free(C);

    return 0;
}

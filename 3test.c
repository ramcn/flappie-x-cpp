#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cblas.h>


#define NCols 5
#define Nrows 4

float A[] = { 8, 4, 7, 3, 5, 1, 1, 3, 2, 1, 2, 3, 2, 0, 1, 1 , 2, 3, 4, 1};

float x[] = { -1, 2, -1, 1, 2 };

float y[Nrows];
float alpha = 1.0, beta = 0.0;
char tbuf[1024];
int main() {
    int i, j;

    // Print original matrix

    // y = Ax
    cblas_sgemv(CblasRowMajor, CblasNoTrans, Nrows, NCols, alpha, A, NCols, x, 1, beta, y, 1);
    // Print resulting vector
    for (j = 0; j < Nrows; j++) {
        printf(" %f\n", y[j]);
    }

    cblas_sgemv(CblasColMajor, CblasNoTrans, Nrows, NCols, alpha, A, Nrows, x, 1, beta, y, 1);
    // Print resulting vector
    for (j = 0; j < Nrows; j++) {
        printf(" %f\n", y[j]);
    }

    return 0;
}


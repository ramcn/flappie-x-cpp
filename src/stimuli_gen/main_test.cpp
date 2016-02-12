#include "../stimuli_gen/matrix_generation.h"
#include "../ref_model/ekf_sw.h"
#include "assert.h"

FILE *A = NULL;
FILE *B = NULL;
FILE *C = NULL;
FILE *D = NULL;
FILE *E = NULL;
FILE *F = NULL;
FILE *G = NULL;
FILE *H = NULL;
FILE *I = NULL;
FILE *J = NULL;
FILE *K = NULL;
FILE *L = NULL;
FILE *J1 = NULL;
FILE *K1 = NULL;
FILE *L1 = NULL;
FILE *O = NULL;
FILE *R = NULL;
FILE *S = NULL;
FILE *Q = NULL;
FILE *T = NULL;
FILE *Q1 = NULL;
FILE *T1 = NULL;
FILE *W = NULL;
FILE *CONFIG = NULL;

#if 0
FILE *CHL_In = NULL;
FILE *CHL_Out = NULL;
FILE *MAT_Cov_LC = NULL;
FILE *MAT_Cov_Gain = NULL;
FILE *MAT_Cov_Gain_t = NULL;
FILE *MAT_Cov_In = NULL;
#endif




int main (int argc, char *argv[])
{

    if(argc <= 8){
        printf("Usage: <exe> <amat_rows> <amat_cols> <bmat_cols> <a_stride> <b_stride> <c_stride> <seed> <test_requirement>\n");
        exit(1);
    }

    uint32_t M = atoi(argv[1]);
    uint32_t N = atoi(argv[2]);
    uint32_t P = atoi(argv[3]);
    uint32_t a_stride = atoi(argv[4]);
    uint32_t b_stride = atoi(argv[5]);
    uint32_t c_stride = atoi(argv[6]);
    uint32_t seed = atoi(argv[7]);
    uint32_t test = atoi(argv[8]); //0 for generate matrix for the given configuration
                                   //1 for generate matrix and take transpose of it
                                   //2 for generate matrix with LT
                                   //3 for generate matrix with LT and take transpose
                                   //4 for generate matrix with LT, take transpose and multiply to get cholesky input
                                   //5:8 for generate zero, inf, hugh, tiny number for LT, take transpose and multiply to get cholesky input
                                   //9 for generate square root and get diagonal element as one.
                                   //10 for generate with nan for cholesky input matrix
                                   //11 for generate natrix with normal number and multiply
                                   //12:15 for generate zero, inf, hugh, tiny number for matrix generation and multiply to get cholesky input
                                
    //stride calculation
    uint32_t x = N;
    if (x%8 != 0){
        x= 8 - (x%8) + x;
    }

    uint32_t x_stride = x;
    
    uint32_t y = P;
    if (y%8 != 0){
        y= 8 - (y%8) + y;
    }

    uint32_t y_stride = y;
        
    uint32_t z = P;
    if (z%8 != 0){
        z= 8 - (z%8) + z;
    }

    uint32_t z_stride = z;


    //file prints
    A = fopen("CR__CHL_Out_Matrix.txt", "w");
    D = fopen("CR__CHL_Inv_Out_Matrix.txt", "w");
    B = fopen("CR__CHLT_Out_Matrix.txt", "w");   
    C = fopen("CR__CHL_In_Matrix.txt", "w");
    E = fopen("CR__LCt_inn_out.txt", "w");
    F = fopen("CR__LCt_inn_in.txt", "w");
    G = fopen("CR__cov_state_in.txt", "w");
    H = fopen("CR__cov_state_out.txt", "w");
    I = fopen("CR__cholesky_sw_out.txt", "w");
    J = fopen("CR__Det1_FPList.txt", "w");
    K = fopen("CR__Det1_SortedFPList.txt", "w");
    L = fopen("CR__Det1_SortedFPPatchList.txt", "w");
    J1 = fopen("CR__Det2_FPList.txt", "w");
    K1 = fopen("CR__Det2_SortedFPList.txt", "w");
    L1 = fopen("CR__Det2_SortedFPPatchList.txt", "w");
    O = fopen("CR__Trk_FPPatchList.txt","w");
    Q = fopen("CR__Det1_Mask.txt", "w");
    Q1 = fopen("CR__Det2_Mask.txt", "w");
    T = fopen("CR__Image1.txt", "w");
    T1 = fopen("CR__Image2.txt", "w");
    R = fopen("CR__Trk_FPPredList.txt","w");
    S = fopen("CR__Trk_FPTrackList.txt","w");
    W = fopen("CR__Kt_out.txt","w");
    CONFIG = fopen("CR__Config.txt", "w");


     #if 0
    CHL_In = fopen("CHL_In_Matrix.txt","w");
    CHL_Out = fopen("CHL_Out_Matrix.txt", "w");
    MAT_Cov_LC = fopen("MAT_Cov_LC.txt","w");
    MAT_Cov_Gain = fopen("MAT_Cov_Gain.txt","w");
    MAT_Cov_Gain_t = fopen("MAT_Cov_Gain_t.txt","w");
    MAT_Cov_In = fopen("MAT_Cov_In.txt","w");
    #endif

    
    //matrix allocation
    //cholesky matrices
    float *a = (float *)malloc(M * x_stride * sizeof(float));
    float *b = (float *)malloc(M * x_stride * sizeof(float));
    float *c = (float *)malloc(M * x_stride * sizeof(float));
    float *d = (float *)malloc(M * x_stride * sizeof(float));
    float *e = (float *)malloc(M * x_stride * sizeof(float));
    float *p = (float *)malloc(M * x_stride * sizeof(float));
    float *q = (float *)malloc(M * x_stride * sizeof(float));
    //matrix multiplication matrices
    float *h = (float *)malloc(N * x_stride * sizeof(float));
    float *i = (float *)malloc(P * y_stride * sizeof(float));
    float *j = (float *)malloc(P * z_stride * sizeof(float));
    float *k = (float *)malloc(P * z_stride * sizeof(float));
    float *l = (float *)malloc(P * z_stride * sizeof(float));
    float *m = (float *)malloc(P * z_stride * sizeof(float));
    //matrix solve matrices
    float *a_ms = (float*)malloc(M * a_stride * sizeof(float));
    float *y_ms = (float*)malloc(P * b_stride * sizeof(float));
    float *y_out_1_t_ms = (float*)malloc(P * b_stride * sizeof(float));
    float *b_in_t_ms = (float*)malloc(P * b_stride * sizeof(float));
    float *a_in_ms = (float*)malloc(M * a_stride * sizeof(float));
    //test generation
#if CHOL
    if(test == 0){
        matrix_gen(c, M, N, a_stride, seed, 0);
    }

    if(test == 1){
        matrix_gen(c, M, N, a_stride, seed, 0);
        matrix_transpose(c, b, M, N, a_stride, a_stride, 1);
    }

    if(test == 2){
        matrix_lt(a, M, N, a_stride, seed, 0);
        matrix_diag_inv(a, d, M, N, a_stride, 1);
    }

    if(test == 3){
        matrix_lt(a, M, N, a_stride, seed, 0);
        matrix_transpose(a, b, M, N, a_stride, a_stride, 1);
        matrix_diag_inv(a, d, M, N, a_stride, 1);
    }

    if(test == 4){
        matrix_lt(a, M, N, a_stride, seed, 1);
        matrix_transpose(a, b, M, N, a_stride, a_stride, 1);
        matrix_gen(c, M, N, a_stride, 0, 2);
        mat_mul_c(a, b, c, M, N, N, 0, 0, 1, 1, a_stride, a_stride, a_stride, d);
        matrix_diag_inv(a, e, M, N, a_stride, 1);
    }

    if(test == 5){
        matrix_lt_num(a, M, N, a_stride, seed, 0);//zero
        matrix_transpose(a, b, M, N, a_stride, a_stride, 1);
        matrix_gen(c, M, N, a_stride, 0, 2);
        mat_mul_c(a, b, c, M, N, N, 0, 0, 1, 1, a_stride, a_stride, a_stride, d);
        matrix_diag_inv(a, e, M, N, a_stride, 1);
    }

    if(test == 6){
        matrix_lt_num(a, M, N, a_stride, seed, 1);//inf
        matrix_transpose(a, b, M, N, a_stride, a_stride, 1);
        matrix_gen(c, M, N, a_stride, 0, 2);
        mat_mul_c(a, b, c, M, N, N, 0, 0, 1, 1, a_stride, a_stride, a_stride, d);
        matrix_diag_inv(a, e, M, N, a_stride, 1);
    }

    if(test == 7){
        matrix_lt_num(a, M, N, a_stride, seed, 2);//hugh
        matrix_transpose(a, b, M, N, a_stride, a_stride, 1);
        matrix_gen(c, M, N, a_stride, 0, 2);
        mat_mul_c(a, b, c, M, N, N, 0, 0, 1, 1, a_stride, a_stride, a_stride, d);
        matrix_diag_inv(a, e, M, N, a_stride, 1);
    }

   
    if(test == 8){
        matrix_lt_num(a, M, N, a_stride, seed, 3);//tiny
        matrix_transpose(a, b, M, N, a_stride, a_stride, 1);
        matrix_gen(c, M, N, a_stride, 0, 2);
        mat_mul_c(a, b, c, M, N, N, 0, 0, 1, 1, a_stride, a_stride, a_stride, d);
        matrix_diag_inv(a, e, M, N, a_stride, 1);
    }
        

    if(test == 9){
        matrix_gen_sqrt(a, b, c, d, M, N, a_stride, seed);//squareroot error
    }

    
    if(test == 10){
         matrix_gen(p, M, N, a_stride, seed, 0);
         mat_chol_c(p, e, d, M, a_stride, a_stride, a_stride);
    }
#endif 
//matrix multiply
#if MATMUL
    if(test == 11){
        matrix_gen(h, M, N, a_stride, seed, 0);
        matrix_gen(i, N, P, b_stride, seed, 0);
        matrix_gen(j, M, P, c_stride, seed, 0);
        mat_mul_c(h, i, j, M, N, P, false, false, -1, 1, a_stride, b_stride, c_stride, k);
    }

    if(test == 12){
        matrix_gen_num(h, M, N, a_stride, seed, 0);//zero
        matrix_gen_num(i, N, P, b_stride, seed, 0);
        matrix_gen(j, M, P, c_stride, seed, 2);
        mat_mul_c(h, i, j, M, N, P, false, false, -1, 1, a_stride, b_stride, c_stride, k);
    }

    if(test == 13){
        matrix_gen_num(h, M, N, a_stride, seed, 1);//INF
        matrix_gen_num(i, N, P, b_stride, seed, 1);
        matrix_gen(j, M, P, c_stride, seed, 2);
        mat_mul_c(h, i, j, M, N, P, false, false, -1, 1, a_stride, b_stride, c_stride, k);
    }

    if(test == 14){
        matrix_gen_num(h, M, N, a_stride, seed, 2);//Hugh
        matrix_gen_num(i, N, P, b_stride, seed, 2);
        matrix_gen(j, M, P, c_stride, seed, 2);
        mat_mul_c(h, i, j, M, N, P, false, false, -1, 1, a_stride, b_stride, c_stride, k);
    }

    if(test == 15){
        matrix_gen_num(h, M, N, a_stride, seed, 3);//Tiny
        matrix_gen_num(i, N, P, b_stride, seed, 3);
        matrix_gen(j, M, P, c_stride, seed, 2);
        mat_mul_c(h, i, j, M, N, P, false, false, -1, 1, a_stride, b_stride, c_stride, k);
    }

    if (test == 16){
        matrix_gen_num(h, M, N, a_stride, seed, 0);//Zero
        matrix_gen_num(i, N, P, b_stride, seed, 1);//INF
        matrix_gen(j, M, P, c_stride, seed, 2);
        mat_mul_c(h, i, j, M, N, P, false, false, -1, 1, a_stride, b_stride, c_stride, k); 
    }
    
    if (test == 17){
        matrix_gen_num(h, M, N, a_stride, seed, 0);//Zero
        matrix_gen_num(i, N, P, b_stride, seed, 3);//Tiny
        matrix_gen(j, M, P, c_stride, seed, 2);
        mat_mul_c(h, i, j, M, N, P, false, false, -1, 1, a_stride, b_stride, c_stride, k); 
    }
    
    if (test == 18){
        matrix_gen_num(h, M, N, a_stride, seed, 0);//Zero
        matrix_gen_num(i, N, P, b_stride, seed, 2);//HUGH
        matrix_gen(j, M, P, c_stride, seed, 2);
        mat_mul_c(h, i, j, M, N, P, false, false, -1, 1, a_stride, b_stride, c_stride, k); 
    }
    
    if (test == 19){
        matrix_gen_num(h, M, N, a_stride, seed, 3);//TINY
        matrix_gen_num(i, N, P, b_stride, seed, 2);//HUGH
        matrix_gen(j, M, P, c_stride, seed, 2);
        mat_mul_c(h, i, j, M, N, P, false, false, -1, 1, a_stride, b_stride, c_stride, k); 
    }
    
#endif


#if MATSOL


    if (test == 20){// normal
        matrix_lt(a_ms, M, M, a_stride, seed, 1);
        matrix_cm(y_ms, M, P, a_stride, seed, 1);
        mat_solve_c(a_ms, y_ms, M, P, a_stride, a_stride, y_out_1_t_ms, b_in_t_ms, a_in_ms);
    }


    if (test == 21){// zero
         matrix_lt_num(a_ms, M, M, a_stride, seed, 0);
         matrix_gen_num_cm(y_ms, M, P, a_stride, seed, 0);
         mat_solve_c(a_ms, y_ms, M, P, a_stride, a_stride, y_out_1_t_ms, b_in_t_ms, a_in_ms);
    }

    if (test == 22){//inf
         matrix_lt_num(a_ms, M, M, a_stride, seed, 1);
         matrix_gen_num_cm(y_ms, M, P, a_stride, seed, 1);
         mat_solve_c(a_ms, y_ms, M, P, a_stride, a_stride, y_out_1_t_ms, b_in_t_ms, a_in_ms);
    }

    if (test == 23){//Tiny
        matrix_lt_num(a_ms, M, M, a_stride, seed, 3);
        matrix_gen_num_cm(y_ms, M, P, a_stride, seed, 3);
        mat_solve_c(a_ms, y_ms, M, P, a_stride, a_stride, y_out_1_t_ms, b_in_t_ms, a_in_ms);
    }

    if (test == 24){//Hugh
        matrix_lt_num(a_ms, M, M, a_stride, seed, 2);
        matrix_gen_num_cm(y_ms, M, P, a_stride, seed, 2);
        mat_solve_c(a_ms, y_ms, M, P, a_stride, a_stride, y_out_1_t_ms, b_in_t_ms, a_in_ms);
    }

#endif 


    

    //print matrices
#if CHOL
    print_mat(a, M, N, a_stride, "CHL_OUT");
    print_mat(b, M, N, a_stride, "CHL_OUTt");
    print_mat(c, M, N, a_stride, "Zero");
    print_mat(d, M, N, a_stride, "CHL_IN");
    print_mat(e, M, N, a_stride, "CHL_INV_OUT");
#endif

#if MATMUL
    print_mat(h, M, N, a_stride, "A_MAT");
    print_mat(i, N, P, b_stride, "B_MAT");
    print_mat(j, M, P, c_stride, "C_MAT");
    print_mat(k, M, P, c_stride, "D_out_MAT");
    //print_mat(l, M, P, c_stride, "D_sw_MAT");
  
#endif 

#if MATSOL
    print_mat(a_ms,         M, M, a_stride, "ms_mat_a_in");
    print_mat(a_in_ms,         M, M, a_stride, "ms_mat_a");
    print_mat(b_in_t_ms,    P, M, a_stride, "ms_mat_b");
    print_mat(y_out_1_t_ms, P, M, a_stride, "ms_mat_x1");
    print_mat(y_ms,         P, M, a_stride, "ms_mat_x2");
#endif
     
//print file
#if CHOL
    print_mat_file_cm(A, a, M, N, a_stride, 0);
    print_mat_file_cm(B, b, M, N, a_stride, 0);
    print_mat_file_cm(D, e, M, N, a_stride, 0);
    print_mat_file_cm(C, d, M, N, a_stride, 0);
    print_config(CONFIG, 640, 480, 15, 5, 120, 400, 400,  1, 1, 231, 77, M, N, P, x_stride, y_stride, z_stride, test, 20, 50, 1, 1, 2);
#endif

#if MATMUL
    print_mat_file_cm(E, h, M, N, a_stride, 1);
    print_mat_file_cm(W, i, N, P, b_stride, 1);
    print_mat_file_cm(G, j, M, P, c_stride, 1);
    print_mat_file_cm(H, k, M, P, c_stride, 1);
    print_config(CONFIG, 640, 480, 15, 5, 120, 400, 400, 1, 1, 231, 77, M, N, P, x_stride, y_stride, z_stride, test, 20, 50, 1, 1, 2);

#endif


#if MATSOL
    print_matsol_file_cm(D, a_in_ms, M, M, x_stride);
    print_matsol_file_cm(E, y_ms, M, P, x_stride);
    print_matsol_file_cm(F, y_out_1_t_ms, M, P, x_stride);
    print_config(CONFIG, 640, 480, 15, 5, 120, 400, 400, 1, 1, 231, 77, M, N, P, x_stride, y_stride, z_stride, test, 20, 50, 1, 1, 2);
#endif

#if 0
    print_mat_file_cm(CHL_In, a, M, N, x_stride, 1);
    print_mat_file_cm(CHL_Out, b, M, N, x_stride, 1);
    print_mat_file_cm(MAT_Cov_LC, c, M, N, x_stride, 1);
    print_mat_file_cm(MAT_Cov_Gain, d, M, N, x_stride, 1);
    print_mat_file_cm(MAT_Cov_Gain_t, e, M, N, x_stride, 1);
    print_mat_file_cm(MAT_Cov_In, f, M, N, x_stride, 1);
#endif
    
    fclose(A);
    fclose(B);
    fclose(C);
    fclose(D);
    fclose(CONFIG);

    return 0;
}

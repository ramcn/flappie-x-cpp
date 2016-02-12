#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <time.h>
#include <cstdio>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgproc/imgproc_c.h>
#include "../ref_model/fast.h"
#include "../hw/fast_hw_h.h"
#include "../ref_model/fast_constants.h"
#include "../stimuli_gen/utils.h"
#include "../stimuli_gen/matrix_generation.h"
//#include "bitmap_image.hpp"

template <class T>
bool predy_cmp(const T &a, const T &b)
{
    return (a.y < b.y); 
}

template <class T>
bool patchy_cmp(const T &a, const T &b)
{
    return (a.xy.y < b.xy.y); 
}


extern int g_ncc_compute;
int g_ncc_compute = 0;

#if DETECTOR_INS
FILE *g_fp_dist = NULL;

FILE *g_fp_dist_1 = NULL;
int g_det_count = 2;
#endif


#if TRACKER_INS
FILE *g_fp_patch = NULL;
FILE *g_fp_list = NULL;
FILE *g_fp_result = NULL;
#endif


#if SORT_INS
FILE *g_fp_sort = NULL;
FILE *g_fp_sort_hex = NULL;
FILE *g_fp_sort_patch = NULL;

FILE *g_fp_sort_1 = NULL;
FILE *g_fp_sort_hex_1 = NULL;
FILE *g_fp_sort_patch_1 = NULL;
#endif

FILE *CONFIG = NULL;
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
FILE *O = NULL;
FILE *R = NULL;
FILE *S = NULL;
FILE *Q = NULL;
FILE *T = NULL;
FILE *W = NULL;
FILE *MASK1 = NULL;
FILE *MASK2 = NULL;
FILE *SEED = NULL;
using namespace cv;

void MyLine(Mat img, Point start, Point end );


static Scalar randomColor( RNG& rng )
{
  int icolor = (unsigned) rng;
  return Scalar( icolor&255, (icolor>>8)&255, (icolor>>16)&255 );
}

void get_patch(const uint8_t *im, int x, int y, uint32_t stride, uint8_t *buf)
{
    uint32_t wd = full_patch_width; uint32_t ht = full_patch_width;
    im = im + ((y - ht/2) * stride) + (x - wd/2);
    for(uint32_t i = 0; i < ht; i++) {
        memcpy(buf, im, wd);
        buf += wd; im += stride;
    }
}





void eighteen_pixel(Mat copyimg_d, uint32_t width, uint32_t height, int inner_start)
{

    //copyimg_d(width, height, CV_8U, Scalar(0));
    //imwrite("rectbox.jpg", copyimg);
        
    //int inner_start = 200;
    int rect_length = 18;
    int rect_height = 3;
    
    //Rect r= Rect(inner_start, inner_start, rect_length, rect_height);
    //rectangle( copyimg_d, r, Scalar(255), -1, 8, 0);
    int fwd = 270;
    int fthr = 30;
    int rthr = 30;
    int rev = 0;
    for (int r = inner_start; r < inner_start + 3; r ++){
        for (int c = inner_start; c < inner_start + 19; c++){
            if ( c < (inner_start + 10)){
                copyimg_d.row(r).col(c).setTo(fwd - fthr);
                fthr = fthr + 30;
            } else if ( c >= (inner_start + 10)) {
                 copyimg_d.row(r).col(c).setTo(rev + rthr);
                rthr = rthr + 30;
            }
        }
        fthr = 30;
        rthr = 30;
    }
}


void sixteen_pixel(Mat& copyimg_d, uint32_t width, uint32_t height, int inner_start_r, int inner_start_c)
{

    //copyimg_d(width, height, CV_8U, Scalar(0));
    //imwrite("rectbox.jpg", copyimg);
        
    //int inner_start = 200;
    int rect_length = 16;
    int rect_height = 2;
    
    //Rect r= Rect(inner_start, inner_start, rect_length, rect_height);
    //rectangle( copyimg_d, r, Scalar(255), -1, 8, 0);
    int fwd = 256;
    int fthr = 32;
    int rthr = 32;
    int rev = 0;


    // printf("inner_start_r:%d, inner_start_c:%d\n", inner_start_r, inner_start_c);
    for (int r = inner_start_r; r < inner_start_r + 2; r ++){
        for (int c = inner_start_c; c < inner_start_c + 17; c++){
            if ( c < (inner_start_c + 9)){
                copyimg_d.row(r).col(c).setTo(fwd - fthr);
                fthr = fthr + 32;
            } else if ( c >= (inner_start_c + 9)) {
                copyimg_d.row(r).col(c).setTo(rev + rthr);
                rthr = rthr + 32;
            }    
        }
        fthr = 32;
        rthr = 32;
    }
    
}


int main (int argc, char *argv[])
{
    //if(argc <= 8) {
    //   printf("Usage: <exe> <input image> <num features> <threshold> <width> <height> <mask_en> <tthr> <test_image> <instr_dir>\n");
    //   exit(1);
    //}
    time_t t;
    unsigned seed = (unsigned) time (&t);
    srand(seed);
    string dir, instr_dir;
    int num_points, threshold, width, height, mask_en, tthreshold, test_image, imageSize, inner_start, sample;
    Mat copyimg;
    unsigned char *copyimg_data;
    const unsigned char *copyimg_data_dup;
    unsigned char *finalimg_data;

    SEED= fopen((instr_dir + "__SEED.txt").c_str(),"w+");

    if (std::string (argv[1]) == "rc_code") {
        string dir_d = argv[2];
        int sample = atoi(argv[3]);
        int num_points_d = atoi(argv[4]);
        int threshold_d  = atoi(argv[5]);
        int width_d = atoi(argv[6]);
        int height_d = atoi(argv[7]);
        int mask_en_d = atoi(argv[8]);
        int tthreshold_d = atoi(argv[9]);
        string instr_dir_d = argc > 10 ? argv[10] : ".";
        dir = dir_d; num_points = num_points_d; threshold = threshold_d; width = width_d; height = height_d;
        mask_en = mask_en_d; tthreshold = tthreshold_d; instr_dir = instr_dir_d;
        enum {stimuli, resize, crop};
        if (sample == stimuli){
            printf("input_image provided\n");
            fprintf(SEED, "stimuli\n");
            string img_file  = dir + "/image.bmp";
            cv::Mat img = cv::imread(img_file.c_str(), CV_LOAD_IMAGE_GRAYSCALE);
            if( !img.data ) {
                std::cout << " Couldn't read input image " << argv[1] << std::endl; 
            }
     
            float img_cols = img.cols;
            float img_rows  = img.rows;
            float x = width/img_cols;
            float y = height/img_rows;
     
            Mat copyimg_d(img.clone());

            imwrite( "copyimg.pgm", copyimg_d);

            copyimg = copyimg_d.clone();
            
        } else if (sample == resize){
            fprintf(SEED, "resize\n");
            printf("input_image provided\n");
            string img_file  = dir + "/image.bmp";
            cv::Mat img = cv::imread(img_file.c_str(), CV_LOAD_IMAGE_GRAYSCALE);
            if( !img.data ) {
                std::cout << " Couldn't read input image " << argv[1] << std::endl; 
            }
            float img_cols = img.cols;
            float img_rows  = img.rows;
            float x = width/img_cols;
            float y = height/img_rows;
            
            Mat copyimg_d(img.clone());
        
            cv::resize(img, copyimg_d, Size(), x, y, INTER_LINEAR); //resize(const Mat& src, Mat& dst, Size dsize, double fx=0, double fy=0, int interpolation=INTER_LINEAR)
            imwrite( "copyimg.pgm", copyimg_d);

            copyimg = copyimg_d.clone();
            
        } else if (sample == crop) {
            fprintf(SEED, "crop\n");
            printf("input_image provided\n");
            string img_file  = dir + "/image.bmp";
            cv::Mat img = cv::imread(img_file.c_str(), CV_LOAD_IMAGE_GRAYSCALE);
            if( !img.data ) {
                std::cout << " Couldn't read input image " << argv[1] << std::endl; 
            }
            float img_cols = img.cols;
            float img_rows  = img.rows;
            
            float x = width;
            float y = height;

            Rect myROI(0, 0, x, y);

            Mat cropped = img(myROI);
            
            cv::imwrite("cropped_im.pgm", cropped);
            copyimg = cropped.clone();

        }
        
    } else if (std::string (argv[1]) == "random") {
        int sample = atoi(argv[2]);
        int num_points_d = atoi(argv[3]);
        int threshold_d  = atoi(argv[4]);
        int width_d = atoi(argv[5]);
        int height_d = atoi(argv[6]);
        int mask_en_d = atoi(argv[7]);
        int tthreshold_d = atoi(argv[8]);
        double angle_d = atoi(argv[9]);
        string instr_dir_d = argc > 10 ? argv[10] : ".";
        num_points = num_points_d; threshold = threshold_d; width = width_d; height = height_d;
        mask_en = mask_en_d; tthreshold = tthreshold_d; instr_dir = instr_dir_d;
        enum {ran_pattern, ran_tri, ran_ecl, ran_lines, ran_rect, ran_rotate_crop};
        
        if (sample == ran_pattern){
            fprintf(SEED, "16 pixel with density random pattern\n");
            //int MAX = 250;
            //int width_r = rand() % MAX;
            //int height_r = rand() % MAX;
            printf ("16 pixel with density random pattern\n");
            printf("width:%d, height:%d\n", width, height);
            Mat copyimg_d(height, width, CV_8U, Scalar(255));
            for (int i = 2; i < width - 16; i = i + 18){
                for (int j = 2 ; j < height - 2; j = j + 4){
                    //printf ("i:%d, j:%d\n", i, j );
                    
                    sixteen_pixel(copyimg_d, width, height, j, i);
                }
            }
            imwrite("random_pixel.pgm", copyimg_d);
            copyimg = copyimg_d.clone();
            
        } else if (sample == ran_tri){
            int MAX = rand() % 10;
            int number = rand() % MAX;
            int x_1 = width/8;
            int x_2 = width;
            int y_1 = height/8;
            int y_2 = height;
            RNG rng (0xFFFFFFFF);
            Mat copyimg_d(height, width, CV_8U, Scalar(255));
            int lineType = 8;
            fprintf(SEED, "random triangle/polygon\n");
            for (int i = 0; i < number; i++)
            {
                Point pt[2][3];
                pt[0][0].x = rng.uniform(x_1, x_2);
                pt[0][0].y = rng.uniform(y_1, y_2);
                pt[0][1].x = rng.uniform(x_1, x_2);
                pt[0][1].y = rng.uniform(y_1, y_2);
                pt[0][2].x = rng.uniform(x_1, x_2);
                pt[0][2].y = rng.uniform(y_1, y_2);
                pt[1][0].x = rng.uniform(x_1, x_2);
                pt[1][0].y = rng.uniform(y_1, y_2);
                pt[1][1].x = rng.uniform(x_1, x_2);
                pt[1][1].y = rng.uniform(y_1, y_2);
                pt[1][2].x = rng.uniform(x_1, x_2);
                pt[1][2].y = rng.uniform(y_1, y_2);

                const Point* ppt[2] = {pt[0], pt[1]};
                int npt[] = {3, 3};

                fillPoly(copyimg_d, ppt, npt, 2, randomColor(rng), lineType );
                
            }
            imwrite("random_poly.pgm", copyimg_d);
            copyimg = copyimg_d.clone();
        } else if(sample == ran_ecl) {
            fprintf(SEED, "Random eclipse\n");
            time_t t;
            int MAX = rand() % 500;
            int number = rand() % MAX;
            int x_1 = -width/2;
            int x_2 = width*3/2;
            int y_1 = -width/2;
            int y_2 = width*3/2;
            RNG rng (0xFFFFFFFF);
            Mat copyimg_d(height, width, CV_8U, Scalar(255));
            int lineType = 8;
            for ( int i = 0; i < number; i++ )
            {
                Point center;
                center.x = rng.uniform(x_1, x_2);
                center.y = rng.uniform(y_1, y_2);

                Size axes;
                axes.width = rng.uniform(0, 200);
                axes.height = rng.uniform(0, 200);

                double angle = rng.uniform(0, 180);

                ellipse( copyimg_d, center, axes, angle, angle - 100, angle + 200,
                         randomColor(rng), rng.uniform(-1,9), lineType );
            }
            imwrite("random_ecl.pgm", copyimg_d);
            copyimg = copyimg_d.clone();  
        } else if (sample == ran_lines){
            fprintf(SEED, "Random lines\n");
            int MAX = rand() % 1000;
            int number = rand() % MAX;
            int x_1 = -width/2;
            int x_2 = width*3/2;
            int y_1 = -width/2;
            int y_2 = width*3/2;
            RNG rng (0xFFFFFFFF);
            Mat copyimg_d(height, width, CV_8U, Scalar(255));

            Point pt1, pt2;

            for( int i = 0; i < number; i++ )
            {
                pt1.x = rng.uniform( x_1, x_2 );
                pt1.y = rng.uniform( y_1, y_2 );
                pt2.x = rng.uniform( x_1, x_2 );
                pt2.y = rng.uniform( y_1, y_2 );

                line( copyimg_d, pt1, pt2, randomColor(rng), rng.uniform(1, 10), 8 );
            }
            imwrite("random_line.pgm", copyimg_d);
            copyimg = copyimg_d.clone();  
        } else if (sample == ran_rect){
            fprintf(SEED, "Random rect\n");
            int MAX = rand() % 1000;
            int number = rand() % MAX;
            int x_1 = -width/2;
            int x_2 = width*3/2;
            int y_1 = -width/2;
            int y_2 = width*3/2;
            RNG rng (0xFFFFFFFF);
            Mat copyimg_d(height, width, CV_8U, Scalar(255));
            Point pt1, pt2;
            int thickness = rng.uniform( -3, 10 );
            int lineType = 8; 

            for( int i = 0; i < number; i++ )
            {
                pt1.x = rng.uniform( x_1, x_2 );
                pt1.y = rng.uniform( y_1, y_2 );
                pt2.x = rng.uniform( x_1, x_2 );
                pt2.y = rng.uniform( y_1, y_2 );

                rectangle( copyimg_d, pt1, pt2, randomColor(rng), MAX( thickness, -1 ), lineType );

            }
           imwrite("random_rect.pgm", copyimg_d);
           copyimg = copyimg_d.clone(); 
        } else if (sample == ran_rotate_crop){
            fprintf(SEED, "16 pixel with density rotate and crop\n");
            printf ("16 pixel with density rotate and crop\n");

            int height_org = 480;
            int width_org = 640;
            int cropped_width = width_d;
            int cropped_height = height_d;
            double angle = angle_d;
            fprintf(SEED, "angle:%f\n", angle);
            
            Mat copyimg_org(height_org, width_org, CV_8U, Scalar(255));
            for (int i = 2; i < width_org - 16; i = i + 18){
                for (int j = 2 ; j < height_org - 2; j = j + 4){
                    //printf ("i:%d, j:%d\n", i, j );
                    
                    sixteen_pixel(copyimg_org, width_org, height_org, j, i);
                }
            }
            imwrite("rotate_1.pgm", copyimg_org);
     
            Point2f center(copyimg_org.cols/2.0, copyimg_org.rows/2.0);
            Mat rot = getRotationMatrix2D(center, angle, 1.0);
            Rect bbox = RotatedRect(center, copyimg_org.size(), angle).boundingRect();
            rot.at<double>(0,2) += bbox.width/2.0 - center.x;
            rot.at<double>(1,2) += bbox.height/2.0 - center.y;

            cv::Mat rotated;
            cv::warpAffine(copyimg_org, rotated, rot, bbox.size());
            cv::imwrite("rotated_im.pgm", rotated);

            Rect myROI(0, 0, cropped_width, cropped_height);

            Mat cropped = rotated(myROI);
            
            cv::imwrite("cropped_im.pgm", cropped);
            copyimg = cropped.clone();
               
        }
        
    } else if (std::string (argv[1]) == "checkerbox") {
        fprintf(SEED, "checkerbox16 pixel with density rotate and crop\n");
        int num_points_d = atoi(argv[2]);
        int threshold_d  = atoi(argv[3]);
        int mask_en_d = atoi(argv[4]);
        int tthreshold_d = atoi(argv[5]);
        int imageSize_d = atoi(argv[6]);
        string instr_dir_d = argc > 7 ? argv[7] : ".";
        num_points = num_points_d; threshold = threshold_d; imageSize = imageSize_d; width = imageSize_d; height = imageSize_d;
        mask_en = mask_en_d; tthreshold = tthreshold_d; instr_dir = instr_dir_d;

        int blockSize = imageSize/8; // 68 * 8 (544)randomize 
        Mat copyimg_d(imageSize, imageSize, CV_8U, Scalar::all(0));
        unsigned char color = 0;
        for(int i = 0;i < imageSize; i = i+blockSize){
            color = ~color;
            for(int j = 0; j < imageSize; j = j + blockSize){
                Mat ROI = copyimg_d(Rect(i, j, blockSize, blockSize));
                ROI.setTo(Scalar::all(color));
                color=~color;
            }
        }
        imwrite("checkerboard.pgm", copyimg_d);
        copyimg = copyimg_d.clone();
        
    } else if (std::string (argv[1]) == "constantimages") {
        int sample = atoi(argv[2]);
        int num_points_d = atoi(argv[3]);
        int threshold_d  = atoi(argv[4]);
        int width_d = atoi(argv[5]);
        int height_d = atoi(argv[6]);
        int mask_en_d = atoi(argv[7]);
        int tthreshold_d = atoi(argv[8]);
        string instr_dir_d = argc > 9 ? argv[9] : ".";
        num_points = num_points_d; threshold = threshold_d; width = width_d; height = height_d;
        mask_en = mask_en_d; tthreshold = tthreshold_d; instr_dir = instr_dir_d;

        enum {black, white, top, bottom, left, right, circular, triangle};

        if (sample == black){
            fprintf(SEED, "Black image\n");
            printf("black image provided\n");
            Mat copyimg_d(height, width, CV_8U, Scalar(0));
            imwrite( "imageblack.pgm", copyimg_d );
            copyimg = copyimg_d.clone();

        } else if (sample == white) {
            fprintf(SEED, "White image\n");
            printf("black image provided\n");
            Mat copyimg_d(height, width, CV_8U, Scalar(255));
            imwrite( "imagewhite.pgm", copyimg_d );
            copyimg = copyimg_d.clone();

        } else if (sample == top) {
            fprintf(SEED, "color gradient top to bottom\n");
            printf("color gradient top to bottom\n");
            Mat copyimg_d(height, width, CV_8U, Scalar(0));
            for (int r = 0; r < copyimg_d.rows; r++)
            {
                copyimg_d.row(r).setTo(r);
            }

            imwrite( "imagegradient_top_bot.pgm", copyimg_d );
            copyimg = copyimg_d.clone();

        } else if (sample == bottom) {
            fprintf(SEED, "color gradient bottom to top\n");
            printf("color gradient top to bottom\n");
            Mat copyimg_d(height, width, CV_8U, Scalar(0));
            for (int r = 0; r < copyimg_d.rows; r++)
            {
                copyimg_d.row(r).setTo(255-r);
            }

            imwrite( "imagegradient_bot_top.pgm", copyimg_d );
            copyimg = copyimg_d.clone();

        } else if (sample == left) {
            fprintf(SEED, "color gradient left to right\n");
            printf("color gradient left to right\n");
            Mat copyimg_d(height, width, CV_8U, Scalar(0));
            for (int c = 0; c < copyimg_d.cols; c++)
            {
                copyimg_d.col(c).setTo(c);
            }

            imwrite( "imagegradient_left_right.pgm", copyimg_d );
            copyimg = copyimg_d.clone();

        } else if (sample == right) {
            fprintf(SEED, "color gradient right to left\n");
            printf("color gradient right to left\n");
            Mat copyimg_d(height, width, CV_8U, Scalar(255));
            for (int c = 0; c < copyimg_d.cols; c++)
            {
                copyimg_d.col(c).setTo(255-c);
            }

            imwrite( "imagegradient_right_left.pgm", copyimg_d );
            copyimg = copyimg_d.clone();

        } else if (sample == circular) {
            fprintf(SEED, "circular gradient provided\n");
            printf("circle gradient provided\n");
            //Mask out circle section
            Mat copyimg_1(height, width, CV_8U, Scalar(0));
            Mat maskimg(copyimg_1.size(), CV_8U, Scalar(0));
            circle(maskimg, Point(maskimg.cols / 2, maskimg.rows / 2), copyimg_1.rows/2, Scalar(255), -1);
            Mat copyimg_d;
            copyimg_1.copyTo(copyimg_d, maskimg);
            imwrite("circle_gradient.pgm", copyimg_d);
            copyimg = copyimg_d.clone();
            
        } else if (sample == triangle){
            fprintf(SEED, "random triangle provided\n");
            printf("random triangle provided\n");
            Mat copyimg_d(height, width, CV_8U, Scalar(255));
        
            MyLine( copyimg_d, Point( 30, 20 ), Point( 40, 80 ) );
            MyLine( copyimg_d, Point( 30, 20 ), Point( 10, 20 ) );
            MyLine( copyimg_d, Point( 40, 80 ), Point( 10, 20 ) );
        
        
        
            MyLine( copyimg_d, Point( 100, 200 ), Point( 80, 160 ) );
            MyLine( copyimg_d, Point( 100, 200 ), Point( 90, 120 ) );
            MyLine( copyimg_d, Point( 90, 120 ), Point( 80, 160 ) );
        
        
            imwrite("randomtri.pgm", copyimg_d);
            //unsigned char *copyimg_data = (unsigned char*)copyimg.data;
            copyimg = copyimg_d.clone();
        } 
        
    } else if (std::string (argv[1]) == "maxcoverage") {
        int sample  =  atoi(argv[2]);
        int num_points_d = atoi(argv[3]);
        int threshold_d  = atoi(argv[4]);
        int width_d = atoi(argv[5]);
        int height_d = atoi(argv[6]);
        int mask_en_d = atoi(argv[7]);
        int tthreshold_d = atoi(argv[8]);
        int inner_start_d = atoi(argv[9]);
        string instr_dir_d = argc > 10 ? argv[10] : ".";
        num_points = num_points_d; threshold = threshold_d; width = width_d; height = height_d; inner_start = inner_start_d;
        mask_en = mask_en_d; tthreshold = tthreshold_d;  instr_dir = instr_dir_d;
        enum {eighteen, sixteen, full, diagonal};

        if (sample == eighteen){
            fprintf(SEED, "18 pixels provided\n");
            printf("18 pixels provided\n");
            Mat copyimg_d(height, width, CV_8U, Scalar(255));
            eighteen_pixel(copyimg_d, width, height, inner_start);
            imwrite("rectbox_rect.pgm", copyimg_d);
            copyimg = copyimg_d.clone();
            
        } else if (sample  ==  sixteen) {
            fprintf(SEED, "16 pixels provided\n");
            printf("16 pixels provided\n");
            Mat copyimg_d(height, width, CV_8U, Scalar(255));
            sixteen_pixel(copyimg_d, width, height, inner_start, inner_start);
            imwrite("rectbox_rect.pgm", copyimg_d);
            copyimg = copyimg_d.clone();

        } else if (sample == full) {
            int counter = 0;
            fprintf(SEED, "16 pixels with full density provided\n");
            printf ("16 pixel with full density\n");
            printf("width:%d, height:%d\n", width, height);
            Mat copyimg_d(height, width, CV_8U, Scalar(255));
            for (int i = 2; i < width - 16; i = i + 18){
                for (int j = 2 ; j < height - 2; j = j + 4){
                    //printf ("i:%d, j:%d\n", i, j );
                    
                    sixteen_pixel(copyimg_d, width, height, j, i);
                }
                counter++;
            }
            printf("counter:%d\n", counter);
            imwrite("rectbox_rect.pgm", copyimg_d);
            copyimg = copyimg_d.clone();
        } else if (sample == diagonal) {
            int counter = 0;
            fprintf(SEED, "16 pixels with diagonal density provided\n");
            printf ("16 pixel with diagonal density\n");
            printf("width:%d, height:%d\n", width, height);
            Mat copyimg_d(height, width, CV_8U, Scalar(255));
            for (int i = 2; i < width - 16; i = i + 18){
                for (int j = 2 ; j < height - 2; j = j + 4){
                    //printf ("i:%d, j:%d\n", i, j );
                    
                    sixteen_pixel(copyimg_d, width, height, j, j);
                }
                counter++;
            }
            printf("counter:%d\n", counter);
            imwrite("rectbox_rect.pgm", copyimg_d);
            copyimg = copyimg_d.clone();
        }
    }
                   
    

    A = fopen((instr_dir + "__CHL_Out_Matrix.txt").c_str(), "w+");
    D = fopen((instr_dir + "__CHL_Inv_Out_Matrix.txt").c_str(), "w+");
    B = fopen((instr_dir + "__CHLT_Out_Matrix.txt").c_str(), "w+");   
    C = fopen((instr_dir + "__CHL_In_Matrix.txt").c_str(), "w+");
    E = fopen((instr_dir + "__LCt_inn_out.txt").c_str(), "w+");
    F = fopen((instr_dir + "__LCt_inn_in.txt").c_str(), "w+");
    G = fopen((instr_dir + "__cov_state_in.txt").c_str(), "w+");
    H = fopen((instr_dir + "__cov_state_out.txt").c_str(), "w");
    I = fopen((instr_dir + "__cholesky_sw_out.txt").c_str(), "w+");
    J = fopen((instr_dir + "__Trk_FPPatchList.txt").c_str(), "w+");
    R = fopen((instr_dir + "__Trk_FPPredList.txt").c_str(),"w+");
    S = fopen((instr_dir + "__Trk_FPTrackList.txt").c_str(),"w+");
    W = fopen((instr_dir + "__Kt_out.txt").c_str(),"w+");
    
    
#if TRACKER_INS
    g_fp_patch = fopen((instr_dir + "__Trk_FPPatchList.txt").c_str(), "w+"); assert(g_fp_patch);
    g_fp_list = fopen((instr_dir + "__Trk_FPPredList.txt").c_str(), "w+"); assert(g_fp_list);
    g_fp_result = fopen((instr_dir + "__Trk_FPTrackList.txt").c_str(), "w+");
#endif

    //copyimg.step = copyimg.cols;
    
    fprintf(SEED, "SEED:%d", seed);
    fclose(SEED);
    copyimg_data = (unsigned char*)copyimg.data;

    //copyimg_data_dup = (const unsigned char*)copyimg.data;
    
    imwrite("copyimg.pgm", copyimg);

    //char img_file[32]; sprintf(img_file, "image_bmp.bmp");

    //printf("step: %u\n", copyimg.step);
    
    //bmp_write(img_file, copyimg_data_dup, copyimg.cols, copyimg.rows, copyimg.step);
    
    //printf("step1: %u\n", copyimg.step);
    
    dump_mem2d((instr_dir + "__Image1.txt").c_str(), copyimg_data, copyimg.cols, copyimg.rows, copyimg.step);
    dump_mem2d((instr_dir + "__Image2.txt").c_str(), copyimg_data, copyimg.cols, copyimg.rows, copyimg.step);

    //printf("step2: %u\n", copyimg.step);
    fast_detector_9 fast;
    fast.init(copyimg.cols, copyimg.rows, copyimg.step, 7, 3);

#if DETECTOR_INS
    g_fp_dist = fopen((instr_dir + "__Det1_FPList.txt").c_str(), "w+"); assert(g_fp_dist);
    g_fp_dist_1 = fopen((instr_dir + "__Det2_FPList.txt").c_str(), "w+"); assert(g_fp_dist_1);
    CONFIG = fopen((instr_dir + "__Config.txt").c_str(), "w+"); assert(CONFIG);
#endif

#if SORT_INS
    g_fp_sort = fopen((instr_dir + "__Sorts1.txt").c_str(), "w+"); assert(g_fp_sort);
    g_fp_sort_hex = fopen((instr_dir + "__Det1_SortedFPList.txt").c_str(), "w+"); assert(g_fp_sort_hex);
    g_fp_sort_patch = fopen((instr_dir + "__Det1_SortedFPPatchList.txt").c_str(), "w+"); assert(g_fp_sort_patch);
    g_fp_sort_1 = fopen((instr_dir + "__Sorts2.txt").c_str(), "w+"); assert(g_fp_sort_1);
    g_fp_sort_hex_1 = fopen((instr_dir + "__Det2_SortedFPList.txt").c_str(), "w+"); assert(g_fp_sort_hex_1);
    g_fp_sort_patch_1 = fopen((instr_dir + "__Det2_SortedFPPatchList.txt").c_str(), "w+"); assert(g_fp_sort_patch_1);
#endif 
   
#if MASK_EN 
// Read mask points binary file
   
    FILE *fp_mask_file = fopen((instr_dir + "__Masked_Coordinates.txt").c_str(), "w+");
    assert(fp_mask_file);
    
    scaled_mask mask(copyimg.cols, copyimg.rows);
    mask.initialize();
    if(mask_en) {
        for(uint32_t fy = 0; fy < copyimg.rows; fy += 8) {
            for(uint32_t fx = 0; fx < copyimg.cols; fx += 8) {
                int frac = (rand() % 100);
                if(frac < 10) {
                    mask.clear(fx, fy);
                }
            }
        }
    }

    FILE *fp_mask_excel = fopen((instr_dir + "__Masked_Bits.txt").c_str(), "w+");
    assert(fp_mask_excel);
    for(uint32_t fy = 0; fy < copyimg.rows; fy += 1) {
        for(uint32_t fx = 0; fx < copyimg.cols; fx += 8) {
            bool status = mask.test(fx, fy);
            for(int i = 8; i--;) {
                fprintf(fp_mask_excel, "%d\t", (int)status);
            }
            if((!status) && !(fy & 0x7)) {
                fprintf(fp_mask_file, "x:%d, y:%d\n", fx, fy); 
            }
        }
        fprintf(fp_mask_excel, "\n");
    }
    fclose(fp_mask_excel);
    fclose(fp_mask_file);
  
    mask.compress();
    
    dump_mem2d((instr_dir + "__Det1_Mask.txt").c_str(), mask.compressed,
               mask.compressed_width, mask.scaled_height, mask.compressed_width);
    dump_mem2d((instr_dir + "__Det2_Mask.txt").c_str(), mask.compressed,
               mask.compressed_width, mask.scaled_height, mask.compressed_width);

#else
    MASK1 = fopen((instr_dir + "__Det1_Mask.txt").c_str(),"w+");
    MASK2 = fopen((instr_dir + "__Det2_Mask.txt").c_str(),"w+");
#endif

    
#if MASK_EN
    fast.detect(copyimg_data, &mask, num_points, threshold, 0, 0, copyimg.cols, copyimg.rows);
#else
    fast.detect(copyimg_data, NULL, num_points, threshold, 0, 0, copyimg.cols, copyimg.rows);
#endif



    vector<xy> v = fast.features;
    printf("Num Key Points: %d\n", v.size());
#if SORT_INS
    fclose(g_fp_sort_patch);
    fclose(g_fp_sort_patch_1);
#endif

    int rateArray[3] = {10, 15, 20};
    int randomArrayind = rand() % 3;
    int mipirate = rateArray[randomArrayind];

    int detworksetmax = 256;
    int detworksetmin = 32;
    int detworkset = ((rand() % (detworksetmax - detworksetmin)) + detworksetmin);
    
    print_config(CONFIG, width, height, threshold, tthreshold, num_points, v.size(), v.size(), 1, 1, num_points, num_points, 8, 8, 8, 8, 8, 8, 0, mipirate, detworkset, 1, 1, 2);

    fclose(CONFIG);


    return 0;
}
 
void MyLine(Mat img, Point start, Point end )
{
    int thickness = 2;
    int lineType = 8;
    line( img,
          start,
          end,
          Scalar( 0, 0, 0 ),
          thickness,
          lineType );
}

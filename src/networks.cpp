/*  Copyright 2018 Oxford Nanopore Technologies, Ltd */

/*  This Source Code Form is subject to the terms of the Oxford Nanopore
 *  Technologies, Ltd. Public License, v. 1.0. If a copy of the License 
 *  was not  distributed with this file, You can obtain one at
 *  http://nanoporetech.com
 */

#include "layers.h"
#include "models/flipflop_r941native.h"
//#include "models/flipflop_r941native5mC.h"
//#include "models/flipflop_r10Cpcr.h"
#include "networks.h"
#include "nnfeatures.h"
#include "flappie_stdlib.h"
#include "util.h"


enum model_type get_flappie_model_type(const char *modelstr){
    assert(NULL != modelstr);
    if(0 == strcmp(modelstr, "r941_native")) {
        return FLAPPIE_MODEL_R941_NATIVE;
    }
    if(0 == strcmp(modelstr, "r941_5mC")) {
        return FLAPPIE_MODEL_R941_5mC;
    }
    if(0 == strcmp(modelstr, "r10c_pcr")) {
        return FLAPPIE_MODEL_R10C_PCR;
    }
    return FLAPPIE_MODEL_INVALID;
}


const char *flappie_model_string(const enum model_type model){
    switch(model){
    case FLAPPIE_MODEL_R941_NATIVE:
        return "r941_native";
    case FLAPPIE_MODEL_R941_5mC:
        return "r941_5mC";
    case FLAPPIE_MODEL_R10C_PCR:
        return "r10c_pcr";
    case FLAPPIE_MODEL_INVALID:
        errx(EXIT_FAILURE, "Invalid Flappie model  %s:%d", __FILE__, __LINE__);
    default:
        errx(EXIT_FAILURE, "Flappie enum failure -- report as bug. %s:%d \n", __FILE__, __LINE__);
    }
    return NULL;
}
        

const char *flappie_model_description(const enum model_type model){
    switch(model){
    case FLAPPIE_MODEL_R941_NATIVE:
        return "R9.4.1 model for MinION.  Trained from native DNA library";
    case FLAPPIE_MODEL_R941_5mC:
        return "R9.4.1 model for PromethION; 5mC aware.  Trained from native NA12878 library";
    case FLAPPIE_MODEL_R10C_PCR:
        return "R10C model for MinION.  Trained from PCR'd DNA library";
    case FLAPPIE_MODEL_INVALID:
        errx(EXIT_FAILURE, "Invalid Flappie model  %s:%d", __FILE__, __LINE__);
    default:
        errx(EXIT_FAILURE, "Flappie enum failure -- report as bug. %s:%d \n", __FILE__, __LINE__);
    }
    return NULL;
}


transition_function_ptr get_transition_function(const enum model_type model){
    switch(model){
    case FLAPPIE_MODEL_R941_NATIVE:
        return flipflop_transitions_r941native;
    case FLAPPIE_MODEL_INVALID:
        errx(EXIT_FAILURE, "Invalid Flappie model  %s:%d", __FILE__, __LINE__);
    default:
        errx(EXIT_FAILURE, "Flappie enum failure -- report as bug. %s:%d \n", __FILE__, __LINE__);
    }
    return NULL;
}


flappie_matrix flipflop_transitions(const raw_table signal, float temperature, enum model_type model){
    transition_function_ptr transfun = get_transition_function(model);
    return transfun(signal, temperature);
}


typedef struct {
    //  Convolution layer
    const flappie_matrix conv_W;
    const flappie_matrix conv_b;
    int conv_stride;
    //  First modified GRU (backward)
    const flappie_matrix gruB1_iW;
    const flappie_matrix gruB1_sW;
    const flappie_matrix gruB1_sW2;
    const flappie_matrix gruB1_b;
    //  Second modified GRU (forward)
    const flappie_matrix gruF2_iW;
    const flappie_matrix gruF2_sW;
    const flappie_matrix gruF2_sW2;
    const flappie_matrix gruF2_b;
    //  Third modified GRU (backward)
    const flappie_matrix gruB3_iW;
    const flappie_matrix gruB3_sW;
    const flappie_matrix gruB3_sW2;
    const flappie_matrix gruB3_b;
    //  Fourth modified GRU (forward)
    const flappie_matrix gruF4_iW;
    const flappie_matrix gruF4_sW;
    const flappie_matrix gruF4_sW2;
    const flappie_matrix gruF4_b;
    //  Fifth modified GRU (backward)
    const flappie_matrix gruB5_iW;
    const flappie_matrix gruB5_sW;
    const flappie_matrix gruB5_sW2;
    const flappie_matrix gruB5_b;
    //  Output
    const flappie_matrix FF_W;
    const flappie_matrix FF_b;
} sloika_model;


typedef struct {
    //  Convolution layer
    const flappie_matrix conv_W;
    const flappie_matrix conv_b;
    int conv_stride;
    //  First modified GRU (backward)
    const flappie_matrix gruB1_iW;
    const flappie_matrix gruB1_sW;
    const flappie_matrix gruB1_b;
    //  Second modified GRU (forward)
    const flappie_matrix gruF2_iW;
    const flappie_matrix gruF2_sW;
    const flappie_matrix gruF2_b;
    //  Third modified GRU (backward)
    const flappie_matrix gruB3_iW;
    const flappie_matrix gruB3_sW;
    const flappie_matrix gruB3_b;
    //  Fourth modified GRU (forward)
    const flappie_matrix gruF4_iW;
    const flappie_matrix gruF4_sW;
    const flappie_matrix gruF4_b;
    //  Fifth modified GRU (backward)
    const flappie_matrix gruB5_iW;
    const flappie_matrix gruB5_sW;
    const flappie_matrix gruB5_b;
    //  Output
    const flappie_matrix FF_W;
    const flappie_matrix FF_b;
} guppy_model;


guppy_model flipflop_r941native_guppy = {
    //  Convolution layer
    .conv_W = &_conv_rnnrf_flipflop_r941native_W,
    .conv_b = &_conv_rnnrf_flipflop_r941native_b,
    .conv_stride = conv_rnnrf_flipflop_r941native_stride,
    //.conv_stride = 2,
    //  First modified GRU (backward)
    .gruB1_iW = &_gruB1_rnnrf_flipflop_r941native_iW,
    .gruB1_sW = &_gruB1_rnnrf_flipflop_r941native_sW,
    .gruB1_b = &_gruB1_rnnrf_flipflop_r941native_b,
    //  Second modified GRU (forward)
    .gruF2_iW = &_gruF2_rnnrf_flipflop_r941native_iW,
    .gruF2_sW = &_gruF2_rnnrf_flipflop_r941native_sW,
    .gruF2_b = &_gruF2_rnnrf_flipflop_r941native_b,
    //  Third modified GRU (backward)
    .gruB3_iW = &_gruB3_rnnrf_flipflop_r941native_iW,
    .gruB3_sW = &_gruB3_rnnrf_flipflop_r941native_sW,
    .gruB3_b = &_gruB3_rnnrf_flipflop_r941native_b,
    //  Fourth modified GRU (forward)
    .gruF4_iW = &_gruF4_rnnrf_flipflop_r941native_iW,
    .gruF4_sW = &_gruF4_rnnrf_flipflop_r941native_sW,
    .gruF4_b = &_gruF4_rnnrf_flipflop_r941native_b,
    //  Fifth modified GRU (backward)
    .gruB5_iW = &_gruB5_rnnrf_flipflop_r941native_iW,
    .gruB5_sW = &_gruB5_rnnrf_flipflop_r941native_sW,
    .gruB5_b = &_gruB5_rnnrf_flipflop_r941native_b,
    //  Output
    .FF_W = &_FF_rnnrf_flipflop_r941native_W,
    .FF_b = &_FF_rnnrf_flipflop_r941native_b
};



flappie_matrix flipflop_gru_transitions(const raw_table signal, float temperature, const sloika_model * net){
    RETURN_NULL_IF(0 == signal.n, NULL);
    RETURN_NULL_IF(NULL == signal.raw, NULL);

    flappie_matrix raw_mat = features_from_raw(signal, 4);
    flappie_matrix conv =
        convolution(raw_mat, net->conv_W, net->conv_b, net->conv_stride, NULL);
    elu_activation_inplace(conv);
    raw_mat = free_flappie_matrix(raw_mat);
    //  First GRU layer
    flappie_matrix gruB1in = feedforward_linear(conv, net->gruB1_iW, net->gruB1_b, NULL);
    flappie_matrix gruB1 = gru_backward(gruB1in, net->gruB1_sW, net->gruB1_sW2, NULL);
    residual_inplace(conv, gruB1);
    conv = free_flappie_matrix(conv);
    gruB1in = free_flappie_matrix(gruB1in);
    //  Second GRU layer
    flappie_matrix gruF2in = feedforward_linear(gruB1, net->gruF2_iW, net->gruF2_b, NULL);
    flappie_matrix gruF2 = gru_forward(gruF2in, net->gruF2_sW, net->gruF2_sW2, NULL);
    residual_inplace(gruB1, gruF2);
    gruB1 = free_flappie_matrix(gruB1);
    gruF2in = free_flappie_matrix(gruF2in);
    //  Third GRU layer
    flappie_matrix gruB3in = feedforward_linear(gruF2, net->gruB3_iW, net->gruB3_b, NULL);
    flappie_matrix gruB3 = gru_backward(gruB3in, net->gruB3_sW, net->gruB3_sW2, NULL);
    residual_inplace(gruF2, gruB3);
    gruF2 = free_flappie_matrix(gruF2);
    gruB3in = free_flappie_matrix(gruB3in);
    //  Fourth GRU layer
    flappie_matrix gruF4in = feedforward_linear(gruB3, net->gruF4_iW, net->gruF4_b, NULL);
    flappie_matrix gruF4 = gru_forward(gruF4in, net->gruF4_sW, net->gruF4_sW2, NULL);
    residual_inplace(gruB3, gruF4);
    gruB3 = free_flappie_matrix(gruB3);
    gruF4in = free_flappie_matrix(gruF4in);
    //  Fifth GRU layer
    flappie_matrix gruB5in = feedforward_linear(gruF4, net->gruB5_iW, net->gruB5_b, NULL);
    flappie_matrix gruB5 = gru_backward(gruB5in, net->gruB5_sW, net->gruB5_sW2, NULL);
    residual_inplace(gruF4, gruB5);
    gruF4 = free_flappie_matrix(gruF4);
    gruB5in = free_flappie_matrix(gruB5in);

    flappie_matrix trans = globalnorm_flipflop(gruB5, net->FF_W, net->FF_b, temperature, NULL);
    gruB5 = free_flappie_matrix(gruB5);

    return trans;
}

#include <sys/time.h>
struct timeval start, end_time;

flappie_matrix flipflop_guppy_transitions(const raw_table signal, float temperature, const guppy_model * net){
    RETURN_NULL_IF(0 == signal.n, NULL);
    RETURN_NULL_IF(NULL == signal.raw, NULL);
    long useconds, seconds, mseconds;

    // FIX padding disable changes
    //flappie_matrix raw_mat = features_from_raw(signal, 4);
    flappie_matrix raw_mat = features_from_raw(signal, 0);

    flappie_matrix conv = convolution_linear(raw_mat, net->conv_W, net->conv_b, net->conv_stride, NULL, net->gruB1_iW, net->gruB1_b);
    raw_mat = free_flappie_matrix(raw_mat);

    flappie_matrix gruB1 = aes_grumod_linear(conv, net->gruB1_sW, NULL, 1, net->gruF2_iW, net->gruF2_b );
    conv = free_flappie_matrix(conv);

    flappie_matrix gruF2 = aes_grumod_linear(gruB1, net->gruF2_sW, NULL, 0, net->gruB3_iW, net->gruB3_b );
    gruB1 = free_flappie_matrix(gruB1);

    flappie_matrix gruB3 = aes_grumod_linear(gruF2, net->gruB3_sW, NULL, 1, net->gruF4_iW, net->gruF4_b);
    gruF2 = free_flappie_matrix(gruF2);

    flappie_matrix gruF4 = aes_grumod_linear(gruB3, net->gruF4_sW, NULL, 0, net->gruB5_iW, net->gruB5_b);
    gruB3 = free_flappie_matrix(gruB3);

    //flappie_matrix gruB5 = aes_grumod_linear(gruF4, net->gruB5_sW, NULL, 2, net->gruB5_iW, net->gruB5_b);
    flappie_matrix gruB5 = aes_grumod(gruF4, net->gruB5_sW, NULL, 1);
    gruF4 = free_flappie_matrix(gruF4);

    flappie_matrix trans = globalnorm_flipflop(gruB5, net->FF_W, net->FF_b, temperature, NULL);
    gruB5 = free_flappie_matrix(gruB5);

    return trans;
}


flappie_matrix flipflop_relu_transitions(const raw_table signal, float temperature, const sloika_model * net){
    RETURN_NULL_IF(0 == signal.n, NULL);
    RETURN_NULL_IF(NULL == signal.raw, NULL);

    flappie_matrix raw_mat = features_from_raw(signal, 4);
    flappie_matrix conv =
        convolution(raw_mat, net->conv_W, net->conv_b, net->conv_stride, NULL);
    elu_activation_inplace(conv);
    raw_mat = free_flappie_matrix(raw_mat);
    //  First GRU layer
    flappie_matrix gruB1in = feedforward_linear(conv, net->gruB1_iW, net->gruB1_b, NULL);
    flappie_matrix gruB1 = gru_relu_backward(gruB1in, net->gruB1_sW, net->gruB1_sW2, NULL);
    residual_inplace(conv, gruB1);
    conv = free_flappie_matrix(conv);
    gruB1in = free_flappie_matrix(gruB1in);
    //  Second GRU layer
    flappie_matrix gruF2in = feedforward_linear(gruB1, net->gruF2_iW, net->gruF2_b, NULL);
    flappie_matrix gruF2 = gru_relu_forward(gruF2in, net->gruF2_sW, net->gruF2_sW2, NULL);
    residual_inplace(gruB1, gruF2);
    gruB1 = free_flappie_matrix(gruB1);
    gruF2in = free_flappie_matrix(gruF2in);
    //  Third GRU layer
    flappie_matrix gruB3in = feedforward_linear(gruF2, net->gruB3_iW, net->gruB3_b, NULL);
    flappie_matrix gruB3 = gru_relu_backward(gruB3in, net->gruB3_sW, net->gruB3_sW2, NULL);
    residual_inplace(gruF2, gruB3);
    gruF2 = free_flappie_matrix(gruF2);
    gruB3in = free_flappie_matrix(gruB3in);
    //  Fourth GRU layer
    flappie_matrix gruF4in = feedforward_linear(gruB3, net->gruF4_iW, net->gruF4_b, NULL);
    flappie_matrix gruF4 = gru_relu_forward(gruF4in, net->gruF4_sW, net->gruF4_sW2, NULL);
    residual_inplace(gruB3, gruF4);
    gruB3 = free_flappie_matrix(gruB3);
    gruF4in = free_flappie_matrix(gruF4in);
    //  Fifth GRU layer
    flappie_matrix gruB5in = feedforward_linear(gruF4, net->gruB5_iW, net->gruB5_b, NULL);
    flappie_matrix gruB5 = gru_relu_backward(gruB5in, net->gruB5_sW, net->gruB5_sW2, NULL);
    residual_inplace(gruF4, gruB5);
    gruF4 = free_flappie_matrix(gruF4);
    gruB5in = free_flappie_matrix(gruB5in);

    flappie_matrix trans = globalnorm_flipflop(gruB5, net->FF_W, net->FF_b, temperature, NULL);
    gruB5 = free_flappie_matrix(gruB5);

    return trans;
}


flappie_matrix flipflop_transitions_r941native(const raw_table signal, float temperature){
    return flipflop_guppy_transitions(signal, temperature, &flipflop_r941native_guppy);
}


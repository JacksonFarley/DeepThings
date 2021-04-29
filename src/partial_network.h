#ifndef PARTIAL_NETWORK_H 
#define PARTIAL_NETWORK_H
#include "ftp.h"
#include "inference_engine_helper.h"
cnn_model* load_partial_cnn_model(char*cfg, char*weights, int start, int cutoff);
void make_partial_weight_files(char*cfg, char*weights);
#endif


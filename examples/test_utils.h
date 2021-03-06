#ifndef TEST_UTIL_H
#define TEST_UTIL_H
#include "darkiot.h"
#include "configure.h"
#include "ftp.h"
#include "cmd_line_parser.h"
#include "frame_partitioner.h"
#include "deepthings_edge.h"
#include "deepthings_gateway.h"
#include "inference_engine_helper.h"
#include "frame_partitioner.h"
#include "reuse_data_serialization.h"
#include "partial_network.h"

/*Functions defined for testing and profiling*/
void process_task_single_device(device_ctxt* ctxt, blob* temp, bool is_reuse, int ftp_num);
void process_everything_in_gateway(void *arg);
void transfer_data_with_number(device_ctxt* client, device_ctxt* gateway, int32_t task_num);
void transfer_data(device_ctxt* client, device_ctxt* gateway);
void client_sink(void* arg); // use a client context but this is for thread reasons
void deepthings_merge_result_thread_single_device(void *arg);
void partition_frame_and_perform_inference_thread_single_device(void *arg);
void partition_secondary_and_perform_inference_thread_single_device(void *arg);
void process_task_single_device_jf(void *arg); 


// for argument passing to process_task_single_device
typedef struct ptsd_arguments {
    device_ctxt * ctxt;
    blob * temp;
    bool is_reuse;
    uint32_t ftp_num;
} ptsd_args; 


#endif/*TEST_UTIL*/

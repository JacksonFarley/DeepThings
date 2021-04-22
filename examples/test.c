#include "test_utils.h"

static const char* addr_list[MAX_EDGE_NUM] = EDGE_ADDR_LIST;


//#define MULTITHREAD
//#define SINGLETHREAD
#define CLIENT_ONLY

int main(int argc, char **argv){
   /*Initialize the data structure and network model*/
   uint32_t total_cli_num = get_int_arg(argc, argv, "-total_edge", 1);
   uint32_t this_cli_id = get_int_arg(argc, argv, "-edge_id", 0);

   uint32_t partitions_h = get_int_arg(argc, argv, "-n", 3);
   uint32_t partitions_w = get_int_arg(argc, argv, "-m", 3);
   uint32_t fused_layers = get_int_arg(argc, argv, "-l", 16);

#ifndef CLIENT_ONLY
   char network_file[30] = "../models/yolo.cfg";
#endif 
   char client_network_file[30] = "../models/yolo_cut.cfg";
   char weight_file[30] = "../models/yolo.weights";

   printf("Including %d partitions of %d fused layers\n",partitions_h*partitions_w,fused_layers);

   device_ctxt* client_ctxt = deepthings_edge_init(partitions_h, partitions_w, fused_layers, client_network_file, weight_file, this_cli_id);
#ifndef CLIENT_ONLY
   device_ctxt* gateway_ctxt = deepthings_gateway_init(partitions_h, partitions_w, fused_layers, network_file, weight_file, total_cli_num, addr_list);
#endif

   /*Multi-threaded version*/ 
#ifdef MULTITHREAD          
   sys_thread_t t1 = sys_thread_new("partition_frame_and_perform_inference_thread_single_device", 
                                     partition_frame_and_perform_inference_thread_single_device, client_ctxt, 0, 0);
   sys_thread_t t2 = sys_thread_new("deepthings_merge_result_thread_single_device", deepthings_merge_result_thread_single_device, gateway_ctxt, 0, 0);
   transfer_data(client_ctxt, gateway_ctxt);
   printf("Finished Transferring Data\n"); 
   sys_thread_join(t1);
   sys_thread_join(t2);
#endif

   /*Single-thread version*/
#ifdef SINGLETHREAD  
   partition_frame_and_perform_inference_thread_single_device(client_ctxt);
   transfer_data_with_number(client_ctxt, gateway_ctxt, FRAME_NUM*partitions_h*partitions_h);
   deepthings_merge_result_thread_single_device(gateway_ctxt);
#endif

   /* Client-only version: comment out line 18, gateway_ctxt as it is not necessary*/
#ifdef CLIENT_ONLY
   sys_thread_t t1 = sys_thread_new("partition_frame_and_perform_inference_thread_single_device", 
                                     partition_frame_and_perform_inference_thread_single_device, client_ctxt, 0, 0);
   client_sink(client_ctxt);
   printf("Finished Client Sink\n"); 
   sys_thread_join(t1);
#endif

   return 0;
}


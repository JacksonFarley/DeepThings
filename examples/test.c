#include "test_utils.h"

static const char* addr_list[MAX_EDGE_NUM] = EDGE_ADDR_LIST;


//#define MULTITHREAD
//#define SINGLETHREAD
//#define CLIENT_ONLY
#define MULTI_CLIENT_FTP

int main(int argc, char **argv){
   /*Initialize the data structure and network model*/
   uint32_t total_cli_num = get_int_arg(argc, argv, "-total_edge", 1);
   uint32_t this_cli_id = get_int_arg(argc, argv, "-edge_id", 0);

   uint32_t partitions_h = get_int_arg(argc, argv, "-n", 3);
   uint32_t partitions_w = get_int_arg(argc, argv, "-m", 3);
   uint32_t fused_layers = get_int_arg(argc, argv, "-l", 16);

#ifdef MULTI_CLIENT_FTP
//   uint32_t this_sec_cli_id = get_int_arg(argc, argv, "-sec_edge_id", 1); 
   uint32_t partitions_h2 = get_int_arg(argc, argv, "-sec_n", 2);
   uint32_t partitions_w2 = get_int_arg(argc, argv, "-sec_m", 2);
#endif
 


#ifndef CLIENT_ONLY
   char network_file[30] = "../models/yolo.cfg";
#endif 
   char client_network_file[30] = "../models/yolo_cut.cfg";
   char weight_file[30] = "../models/yolo.weights";

   printf("Including %d partitions of %d fused layers\n",partitions_h*partitions_w,fused_layers);
   printf("initializing client with %d fused layers\n",fused_layers/2);
   device_ctxt* client_ctxt = deepthings_edge_init(partitions_h, partitions_w, fused_layers/2, client_network_file, weight_file, this_cli_id);

#ifdef MULTITHREAD
   printf("initializing gateway\n");
   device_ctxt* gateway_ctxt = deepthings_gateway_init(partitions_h, partitions_w, fused_layers, network_file, weight_file, total_cli_num, addr_list);
#endif

#ifdef MULTI_CLIENT_FTP
   // this device should get updated with the remaining fused layers
   printf("initializing secondary client\n");
   device_ctxt* secondary_client_ctxt = deepthings_secondary_edge_init(partitions_h, partitions_w, fused_layers/2, fused_layers, client_network_file, weight_file, this_cli_id); 
   
   printf("initializing gateway\n");
   device_ctxt* gateway_ctxt = deepthings_gateway_init(partitions_h2, partitions_w2, fused_layers, network_file, weight_file, total_cli_num, addr_list);
   
#endif

printf("starting execution");
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

#ifdef MULTI_CLIENT_FTP
   printf("running multi-client version\n"); 
   sys_thread_t t1 = sys_thread_new("partition_frame_and_perform_inference_thread_single_device", 
                                     partition_frame_and_perform_inference_thread_single_device, client_ctxt, 0, 0);
   sys_thread_t t2 = sys_thread_new("partition_secondary_and_perform_inference_thread_single_device",
                                     partition_secondary_and_perform_inference_thread_single_device, secondary_client_ctxt, 0, 0);
   sys_thread_t t3 = sys_thread_new("deepthings_merge_result_thread_single_device", deepthings_merge_result_thread_single_device, gateway_ctxt, 0, 0);
   transfer_data(client_ctxt, secondary_client_ctxt);
   printf("Finished transferring across first FTP cut\n");
   transfer_data(secondary_client_ctxt, gateway_ctxt);
   printf("Finished Transferring Data\n"); 
   sys_thread_join(t1);
   sys_thread_join(t2);
   sys_thread_join(t3); 
#endif    

   return 0;
}


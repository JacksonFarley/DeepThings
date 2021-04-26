#include "test_utils.h"

static const char* addr_list[MAX_EDGE_NUM] = EDGE_ADDR_LIST;

// Local Data Types

typedef enum mode_of_execution {
   MultiThread_mode,
   ClientOnly_mode,
   MultiFuse_mode,
   MultiFusesOnly_mode
} mode_of_exe;

typedef struct main_parameters {
   uint32_t partitions_h;
   uint32_t partitions_w;
   uint32_t cutoff;
   uint32_t fused_layers;
   uint32_t partitions_h2;
   uint32_t partitions_w2;
   bool verbose;    
   
} main_para;

main_para default_main_parameters(void){
   main_para args = {3,3,8,16,2,2,1};
   return args;
}


// Local Functions 

int execute_MultiThread(main_para *);
int execute_ClientOnly(main_para *);
int execute_MultiFuse(main_para *);
int execute_MultiFusesOnly(main_para *);


// GLOBALS

// filenames
char network_file[30] = "../models/yolo.cfg";
char client_network_file[30] = "../models/yolo_cut.cfg";
char weight_file[30] = "../models/yolo.weights";


// client globals. Currently only using one client so these can be set here
uint32_t total_cli_num = 1;
uint32_t this_cli_id = 0;



/**********************************************************************************/
/**                                 MAIN                                         **/
/**********************************************************************************/
int main(int argc, char **argv){
/* main function and argument parsing, takes the mode of execution as an input 
 * Possible modes of execution
 * 0: MultiThread    - using a client thread and a gateway with one ftp from 0 - l
 * 1: ClientOnly     - using only the client thread to analyze just the fused layers. 
 *                     Only one ftp from 0 - l 
 * 2: MultiFuse      - using multiple fuses and corresponding client threads along with a gateway
 *                   - the two fuses 
 * 3: MultiFusesOnly - Using multiple fuses (currently max 2) and client threads. The ftps will 
 *                     be from 0 - cutoff and cutoff - l
 *
 * Note: Data Reuse must be toggled in the include/configure.h file and made again
 */

   mode_of_exe mode = (mode_of_exe) get_int_arg(argc,argv,"-mode",0);
   main_para args = default_main_parameters(); 

/*
   uint32_t total_cli_num = get_int_arg(argc, argv, "-total_edge", 1);
   uint32_t this_cli_id = get_int_arg(argc, argv, "-edge_id", 0);
*/
   args.partitions_h  = get_int_arg(argc, argv, "-n", 3);
   args.partitions_w  = get_int_arg(argc, argv, "-m", 3);
   args.cutoff        = get_int_arg(argc, argv, "-cut", 8); 
   args.fused_layers  = get_int_arg(argc, argv, "-l", 16);
   args.partitions_h2 = get_int_arg(argc, argv, "-sec_n", 2);
   args.partitions_w2 = get_int_arg(argc, argv, "-sec_m", 2);
   args.verbose =   (bool) get_int_arg(argc, argv, "-v", 1);    

   switch(mode)
   {
      case MultiThread_mode:
         execute_MultiThread(&args); 
         break;
      case ClientOnly_mode:
         execute_ClientOnly(&args);
         break;
      case MultiFuse_mode:
         execute_MultiFuse(&args);
         break;
      case MultiFusesOnly_mode:   
         execute_MultiFusesOnly(&args);
   }
   return 0;
}


int execute_MultiThread(main_para * args){
   // Initialization
   if(args->verbose){
      printf("Executing in MultiThread mode\n");
      printf("Initializing client with %dx%d partitions of %d fused layers\n", args->partitions_h, args->partitions_w, args->fused_layers); 
   }
   device_ctxt* client_ctxt = deepthings_edge_init(args->partitions_h, args->partitions_w, args->fused_layers, client_network_file, weight_file, this_cli_id);
   
   if(args->verbose){
      printf("initializing gateway\n");
   }
   device_ctxt* gateway_ctxt = deepthings_gateway_init(args->partitions_h, args->partitions_w, args->fused_layers, network_file, weight_file, total_cli_num, addr_list);

   // Thread Creation
   sys_thread_t t1 = sys_thread_new("client_thread", partition_frame_and_perform_inference_thread_single_device, client_ctxt, 0, 0);
   sys_thread_t t2 = sys_thread_new("gateway_thread", deepthings_merge_result_thread_single_device, gateway_ctxt, 0, 0);
   transfer_data(client_ctxt, gateway_ctxt);
   
   if(args->verbose){
      printf("Finished Transferring Data\n"); 
   }
   sys_thread_join(t1);
   sys_thread_join(t2);

   return 0; 
    
}

int execute_ClientOnly(main_para * args){
   // Initialization
   if(args->verbose){
      printf("Executing in ClientOnly mode\n");
      printf("Initializing client with %dx%d partitions of %d fused layers\n",args->partitions_h, args->partitions_w, args->fused_layers); 
   } 
   device_ctxt* client_ctxt = deepthings_edge_init(args->partitions_h, args->partitions_w, args->fused_layers, client_network_file, weight_file, this_cli_id);

   // Thread Creation
   sys_thread_t t1 = sys_thread_new("client_thread", partition_frame_and_perform_inference_thread_single_device, client_ctxt, 0, 0);
   client_sink(client_ctxt);
   if(args->verbose){
     printf("Finished Client Sink\n"); 
   }
   sys_thread_join(t1);
   
   return 0; 
}

int execute_MultiFuse(main_para * args){
   // Initialization    
   if(args->verbose){
      printf("Executing in MultiFuse mode\n");
      printf("Initializing first client with %dx%d partitions of %d fused layers\n",args->partitions_h, args->partitions_w, args->cutoff); 
   }
   device_ctxt* client_ctxt = deepthings_edge_init(args->partitions_h, args->partitions_w, args->cutoff, client_network_file, weight_file, this_cli_id);
   
   if(args->verbose){
      printf("initializing secondary client with %dx%d partitions from layer %d to layer %d\n",
             args->partitions_h2,args->partitions_w2,args->cutoff,args->fused_layers);
   }
   // TODO: parameterize the secondary partition size
   device_ctxt* secondary_client_ctxt = deepthings_secondary_edge_init(args->partitions_h, args->partitions_w, args->partitions_h2, args->partitions_w2,
                                                                        args->cutoff, args->fused_layers, client_network_file, weight_file, this_cli_id); 
   
   if(args->verbose){
      printf("initializing gateway\n");
   }
   device_ctxt* gateway_ctxt = deepthings_gateway_init(args->partitions_h2, args->partitions_w2, args->fused_layers, network_file, weight_file, total_cli_num, addr_list);
   

   // Thread Creation
   sys_thread_t t1 = sys_thread_new("first_client_thread", partition_frame_and_perform_inference_thread_single_device, client_ctxt, 0, 0);
   sys_thread_t t2 = sys_thread_new("second_client_thread", partition_secondary_and_perform_inference_thread_single_device, secondary_client_ctxt, 0, 0);
   sys_thread_t t3 = sys_thread_new("gateway_thread", deepthings_merge_result_thread_single_device, gateway_ctxt, 0, 0);
   
   // TODO: make this a thread so that it works across multiple images
   transfer_data(client_ctxt, secondary_client_ctxt);
   if(args->verbose){
      printf("Finished transferring across first FTP cut\n");
   }
   transfer_data(secondary_client_ctxt, gateway_ctxt);
   if(args->verbose){
      printf("Finished transferring data to gateway\n"); 
   }
   sys_thread_join(t1);
   sys_thread_join(t2);
   sys_thread_join(t3); 

   return 0;
}

int execute_MultiFusesOnly(main_para * args){
   // Initialization
   if(args->verbose){
      printf("executing in MultiFuse Clients Only mode\n");
      printf("Initializing first client with %dx%d partitions of %d fused layers\n",
             args->partitions_h,args->partitions_w,args->cutoff); 
   }
   device_ctxt* client_ctxt = deepthings_edge_init(args->partitions_h, args->partitions_w, args->cutoff, client_network_file, weight_file, this_cli_id);
   
   if(args->verbose){
      printf("Initializing secondary client with %dx%d partitions from layer %d to layer %d\n",
             args->partitions_h2,args->partitions_w2,args->cutoff,args->fused_layers);
   }
   device_ctxt* secondary_client_ctxt = deepthings_secondary_edge_init(args->partitions_h, args->partitions_w, args->partitions_h2, args->partitions_w2,
                                                                        args->cutoff, args->fused_layers, client_network_file, weight_file, this_cli_id); 
   
   // Thread Creation
   sys_thread_t t1 = sys_thread_new("first_client_thread", partition_frame_and_perform_inference_thread_single_device, client_ctxt, 0, 0);
   sys_thread_t t2 = sys_thread_new("second_client_thread", partition_secondary_and_perform_inference_thread_single_device, secondary_client_ctxt, 0, 0);
   transfer_data(client_ctxt, secondary_client_ctxt);
   if(args->verbose){
      printf("Finished transferring across first FTP cut\n");
   }
   client_sink(secondary_client_ctxt);
   if(args->verbose){
      printf("Finished secondary client sink\n"); 
   }
   sys_thread_join(t1);
   sys_thread_join(t2);
   
   return 0; 
}


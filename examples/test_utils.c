#include "test_utils.h"
      // need to tell dequeue_and_merge to stitch on the 8th layer
      // of the network. For this, we say that the fused cutoff, 
      // considered the fused_layers parameter, is at the first cutoff, rather than the second. 

#define QUIET_FLAG
#undef QUIET_FLAG

#define MULTI_THREAD_TASKS



void process_everything_in_gateway(void *arg){
   cnn_model* model = (cnn_model*)(((device_ctxt*)(arg))->model);
#ifdef NNPACK
   nnp_initialize();
   model->net->threadpool = pthreadpool_create(THREAD_NUM);
#endif
   int32_t frame_num;
   for(frame_num = 0; frame_num < FRAME_NUM; frame_num ++){
      image_holder img = load_image_as_model_input(model, frame_num);
      forward_all(model, 0);   
      draw_object_boxes(model, frame_num);
      free_image_holder(model, img);
   }
#ifdef NNPACK
   pthreadpool_destroy(model->net->threadpool);
   nnp_deinitialize();
#endif
}



void process_task_single_device(device_ctxt* ctxt, blob* temp, bool is_reuse, int ftp_num){
   
#ifndef QUIET_FLAG
   printf("Task is: %d, frame number is %d\n", get_blob_task_id(temp), get_blob_frame_seq(temp));
#endif
   cnn_model* model = (cnn_model*)(ctxt->model);
   ftp_parameters_reuse* ftp_para_reuse;
    if(ftp_num == 0){
      ftp_para_reuse = model->ftp_para_reuse;
   } else {
      ftp_para_reuse = model->sec_ftp_para_reuse;
   } 
   blob* result;
   set_model_input(model, (float*)temp->data);
   forward_partition(model, get_blob_task_id(temp), is_reuse);  
   result = new_blob_and_copy_data(0, 
                                      get_model_byte_size(model, model->ftp_para->fused_layers-1), 
                                      (uint8_t*)(get_model_output(model, model->ftp_para->fused_layers-1))
                                     );
#if DATA_REUSE
   set_coverage(ftp_para_reuse, get_blob_task_id(temp), get_blob_frame_seq(temp));
   /*send_reuse_data(ctxt, temp);*/
   /*if task doesn't generate any reuse_data*/
   blob* task_input_blob=temp;
   if(ftp_para_reuse->schedule[get_blob_task_id(task_input_blob)] != 1){
#ifndef QUIET_FLAG
      printf("Serialize reuse data for task %d:%d \n", get_blob_cli_id(task_input_blob), get_blob_task_id(task_input_blob)); 
#endif
      blob* serialized_temp  = self_reuse_data_serialization(ctxt, get_blob_task_id(task_input_blob), get_blob_frame_seq(task_input_blob), ftp_num);
      copy_blob_meta(serialized_temp, task_input_blob);
      free_blob(serialized_temp);
   }
#endif
   copy_blob_meta(result, temp);
   enqueue(ctxt->result_queue, result); 
   free_blob(result);
}

void process_task_single_device_jf(void * arg){
   // pointer arg must actually be a struct of type ptsd_args
   ptsd_args * arg_ptr = (ptsd_args*) arg; 
   device_ctxt * ctxt = arg_ptr->ctxt;  
   blob * temp = arg_ptr->temp;
   uint32_t ftp_num = arg_ptr->ftp_num;  
   bool is_reuse = arg_ptr->is_reuse;

#ifndef QUIET_FLAG
   printf("Task is: %d, frame number is %d\n", get_blob_task_id(temp), get_blob_frame_seq(temp));
#endif

   cnn_model* model = (cnn_model*)(ctxt->model);
   ftp_parameters* ftp_para; 
   ftp_parameters_reuse* ftp_para_reuse;
    if(ftp_num == 0){
      ftp_para = model->ftp_para; 
      ftp_para_reuse = model->ftp_para_reuse;
   } else {
      ftp_para = model->sec_ftp_para;
      ftp_para_reuse = model->sec_ftp_para_reuse;
   }
   blob* result;
   set_model_input(model, (float*)temp->data);
  
   if(ftp_num == 0){
      forward_partition(model, get_blob_task_id(temp), is_reuse);  
   } else { 
      forward_second_partition(model, get_blob_task_id(temp), is_reuse, ftp_para->fused_start);  
   }
      result = new_blob_and_copy_data(0, 
                                      get_model_byte_size(model, ftp_para->fused_layers-1), 
                                      (uint8_t*)(get_model_output(model, ftp_para->fused_layers-1))
                                     );


#if DATA_REUSE
   set_coverage(ftp_para_reuse, get_blob_task_id(temp), get_blob_frame_seq(temp));
   
   /*send_reuse_data(ctxt, temp);*/
   /*if task doesn't generate any reuse_data*/
   blob* task_input_blob=temp;
   

   if(ftp_para_reuse->schedule[get_blob_task_id(task_input_blob)] != 1){
#ifndef QUIET_FLAG
      printf("Serialize reuse data for task %d:%d \n", get_blob_cli_id(task_input_blob), get_blob_task_id(task_input_blob)); 
#endif
      blob* serialized_temp  = self_reuse_data_serialization(ctxt, get_blob_task_id(task_input_blob), get_blob_frame_seq(task_input_blob), ftp_num);
      copy_blob_meta(serialized_temp, task_input_blob);
      free_blob(serialized_temp);
   }

#endif
   copy_blob_meta(result, temp);
   enqueue(ctxt->result_queue, result); 
   free_blob(result);
   
    
}

void partition_frame_and_perform_inference_thread_single_device(void *arg){
   device_ctxt* ctxt = (device_ctxt*)arg;
   cnn_model* model = (cnn_model*)(ctxt->model);
#ifdef NNPACK
   nnp_initialize();
   model->net->threadpool = pthreadpool_create(THREAD_NUM);
#endif
   blob* temp;
   uint32_t frame_num;
   /*bool* reuse_data_is_required;*/   
   for(frame_num = 0; frame_num < FRAME_NUM; frame_num ++){
      /*Wait for i/o device input*/
      /*recv_img()*/

      /*Load image and partition, fill task queues*/
      load_image_as_model_input(model, frame_num);
      
      /* the client thread sends a blob in the results queue, informing the transfer_data function
	  that there is information to send. */
      temp = new_empty_blob(0);
      annotate_blob(temp, 0, 1, 0);
      enqueue(ctxt->result_queue, temp);
      free_blob(temp);

      partition_and_enqueue(ctxt, frame_num);
	  /*register_client(ctxt);*/

      /*Dequeue and process task*/
      while(1){
         temp = try_dequeue(ctxt->task_queue);
         if(temp == NULL) break;

         bool data_ready = false;
#ifndef QUIET_FLAG
	 printf("====================Processing task id is %d, data source is %d, frame_seq is %d====================\n", get_blob_task_id(temp), get_blob_cli_id(temp), get_blob_frame_seq(temp));
#endif

     /* TESTING: Make each task a separate process */ 

#if DATA_REUSE
         data_ready = is_reuse_ready(model->ftp_para_reuse, get_blob_task_id(temp), frame_num);
         if((model->ftp_para_reuse->schedule[get_blob_task_id(temp)] == 1) && data_ready) {
            blob* shrinked_temp = new_blob_and_copy_data(get_blob_task_id(temp), 
                       (model->ftp_para_reuse->shrinked_input_size[get_blob_task_id(temp)]),
                       (uint8_t*)(model->ftp_para_reuse->shrinked_input[get_blob_task_id(temp)]));
            copy_blob_meta(shrinked_temp, temp);
            free_blob(temp);
            temp = shrinked_temp;

            /*Assume all reusable data is generated locally*/
            /*
            reuse_data_is_required = check_missing_coverage(model, get_blob_task_id(temp), get_blob_frame_seq(temp));
            request_reuse_data(ctxt, temp, reuse_data_is_required);
            free(reuse_data_is_required);
	    */
         }
#if DEBUG_DEEP_EDGE
         if((model->ftp_para_reuse->schedule[get_blob_task_id(temp)] == 1) && (!data_ready))
            printf("The reuse data is not ready yet!\n");
#endif/*DEBUG_DEEP_EDGE*/

#endif/*DATA_REUSE*/
         /*process_task(ctxt, temp, data_ready);*/

#ifdef MULTI_THREAD_TASKS
         ptsd_args myargs = {ctxt, temp, data_ready, 0}; 

         sys_thread_t t1 = sys_thread_new("process_task_single_device_jf", process_task_single_device_jf, &myargs, 0, 0);

         sys_thread_join(t1);

#else  /* Single-threaded tasks as normal */
         process_task_single_device(ctxt, temp, data_ready, 0);
        
#endif /* MULTI_THREAD_TASKS */

        free_blob(temp);
        
      }
      /*Unregister and prepare for next image*/
      /*cancel_client(ctxt);*/
   }
#ifdef NNPACK
   pthreadpool_destroy(model->net->threadpool);
   nnp_deinitialize();
#endif

   /* Signal to transfer data that there is no more tasks and therefore no more infomration
	* effectively killing transfer_data */
   temp = new_empty_blob(0);
   annotate_blob(temp, 0, -1, 0);
   enqueue(ctxt->result_queue, temp);
   free_blob(temp);

   printf("\nPartitioning and inference thread completed\n");
   
}

void partition_secondary_and_perform_inference_thread_single_device(void *arg){
   device_ctxt* ctxt = (device_ctxt*)arg;
   cnn_model* model = (cnn_model*)(ctxt->model);
   ftp_parameters * ftp_para = model->ftp_para;
#ifdef NNPACK
   nnp_initialize();
   model->net->threadpool = pthreadpool_create(THREAD_NUM);
#endif
   blob* temp;
   blob* temp2;
   uint32_t frame_num;
   
   int32_t cli_id;
   int32_t frame_seq;
   int32_t count = 0;
   for(count = 0; count < FRAME_NUM; count ++){
      frame_num = count;
      temp2 = dequeue_and_merge(ctxt);
      // at this point, the stitching should be correct
      cli_id = get_blob_cli_id(temp2);
      frame_seq = get_blob_frame_seq(temp2);
#if DEBUG_FLAG
      printf("Client %d, frame sequence number %d, all partitions are merged in deepthings_merge_result_thread\n", cli_id, frame_seq);
#endif
      float* fused_output = (float*)(temp2->data);

      printf("some fused output %f %f %f\n", *fused_output, *(fused_output + 32), *(fused_output+100));
      set_model_input(model, fused_output);
      
      // inform transfer_data of information to send 

      
      /* the client thread sends a blob in the results queue, informing the transfer_data function
	  that there is information to send. */
      temp = new_empty_blob(0);
      annotate_blob(temp, 0, 1, 0);
      enqueue(ctxt->result_queue, temp);
      free_blob(temp);

      /* in this thread currently, we're planning on having the task queue
         filled by the special transfer data thread */


      partition_secondary_and_enqueue(ctxt, model->sec_ftp_para->fused_start, 0);
      // all information is copied in the crop feature map section
      free_blob(temp2);
      
      // TODO: partition and enqueue get the cutoff and frame_num from somewhere!!
      //partition_and_enqueue(ctxt, frame_num);
      /*register_client(ctxt);*/

      /*Dequeue and process task*/
      while(1){
         temp = try_dequeue(ctxt->task_queue);
         if(temp == NULL) break;

         bool data_ready = false;
#ifndef QUIET_FLAG
	 printf("====================Processing task id is %d, data source is %d, frame_seq is %d====================\n", get_blob_task_id(temp), get_blob_cli_id(temp), get_blob_frame_seq(temp));
#endif


#if DATA_REUSE
         data_ready = is_reuse_ready(model->sec_ftp_para_reuse, get_blob_task_id(temp), frame_num);
         if((model->sec_ftp_para_reuse->schedule[get_blob_task_id(temp)] == 1) && data_ready) {
            blob* shrinked_temp = new_blob_and_copy_data(get_blob_task_id(temp), 
                       (model->sec_ftp_para_reuse->shrinked_input_size[get_blob_task_id(temp)]),
                       (uint8_t*)(model->sec_ftp_para_reuse->shrinked_input[get_blob_task_id(temp)]));
            copy_blob_meta(shrinked_temp, temp);
            free_blob(temp);
            temp = shrinked_temp;

            /*Assume all reusable data is generated locally*/
            /* 
            reuse_data_is_required = check_missing_coverage(model, get_blob_task_id(temp), get_blob_frame_seq(temp));
            request_reuse_data(ctxt, temp, reuse_data_is_required);
            free(reuse_data_is_required);
            */
         }
#if DEBUG_DEEP_EDGE
         if((model->sec_ftp_para_reuse->schedule[get_blob_task_id(temp)] == 1) && (!data_ready))
            printf("The reuse data is not ready yet!\n");
#endif/*DEBUG_DEEP_EDGE*/

#endif/*DATA_REUSE*/
         /*process_task(ctxt, temp, data_ready);*/

#ifdef MULTI_THREAD_TASKS
         /* TESTING: Make each task a separate process */ 

         ptsd_args myargs = {ctxt, temp, data_ready, 1}; 

         sys_thread_t t1 = sys_thread_new("process_task_single_device_jf", process_task_single_device_jf, &myargs, 0, 0);

         sys_thread_join(t1);

#else  /* Single-threaded tasks as normal */
         process_task_single_device(ctxt, temp, data_ready, 1);
        
#endif /* MULTI_THREAD_TASKS */

        free_blob(temp);
        
      }
      /*Unregister and prepare for next image*/
      /*cancel_client(ctxt);*/
   }
#ifdef NNPACK
   pthreadpool_destroy(model->net->threadpool);
   nnp_deinitialize();
#endif

   /* Signal to transfer data that there is no more tasks and therefore no more infomration
	* effectively killing transfer_data */
   temp = new_empty_blob(0);
   annotate_blob(temp, 0, -1, 0);
   enqueue(ctxt->result_queue, temp);
   free_blob(temp);

   printf("\nPartitioning secondary and inference thread completed\n");
   
}



void deepthings_merge_result_thread_single_device(void *arg){
   cnn_model* model = (cnn_model*)(((device_ctxt*)(arg))->model);
#ifdef NNPACK
   nnp_initialize();
   model->net->threadpool = pthreadpool_create(THREAD_NUM);
#endif
   blob* temp;
   int32_t cli_id;
   int32_t frame_seq;
   int32_t count = 0;
   for(count = 0; count < FRAME_NUM; count ++){
      temp = dequeue_and_merge((device_ctxt*)arg);
      cli_id = get_blob_cli_id(temp);
      frame_seq = get_blob_frame_seq(temp);
#if DEBUG_FLAG
      printf("Client %d, frame sequence number %d, all partitions are merged in deepthings_merge_result_thread\n", cli_id, frame_seq);
#endif
      float* fused_output = (float*)(temp->data);
      // TODO: Remove
      printf("some fused output %f %f %f\n", *fused_output, *(fused_output + 32), *(fused_output+100)); 
      image_holder img = load_image_as_model_input(model, get_blob_frame_seq(temp));
      set_model_input(model, fused_output);
      forward_all(model, model->ftp_para->fused_layers);   
      draw_object_boxes(model, get_blob_frame_seq(temp));
      free_image_holder(model, img);
      free_blob(temp);
#if DEBUG_FLAG
      printf("Client %d, frame sequence number %d, finish processing\n", cli_id, frame_seq);
#endif
   }
#ifdef NNPACK
   pthreadpool_destroy(model->net->threadpool);
   nnp_deinitialize();
#endif
}

void transfer_data(device_ctxt* client, device_ctxt* gateway){
   /* This function is the pipe from the client module to the gateway module.
    * In the test, these two are on the same device, so communication is simulated
    * by taking information from the client's 'result_queue' and copying it to the 
    * 'ready_pool' of the gateway. */

   int32_t cli_id = client->this_cli_id;
   int32_t frame_num;
   
   blob* temp = dequeue(client->result_queue);
#if DEBUG_FLAG
   printf("Checking with client... : Client %d is ready, begin transferring data\n", temp->id);
#endif
   frame_num = get_blob_frame_seq(temp);
   free_blob(temp);

   if(frame_num < 0){
#if DEBUG_FLAG
   	printf("There is not any data to transfer at this time\n");
#endif 
   } else {  
       while(1) {
	      /* obtain one partition from the client */
	      temp = dequeue(client->result_queue);
	      
	      printf("Transferring data from client %d to gateway\n", cli_id);
	      enqueue(gateway->results_pool[cli_id], temp);
	      gateway->results_counter[cli_id]++;
	      frame_num = get_blob_frame_seq(temp);
	      free_blob(temp);

	      if(gateway->results_counter[cli_id] < gateway->batch_size) continue; 
		  
		  /* here, the client has sent a full batch of one image to the gateway.
		   * the transfer data function now resets, informs the gateway it may 
		   * continue via sending a blob to the 'ready_pool', and waits for the 
		   * client to send a message of whether to continue or to exit */

		  /* resetting */
	      gateway->results_counter[cli_id] = 0; 
	      
		  /* informing gateway all information is present */
	      temp = new_empty_blob(cli_id);
	      annotate_blob(temp, cli_id, frame_num, 0);
	      enqueue(gateway->ready_pool, temp);
	      free_blob(temp);

		  /* waiting for the client to signal to continue (via the frame number) */
	      temp = dequeue(client->result_queue);
	      frame_num = get_blob_frame_seq(temp);
	      free_blob(temp);

	      /* determine if the client has an additional task that it plans to send */
		  if(frame_num >= 0){
				continue;
		  } else {
		    /* the client has signaled that it has no more information to transmit
			 * to the gateway. Exit the loop. */
			break;
	      }
	   }
   }

}

/* The function takes a device context as an input but the void is used so it 
   can be spawned as another thread */
void client_sink(void * arg){
   /* This function simulates the transfer_data function for the client, but
    * It actually doesn't pass the information along to anything. this allows 
    * For measurement of just the client performance without a gateway.*/
   device_ctxt* client = (device_ctxt*) arg;
   int32_t frame_num;
   int32_t results_counter = 0; 
   
   blob* temp = dequeue(client->result_queue);
#if DEBUG_FLAG
   printf("Checking with client... : Client %d is ready, begin transferring data\n", temp->id);
#endif
   frame_num = get_blob_frame_seq(temp);
   free_blob(temp);

   if(frame_num < 0){
#if DEBUG_FLAG
   	printf("There is not any data to transfer at this time\n");
#endif 
   } else {  
       while(1) {
	      /* obtain one partition from the client */
	      temp = dequeue(client->result_queue);
#if DEBUG_FLAG  
	      // printf("Sink data from client %d\n", cli_id);
#endif
	      //enqueue(gateway->results_pool[cli_id], temp);
	      //gateway->results_counter[cli_id]++;
	      results_counter++; 
	      frame_num = get_blob_frame_seq(temp);
	      free_blob(temp);

	      /* determine if the client has an additional task that it plans to send */
		  if(frame_num >= 0){
				continue;
		  } else {
		    /* the client has signaled that it has no more information to transmit
			 * to the gateway. Exit the loop. */
			break;
	      }
	   }
   }

}


/* Deprecated */
void transfer_data_continuous(device_ctxt* client, device_ctxt* gateway){
   
   int32_t cli_id = client->this_cli_id;
   int32_t frame_num;
   
   while(1){
      blob* temp = dequeue(client->result_queue);
      printf("Transfering data from client %d to gateway\n", cli_id);
      enqueue(gateway->results_pool[cli_id], temp);
      gateway->results_counter[cli_id]++;
      frame_num = get_blob_frame_seq(temp);
      free_blob(temp);
      if(gateway->results_counter[cli_id] == gateway->batch_size){
         temp = new_empty_blob(cli_id);
	 annotate_blob(temp, cli_id, frame_num, 0);
         enqueue(gateway->ready_pool, temp);
         free_blob(temp);
         gateway->results_counter[cli_id] = 0;
      }
   }

}

void transfer_data_with_number(device_ctxt* client, device_ctxt* gateway, int32_t task_num){
   int32_t cli_id = client->this_cli_id;
   int32_t count = 0;
   for(count = 0; count < task_num; count ++){
      blob* temp = dequeue(client->result_queue);
      printf("Transfering data from client %d to gateway\n", cli_id);
      enqueue(gateway->results_pool[cli_id], temp);
      gateway->results_counter[cli_id]++;
      free_blob(temp);
      if(gateway->results_counter[cli_id] == gateway->batch_size){
         temp = new_empty_blob(cli_id);
         enqueue(gateway->ready_pool, temp);
         free_blob(temp);
         gateway->results_counter[cli_id] = 0;
      }
   }
}


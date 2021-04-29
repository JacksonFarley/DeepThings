#include "partial_network.h"

network *load_partial_network(char *cfg, char *weights, int clear, int start, int cutoff)
{
   char   fn[100];
   // load only the weights that are interesting to you
   network *net = parse_network_cfg(cfg);
   if(weights && weights[0] != 0){
      for(int l = start; l < cutoff; l++){
         if(net->layers[l].type == CONVOLUTIONAL){
            
            sprintf(fn, "layer_weights/l%i.weights", l);
            load_weights_upto(net, fn, l, l+1);
         }
      }
   }
   if(clear) (*net->seen) = 0;
   return net;
}


cnn_model* load_partial_cnn_model(char* cfg, char* weights, int start, int cutoff){
   cnn_model* model = (cnn_model*)malloc(sizeof(cnn_model));
   network *net = load_partial_network(cfg, weights, 0, start, cutoff);
   set_batch_network(net, 1);
   net->truth = 0;
   net->train = 0;
   net->delta = 0;
   srand(2222222);
   model->net = net;
   /*Extract and record network parameters*/
   model->net_para = (network_parameters*)malloc(sizeof(network_parameters));
   model->net_para->layers= net->n;   
   model->net_para->stride = (uint32_t*)malloc(sizeof(uint32_t)*(net->n));
   model->net_para->filter = (uint32_t*)malloc(sizeof(uint32_t)*(net->n));
   model->net_para->type = (uint32_t*)malloc(sizeof(uint32_t)*(net->n));
   model->net_para->input_maps = (tile_region*) malloc(sizeof(tile_region)*(net->n));
   model->net_para->output_maps = (tile_region*) malloc(sizeof(tile_region)*(net->n));
   uint32_t l;
   for(l = 0; l < (net->n); l++){
      model->net_para->stride[l] = net->layers[l].stride;
      model->net_para->filter[l] = net->layers[l].size;
      model->net_para->type[l] = net->layers[l].type;
      model->net_para->input_maps[l].w1 = 0;
      model->net_para->input_maps[l].h1 = 0;
      model->net_para->input_maps[l].w2 = net->layers[l].w - 1;
      model->net_para->input_maps[l].h2 = net->layers[l].h - 1;
      model->net_para->input_maps[l].w = net->layers[l].w;
      model->net_para->input_maps[l].h = net->layers[l].h;
      model->net_para->input_maps[l].c = net->layers[l].c;
//      printf("layer %d has input parameters w1:0 h1:0 w2:%d h2:%d w:%d h:%d c:%d\n",l,
//             net->layers[l].w - 1, net->layers[l].h - 1, net->layers[l].w,
//             net->layers[l].h, net->layers[l].c);
      model->net_para->output_maps[l].w1 = 0;
      model->net_para->output_maps[l].h1 = 0;
      model->net_para->output_maps[l].w2 = net->layers[l].out_w - 1;
      model->net_para->output_maps[l].h2 = net->layers[l].out_h - 1;
      model->net_para->output_maps[l].w = net->layers[l].out_w;
      model->net_para->output_maps[l].h = net->layers[l].out_h;
      model->net_para->output_maps[l].c = net->layers[l].out_c;
//      printf("layer %d has output parameters w1:0 h1:0 w2:%d h2:%d w:%d h:%d c:%d\n",l,
//             net->layers[l].out_w - 1, net->layers[l].out_h - 1, net->layers[l].out_w,
//             net->layers[l].out_h, net->layers[l].out_c);
//      printf("layer %d has layer weights %lu, of size %d and is calculated to be %lu size by %d filters (or %d out channels) by %d channels\n",l,net->layers[l].nweights,sizeof(float),
//             net->layers[l].size, net->layers[l].n, net->layers[l].out_c, net->layers[l].c);  
   }
   return model;
}

/* This is a pre-processing function in order to make the necessary weights */
void make_partial_weight_files(char * cfg, char * weights){

   char   fn[100];
   //FILE*  fh;
   network* net;
   network* new_net;
   /*
   int filterSize;
   int numChannels;
   int numFilters;
   */
   uint32_t numWeights; 
   // setup
   net = parse_network_cfg(cfg);
   load_weights(net, weights);   
  
    new_net = parse_network_cfg(cfg); 
   
   // loop over layers
   for(int l=0; l<net->n; l++)
   {    
      if(net->layers[l].type == CONVOLUTIONAL){

         sprintf(fn, "layer_weights/l%i.weights", l);
         
         save_weights_fromto(net, fn, l, l+1); 
         
         //fh = fopen(fn, "wb");
         // should create a new network in memory
         load_weights_upto(new_net, fn, l, l+1);

         printf("Layer %d: weight start old: %f new: %f\n",l,net->layers[l].weights[0],new_net->layers[l].weights[0]);
         numWeights = net->layers[l].nweights;
         printf("num %d   weight end   old: %f new: %f\n",numWeights,net->layers[l].weights[numWeights-1], new_net->layers[l].weights[numWeights-1]);
         if(memcmp(&net->layers[l].weights[0],&new_net->layers[l].weights[0],net->layers[l].nweights*sizeof(float))!=0){
            printf("Layer %d weights DO NOT match\n",l);
         } else {
            printf("Layer %d weights match\n",l); 
         }
       
         // compare weights
         
         /*   
         numChannels = net->layers[l].c;
         numFilters  = net->layers[l].n;
         filterSize  = net->layers[l].size * net->layers[l].size;
     
         fwrite((void*) &net->layers[l].weights[0], sizeof(float), numChannels*numFilters*filterSize, fh); 

         */

         /* 
         // loop over filters
         for(int filterIdx=0; filterIdx<net->layers[l].n; filterIdx++)
         {
            // loop over channels
            for(int channelIdx=0; channelIdx < numChannels; channelIdx++)
               weightOffset = filterSize * ((filterIdx * numChannels) + channelIdx); 
               
               fprintf(fh, "%f ", net->layers[l].weights[weightOffset]); 

            fprintf(fh, "\n");
         }
         */
         //fclose(fh);
      }
   }
}

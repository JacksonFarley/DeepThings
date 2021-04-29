#ifndef REUSE_DATA_SERIALIZATION
#define REUSE_DATA_SERIALIZATION
#include "ftp.h"
#include "darkiot.h"
#include "inference_engine_helper.h"
#if DATA_REUSE
bool* check_local_coverage(cnn_model* model, uint32_t task_id, uint32_t frame_num, int ftp_num);
bool* check_missing_coverage(cnn_model* model, uint32_t task_id, uint32_t frame_num, int ftp_num);
blob* adjacent_reuse_data_serialization(device_ctxt* ctxt, uint32_t task_id, uint32_t frame_num, bool *reuse_data_is_required, int ftp_num);
overlapped_tile_data** adjacent_reuse_data_deserialization(cnn_model* model, uint32_t task_id, float* input, uint32_t frame_num, bool *reuse_data_is_required, int ftp_num);
void place_adjacent_deserialized_data(cnn_model* model, uint32_t task_id, overlapped_tile_data** regions_and_data_ptr_array, bool *reuse_data_is_required, int ftp_num);
bool need_reuse_data_from_gateway(bool* reuse_data_is_required);

int32_t* get_adjacent_task_id_list(cnn_model* model, uint32_t task_id, int ftp_num);
void print_reuse_data_is_required(bool* reuse_data_is_required);

blob* self_reuse_data_serialization(device_ctxt* ctxt, uint32_t task_id, uint32_t frame_num, int ftp_num);
overlapped_tile_data* self_reuse_data_deserialization(cnn_model* model, uint32_t task_id, float* input, uint32_t frame_num, int ftp_num);
void place_self_deserialized_data(cnn_model* model, uint32_t task_id, overlapped_tile_data* regions_and_data_ptr, int ftp_num);
void free_self_overlapped_tile_data(cnn_model* model,  overlapped_tile_data* tiles, int ftp_num);

void free_overlapped_tile_data_ptr_array(overlapped_tile_data** regions_and_data_ptr_array);
#endif
#endif

#include <vector>
#include <opencv2/opencv.hpp>

#include "gstnvdsmeta.h"
#include "nvdsmeta_schema.h"
#include "gstnvdsinfer.h"
#include "nvdsgstutils.h"
#include "nvbufsurface.h"

#include "common.h"
#include "bodypose_postprocess.h"
#include "msgconv.h"

template <class T>
using Vec1D = std::vector<T>;
template <class T>
using Vec2D = std::vector<Vec1D<T>>;
template <class T>
using Vec3D = std::vector<Vec2D<T>>;

/*Method to parse information returned from the model*/
static BodyKeyPointsCoords* body_landmarks_parse_from_tensor_meta(NvDsInferTensorMeta *tensor_meta);

static void 
build_keypoints_from_output(cv::Mat& heatmap3D_mat, cv::Mat& offset3D_mat);

static void create_body_landmarks_display_meta(Vec2D<float> &object, NvDsFrameMeta *frame_meta, \
		int frame_width, int frame_height, int x_offset, int y_offset);

static void create_body_landmarks_event_meta(NvDsBatchMeta* batch_meta, BodyKeyPointsCoords* body_keypoints_coords, \
    NvDsFrameMeta *frame_meta, int frame_width, int frame_height, int x_offset, int y_offset);

/***************************************************************
 * pgie_src_pad_buffer_probe will extract received metadata 
 * and update params for drawing rectangle, object information etc. 
 *
 ***************************************************************/
GstPadProbeReturn
body_landmarks_src_pad_buffer_probe(GstPad *pad, GstPadProbeInfo *info, gpointer u_data)
{
  gchar *msg = NULL;
  GstBuffer *buf = (GstBuffer *)info->data;
  NvDsMetaList *l_frame = NULL;
  NvDsMetaList *l_obj = NULL;
  NvDsMetaList *l_user = NULL;
  NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta(buf);

  for (l_frame = batch_meta->frame_meta_list; l_frame != NULL; l_frame = l_frame->next) {
    NvDsFrameMeta *frame_meta = (NvDsFrameMeta *)(l_frame->data);
    for (l_user = frame_meta->frame_user_meta_list; l_user != NULL; l_user = l_user->next) {
      NvDsUserMeta *user_meta = (NvDsUserMeta *)l_user->data;
      if (user_meta->base_meta.meta_type == NVDSINFER_TENSOR_OUTPUT_META) {
        NvDsInferTensorMeta *tensor_meta = (NvDsInferTensorMeta *)user_meta->user_meta_data;
        BodyKeyPointsCoords* p_body_landmarks = body_landmarks_parse_from_tensor_meta(tensor_meta);
	// create_body_landmarks_display_meta(object, frame_meta, frame_meta->source_frame_width, frame_meta->source_frame_height, 0, 0);
	create_body_landmarks_event_meta(batch_meta, p_body_landmarks, frame_meta, frame_meta->source_frame_width, \
			frame_meta->source_frame_height, 0, 0);
      }
    }
  }
  return GST_PAD_PROBE_OK;
}

/***************************************************************
 * Method to parse information returned from the model
 * 
 *
 ***************************************************************/
BodyKeyPointsCoords* body_landmarks_parse_from_tensor_meta(NvDsInferTensorMeta *tensor_meta) {
  float *heatMap3D_data = (float *)tensor_meta->out_buf_ptrs_host[2];
  NvDsInferDims &heatMap3D_dims = tensor_meta->output_layers_info[2].inferDims;
  float *offset3D_data =  (float *)tensor_meta->out_buf_ptrs_host[3];
  NvDsInferDims &offset3D_dims = tensor_meta->output_layers_info[3].inferDims;
  
  g_print("heatMap3D_data: %d %d %d ", heatMap3D_dims.d[0], heatMap3D_dims.d[1], heatMap3D_dims.d[2]);
  g_print("offset3D_data : %d %d %d\n", offset3D_dims.d[0], offset3D_dims.d[1], offset3D_dims.d[2]);

  BodyKeyPointsCoords* p_body_landmarks = (BodyKeyPointsCoords*)g_malloc0 (sizeof (BodyKeyPointsCoords));
  // build_keypoints_from_output(heatmap3D_mat, offset3D_mat);
  
  float *heatMap3D_data_c = (float *)heatMap3D_data;
  float *offset3D_data_c =  (float *)offset3D_data;
  
  for (unsigned int key_num = 0; key_num < 24; key_num++){
    float max_data2 = 0;
    int   x = 0 , y = 0, z = 0;
    for (unsigned int i = 0; i < 28; i++) {
      for (unsigned int j = 0; j < heatMap3D_dims.d[1]; j++) {
        for (unsigned int k = 0; k < heatMap3D_dims.d[2]; k++) {
	  if (heatMap3D_data_c[(i + key_num * 28) * heatMap3D_dims.d[1] * heatMap3D_dims.d[2] + j * heatMap3D_dims.d[2] + k] > max_data2){
            max_data2 = heatMap3D_data_c[(i + key_num * 28) * heatMap3D_dims.d[1] * heatMap3D_dims.d[2] + j * heatMap3D_dims.d[2] + k];
	    x = i;// + data3_c[(key_num * 28 + i) * 28 * 28 + j * 28 + k];
	    y = j;// + data3_c[(24 * 28 + key_num * 28 + i) * 28 * 28 + j * 28 + k];
	    z = k;// + data3_c[(2 * 24 * 28 + key_num * 28 + i) * 28 * 28 + j * 28 + k];
	  }
	 // g_print("%.2f ", data2_c[i * data2_dims.d[0] + j * data2_dims.d[1] + k]);
	}
      }
    }
    p_body_landmarks->keypoints[key_num].x = x + offset3D_data_c[(key_num * 28 + x) * 28 * 28 + y * 28 + z];
    p_body_landmarks->keypoints[key_num].y = y + offset3D_data_c[(24 * 28 + key_num * 28 + x) * 28 * 28 + y * 28 + z];
    p_body_landmarks->keypoints[key_num].z = z + offset3D_data_c[(2 * 24 * 28 + key_num * 28 + x) * 28 * 28 + y * 28 + z];
    /* g_print("%d: (%.2f, %.2f, %.2f)\n", key_num, p_body_landmarks->keypoints[key_num].x, p_body_landmarks->keypoints[key_num].y, \
       p_body_landmarks->keypoints[key_num].z);*/
	// g_print("%.2f ", max_data2);
  }

  /*object[i][0] = point_data_c[i * point_data_dims.d[1]];
    object[i][1] = point_data_c[i * point_data_dims.d[1] + 1]; */

  return p_body_landmarks;
}

/***************************************************************
 *  
 *  
 *
 ***************************************************************/
static void
build_keypoints_from_output(cv::Mat& heatmap3D_mat, cv::Mat& offset3D_mat) {
  //Initialize m
  double minVal;
  double maxVal;

#if 0
  for (int i = 0; i < 28; i++)
    for (int j = 0; j < 28; j++)
      for (int k = 0; k < 28; k++) 
        g_print("%f ", heatmap3D_mat.at<float>(i,j,k));
  g_print("\n");
#endif

  /* x = int(x[-1])
     y = int(y[-1])
     z = int(z[-1])
     pos_x = offset3D[j * 28 + x, y, z] + x
     pos_y = offset3D[24 * 28 + j * 28 + x, y, z] + y
     pos_z = offset3D[24 * 28 * 2 + j * 28 + x, y, z] + z */

}

/***************************************************************
 *  MetaData to handle drawing onto the on-screen-display
 * 
 *
 ***************************************************************/
static void
create_body_landmarks_display_meta(Vec2D<float> &object, NvDsFrameMeta *frame_meta, int frame_width, int frame_height, int x_offset, int y_offset) {
}

/***************************************************************
 *
 * 
 *
 ***************************************************************/
static void
create_body_landmarks_event_meta(NvDsBatchMeta *batch_meta, BodyKeyPointsCoords* body_keypoints_coords, \
		NvDsFrameMeta *frame_meta, int frame_width, int frame_height, int x_offset, int y_offset) 
{
  NvDsEventMsgMeta *msg_meta = (NvDsEventMsgMeta *) g_malloc0 (sizeof (NvDsEventMsgMeta));
  
  msg_meta->sensorId = 0;
  msg_meta->placeId  = 0;
  msg_meta->moduleId = 0;
  msg_meta->sensorStr = g_strdup ("sensor-0");

  msg_meta->objType = (NvDsObjectType)TYPE_BODY_KEYPOINTS;

  msg_meta->ts = (gchar *) g_malloc0 (MAX_TIME_STAMP_LEN + 1);
  msg_meta->objectId = (gchar *) g_malloc0 (MAX_LABEL_SIZE);

  strncpy(msg_meta->objectId, "body", MAX_LABEL_SIZE);
  generate_ts_rfc3339(msg_meta->ts, MAX_TIME_STAMP_LEN);

  msg_meta->extMsg = body_keypoints_coords;//
  msg_meta->extMsgSize = sizeof(BodyKeyPointsCoords);//

  NvDsUserMeta *user_event_meta = nvds_acquire_user_meta_from_pool (batch_meta);
  if (user_event_meta) {
    user_event_meta->user_meta_data = (void *) msg_meta;
    user_event_meta->base_meta.meta_type = NVDS_EVENT_MSG_META;
    user_event_meta->base_meta.copy_func = (NvDsMetaCopyFunc) meta_copy_func;
    user_event_meta->base_meta.release_func = (NvDsMetaReleaseFunc) meta_free_func;
    nvds_add_user_meta_to_frame(frame_meta, user_event_meta);
  } else {
    g_print ("Error in attaching event meta to buffer\n");
  }
}


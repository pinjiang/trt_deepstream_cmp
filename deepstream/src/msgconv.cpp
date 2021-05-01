#include <glib.h>
#include <gst/gst.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/timeb.h>

#include "gstnvdsmeta.h"
#include "nvdsmeta_schema.h"

#include "common.h"
#include "body_struct.h"

#define MAX_TIME_STAMP_LEN 32
#define PGIE_CLASS_ID_FACE 0

/***************************************************************
 *
 *
 *
 ***************************************************************/
static void
generate_face_meta (gpointer data, NvDsObjectMeta * obj_meta) {
  NvDsFaceObject *obj = (NvDsFaceObject *) data;
	NvDsMetaList *l_user = NULL;

  g_print("In generate face meta\n");
	
	for (l_user = obj_meta->obj_user_meta_list; l_user != NULL; l_user = l_user->next) {
	  NvDsUserMeta *user_meta = (NvDsUserMeta *)l_user->data;
		g_print("Exist obj-user-meta\n");
    if (user_meta->base_meta.meta_type == NVDSINFER_TENSOR_OUTPUT_META) {
		  g_print("Exist obj-user-meta tensor output\n");
      // NvDsInferTensorMeta *tensor_meta = (NvDsInferTensorMeta *)user_meta->user_meta_data;
    }
  }

  obj->gender   = g_strdup ("male");
  obj->age      = 25;
  obj->hair     = g_strdup ("black");
  obj->cap      = g_strdup ("no");
  obj->glasses  = g_strdup ("no");
  obj->eyecolor = g_strdup ("black");
}

/***************************************************************
 *
 *
 *
 ***************************************************************/

#if 0
static void 
generate_body_meta (gpointer data) {
  NvDsPersonObject *obj = (NvDsPersonObject *) data;
  obj->age = 45;
  obj->cap = g_strdup ("none");
  obj->hair = g_strdup ("black");
  obj->gender = g_strdup ("male");
  obj->apparel= g_strdup ("formal");
}
#endif

/***************************************************************
 *
 *
 *
 ***************************************************************/
gpointer meta_copy_func (gpointer data, gpointer user_data) {
  NvDsUserMeta *user_meta = (NvDsUserMeta *) data;
  NvDsEventMsgMeta *srcMeta = (NvDsEventMsgMeta *) user_meta->user_meta_data;
  NvDsEventMsgMeta *dstMeta = NULL;

  dstMeta = (NvDsEventMsgMeta *)g_memdup(srcMeta, sizeof(NvDsEventMsgMeta));

  if (srcMeta->ts)
    dstMeta->ts = g_strdup (srcMeta->ts);

  if (srcMeta->sensorStr)
    dstMeta->sensorStr = g_strdup (srcMeta->sensorStr);

  if (srcMeta->objSignature.size > 0) {
    dstMeta->objSignature.signature = (gdouble *)g_memdup(srcMeta->objSignature.signature,
                                                srcMeta->objSignature.size);
    dstMeta->objSignature.size = srcMeta->objSignature.size;
  }

  if(srcMeta->objectId) {
    dstMeta->objectId = g_strdup (srcMeta->objectId);
  }

  dstMeta->objType = srcMeta->objType;

  if (srcMeta->extMsgSize > 0 ) {

    if (srcMeta->objType == (NvDsObjectType)TYPE_BODY_KEYPOINTS ) {
      BodyKeyPointsCoords *srcObj = (BodyKeyPointsCoords *) srcMeta->extMsg;
      BodyKeyPointsCoords *obj = (BodyKeyPointsCoords *) g_malloc0 (sizeof (BodyKeyPointsCoords));
      memcpy(obj, srcObj, sizeof(BodyKeyPointsCoords));
      dstMeta->extMsg = obj;
      dstMeta->extMsgSize = sizeof(BodyKeyPointsCoords);
    }
  }
  return dstMeta;
}

/***************************************************************
 *
 *
 *
 ***************************************************************/
void meta_free_func (gpointer data, gpointer user_data) {
  NvDsUserMeta *user_meta = (NvDsUserMeta *) data;
  NvDsEventMsgMeta *srcMeta = (NvDsEventMsgMeta *) user_meta->user_meta_data;

  g_free (srcMeta->ts);
  g_free (srcMeta->sensorStr);

  if (srcMeta->objSignature.size > 0) {
    g_free (srcMeta->objSignature.signature);
    srcMeta->objSignature.size = 0;
  }

  if(srcMeta->objectId) {
    g_free (srcMeta->objectId);
  }

  if (srcMeta->extMsgSize > 0) {
    g_free (srcMeta->extMsg);
    srcMeta->extMsgSize = 0;
  }
  g_free (user_meta->user_meta_data);
  user_meta->user_meta_data = NULL;
}

/***************************************************************
 *
 *
 *
 ***************************************************************/
void generate_ts_rfc3339 (char *buf, int buf_size) {
  time_t tloc;
  struct tm tm_log;
  struct timespec ts;
  char strmsec[6]; //.nnnZ\0

  clock_gettime(CLOCK_REALTIME,  &ts);
  memcpy(&tloc, (void *)(&ts.tv_sec), sizeof(time_t));
  gmtime_r(&tloc, &tm_log);
  strftime(buf, buf_size,"%Y-%m-%dT%H:%M:%S", &tm_log);
  int ms = ts.tv_nsec/1000000;
  g_snprintf(strmsec, sizeof(strmsec),".%.3dZ", ms);
  strncat(buf, strmsec, buf_size);
}

/***************************************************************
 *
 *
 *
 ***************************************************************/
void generate_event_msg_meta (gpointer data, gint class_id, NvDsObjectMeta * obj_params) {
  NvDsEventMsgMeta *meta = (NvDsEventMsgMeta *) data;
  meta->sensorId = 0;
  meta->placeId  = 0;
  meta->moduleId = 0;
  meta->sensorStr = g_strdup ("sensor-0");

  meta->ts = (gchar *) g_malloc0 (MAX_TIME_STAMP_LEN + 1);
  meta->objectId = (gchar *) g_malloc0 (MAX_LABEL_SIZE);

  strncpy(meta->objectId, obj_params->obj_label, MAX_LABEL_SIZE);
  generate_ts_rfc3339(meta->ts, MAX_TIME_STAMP_LEN);

  /*
   * This demonstrates how to attach custom objects.
   * Any custom object as per requirement can be generated and attached
   * like NvDsVehicleObject / NvDsPersonObject. Then that object should
   * be handled in payload generator library (nvmsgconv.cpp) accordingly.
   */
  if (class_id == PGIE_CLASS_ID_FACE) {
    meta->objType    = NVDS_OBJECT_TYPE_FACE; // NVDS_OBJECT_TYPE_FACE;
    meta->objClassId = PGIE_CLASS_ID_FACE;    // NVDS_OBJECT_TYPE_FACE;

    NvDsFaceObject *obj = (NvDsFaceObject *) g_malloc0 (sizeof (NvDsFaceObject));
    generate_face_meta (obj, obj_params);

    meta->extMsg = obj;
    meta->extMsgSize = sizeof (NvDsFaceObject);
  }
}

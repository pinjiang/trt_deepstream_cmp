#include <glib.h>
#include "gstnvdsmeta.h"
#include "nvdsmeta_schema.h"
#include "nvdsgstutils.h"
#include "nvbufsurface.h"

#include "deepstream_perf.h"
#include "perf_measurement.h"

/**
 * callback function to print the performance numbers of each stream.
*/
void perf_cb(gpointer context, NvDsAppPerfStruct *str) {
  PerfCtx *thCtx = (PerfCtx *) context;

  g_mutex_lock(&thCtx->fps_lock);
  /** str->num_instances is == num_sources */
  guint32 numf = str->num_instances;
  guint32 i;

  for (i = 0; i < numf; i++) {
    thCtx->fps[i] = str->fps[i];
    thCtx->fps_avg[i] = str->fps_avg[i];
  }
  thCtx->context = thCtx;
  g_print("**PERF: ");
  for (i = 0; i < numf; i++) {
    g_print("%.2f (%.2f)\t", thCtx->fps[i], thCtx->fps_avg[i]);
  }
  g_print("\n");
  g_mutex_unlock(&thCtx->fps_lock);
}

/**
 * callback function to print the latency of each component in the pipeline.
 */
GstPadProbeReturn
latency_measurement_buf_prob(GstPad *pad, GstPadProbeInfo *info, gpointer u_data) {
  LatencyCtx *ctx = (LatencyCtx *) u_data;
  static int batch_num = 0;
  guint i = 0, num_sources_in_batch = 0;
  if (nvds_enable_latency_measurement) {
    GstBuffer *buf = (GstBuffer *) info->data;
    NvDsFrameLatencyInfo *latency_info = NULL;
    g_mutex_lock(ctx->lock);
    latency_info = (NvDsFrameLatencyInfo *)
        calloc(1, ctx->num_sources * sizeof(NvDsFrameLatencyInfo));;
    g_print("\n************BATCH-NUM = %d**************\n", batch_num);
    num_sources_in_batch = nvds_measure_buffer_latency(buf, latency_info);

    for (i = 0; i < num_sources_in_batch; i++) {
      g_print("Source id = %d Frame_num = %d Frame latency = %lf (ms) \n",
              latency_info[i].source_id,
              latency_info[i].frame_num,
              latency_info[i].latency);
    }
    g_mutex_unlock(ctx->lock);
    batch_num++;
  }

  return GST_PAD_PROBE_OK;
}

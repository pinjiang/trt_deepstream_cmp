#ifndef  PERF_MEASUREMENT_H
#define  PERF_MEASUREMENT_H

#define MAX_STREAMS 64

typedef struct {
  /** identifies the stream ID */
  guint32 stream_index;
  gdouble fps[MAX_STREAMS];
  gdouble fps_avg[MAX_STREAMS];
  guint32 num_instances;
  guint header_print_cnt;
  GMutex fps_lock;
  gpointer context;

  /** Test specific info */
  guint32 set_batch_size;
} PerfCtx;

typedef struct {
  GMutex *lock;
  int num_sources;
} LatencyCtx;

void perf_cb(gpointer context, NvDsAppPerfStruct *str);

GstPadProbeReturn latency_measurement_buf_prob(GstPad *pad, GstPadProbeInfo *info, gpointer u_data);

#endif

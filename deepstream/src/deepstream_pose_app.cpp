// Copyright 2020 - NVIDIA Corporation
// SPDX-License-Identifier: MIT

#include <gst/gst.h>
#include <glib.h>
#include <stdio.h>

#include "gstnvdsmeta.h"
#include "nvdsmeta_schema.h"
#include "deepstream_perf.h"
#include "perf_measurement.h"
#include "nvdsgstutils.h"
#include "nvbufsurface.h"
#include "bodypose_postprocess.h"
#include "msgconv.h"

#include <vector>
#include <array>
#include <queue>
#include <cmath>
#include <string>

#define EPS 1e-6
#define MAX_DISPLAY_LEN 64

#define MSCONV_CONFIG_FILE "./config/msgconv_config.txt"

/* Muxer batch formation timeout, for e.g. 40 millisec. Should ideally be set
 * based on the fastest source's framerate. */
#define MUXER_BATCH_TIMEOUT_USEC 4000000
#define MAX_STREAMS 64

/* Structure to contain all our information, so we can pass it to callbacks */
typedef struct _CustomData {
  GstElement *pipeline;
  GstElement *source;
  GstElement *decoder;
  GstElement *streammux;
  gint muxer_output_width;
  gint muxer_output_height;
} CustomData;

/* The muxer output resolution must be set if the input streams will be of
 * different resolution. The muxer will scale all the input frames to this
 * resolution. */
gint MUXER_OUTPUT_WIDTH  = 1920;
gint MUXER_OUTPUT_HEIGHT = 1080;

static gchar *input_file  = NULL;
static gchar *output_file = NULL;
static gboolean msg_broker_enb = FALSE;
static gchar *cfg_file  = NULL;
static gchar *topic     = NULL;
static gchar *conn_str  = NULL;
static gchar *proto_lib = NULL;
static gchar *msg2p_lib = FALSE;
static gint schema_type = NVDS_PAYLOAD_CUSTOM;

CustomData data;

gint frame_number = 0;

GOptionEntry entries[] = {
  {"input-file", 'i', 0, G_OPTION_ARG_FILENAME, &input_file,
   "Set the input MP4 file", NULL},
  {"output-file", 'o', 0, G_OPTION_ARG_FILENAME, &output_file,
   "Set the input MP4 file", NULL},
  {"enable-msg-broker", 'b', 0, G_OPTION_ARG_NONE, &msg_broker_enb,
   "Message broker enabled", NULL},
  {"cfg-file", 'c', 0, G_OPTION_ARG_FILENAME, &cfg_file,
   "Set the adaptor config file. Optional if connection string has relevant details.", NULL},
  {"topic", 't', 0, G_OPTION_ARG_STRING, &topic,
   "Name of message topic. Optional if it is part of connection string or config file.", NULL},
  {"conn-str", 0, 0, G_OPTION_ARG_STRING, &conn_str,
   "Connection string of backend server. Optional if it is part of config file.", NULL},
  {"proto-lib", 'p', 0, G_OPTION_ARG_STRING, &proto_lib,
   "Absolute path of adaptor library", NULL},
  {"msg2p-lib", 'm', 0, G_OPTION_ARG_STRING, &msg2p_lib,
   "Absolute path of adaptor library", NULL},
  {"schema", 's', NVDS_PAYLOAD_CUSTOM, G_OPTION_ARG_INT, &schema_type,
   "Type of message schema (0=Full, 1=minimal), default=1", NULL},
  {NULL}
};

/* Handler for the pad-added signal */
static void pad_added_handler (GstElement *src, GstPad *pad, CustomData *data);

/* osd_sink_pad_buffer_probe  will extract metadata received from OSD
 * and update params for drawing rectangle, object information etc. */
static GstPadProbeReturn
osd_sink_pad_buffer_probe(GstPad *pad, GstPadProbeInfo *info,
                          gpointer u_data)
{
  GstBuffer *buf = (GstBuffer *)info->data;
  guint num_rects = 0;
  NvDsObjectMeta *obj_meta = NULL;
  NvDsMetaList *l_frame = NULL;
  NvDsMetaList *l_obj = NULL;
  NvDsMetaList *l_user = NULL;
  NvDsDisplayMeta *display_meta = NULL;

  NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta(buf);

  for (l_frame = batch_meta->frame_meta_list; l_frame != NULL;
       l_frame = l_frame->next)
  {
    NvDsFrameMeta *frame_meta = (NvDsFrameMeta *)(l_frame->data);
    int offset = 0;
    for (l_obj = frame_meta->obj_meta_list; l_obj != NULL; l_obj = l_obj->next) {
      obj_meta = (NvDsObjectMeta *)(l_obj->data);
      NvOSD_RectParams *rect_obj = &obj_meta->rect_params;
      NvOSD_TextParams *text_obj = &obj_meta->text_params;
      //text_obj->x_offset = 20 + obj_meta->detector_bbox_info.org_bbox_coords.left;
      //text_obj->y_offset = 50 + obj_meta->detector_bbox_info.org_bbox_coords.top;
      //text_obj->display_text = (char *)g_malloc0(MAX_DISPLAY_LEN);
      //offset = snprintf(text_obj->display_text, MAX_DISPLAY_LEN, "");
      //text_obj->set_bg_clr = FALSE;
      for (l_user = obj_meta->obj_user_meta_list; l_user != NULL; l_user = l_user->next) {
        NvDsUserMeta *user_meta = (NvDsUserMeta *)l_user->data;
        // g_print("l_user in obj\n");
      }
    }
    for (l_user = frame_meta->frame_user_meta_list; l_user != NULL; l_user = l_user->next) {
      NvDsUserMeta *user_meta = (NvDsUserMeta *) (l_user->data);
      // g_print("l_user in frame\n");
    }

    display_meta = nvds_acquire_display_meta_from_pool(batch_meta);

    /* Parameters to draw text onto the On-Screen-Display */
    NvOSD_TextParams *txt_params = &display_meta->text_params[0];
    display_meta->num_labels = 1;
    txt_params->display_text = (char *)g_malloc0(MAX_DISPLAY_LEN);
    offset = snprintf(txt_params->display_text, MAX_DISPLAY_LEN, "Frame Number =  %d", frame_number);
    offset = snprintf(txt_params->display_text + offset, MAX_DISPLAY_LEN, "");

    txt_params->x_offset = 10;
    txt_params->y_offset = 12;

    txt_params->font_params.font_name = "Mono";
    txt_params->font_params.font_size = 10;
    txt_params->font_params.font_color.red = 1.0;
    txt_params->font_params.font_color.green = 1.0;
    txt_params->font_params.font_color.blue = 1.0;
    txt_params->font_params.font_color.alpha = 1.0;

    txt_params->set_bg_clr = 1;
    txt_params->text_bg_clr.red = 0.0;
    txt_params->text_bg_clr.green = 0.0;
    txt_params->text_bg_clr.blue = 0.0;
    txt_params->text_bg_clr.alpha = 1.0;

    nvds_add_display_meta_to_frame(frame_meta, display_meta);
  }
  frame_number++;
  return GST_PAD_PROBE_OK;
}

/***************************************************************
 * The function used to handle the signal
 *
 *
 ***************************************************************/
void handle_SIGINT(int sig) {
  //gst_element_set_state (data.pipeline, GST_STATE_NULL);
  gst_element_send_event(data.pipeline, gst_event_new_eos ());
}

/***************************************************************
 * 
 *
 *
 ***************************************************************/
static gboolean
bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
  GMainLoop *loop = (GMainLoop *)data;
  switch (GST_MESSAGE_TYPE(msg))
  {
  case GST_MESSAGE_EOS:
    g_print("End of Stream\n");
    g_main_loop_quit(loop);
    break;

  case GST_MESSAGE_ERROR:
  {
    gchar *debug;
    GError *error;
    gst_message_parse_error(msg, &error, &debug);
    g_printerr("ERROR from element %s: %s\n",
               GST_OBJECT_NAME(msg->src), error->message);
    if (debug)
      g_printerr("Error details: %s\n", debug);
    g_free(debug);
    g_error_free(error);
    g_main_loop_quit(loop);
    break;
  }

  default:
    break;
  }
  return TRUE;
}

/***************************************************************
 * Functions below print the Capabilities in a human-friendly format
 *
 *
 ***************************************************************/
static gboolean print_field (GQuark field, const GValue * value, gpointer pfx) {
  gchar *str = gst_value_serialize (value);

  g_print ("%s  %15s: %s\n", (gchar *) pfx, g_quark_to_string (field), str);
  g_free (str);
  return TRUE;
}

/***************************************************************
 * This function will be called by the pad-added signal
 *
 *
 ***************************************************************/
static void pad_added_handler (GstElement *src, GstPad *new_pad, CustomData *data) {
  GstPad *sinkpad = NULL;
  gchar pad_name_sink[16] = "sink_0";
  GstCaps *new_pad_caps = NULL;
  GstStructure *new_pad_struct = NULL;
  const gchar *new_pad_type = NULL;
  const gchar * pfx = "      "; 

  g_print ("Received new pad '%s' from '%s':\n", GST_PAD_NAME (new_pad), GST_ELEMENT_NAME (src));

  /* Check the new pad's type */
  new_pad_caps = gst_pad_get_current_caps (new_pad);
  new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);

  // g_print ("%s%s\n", pfx, gst_structure_get_name (new_pad_struct));
  // gst_structure_foreach (new_pad_struct, print_field, (gpointer) pfx);

  new_pad_type = gst_structure_get_name (new_pad_struct);
  if (!g_str_has_prefix (new_pad_type, "video/x-raw")) {
    g_print ("It has type '%s' which is not raw video. Ignoring.\n", new_pad_type);
    goto exit;
  } else {
    gchar *width = gst_value_serialize (gst_structure_get_value(new_pad_struct, "width"));
    gchar *height = gst_value_serialize (gst_structure_get_value(new_pad_struct, "height"));
    MUXER_OUTPUT_WIDTH = g_ascii_strtoll(width, NULL, 0);
    MUXER_OUTPUT_HEIGHT = g_ascii_strtoll(height, NULL, 0);
    g_object_set(G_OBJECT(data->streammux), "width", MUXER_OUTPUT_WIDTH, "height", MUXER_OUTPUT_HEIGHT, \
		    "batch-size", 1, "batched-push-timeout", MUXER_BATCH_TIMEOUT_USEC, NULL);

    g_free(width);
    g_free(height);
  }

  sinkpad = gst_element_get_request_pad(data->streammux, pad_name_sink);
  if (!sinkpad) {
    g_printerr("Streammux request sink pad failed. Exiting.\n");
    goto exit;
  }

  if (gst_pad_link(new_pad, sinkpad) != GST_PAD_LINK_OK) {
    g_printerr("Failed to link decoder to stream muxer. Exiting.\n");
    goto exit;
  }

exit:
  /* Unreference the new pad's caps, if we got them */
  if (new_pad_caps != NULL)
    gst_caps_unref (new_pad_caps);

  /* Unreference the sink pad */
  if ( sinkpad != NULL)
    gst_object_unref (sinkpad);

}

/***************************************************************
 * 
 *
 *
 ***************************************************************/
gboolean
link_element_to_tee_src_pad(GstElement *tee, GstElement *sinkelem)
{
  gboolean ret = FALSE;
  GstPad *tee_src_pad = NULL;
  GstPad *sinkpad = NULL;
  GstPadTemplate *padtemplate = NULL;

  padtemplate = (GstPadTemplate *)gst_element_class_get_pad_template(GST_ELEMENT_GET_CLASS(tee), "src_%u");
  tee_src_pad = gst_element_request_pad(tee, padtemplate, NULL, NULL);

  if (!tee_src_pad) {
    g_printerr("Failed to get src pad from tee");
    goto done;
  }

  sinkpad = gst_element_get_static_pad(sinkelem, "sink");
  if (!sinkpad) {
    g_printerr("Failed to get sink pad from '%s'",
               GST_ELEMENT_NAME(sinkelem));
    goto done;
  }

  if (gst_pad_link(tee_src_pad, sinkpad) != GST_PAD_LINK_OK) {
    g_printerr("Failed to link '%s' and '%s'", GST_ELEMENT_NAME(tee),
               GST_ELEMENT_NAME(sinkelem));
    goto done;
  }
  ret = TRUE;

done:
  if (tee_src_pad) {
    gst_object_unref(tee_src_pad);
  }
  if (sinkpad)  {
    gst_object_unref(sinkpad);
  }
  return ret;
}

int main(int argc, char *argv[])
{
  GError *error = NULL;
  GOptionContext *ctx = NULL;
  GOptionGroup *group = NULL;
  GMainLoop *loop = NULL;
  GstCaps *caps = NULL;
  GstElement *pipeline = NULL, *source = NULL, *decoder = NULL, *streammux = NULL, *sink = NULL, *pgie = NULL, \
	  *nvvidconv = NULL, *nvosd = NULL, *nvvideoconvert = NULL, *queue4 = NULL, *msgconv = NULL, *msgbroker = NULL, *fakesink = NULL, \
	  *tee = NULL, *h264encoder = NULL, *cap_filter = NULL, *filesink = NULL, *queue3 = NULL, \
	  *qtmux = NULL, *h264parser = NULL, *nvsink = NULL;

  // CustomData data;

   /* Add a transform element for Jetson*/
#ifdef PLATFORM_TEGRA
  GstElement *transform = NULL;
#endif
  GstBus *bus = NULL;
  guint bus_watch_id;
  GstPad *osd_sink_pad = NULL;

  ctx = g_option_context_new ("Deep Pose Engine");
  group = g_option_group_new ("test4", NULL, NULL, NULL, NULL);
  g_option_group_add_entries (group, entries);

  g_option_context_set_main_group (ctx, group);
  g_option_context_add_group (ctx, gst_init_get_option_group ());

  if (!g_option_context_parse (ctx, &argc, &argv, &error)) {
    g_option_context_free (ctx);
    g_printerr ("%s", error->message);
    return -1;
  }
  g_option_context_free (ctx);

  if (!proto_lib || !input_file || !output_file ) {
    g_printerr("missing arguments\n");
    g_printerr ("Usage: %s -i <filename> -o <filename> -p <Proto adaptor library> --conn-str=<Connection string>\n", argv[0]);
    return -1;
  }

  //Setup the signal handler
  struct sigaction handler;
  handler.sa_handler = handle_SIGINT;
  sigaction(SIGINT, &handler, NULL);

  /* Standard GStreamer initialization */
  gst_init(NULL, NULL);
  loop = g_main_loop_new(NULL, FALSE);

  /* Create gstreamer elements */
  /* Create Pipeline element that will form a connection of other elements */
  pipeline = gst_pipeline_new("deeppose-pipeline");

  /* Source element for reading from the file */
  source = gst_element_factory_make("uridecodebin", "source");

  /* Since the data format in the input file is elementary h264 stream,
   * we need a h264parser */
  h264parser = gst_element_factory_make("h264parse", "h264-parser");

  /* Use nvdec_h264 for hardware accelerated decode on GPU */
  decoder = gst_element_factory_make("decodebin", "nvv4l2-decoder");

  /* Create nvstreammux instance to form batches from one or more sources. */
  streammux = gst_element_factory_make("nvstreammux", "stream-muxer");

  if (!pipeline || !streammux)
  {
    g_printerr("One element could not be created. Exiting.\n");
    return -1;
  }

  /* Use nvinfer to run inferencing on decoder's output,
   * behaviour of inferencing is set through config file */
  pgie = gst_element_factory_make("nvinfer", "primary-nvinference-engine");

  /* Use convertor to convert from NV12 to RGBA as required by nvosd */
  nvvidconv = gst_element_factory_make("nvvideoconvert", "nvvideo-converter");

  GstPad *sink_pad = gst_element_get_static_pad(nvvidconv, "src");
  if (!sink_pad)
    g_print("Unable to get sink pad\n");
  else {
    LatencyCtx *ctx = (LatencyCtx *) g_malloc0(sizeof(LatencyCtx));
    ctx->lock = (GMutex *) g_malloc0(sizeof(GMutex));
    ctx->num_sources = 1;
    gst_pad_add_probe(sink_pad, GST_PAD_PROBE_TYPE_BUFFER,
                      latency_measurement_buf_prob, ctx, NULL);
  }
  gst_object_unref(sink_pad);
  GstPad *conv_pad = gst_element_get_static_pad(nvvidconv, "sink");
  if (!conv_pad)
    g_print("Unable to get conv_pad pad\n");
  else {
    NvDsAppPerfStructInt *str = (NvDsAppPerfStructInt *) g_malloc0(sizeof(NvDsAppPerfStructInt));
    PerfCtx *perf_ctx = (PerfCtx *) g_malloc0(sizeof(PerfCtx));
    g_mutex_init(&perf_ctx->fps_lock);
    str->context = perf_ctx;
    enable_perf_measurement(str, conv_pad, 1, 1, 0, perf_cb);
  }
  gst_object_unref(conv_pad);

  queue3 = gst_element_factory_make("queue", "queue3");
  filesink = gst_element_factory_make("filesink", "filesink");

  data.pipeline = pipeline;
  data.decoder = decoder;
  data.streammux = streammux;
  
  /* Set output file location */
  g_object_set(G_OBJECT(filesink), "location", output_file, NULL);
  
  nvvideoconvert = gst_element_factory_make("nvvideoconvert", "nvvideo-converter1");
  tee = gst_element_factory_make("tee", "TEE");
  h264encoder = gst_element_factory_make("nvv4l2h264enc", "video-encoder");
  cap_filter = gst_element_factory_make("capsfilter", "enc_caps_filter");
  caps = gst_caps_from_string("video/x-raw(memory:NVMM), format=I420");
  g_object_set(G_OBJECT(cap_filter), "caps", caps, NULL);
  qtmux = gst_element_factory_make("qtmux", "muxer");

  /* Create OSD to draw on the converted RGBA buffer */
  nvosd = gst_element_factory_make("nvdsosd", "nv-onscreendisplay");

  queue4 = gst_element_factory_make("queue", "queue4");

  /* Create msg converter to generate payload from buffer metadata */
  msgconv = gst_element_factory_make ("nvmsgconv", "nvmsg-converter");

  /* Create msg broker to send payload to server */
  if( msg_broker_enb ) {
    msgbroker = gst_element_factory_make ("nvmsgbroker", "nvmsg-broker");
  } else {
    msgbroker = gst_element_factory_make ("fakesink", "fakesink");
  }

  /* Finally render the osd output */
#ifdef PLATFORM_TEGRA
  transform = gst_element_factory_make("nvegltransform", "nvegl-transform");
#endif
  nvsink = gst_element_factory_make("nveglglessink", "nvvideo-renderer");
  // nvsink = gst_element_factory_make("fakesink", "nvvideo-renderer");
  sink = gst_element_factory_make("fpsdisplaysink", "fps-display");

  g_object_set(G_OBJECT(sink), "text-overlay", FALSE, "video-sink", nvsink, "sync", FALSE, NULL);

  if (!source || !decoder || !pgie || !nvvidconv || !nvosd || !queue4 || !msgconv \
              || !msgbroker || !sink || !cap_filter || !tee || !nvvideoconvert || \
      !h264encoder || !filesink || !queue3 || !qtmux || !h264parser )
  {
    g_printerr("One element could not be created. Exiting.\n");
    return -1;
  }
#ifdef PLATFORM_TEGRA
  if (!transform)
  {
    g_printerr("One tegra element could not be created. Exiting.\n");
    return -1;
  }
#endif

  /* we set the input filename to the source element */
  g_object_set(G_OBJECT(source), "uri", input_file, NULL);

  g_object_set(G_OBJECT(streammux), "width", MUXER_OUTPUT_WIDTH, "height",
               MUXER_OUTPUT_HEIGHT, "batch-size", 1,
               "batched-push-timeout", MUXER_BATCH_TIMEOUT_USEC, NULL);

  /* Set all the necessary properties of the nvinfer element,
   * the necessary ones are : */
  g_object_set(G_OBJECT(pgie), \
          "config-file-path", "./config/pgie_config.txt", NULL);

  g_object_set (G_OBJECT(msgconv), "config", MSCONV_CONFIG_FILE, \
	  "payload-type", schema_type, NULL);

  if ( msg2p_lib ) {
    g_object_set (G_OBJECT(msgconv), "msg2p_lib", msg2p_lib, NULL);
  }

  if( msg_broker_enb ) {
    g_object_set (G_OBJECT(msgbroker), "proto-lib", proto_lib, \
		    "conn-str", conn_str, "sync", FALSE, NULL);

    if (topic) {
      g_object_set (G_OBJECT(msgbroker), "topic", topic, NULL);
    }
    if (cfg_file) {
      g_object_set (G_OBJECT(msgbroker), "config", cfg_file, NULL);
    }
  }

  /* Set necessary properties of the tracker element. */
  //if (!set_tracker_properties(nvtracker)) {
    //g_printerr ("Failed to set tracker properties. Exiting.\n");
    //return -1;
  //}

  /* we add a message handler */
  bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
  bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
  gst_object_unref(bus);

    /* Set up the pipeline */
  /* we add all elements into the pipeline */
#ifdef PLATFORM_TEGRA
  gst_bin_add_many(GST_BIN(pipeline),
                   source, streammux, pgie, nvvidconv, nvosd, transform, tee, queue4, msgconv, msgbroker, /*sink,*/
                   nvvideoconvert, h264encoder, cap_filter, filesink, queue3, h264parser, qtmux, NULL);
#else
  gst_bin_add_many(GST_BIN(pipeline),
                   source, streammux, pgie, nvvidconv, nvosd, tee, queue4, msgconv, msgbroker, /* sink,*/
                   nvvideoconvert, h264encoder, cap_filter, filesink, queue3, h264parser, qtmux, NULL);
#endif

  if (!link_element_to_tee_src_pad(tee, queue3))
  {
    g_printerr("Could not link tee to nvvideoconvert\n");
    return -1;
  }
  if (!gst_element_link_many(queue3, nvvideoconvert, cap_filter, h264encoder,
                             h264parser, qtmux, filesink, NULL))
  {
    g_printerr("Elements could not be linked\n");
    return -1;
  }

#ifdef PLATFORM_TEGRA
  if (!gst_element_link_many(streammux, pgie, nvvidconv, tee, queue4, msgconv, msgbroker, NULL))
  {
    g_printerr("Elements could not be linked: 2. Exiting.\n");
    return -1;
  }
#else
  if ( !gst_element_link_many(streammux, pgie, nvvidconv, nvosd, tee, queue4, msgconv, msgbroker, NULL) ) {
    g_printerr("Elements could not be linked: 2. Exiting.\n");
    return -1;
  }
#endif

  /* Connect to the pad-added signal */
  g_signal_connect(source, "pad-added", G_CALLBACK (pad_added_handler), &data);

  /* GstPad *sgie_src_pad = gst_element_get_static_pad(sgie2, "src");
  if (!sgie_src_pad)
    g_print("Unable to get pgie src pad\n");
  else
    gst_pad_add_probe(sgie_src_pad, GST_PAD_PROBE_TYPE_BUFFER,
                      sgie_src_pad_buffer_probe, (gpointer)sink, NULL); */

  GstPad *pgie_src_pad = gst_element_get_static_pad(pgie, "src");
  if (!pgie_src_pad)
    g_print("Unable to get pgie src pad\n");
  else
    gst_pad_add_probe(pgie_src_pad, GST_PAD_PROBE_TYPE_BUFFER,
                      body_landmarks_src_pad_buffer_probe, (gpointer)pgie, NULL); 

  /* Lets add probe to get informed of the meta data generated, we add probe to
   * the sink pad of the osd element, since by that time, the buffer would have
   * had got all the metadata. */
  osd_sink_pad = gst_element_get_static_pad(nvosd, "sink");
  if (!osd_sink_pad)
    g_print("Unable to get sink pad\n");
  else
    gst_pad_add_probe(osd_sink_pad, GST_PAD_PROBE_TYPE_BUFFER,
                      osd_sink_pad_buffer_probe, (gpointer)sink, NULL);

  /* Set the pipeline to "playing" state */
  g_print("Now playing: %s\n", input_file);
  gst_element_set_state(pipeline, GST_STATE_PLAYING);

  /* Wait till pipeline encounters an error or EOS */
  g_print("Running...\n");
  g_main_loop_run(loop);

  /* Out of the main loop, clean up nicely */
  g_print("Returned, stopping playback\n");
  /* should check if we got an error message here or an eos */
  gst_element_set_state (pipeline, GST_STATE_NULL);

  g_print("Deleting pipeline\n");
  gst_object_unref(GST_OBJECT(pipeline));
  g_source_remove(bus_watch_id);
  g_main_loop_unref(loop);

  return 0;
}

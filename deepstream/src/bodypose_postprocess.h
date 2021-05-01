#ifndef  BODY_POSE_POST_PROCESS_H
#define  BODY_POSE_POST_PROCESS_H

#include <gst/gst.h>
#include "body_struct.h"

GstPadProbeReturn body_landmarks_src_pad_buffer_probe(GstPad *pad, GstPadProbeInfo *info, gpointer u_data);

#endif

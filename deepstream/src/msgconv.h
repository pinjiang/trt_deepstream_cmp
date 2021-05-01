#include <glib.h>

#define MAX_TIME_STAMP_LEN 32

void meta_free_func (gpointer data, gpointer user_data);

gpointer meta_copy_func (gpointer data, gpointer user_data);

void generate_ts_rfc3339 (char *buf, int buf_size);

void generate_event_msg_meta (gpointer data, gint class_id, NvDsObjectMeta * obj_params);

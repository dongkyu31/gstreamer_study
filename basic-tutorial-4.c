/* Basic toturial 4 : Time management - Seeking example
 *
 * Goal
 *
 * this tutorial shows how to use GStreamer time-related facilities, In particular
 *  - How to query the pipeline for information like stream position or duration
 *  - How to seek (jump) to a different position (time) inside the stream
 *
 * Introduction
 *  GstQuery is a mechanism that allows asking an element or pad for a piece of information.
 * In this example we ask the pipeline if seeking is allowed (some source, likeke live streams, do not allow seeking).
 * If it is allowed, then, once the movie has been running for ten seconds, we skip to a defferent position using a seek.
 *
 * In the previous tutorials, once we had the pipeline setup and running, our main function just sat and waited to receive an ERROR or an EOS through the bus.
 * Here, we modify this function to periodically wake up and query the pipeline for the stream position, so we can print it on the screen.
 * This is similar to what a media player would do, updating the user interface on a periodic basis.
 *
 * Finally, the stream duration is queried and updated whenever it changes.
 *
 */
#include <gst/gst.h>

/* Structure to contain all our information, so we can pass it around */
typedef struct _CustomData {
	GstElement* playbin;	/* Our one and only element */
	gboolean playing;		/* Are we in the PLAYING state? */
	gboolean terminate;		/* Should we terminate execution? */
	gboolean seek_enabled; 	/* Is seeking enable for this media? */
	gboolean seek_done;   	/* Have we performed the seek already? */
	gint64 duration; 		/* How long does this media last, in nanoseconds */
} CustomData;

/* Forward definition of the message processing function */
static void handle_message(CustomData* data, GstMessage* msg);

int main(int argc, char* argv[]) {
	CustomData data;
	GstBus* bus;
	GstMessage* msg;
	GstStateChangeReturn ret;

	data.playing = FALSE;
	data.terminate = FALSE;
	data.seek_enabled = FALSE;
	data.seek_done = FALSE;
	data.duration = GST_CLOCK_TIME_NONE;

	/* Initialize GStreamer */
	gst_init(&argc, &argv);

	/* Create the elements */
	// playbin은 그자체로 pipeline이 되는 element이다. 이번 코드에서는 playbin 하나만 사용해서 재생하는 예제이다.
	data.playbin = gst_element_factory_make("playbin", "playbin");

	if( !data.playbin) {
		g_printerr("Not all elements could be created.\n");
		return -1;
	}

	/* Set the URI to play */
	g_object_set(data.playbin, "uri", "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm", NULL);

	/* Starting playing */
	ret = gst_element_set_state(data.playbin, GST_STATE_PLAYING);
	if (ret == GST_STATE_CHANGE_FAILURE) {
		g_printerr("Unable to set the pipeline to the playing state.\n");
		gst_object_unref(data.playbin);
		return -1;
	}

	/* Listen to the bus */
	bus = gst_element_get_bus(data.playbin);
	do {
		/*  Previously, we did not provide a timeout to gst_bus_timed_pop_filtered(), meaming that it didn't return until a message was received.
		 * Now we use a timeout of 100 milliseconds, so, if no message is received during one tenth of a second, the function will return NULL.
		 * We are going to use this logic to update our "UI".
		 *  Note that the desired timeout must be specified as a GstClockTime, hence, in nanoseconds. Numbers expressing different time units then, 
		 * should be multiplied by macros like GST_SECOND or GST_MSECOND. This also makes your code more readable.  */
		msg = gst_bus_timed_pop_filtered(bus, 100*GST_MSECOND, 
			GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_DURATION);

		/* Parse message */
		if(msg != NULL) {
			handle_message(&data, msg);
		} else {
			/* We got no message, this means the timeout expired */
			if(data.playing) {
				gint64 current = -1;


				/* Query the current position of the stream */
				if (!gst_element_query_position(data.playbin, GST_FORMAT_TIME, &current)) {
					g_printerr("Could not query current position.\n");
				}

				/* If we didn't know it yet, query the stream duration */
				if(!GST_CLOCK_TIME_IS_VALID(data.duration)) {
					if(!gst_element_query_duration(data.playbin, GST_FORMAT_TIME, &data.duration)) {
						g_printerr("Could not query current position.\n");
					}
				}

				/* Print current postion and total duration */
				// Note that usage of the GST_TIME_FORMAT and GST_TIME_ARGS macros to provide a user-friendly representation of GStreamer times.
				g_print("Position %" GST_TIME_FORMAT " / %" GST_TIME_FORMAT "\r",
					GST_TIME_ARGS(current), GST_TIME_ARGS(data.duration));

				/* IF seeking is enabled, we have not done it yet, and the time is right, seek */
				/*
				 * GST_FORMAT_TIME : This discards all data currently in the pipeline befor doing the seek. Might pause a bit while the pipeline is refilled
				 *  	and the new data starts to show up, but greatly increases the "responsiveness" of the application.
				 *  	If this flag is not provided, "stale" data might be shown for a while until the new position apprears at the end of the pipeline.
				 *
				 * GST_SEEK_FLAG_KEY_UNIT : With most encoded video data streams, 
				 * 		seeking to arbitrary positions is not possible but only to certain frames called Key frames. When this flag is used,
				 * 		the pipeline will actually move to the closest key frame and start producing data straight away. 
				 * 		If this flag is not used, the pipeline will move internally to the closest key frame (it has no other alternative) but data will not be shown
				 * 		until it reaches the requested position. This last alternative is more accurate, but might take longer.
				 *
				 * GST_SEEK_FLAG_ACCURATE : Some media clips do not provide enough indexing information, 
				 * 		meaning that seeking to arbitrary position is time-consuming. In these cases, GStreamer usually estimates the position to seek to, and 
				 * 		usually works just fine. If this precision is not good enough for your case (you see seeks not going to the exact time you asked for ), 
				 * 		then provide this flag. Be warned that it might take longer to calculate the seeking position (very long, on some files).
				 */
				if(data.seek_enabled && !data.seek_done && current > 10 * GST_SECOND) {
					g_print("\nReached 10s, performing seek...\n");
					gst_element_seek_simple(data.playbin, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, 30*GST_SECOND);
					data.seek_done = TRUE;		
				}
			}
		}
	} while (!data.terminate);
	
	/* Free resources */
	gst_object_unref(bus);
	gst_element_set_state(data.playbin, GST_STATE_NULL);
	gst_object_unref(data.playbin);
	return 0;

}

static void handle_message(CustomData* data, GstMessage* msg) {
	GError* err;
	gchar* debug_info;

	switch(GST_MESSAGE_TYPE(msg)) {
		case GST_MESSAGE_ERROR :
			gst_message_parse_error(msg, &err, &debug_info);
			g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
			g_printerr("Debugging information: %s\n", debug_info? debug_info:"none");
			g_clear_error(&err);
			g_free(debug_info);
			data->terminate = TRUE;
			break;
		case GST_MESSAGE_EOS :
			g_print("\nEnd-Of_Stream reached. \n");
			data->terminate = TRUE;
			break;
		case GST_MESSAGE_DURATION:	 // this message is posted on whenever the duration of the stream changes.
			/* The duration has changed, mark the current on as invalid */
			g_print("Get message (GST_MESSAGE_DURATION) ");
			data->duration = GST_CLOCK_TIME_NONE;
			break;
		case GST_MESSAGE_STATE_CHANGED: {
			GstState old_state, new_state, pending_state;
			gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
			if(GST_MESSAGE_SRC(msg) == GST_OBJECT(data->playbin)) {
				g_print("Pipeline state changed from %s to %s: \n", gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));
				/* Remember whether we are in the PLAYING state or not  */
				data->playing = (new_state == GST_STATE_PLAYING);

				if(data->playing) {
					/* We just moved to PLAYING. Check if seeking is possible */
					GstQuery* query;
					gint64 start, end;
					query = gst_query_new_seeking(GST_FORMAT_TIME);
					if(gst_element_query(data->playbin, query)) {
						gst_query_parse_seeking(query, NULL, &data->seek_enabled, &start, &end);
						if(data->seek_enabled) {
							g_print("Seeking is ENABLED from %" GST_TIME_FORMAT " to %" GST_TIME_FORMAT "\n",
								GST_TIME_ARGS(start), GST_TIME_ARGS(end));
						} else {
							g_print("Seeking is DISABLED for this stream.\n");
						}

					} else {
						g_printerr("Seeking query failed");
					}
					gst_query_unref(query);

				}
			}

		}
			break;

		default:
			/* We should not reach here */
			g_printerr("Unexpected message received.\n");
			break;
	}
	gst_message_unref(msg);
}

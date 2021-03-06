/* Basic tutorial 7 : Multithreading and Pad Availability
 *
 * Goal
 *
 * GStreamer handles multithreading automatically, but, under some circumstances, you might need to decouple threads manually.
 * This tutorial shows how to do this and, in addition, completes the exposition about Pad Availability. 
 * More precisely, this document explains:
 *
 *	- How to create new threads of execution for some parts of the pipeline
 *	- What is the Pad Availability
 *	- How to replicate streams
 *
 *
 * Introduction
 *
 * Multithreading
 * : GStreamer is a multithreading framework. This means that, internally, it creates and destroys threads as it needs them, 
 * for example, to decouple streaming from the application thread. Moreover, plugins are also free to create threads for
 * their own processing, for example, a video decoder could create 4 threads to take full advantage of a CPU with 4 cores.
 *
 * On top of this, when building the pipeline an application can specify explicitly that a branch (a part of the pipeline) runs
 * on a different thread (for example, to have the audio and video decoders executing simultaneously).
 *
 * This is accomplished using the queue element, which works as follows. The sink pad just enqueues data and returns control.
 * On a different thread, data is dequeued and pushed downstream. This element is also used for buffering, as seen later 
 * in the streaming tutorials. The size of the queue can be controlled through properties.
 *
 *
 * The example pipeline
 * 
 * This example builds the following pipeline:
 * https://gstreamer.freedesktop.org/documentation/tutorials/basic/images/tutorials/basic-tutorial-7.png
 *
 *            |<-- Thread 1 ------------------------------->|<--- Thread 2 -------------------------------------------->|
 *                                                ----------------     ----------------     ----------------     ----------------
 *                                                |Queue         |     |Audio convert |     |Audio resample|     |Audio sink    |
 *                                            --->|sink|     |src| --> |sink|     |src| --> |sink|     |src| --> |sink|         |
 * -------------------    ------------------  |   |              |     |              |     |              |     |              |
 * | App source      |    |Tee         |src|---   ----------------     ----------------     ----------------     ----------------
 * |            |sink|--> |sink|           |   
 * |                 |    |            |src|--    ----------------     ----------------     ----------------     ----------------  
 * -------------------    ------------------  |   |Queue         |     |Wave scope    |     |Video convert |     |Video sink    |
 *                                            --->|sink|     |src| --> |sink|     |src| --> |sink|     |src| --> |sink|         |
 *                                                |              |     |              |     |              |     |              |
 *                                                ----------------     ----------------     ----------------     ----------------
 *                                                          |<--- Thread 3 -------------------------------------------->|
 *
 *
 * The source is a synthetic audio signal (a continuous tone) which is split using a tee element (it sends through its source pads everything
 * it recieves through its sink pad). One branch then sends the signal to the audio card, and the other renders a video of the waveform
 * and sends it to the screen.
 *
 * As seen in the picture, queues create a new thread, so this pipeline runs in 3 threads. Pipelines with more than one sink usually need to 
 * be mutithreaded, because, to be synchoronized, sinks usaually block execution until all other sinks are ready, and they cannot get ready 
 * if there is only one thread, being blocked by the first sink.
 * 
 *
 * Requested pads
 *
 * In Basic-tutorial 3: Dynamic pipelines we saw an element (uridecodebin) which had no pads to begin with, and they appeared as data started 
 * to flow and the element learned about the media. These are called Sometimes Pads, and contrast with the regular pads which are always 
 * and are called Always Pads.
 *
 * The third kind of pad is the Request Pad, which is created on demand. The classical example is the tee element, which has one sink pad and
 * no initial source pads: they need to be requested and the tee adds them. In this way, and input stream can be replicated any number of times.
 * The disadvantage is that linking elements with Requested Pads is not as automatic, as linking Always Pads, as the walkthrough for this example will show.
 *
 * Also, to request (or release) pads in the PLAYING or PAUSE states, you need to take additional cautions (Pad blocking) which are not described
 * in this tutorial. It is safe to request (or release) pads in the NULL or READY states, though.
 *
 * Without further delay, let's see the code.
 *
 *
 *
 */

#include <gst/gst.h>

int main(int argc, char *argv[]) {
	GstElement *pipeline, *audio_source, *tee, *audio_queue, *audio_convert, *audio_resample, *audio_sink;
	GstElement *video_queue, *visual, *video_convert, *video_sink;
	GstBus *bus;
	GstMessage *msg;
	GstPad *tee_audio_pad, *tee_video_pad;
	GstPad *queue_audio_pad, *queue_video_pad;

	/* Initialize GStreamer */
	gst_init(&argc, &argv);

	/* Create the elements */
	audio_source = gst_element_factory_make("audiotestsrc", "audio_source");
	tee = gst_element_factory_make("tee", "tee")
	audio_queue = gst_element_factory_make("queue", "audio_queue");
	audio_convert = gst_element_factory_make("audioconvert", "audio_conert");
	audio_resample = gst_element_factory_make("audioresample", "audio_resample");
	audio_sink = gst_element_factory_make("autoaudiosink", "audio_sink");
	video_queue = gst_element_factory_make("queue", "video_queue");
	visual = gst_element_factory_make("wavescope", "visual");
	video_convert = gst_element_factory_make("videoconvert", "csp");
	video_sink = gst_element_factory_make("autovideosink", "video_sink");

	/* Create the empty pipeline */
	pipeline = gst_pipeline_new("test-pipeline");

	if(!pipeline || !audio_source || !tee || !audio_queue || !audio_convert || !audio_resample || !audio_sink ||
      !video_queue || !visual || !video_convert || !video_sink) {
    	g_printerr ("Not all elements could be created.\n");
    	return -1;
  	}

	
	/* Configure elements */
	g_object_set(audio_source, "freq", 215.0f, NULL);
	g_object_set(visual, "shader", 0, "style", 1, NULL);

	/* Link all elements that can be automatically linked because they because they have "Always" pads */
	gst_bin_add_many (GST_BIN (pipeline), audio_source, tee, audio_queue, audio_convert, audio_resample, audio_sink,
			video_queue, visual, video_convert, video_sink, NULL);
	if (gst_element_link_many (audio_source, tee, NULL) != TRUE ||
			gst_element_link_many (audio_queue, audio_convert, audio_resample, audio_sink, NULL) != TRUE ||
			gst_element_link_many (video_queue, visual, video_convert, video_sink, NULL) != TRUE) {
		g_printerr ("Elements could not be linked.\n");
		gst_object_unref (pipeline);
		return -1;
	}

	/* Manually link the Tee, which has "Request" pads */
	tee_audio_pad = gst_element_request_pad_simple (tee, "src_%u");
	g_print ("Obtained request pad %s for audio branch.\n", gst_pad_get_name (tee_audio_pad));
	queue_audio_pad = gst_element_get_static_pad (audio_queue, "sink");
	tee_video_pad = gst_element_request_pad_simple (tee, "src_%u");
	g_print ("Obtained request pad %s for video branch.\n", gst_pad_get_name (tee_video_pad));
	queue_video_pad = gst_element_get_static_pad (video_queue, "sink");
	if (gst_pad_link (tee_audio_pad, queue_audio_pad) != GST_PAD_LINK_OK ||
			gst_pad_link (tee_video_pad, queue_video_pad) != GST_PAD_LINK_OK) {
		g_printerr ("Tee could not be linked.\n");
		gst_object_unref (pipeline);
		return -1;
	}
	gst_object_unref (queue_audio_pad);
	gst_object_unref (queue_video_pad);

	/* Start playing the pipeline */
	gst_element_set_state (pipeline, GST_STATE_PLAYING);

	/* Wait until error or EOS */
	bus = gst_element_get_bus (pipeline);
	msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

	/* Release the request pads from the Tee, and unref them */
	gst_element_release_request_pad (tee, tee_audio_pad);
	gst_element_release_request_pad (tee, tee_video_pad);
	gst_object_unref (tee_audio_pad);
	gst_object_unref (tee_video_pad);

	/* Free resources */
	if (msg != NULL)
		gst_message_unref (msg);
	gst_object_unref (bus);
	gst_element_set_state (pipeline, GST_STATE_NULL);

	gst_object_unref (pipeline);

	return 0;
}

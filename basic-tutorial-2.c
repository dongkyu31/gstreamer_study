
/*
 * Basic tutorial 2 : GStreamer concepts
 */


#include <gst/gst.h>

int main(int argc, char * argv[])
{
	GstElement *pipeline, *source, *sink;
	GstBus *bus;
	GstMessage *msg;
	GstStateChangeReturn ret;

	/* Initialize GStreamer */
	gst_init(&argc, &argv);

	/* Create the elements */
	// gst_element_factory_make() 함수를 통해서 element를 생성한다.
	source = gst_element_factory_make("videotestsrc", "source");
	sink = gst_element_factory_make("autovideosink", "sink");

	/* Create the empty pipeline */
	// gst_pipeline_new() 함수를 통해서 빈 pipeline을 생성한다.
	pipeline = gst_pipeline_new("test-pipeline");

	if (!pipeline || !source || !sink) {
		g_printerr("Not all elements could be created.\n");
		return -1;
	}


	/* Build the pipeline */
	// pipeline을 구성하기 위해 gst_bin_add_many() 함수를 이용해 pipeline에 element를추가하고,
	// gst_element_link() 함수를 이용해서 element 간에 link를 생성한다.
	// link 전에 link될 element들은 반드시 같은 pipeline에 추가시겨야 한다.
	gst_bin_add_many(GST_BIN(pipeline), source, sink, NULL);
	if (gst_element_link(source, sink) != TRUE) {
		g_printerr("Elements could not be linked.\n");
		gst_object_unref(pipeline);
		return -1;
	}


	/* Modify the source's properties */
	// 대부분의 GStreamer element는 행동을 바꿀 수 있는 writable property와 상태를 알 수 있는 readable property를 가지고 있다.
	// 그래서 g_object_get()으로 속성 값을 가져오고 g_object_set()을 통해서 속성값을 입력할 수 있다.
	// 만약, element에서 사용될 수 있는 property의 값들을 알고 싶다면, gst-inspect-1.0 툴을 사용하면 된다.
	g_object_set (source, "pattern", 0, NULL);

	/* Start playing */
	ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
	if (ret == GST_STATE_CHANGE_FAILURE) {
		g_printerr("Unalbe to set the pipeline to the playing state.\n");
		gst_object_unref(pipeline);
		return -1;
	}

	/* Wait until error or EOS */
	bus = gst_element_get_bus(pipeline);
	msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

	/* Parse message  */
	if (msg != NULL) {
		GError *err;
		gchar *debug_info;

		switch (GST_MESSAGE_TYPE(msg)) {
			case GST_MESSAGE_ERROR:
				gst_message_parse_error(msg, &err, &debug_info);
				g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
				g_printerr("Debugging information:%s \n", debug_info? debug_info :"none");
				g_clear_error(&err);
				g_free(debug_info);
				break;

			case GST_MESSAGE_EOS:
				g_print("End-Of-Stream reached.\n");
				break;

			default :
				/* We should not reach here because we only asked for ERRORs and EOS */
				g_printerr("Unexpected message received.\n");
				break;
		}
		gst_message_unref(msg);

	}

	/* Free resources  */
	gst_object_unref(bus);
	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_object_unref(pipeline);
	
	return 0;

}

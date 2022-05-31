/* Basic tutorial 6 : Media formats and Pad Capabilities.
 *
 * Goal
 *
 * Pad Capabilities are fundamental element of GStreamer, although most of the time they are invisible because the framework handles them automatically.
 * This somewhat theoretical tutorial shows.
 *
 *   - What is Pad Capabilities.
 *   - How to retrieve them.
 *   - When to retrieve them.
 *   - Why you need to know about them.
 *
 * Introduction
 *
 * Pads
 * As it has already been shown, Pads allow information to enter and leave an element. The Capabilities (or Caps, for short) of a Pad, then,
 * specify what kind of information travel through the Pad.
 * For example, "RGB video with a resolution of 320x200 pixels and 30 frames per second", or "16-bits per sample audio, 5.1 channels at 44100 samples per second",
 * or even compressed formats like mp3 or h264.
 *
 * Pads can support multiple Capabilities (for example, a video sink can support video in different types of RGB or YUV formats) 
 * and Capabilities can be specified as ranges (for example, an audio sink can support samples rates from 1 to 48000 samples per second). 
 * However, the actual information traveling from Pad to Pad must have only one well-specified type. Through a process known as negotiation, 
 * two linked Pads agree on a common type, and thus the Capabilities of the Pads become fixed (they only have one type and do not contain ranges). 
 * The walkthrough of the sample code below should make all this clear.
 *
 * In order for two elements to be linked together, they must share a common subset of Capabilities (Otherwise they could not possibly understand each other). 
 * This is the main goal of Capabilities.
 *
 * As an application developer, you will usually build pipelines by linking elements together (to a lesser extent if you use all-in-all elements like playbin). 
 * In this case, you need to know the Pad Caps (as they are familiarly referred to) of your elements, or, at least, know what they are when GStreamer refuses 
 * to link two elements with a negotiation error.
 *
 * Pad templates
 * : Pads are created from Pad templates, which indicate all possible Capabilities a Pad could ever have. Templates are useful to create several similar Pads,
 * and also allow early refusal of connectoins between elements: If the Capabilities of their Pad Templates do not have a common subset (their intersection is empty),
 * there is no need to engotiate further.
 *
 * Pad Templates can be viewed as the first step in the negotiation process. As the process evolves, actual Pads are instantiated and their Capabilities refined
 * until they are fixed (or negotiation fails).
 *
 *
 * Capabilities examples
 * -----------------------------------------------------------------------------------------------------------------
 *  SINK template: 'sink'
 *    Availability: Always
 *    Capabilities:
 *      audio/x-raw
 *                 format: S16LE
 *                   rate: [ 1, 2147483647 ]
 *               channels: [ 1, 2 ]
 *      audio/x-raw
 *                 format: U8
 *                   rate: [ 1, 2147483647 ]
 *               channels: [ 1, 2 ]
 * -------------------------------------------------------------------------------------------------------------------
 * This pad is a sink which is always available on the element (we will not talk about availability for now).
 * It supports two kinds of media, both raw audio in integer format(audio/x-raw):signed, 16-bit little endian
 * and unsigned 8-bit. The square brackets indicate a range: for instance, the number of channels varies from 1 to 2.
 *
 * ---------------------------------------------------------------------------------------------------------------
 *  SRC template: 'src'
 *    Availability: Always
 *    Capabilities:
 *      video/x-raw
 *                  width: [ 1, 2147483647 ]
 *                 height: [ 1, 2147483647 ]
 *              framerate: [ 0/1, 2147483647/1 ]
 *                 format: { I420, NV12, NV21, YV12, YUY2, Y42B, Y444, YUV9, YVU9, Y41B, Y800, Y8, GREY, Y16 , UYVY, YVYU, IYU1, v308, AYUV, A420 }
 * ----------------------------------------------------------------------------------------------------------------
 * video/x-raw indicates that this source pad outputs raw video . It supports a wide range of dimenstions and framerates, and 
 * a set of YUV formats (The curly braces indicate a list). All these formats indicate different packing and subsampling of the image planes.
 * 
 * 
 *
 *
 * Last remarks
 * You can use the gst-inspect-1.0 tool described in Basic tutorial 10: GStreamer tools to learn about the Caps of any GStreamer element.
 *
 * Bear in mind that some elements query the underlying hardware for supported formats and offer their Pad Caps accordingly (They usually do this when entering the READY state or higher). Therfore, the shown caps can vary from platform to platform, or even from one execution to the next (even though this case is rare).
 *
 * This tutorial instantiates two elements (this time, through their factories), shows their Pad Templates, link them and sets the pipeline to play. 
 * On each state change, the Capabilities of the sink element's Pad are shown, so you can observe how the negotiation proceeds until the Pad Caps are fixed.
 *
 *
 *
 */

#include <gst/gst.h>

/* Functions below print the Capabilities in a human-friendly format */
static gboolean print_field(GQuark field, const GValue* value, gpointer pfx) {
	gchar* str = gst_value_serialize(value);

	g_print("%s %15s: %s\n", (gchar*) pfx, g_quark_to_string(field), str);
	g_free(str);
	return TRUE;
}




static void print_caps(const GstCaps* caps, const gchar* pfx) {
	guint i;
	g_return_if_fail(caps != NULL);

	if(gst_caps_is_any(caps)) {
		g_print("%sANY\n", pfx);
		return;
	}

	if(gst_caps_is_empty(caps)) {
		g_print("%sEMPTY\n", pfx);
		return;
	}

	for(i=0; i<gst_caps_get_size(caps); i++) {
		GstStructure* structure = gst_caps_get_structure(caps, i);

		g_print("%s%s\n", pfx, gst_structure_get_name(struecture));
		gst_structure_foreach(structure, print_field, (gpointer)pfx);
	}
}

/* Prints information about a Pad Template, including its Capabilities */
static void print_pad_templates_information(GstElementFactory* factory) {
	const GList* pads;
	GstStaticPadTemplate* padtemplate;

	g_print("Pad Templates for %s:\n", gst_elements_factory_get_longname(factory));
	if(!gst_element_factory_get_num_pad_templates(factory)) {
		g_print("    none\n");
		return;
	}
	
	pads = gst_element_factory_get_static_pad_templates(factory);
	while(pads) {
		padtemplate = pads->data;
		pads = g_list_next(pads);
		
		if(padtemplate->direction == GST_PAD_SRC)
			g_print("	SRC template: '%s'\n", padtemplate->name_template);
		else if(padtemplate->direction == GST_PAD_SINK)
			g_print("	SINK template: '%s'\n", padtemplate->name_template);
		else
			g_print("	UNKNOWN!! template: '%s'\n", padtemplate->name_template);

		if(padtemplate->presence == GST_PAD_ALWAYS)
			g_print("	Availability: Always\n");
		else if(padtemplate->presence == GST_PAD_SOMETIMES)
			g_print("	Availability: sometimes\n");
		else 
			g_print("	Availability: UNKNOWN!!!\n");

		if(padtemplate->static_caps.string) {
			GstCaps* caps;
			g_print(" Capabilities: \n");
			caps = gst_static_caps_get(&padtemplate->static_caps);
			print_caps(caps, "		");
			gst_caps_unref(caps);
		}
		g_print("\n");

	}

}

int main(int argc, char *argv[]) {
	GstElement *pipeline, *source, *sink;
	GstElementFactory *source_factory, *sink_factory;
	GstBus *bus;
	GstMessage *msg;
	GstStateChangeReturn ret;
	gboolean terminate = FALSE;

	/* Initialize GStreamer */
	gst_init(&argc, &argv);


	/* Create the element factorise */
	source_factory = gst_element_factory_find("audiotestsrc");
	sink_factory = gst_element_factory_find("autoaudiosink");
	if(!source_factory || !sink_factory) { 
		g_printerr("Not all element factories could be created.\n");
		return -1;
	}
	

}





/* Basic tutorial 5 : GUI toolkit integration
 *
 * This tutorial shows how to integrate GStreamer in a Graphical User Interface(GUI) toolkit like GTK+.
 * Basically, GStreamer takes care of media playback while the GUI toolkit handles user interaction.
 * Then most interesting parts are those in which both libraries have to interact : Instructing GStreamer to output video to a GTK+ window and 
 * forwarding user actions to GStreamer.
 *
 * In particular, you will learn:
 *  - How to tell GStreamer to output video to a particular window(instead of creating its own window)
 *  - How to continuously refresh the GUI with information from GStreamer.
 *  - How to update the GUI from the multiple threads of GStreamer, an operation forbidden on most GUI toolkits.
 *  - A mechanism to subscribe only to the messages you are interested of being notified of all of them.
 *
 *
 * Introduction
 *
 *
 *
 *
 *
 *
 *
 *
 */
 
#include <gst/gst.h>

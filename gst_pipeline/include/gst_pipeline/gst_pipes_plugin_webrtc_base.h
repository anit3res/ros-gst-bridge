#ifndef GST_PIPELINE__GST_PIPES_PLUGIN_WEBRTC_BASE_H_
#define GST_PIPELINE__GST_PIPES_PLUGIN_WEBRTC_BASE_H_

#include <gst_bridge/gst_bridge.h>
#include <gst_pipes_plugin_base.h>

#include "rclcpp/rclcpp.hpp"

namespace gst_pipes
{
/*
  This plugin manages the connection between a webrtcbin and its peer discovery mechanism


*/
class gst_pipes_webrtc_base : public gst_pipes_plugin
{
public:
  // during init, we need to

  //  fetch parameters describing the src and sink bins/pads to connect when webrtc goes online
  //    audio_sink_descr
  //    video_sink_descr
  
  //  fetch parameters indicating if we should:
  //    dialout automatically
  //    pick up automatically

  //  fetch a param from the node for the element name of our webrtcbin
  //  search the pipeline for the webrtcbin
  //  connect callbacks to the webrtcbin
  protected:

  void initialise(
    std::string name,  // the config name of the plugin
    std::shared_ptr<gst_bridge::node_interface_collection> node_if,
    GstElement * pipeline
  );


  // ############ virtual methods for different signalling servers ############

  // connect to your signalling server
  virtual void init_signalling_server_client();

  // we're ready to start a call, or receive incoming calls
  virtual void begin_negotiate();

  // called when the webrtcbin wants to send a SDP answer
  // default calls  send_sdp(descr)
  virtual void send_sdp_answer(GstWebRTCSessionDescription * desc);

  // called when the webrtcbin is instructed to send a sdp offer
  // default calls  send_sdp(descr)
  virtual void send_sdp_offer(GstWebRTCSessionDescription * desc);

  // send a sdp description to the remote server
  virtual void send_sdp(GstWebRTCSessionDescription * desc) = 0;

  // send an ice candidat to the remote server
  virtual void send_ice_candidate(guint mline_index, gchararray candidate) = 0;


  // ############# handlers for remote peer events #############

  // to be called by the implementation when the remote peer has a SDP answer
  
  // thin wrapper over g_signal_emit_by_name(webrtc_, "set-remote-description", desc, promise);
  void
  sdp_received(
    // GstElement * object,               //the webrtcbin
    GstWebRTCSessionDescription * desc,   //a GstWebRTCSessionDescription description
    //GstPromise * promise                //a GstPromise to be notified when it's set 
  );

  // thin wrapper over g_signal_emit_by_name(webrtc_, "add-ice-candidate", mline_index, ice_candidate);
  void
  ice_candidate_received(
    // GstElement * object,     // the webrtcbin
    guint mline_index,          // the index of the media description in the SDP
    gchararray ice_candidate,   // an ice candidate or NULL/"" to mark that no more candidates will arrive
  );



  // ############# Callbacks for establishing a webrtc connection #############

  // called when webrtcbin wants to start connecting with the remote peer.
  static void
  on_negotiation_needed_cb(
    GstElement * object,
    gpointer udata
  );

  // called when the webrtcbin has generated an ICE candidate to send to the remote peer
  static void
  on_ice_candidate_cb(
    GstElement * object,
    guint mline_index,
    gchararray candidate,
    gpointer udata
  );

  // diagnostic only,
  // connected to a notify event that tells us how we're progressing to a NAT punching config
  static void
  on_notify_ice_gathering_state_cb(
    GstElement *webrtcbin,
    GParamSpec *pspec,
    gpointer user_data
  );


  // ########## Callbacks for waiting on functions in the webrtcbin ##########

  // gst promise that is called after set_remote_description completes.
  // this asks the bin to create a SDP answer,
  // the answer will be passed to create_answer_prom
  static void 
  gst_pipes_webrtc::set_remote_description_prom(
    GstPromise *promise,
    gpointer user_data
  );


  // gst promise that is called after the bin creates a SDP answer,
  // This call is where we send the SDP answer to the remote peer
  static void 
  gst_pipes_webrtc::create_answer_prom(
    GstPromise *promise,
    gpointer user_data
  );

  // This callback is used to add sinks to the webrtcbin
  static void
  pad_added_cb(
    GstElement *webrtc,
    GstPad *pad,
    gpointer user_data
  );


  // ###################### Callbacks for data channels ######################
  // the remote peer opened an outbound a data channel, and the bin has accepted it.
  static void
  on_data_channel_cb(
    GstElement * object,
    GstWebRTCDataChannel * channel,
    gpointer user_data
  );

  // the data channel has received new data
  static void
  data_channel_on_message_data_cb(
    GstWebRTCDataChannel * self,
    GBytes * data,
    gpointer user_data
  );

  static void
  data_channel_on_error_cb(
    GstWebRTCDataChannel * self,
    GError * error,
    gpointer user_data
  );

  static void
  data_channel_on_open_cb(
    GstWebRTCDataChannel * self,
    gpointer user_data
  );

  static void
  data_channel_on_close_cb(
    GstWebRTCDataChannel * self,
    gpointer user_data
  );




protected:

  // the name of the target element in the pipeline
  std::string elem_name_;

  // a pointer to the bridge elements in the pipeline
  GstElement * webrtc_;

  GstWebRTCDataChannel * data_channel_rx_;
  GstWebRTCDataChannel * data_channel_tx_;

};

}  // namespace gst_pipes

#endif  //GST_PIPELINE__GST_PIPES_PLUGIN_WEBRTC_BASE_H_

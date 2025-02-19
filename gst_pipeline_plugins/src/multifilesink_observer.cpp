#include <type_traits>
#include <multifilesink_observer.h>


// XXX element should be an optional string, in which case the plugin would
//     listen pipeline-wide for multifilesink bus messages and publish reports
//     from all multifilesinks that it finds.


namespace gst_pipeline_plugins
{
  void multifilesink_observer::initialise(
    std::string name,
    std::shared_ptr<gst_bridge::node_interface_collection>node_if,
    GstPipeline * pipeline
) {
    name_ = name;
    node_if_ = node_if;
    pipeline_ = pipeline;

    frame_id_ = node_if_->parameters
      ->declare_parameter(
                          name_ + ".frame_id", rclcpp::ParameterValue(node_if_->base->get_name()),
                          descr("the frame_id denoting the observation", true))
      .get<std::string>();

    elem_name_ = node_if_->parameters
      ->declare_parameter(
                          name_ + ".element_name", rclcpp::ParameterValue("mysink"),
                          descr("the name of the sink element inside the pipeline", true))
      .get<std::string>();

    auto topic_name_ = node_if_->parameters
      ->declare_parameter(
                          name_ + ".event_topic", rclcpp::ParameterValue("~/" + name_ + "/gst_multifilesink"),
                          descr("the topic name to post events from the sink", true))
      .get<std::string>();

    rclcpp::QoS qos = rclcpp::SensorDataQoS().reliable();

    event_pub_ = rclcpp::create_publisher<gst_msgs::msg::MultifilesinkEvent>(
                                                                             node_if_->parameters, node_if_->topics, topic_name_, qos);

    if (GST_IS_BIN(pipeline_)) {
      GstElement* bin = gst_bin_get_by_name(GST_BIN_CAST(pipeline_), elem_name_.c_str());
      if (bin) {
        RCLCPP_INFO(
                    node_if->logging->get_logger(), "plugin multifilesink_observer '%s' found '%s'",
                    name_.c_str(), elem_name_.c_str());

        // Set messages posting cap manually
        g_object_set(bin, "post-messages", true, nullptr);

        // hook to the async "message" signal emitted by the bus
        GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE (pipeline_));
        g_signal_connect (bus, "message::element", (GCallback) multifilesink_observer::gst_bus_cb, static_cast<gpointer>(this));
        gst_object_unref(bus);

        RCLCPP_INFO(
                    node_if->logging->get_logger(), "plugin multifilesink_observer '%s' attached signal callback for '%s'",
                    name_.c_str(), elem_name_.c_str());

    } else {
        RCLCPP_ERROR(
                     node_if->logging->get_logger(),
                     "plugin multifilesink_observer '%s' failed to locate a gstreamer element called '%s'",
                     name_.c_str(), elem_name_.c_str());
      }
    } else {
      RCLCPP_ERROR(
                   node_if->logging->get_logger(),
                   "plugin multifilesink_observer '%s' received invalid pipeline in initialisation",
                   name_.c_str());
    }

  }

  gboolean multifilesink_observer::gst_bus_cb(GstBus* bus, GstMessage* message, gpointer user_data) {
    (void)bus;
    auto* this_ptr = static_cast<multifilesink_observer*>(user_data);
    const GstStructure* s;

    if(GST_MESSAGE_ELEMENT== GST_MESSAGE_TYPE(message)) {
      s = gst_message_get_structure(message);
      if (0 == g_strcmp0(gst_structure_get_name(s), "GstMultiFileSink")) {
        if (0 == g_strcmp0(GST_OBJECT_NAME (message->src), this_ptr->elem_name_.c_str())) {

        RCLCPP_DEBUG(this_ptr -> node_if_ -> logging -> get_logger(), "got bus msg, %s, originating from %s",
          gst_structure_get_name(s), GST_OBJECT_NAME (message->src));

          // Create the ROS Message
          auto msg = gst_msgs::msg::MultifilesinkEvent();
          // Fill the standard header
          rclcpp::Clock::SharedPtr ros_clock = this_ptr->node_if_->clock->get_clock();
          msg.header.stamp = ros_clock->now();
          msg.header.frame_id = this_ptr->frame_id_;
          // Now populate it with the GST Message
          msg.filename = std::string(gst_structure_get_string(s, "filename"));
          gst_structure_get_int(s, "index", &msg.index);
          gst_structure_get_clock_time(s, "timestamp", static_cast<GstClockTime*>(&msg.timestamp));
          gst_structure_get_clock_time(s, "stream-time", static_cast<GstClockTime*>(&msg.stream_time));
          gst_structure_get_clock_time(s, "running-time", static_cast<GstClockTime*>(&msg.running_time));
          gst_structure_get_clock_time(s, "duration", static_cast<GstClockTime*>(&msg.duration));
          gst_structure_get_uint64(s, "offset", &msg.offset);
          gst_structure_get_uint64(s, "offset-end", &msg.offset_end);
          // Publish
          this_ptr -> event_pub_ -> publish(msg);
        }
      }
    }

    return true;
  }

}  // namespace gst_pipeline_plugins

#include <pluginlib/class_list_macros.hpp>
PLUGINLIB_EXPORT_CLASS(gst_pipeline_plugins::multifilesink_observer, gst_pipeline::plugin_base)

#ifndef _GST_UTILITIES_H_
#define _GST_UTILITIES_H_

#include <yaml-cpp/yaml.h>

std::shared_ptr<YAML::Node> loadYAMLFile(const std::string &cfgPath) {
  // src:
  // https://github.com/ros-planning/navigation2/blob/778c0faee41788c9e3323a420ff2c10122ba4aae/nav2_map_server/src/map_io.cpp#L284
  try {
    auto node = std::make_shared<YAML::Node>(YAML::LoadFile(cfgPath));
    return node;
  } catch (YAML::Exception &e) {
    std::stringstream ss;
    ss << "Failed processing YAML file " << cfgPath << " at position ("
       << e.mark.line << ":" << e.mark.column << ") for reason: " << e.what()
       << std::endl;
    throw YAML::Exception(e.mark, ss.str());
  } catch (std::exception &e) {
    std::stringstream ss;
    ss << "Failed to parse map YAML loaded from file " << cfgPath
       << " for reason: " << e.what() << std::endl;
    throw;
  }
}

template <typename T>
T yaml_get_value(const YAML::Node &node, const std::string &key) {
  // src:
  // https://github.com/ros-planning/navigation2/blob/778c0faee41788c9e3323a420ff2c10122ba4aae/nav2_map_server/src/map_io.cpp#L106C1-L106C1
  try {
    return node[key].as<T>();
  } catch (YAML::Exception &e) {
    std::stringstream ss;
    ss << "Failed to parse YAML tag '" << key << "' for reason: " << e.msg;
    throw YAML::Exception(e.mark, ss.str());
  }
}

#endif

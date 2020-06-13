#ifndef _SCENELOADER_HPP_
#define _SCENELOADER_HPP_

#include "nlohmann/json.hpp"
#include <embree3/rtcore.h>
#include <spdlog/spdlog.h>
#include <fstream>
#include <vector>

// For paths handling
#include <filesystem>

bool load_VSSD_embree(const std::filesystem::path& path, RTCScene& scene);

#endif
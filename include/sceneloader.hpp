#ifndef _SCENELOADER_HPP_
#define _SCENELOADER_HPP_

#include "nlohmann/json.hpp"
#include <embree3/rtcore.h>
#include <spdlog/spdlog.h>
#include <fstream>
#include <vector>

// Paths handling
#include <filesystem>

// memcpy
#include <cstring>

RTCScene load_VSSD_embree
(
	const std::filesystem::path& json_path, 
	RTCDevice& rtc_device
);

#endif
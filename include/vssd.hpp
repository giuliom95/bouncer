#ifndef _VSSD_HPP_
#define _VSSD_HPP_

#include "nlohmann/json.hpp"
#include <embree3/rtcore.h>
#include <spdlog/spdlog.h>
#include <fstream>
#include <vector>

// Paths handling
#include <filesystem>

// memcpy
#include <cstring>

class VSSD_embree_scene
{
public:
	RTCScene embree_scene;

	~VSSD_embree_scene();

	void load
	(
		const std::filesystem::path& json_path, 
		RTCDevice& embree_device
	);
};

#endif
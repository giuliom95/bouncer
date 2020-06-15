#ifndef _SCENE_HPP_
#define _SCENE_HPP_

#include "nlohmann/json.hpp"
#include <embree3/rtcore.h>
#include <spdlog/spdlog.h>
#include <fstream>
#include <vector>

// Paths handling
#include <filesystem>

class Scene
{
public:
	RTCScene embree_scene;

	~Scene();

	void load
	(
		const std::filesystem::path& json_path, 
		RTCDevice& embree_device
	);
};

#endif
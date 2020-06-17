#ifndef _SCENE_HPP_
#define _SCENE_HPP_

#include "math.hpp"
#include "nlohmann/json.hpp"

#include <boost/log/trivial.hpp>

#include <embree3/rtcore.h>
#include <fstream>
#include <vector>

// Paths handling
#include <filesystem>

struct Camera
{
	float gate;
	float focal;
	Vec3f eye;
	Vec3f up;
	Vec3f look;
};

class Scene
{
public:
	RTCScene	embree_scene;
	Camera		camera;
	
	~Scene();

	void load
	(
		const std::filesystem::path& json_path, 
		RTCDevice& embree_device
	);
};

#endif
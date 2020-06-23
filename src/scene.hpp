#ifndef _SCENE_HPP_
#define _SCENE_HPP_

#include "math.hpp"
#include "camera.hpp"
#include "material.hpp"
#include "nlohmann/json.hpp"

#include <boost/log/trivial.hpp>
#include <boost/filesystem.hpp>

#include <embree3/rtcore.h>
#include <fstream>
#include <vector>

class Scene
{
public:
	RTCScene				embree_scene;
	Camera					camera;
	std::vector<Material> 	materials;

	~Scene();

	void load
	(
		const boost::filesystem::path& json_path, 
		RTCDevice& embree_device
	);
};

#endif
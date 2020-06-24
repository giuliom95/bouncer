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

#include <OpenImageIO/imagebuf.h>

class Image : 
public OIIO::ImageBuf
{
public:
	Image() : OIIO::ImageBuf() {};
	Image(const OIIO::ImageSpec& spec);
	Vec2f begin;
	Vec2f end;
	Vec2f size;
};

class Scene
{
public:
	RTCScene				embree_scene;
	Image					out_image;
	Camera					camera;
	std::vector<Material>	materials;

	~Scene();

	Scene
	(
		const boost::filesystem::path& json_path, 
		RTCDevice& embree_device
	);

	Vec2f film_space(
		const Vec2f xy,
		const Vec2f pixel_space
	);
};

#endif
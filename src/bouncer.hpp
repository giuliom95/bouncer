#ifndef _BOUNCER_HPP_
#define _BOUNCER_HPP_

#include "scene.hpp"
#include "gatherer.hpp"

#include <xmmintrin.h>
#include <pmmintrin.h>
#include <embree3/rtcore.h>

#include <OpenImageIO/imagebuf.h>

#include <boost/log/trivial.hpp>

#include <random>
#include <thread>

class Rand
{
public:
	float operator()()
	{
		return dist(mt);
	}
private:
	std::mt19937 							mt;
	std::uniform_real_distribution<float> 	dist;
};

class Image : 
public OIIO::ImageBuf
{
public:
	Image(const OIIO::ImageSpec& spec);
	Vec2f begin;
	Vec2f end;
	Vec2f size;
};

class Bouncer
{
public:
	Bouncer(const boost::filesystem::path& scenepath);
	~Bouncer();
	void render();
	void writeimage(const boost::filesystem::path& imagepath);
private:
	unsigned	nthreads;
	RTCDevice	embree_device;
	Scene		scene;
	Gatherer	gatherer;
	Image		out_image;

	void render_roi(const OIIO::ROI roi, const unsigned thread_id);
	Vec3f estimate_li
	(
		RTCRay r, 
		RTCIntersectContext* ic, 
		int bounces,
		Rand& rand,
		const unsigned thread_id
	);
};

#endif
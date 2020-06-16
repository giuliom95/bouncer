#include "scene.hpp"

#include <xmmintrin.h>
#include <pmmintrin.h>
#include <embree3/rtcore.h>

int main()
{
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
	_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

	RTCDevice embree_device = rtcNewDevice("verbose=3");

	Scene scene;
	scene.load
	(
		"../scenes/cube/cube.json",
		embree_device
	);

	rtcReleaseDevice(embree_device);
}
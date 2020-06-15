#include "vssd.hpp"

#include <iostream>
#include <fstream>

#include <xmmintrin.h>
#include <pmmintrin.h>
#include <embree3/rtcore.h>

#include <filesystem>

int main() {

	// Activation of "Flush to Zero" and "Denormals are Zero" CPU modes.
	// Embree reccomends them in sake of performance.
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
	_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
	

	RTCDevice	embree_device	= rtcNewDevice("verbose=3");

	VSSD_embree_scene scene;
	scene.load("../scenes/cube/cube.json", embree_device);

	rtcReleaseDevice(embree_device);

	return 0;
}

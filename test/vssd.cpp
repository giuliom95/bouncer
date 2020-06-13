#include "sceneloader.hpp"

#include <iostream>
#include <fstream>

#include <xmmintrin.h>
#include <pmmintrin.h>
#include <embree3/rtcore.h>

#include <filesystem>

int main() {

	/*std::ifstream infile{"../scenes/test_0000_vertices.bin", std::ios::binary};
	float v[3];
	infile.read((char*)&v, 3*sizeof(float));
	std::cout << v[0] << std::endl;
	std::cout << v[1] << std::endl;
	std::cout << v[2] << std::endl;
	infile.close();*/

	// Activation of "Flush to Zero" and "Denormals are Zero" CPU modes.
	// Embree reccomends them in sake of performance.
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
	_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
	

	RTCDevice	embree_device	= rtcNewDevice("verbose=3");
	RTCScene	embree_scene	= rtcNewScene(embree_device);

	load_VSSD_embree("../scenes/cube/cube.json", embree_scene);

	rtcReleaseScene(embree_scene);
	rtcReleaseDevice(embree_device);

	return 0;
}

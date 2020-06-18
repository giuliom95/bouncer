#ifndef _CAMERA_HPP_
#define _CAMERA_HPP_

#include "math.hpp"

class Camera
{
public:
	float gate;
	float focal;
	Vec3f eye;
	Vec3f up;
	Vec3f look;
};

#endif
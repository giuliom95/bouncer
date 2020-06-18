#ifndef _CAMERA_HPP_
#define _CAMERA_HPP_

#include "math.hpp"
#include <embree3/rtcore.h>

class Camera
{
public:
	float gate;
	float focal;
	float aspect;
	Vec3f eye;
	Vec3f up;
	Vec3f look;
};

#endif
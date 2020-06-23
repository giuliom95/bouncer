#ifndef _CAMERA_HPP_
#define _CAMERA_HPP_

#include "math.hpp"
#include <embree3/rtcore.h>

class Camera
{
public:
	inline Camera() {}

	Camera(
		const float vertical_gate_size,
		const float focal_lenght,
		const float aspect_ratio,
		const Vec3f eye_positon,
		const Vec3f look_direction,
		const Vec3f up_vector
	);

	// Ray from film coords
	RTCRay generate_ray (const Vec2f ij);

private:
 	float gate;
	float focal;
	float aspect;
	Mat4f mat;
};

#endif
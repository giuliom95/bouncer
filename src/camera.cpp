#include "camera.hpp"

Camera::Camera(
	const float vertical_gate_size,
	const float focal_lenght,
	const float aspect_ratio,
	const Vec3f eye_positon,
	const Vec3f look_direction,
	const Vec3f up_vector
) {
	gate = vertical_gate_size * INCH_TO_CM;
	focal = focal_lenght;
	aspect = aspect_ratio;

	const Vec3f y = normalize(up_vector);
	const Vec3f z = normalize(-1*look_direction);
	const Vec3f x = cross(y, z);
	mat = Mat4f(x, y, z, eye_positon);
}

/*
 * Center of the film is (0,0).
 * Lower-left corner is (-1,-1).
 * Upper-right is (1,1).
 */
RTCRay Camera::generate_ray(const Vec2f ij)
{
	RTCRay r;
	r.tnear = 0;
	r.tfar  = INFINITY;
	r.time  = 0;
	r.mask  = 0;
	r.id    = 0;
	r.flags = 0;

	// r.org_x = eye[0];
	// r.org_y = eye[1];
	// r.org_z = eye[2];

	return r;
}
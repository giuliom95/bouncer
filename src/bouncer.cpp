#include "scene.hpp"

#include <xmmintrin.h>
#include <pmmintrin.h>
#include <embree3/rtcore.h>

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

void render_roi(Scene& scene, const OIIO::ROI roi)
{
	RTCIntersectContext intersect_context;
	rtcInitIntersectContext(&intersect_context);
	
	Rand rand;
	
	const int pixel_samples = 8;
	
	for
	(
		OIIO::ImageBuf::Iterator<float> it(scene.out_image, roi); 
		!it.done(); ++it
	) {
		Vec3f c{};
		for(int s = 0; s < pixel_samples; ++s)
		{
			const Vec2f pixel_ij{rand(),        rand()};
			const Vec2f xy      {(float)it.x(), (float)it.y()}; 
			const Vec2f uv = scene.film_space(xy, pixel_ij);
			
			RTCRayHit rh{scene.camera.generate_ray(uv), {}};
			rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;

			rtcIntersect1(scene.embree_scene, &intersect_context, &rh);

			if(rh.hit.geomID != RTC_INVALID_GEOMETRY_ID)
			{
				RTCGeometry geom = rtcGetGeometry
				(
					scene.embree_scene, rh.hit.geomID
				);
				Vec3f dpdu, dpdv;
				RTCInterpolateArguments ia;
				ia.geometry		= geom;
				ia.primID		= rh.hit.primID;
				ia.u			= rh.hit.u;
				ia.v			= rh.hit.v;
				ia.bufferType	= RTC_BUFFER_TYPE_VERTEX;
				ia.bufferSlot	= 0;
				ia.P			= nullptr;
				ia.dPdu			= (float*)(&dpdu);
				ia.dPdv			= (float*)(&dpdv);
				ia.ddPdudu		= nullptr;
				ia.valueCount	= 3;
				rtcInterpolate(&ia);
				Vec3f n = normalize(cross(dpdu, dpdv));
				// Debug lambertian shading
				const Material mat = scene.materials[rh.hit.geomID];
				const Vec3f v = 
					(max(dot(n, normalize({0.5,1.2,-.8})), 0) + .2f) * 
					mat.albedo;
				c = c + v;
			}
		}
		c = c / pixel_samples;
		it[0] = c[0];
		it[1] = c[1];
		it[2] = c[2];
	}
}

void rt(Scene& scene)
{
	const unsigned int nthreads = std::thread::hardware_concurrency();
	std::vector<std::thread> threads(nthreads);
	const Vec2f roi_size
	{
		scene.out_image.size[0] / nthreads,
		scene.out_image.size[1]
	};
	for(int ti = 0; ti < nthreads; ++ti)
	{
		const Vec2f begin{scene.out_image.begin[0] + ti*roi_size[0], 0};
		const Vec2f end  {begin + roi_size};
		OIIO::ROI roi(begin[0], end[0], begin[1], end[1]);
		threads[ti] = std::thread
		(
			&render_roi, 
			std::ref(scene), roi
		);
	}

	for(int ti = 0; ti < nthreads; ++ti) threads[ti].join();
}

int main()
{
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
	_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

	RTCDevice embree_device = rtcNewDevice("verbose=3");

	Scene scene(
		"../scenes/cornellbox/cornellbox.json",
		//"../scenes/cube/cube.json",
		embree_device
	);
	rt(scene);
	scene.out_image.write("test.exr");

	rtcReleaseDevice(embree_device);
}
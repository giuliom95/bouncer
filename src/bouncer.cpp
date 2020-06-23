#include "scene.hpp"

#include <xmmintrin.h>
#include <pmmintrin.h>
#include <embree3/rtcore.h>

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>

#include <boost/log/trivial.hpp>

#include <random>
#include <thread>


Vec2f film_space(
	const Vec2f xy,
	const Vec2f xy_begin,
	const Vec2f xy_end,
	const Vec2f pixel_space
){
	const Vec2f image_size = xy_end - xy_begin;
	const Vec2f ij = 2.0f*(
		(xy - xy_begin) / (image_size - 1)
	) - 1.0f;
	const Vec2f flipper{1,-1};
	return flipper*ij + 2*(pixel_space / image_size);
}

void rt(OIIO::ImageBuf& image, Scene& scene)
{
	RTCIntersectContext intersect_context;
	rtcInitIntersectContext(&intersect_context);

	std::mt19937 rand;
	std::uniform_real_distribution<float> dis(0,1);

	// When handling pixel coordinates all casts from int to float 
	//  should be safe. These usually fit in a couple of floats.
	const Vec2f image_xybegin{(float)image.xbegin(), (float)image.ybegin()};
	const Vec2f image_xyend  {(float)image.xend(),   (float)image.yend()};
	const Vec2f image_size = image_xyend - image_xybegin;

	const unsigned int nthreads = std::thread::hardware_concurrency();
	std::vector<OIIO::ROI> rois(nthreads);
	const Vec2f roi_size
	{
		image_size[0] / nthreads,
		image_size[1]
	};
	for(int t = 0; t < nthreads; ++t)
	{
		const Vec2f begin{image_xybegin[0] + t*roi_size[0], 0};
		const Vec2f end  {begin + roi_size};
		rois[t] = OIIO::ROI(begin[0], end[0], begin[1], end[1]);
	}

	for(int t = 0; t < nthreads; ++t)
	{
		for(OIIO::ImageBuf::Iterator<float> it(image, rois[t]); !it.done(); ++it)
		{
			const float m = 1.0f / nthreads;
			const float v = m*(t+1);
			it[0] = v;
			it[1] = v;
			it[2] = v;
		}	
	}

	/*for(OIIO::ImageBuf::Iterator<float> it(image); !it.done(); ++it)
	{
		const int pixel_samples = 8;
		Vec3f c{};
		for(int s = 0; s < pixel_samples; ++s)
		{
			const Vec2f pixel_ij{dis(rand), dis(rand)};
			Vec2f uv = film_space(
				{(float)it.x(), (float)it.y()},
				image_xybegin, image_xyend,
				pixel_ij
			);
			
			RTCRayHit rh{scene.camera.generate_ray(uv), {}};
			rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;

			rtcIntersect1(scene.embree_scene, &intersect_context, &rh);

			if(rh.hit.geomID != RTC_INVALID_GEOMETRY_ID)
			{
				Vec3f dpdu, dpdv;
				RTCInterpolateArguments ia;
				ia.geometry		= rtcGetGeometry(scene.embree_scene, rh.hit.geomID);
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
					(max(dot(n, normalize({0.5,1.2,-.8})), 0) + .2f) * mat.albedo;
				c = c + v;
			}
		}
		c = c / pixel_samples;
		it[0] = c[0];
		it[1] = c[1];
		it[2] = c[2];
	}*/
}

int main()
{
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
	_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

	RTCDevice embree_device = rtcNewDevice("verbose=3");

	Scene scene;
	scene.load
	(
		"../scenes/cornellbox/cornellbox.json",
		//"../scenes/cube/cube.json",
		embree_device
	);

	OIIO::ImageBuf image{{1024, 1024, 3, OIIO::TypeDesc::FLOAT}};
	rt(image, scene);
	image.write("test.exr");

	rtcReleaseDevice(embree_device);
}
#include "scene.hpp"

#include <xmmintrin.h>
#include <pmmintrin.h>
#include <embree3/rtcore.h>

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>

#include <boost/log/trivial.hpp>

void rt(OIIO::ImageBuf& image, Scene& scene)
{
	RTCIntersectContext intersect_context;
	rtcInitIntersectContext(&intersect_context);

	for(OIIO::ImageBuf::Iterator<float> it(image); !it.done(); ++it)
	{
		const Vec2f ij
		{
			2.0f*(
				(float)(it.x() - image.xbegin()) / 
				(float)(image.xend() - image.xbegin() - 1)
			) - 1.0f,
			-2.0f*(
				(float)(it.y() - image.ybegin()) / 
				(float)(image.yend() - image.ybegin() - 1)
			) + 1.0f
		};
		
		RTCRayHit rh
		{
			scene.camera.generate_ray(ij),
			{}
		};
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
			float v = 0.8*max(dot(n, normalize({0.5,1.2,-.8})), 0) + 0.2f;
			it[0] = v; //n[0];
			it[1] = v; //n[1];
			it[2] = v; //n[2];
		}
	}
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
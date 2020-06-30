#include "scene.hpp"

#include <xmmintrin.h>
#include <pmmintrin.h>
#include <embree3/rtcore.h>

#include <boost/log/trivial.hpp>

#include <random>
#include <thread>

#include "gatherer.hpp"

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

RTCRay ray(const Vec3f& o, const Vec3f& d)
{
	RTCRay r;
	r.tnear = 0;
	r.tfar  = INFINITY;
	r.time  = 0;
	r.mask  = 0;
	r.id    = 0;
	r.flags = 0;

	r.org_x = o[0];
	r.org_y = o[1];
	r.org_z = o[2];

	r.dir_x = d[0];
	r.dir_y = d[1];
	r.dir_z = d[2];

	return r;
}

Vec3f hemisphere_sampling(Rand& rand, const Vec3f& n)
{
	const auto r0 = rand();
	const auto r1 = rand();
	const auto z = std::sqrt(r0);
    const auto r = std::sqrt(1 - z * z);
    const auto phi = 2 * PI * r1;

	const auto m = refFromVec(n);
	return transformVector(m, {z, r * std::cos(phi), r * std::sin(phi)});
}

bool valid(const Vec3f& li)
{
	return
	!(
		li[0] == -1 &&
		li[1] == -1 &&
		li[2] == -1
	);
}

Vec3f estimate_li
(
	Scene& scene,
	RTCRay r, 
	RTCIntersectContext* ic, 
	int bounces,
	Rand& rand,
	Path& path
) {
	RTCRayHit rh{r, {}};
	rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;

	rtcIntersect1(scene.embree_scene, ic, &rh);

	if(rh.hit.geomID != RTC_INVALID_GEOMETRY_ID)
	{
		RTCGeometry geom = rtcGetGeometry
		(
			scene.embree_scene, rh.hit.geomID
		);
		Vec3f p, dpdu, dpdv;
		RTCInterpolateArguments ia;
		ia.geometry		= geom;
		ia.primID		= rh.hit.primID;
		ia.u			= rh.hit.u;
		ia.v			= rh.hit.v;
		ia.bufferType	= RTC_BUFFER_TYPE_VERTEX;
		ia.bufferSlot	= 0;
		ia.P			= (float*)(&p);
		ia.dPdu			= (float*)(&dpdu);
		ia.dPdv			= (float*)(&dpdv);
		ia.ddPdudu		= nullptr;
		ia.valueCount	= 3;
		rtcInterpolate(&ia);
		const Vec3f n  = normalize(cross(dpdu, dpdv));
		const Vec3f t  = normalize(dpdu);
		const Vec3f bt = cross(t, n);
		const Mat4f m{t, n, bt, {}};

		path.add_point(p);

		const Material mat = scene.materials[rh.hit.geomID];
		const Vec3f kd = mat.albedo;
		const Vec3f ke = mat.emittance;

		if(bounces == 0)
		{
			return ke;
		}
		else
		{
			const Vec3f new_d = hemisphere_sampling(rand, n);
			RTCRay new_r = ray(p + 0.001f*n, new_d);
			const Vec3f li = estimate_li
			(
				scene, new_r, 
				ic, bounces-1, 
				rand, path
			);
			if(!valid(li))
			{
				return ke;
			}
			else 
			{
				const float c = abs(dot(n, new_d));
				return ke + (c*kd*li);
			}
		}
	}
	return {-1, -1, -1};
}

void render_roi(Scene& scene, const OIIO::ROI roi, RenderData& rd)
{
	RTCIntersectContext intersect_context;
	rtcInitIntersectContext(&intersect_context);
	
	Rand rand;
	
	const unsigned pixel_samples = 128;
	const unsigned bounces = 4;
	const unsigned npaths = pixel_samples * roi.npixels();
	PathsGroup paths(npaths);

	unsigned p = 0;
	for
	(
		OIIO::ImageBuf::Iterator<float> it(scene.out_image, roi); 
		!it.done(); ++it
	) {
		const Vec2f xy  {(float)it.x(), (float)it.y()}; 
		Vec3f c{};
		paths[p] = Path();
		for(unsigned s = 0; s < pixel_samples; ++s)
		{
			const Vec2f pixel_ij{rand(),        rand()};
			const Vec2f uv = scene.film_space(xy, pixel_ij);
			RTCRay r = scene.camera.generate_ray(uv);

			paths[p].add_point({r.org_x, r.org_y, r.org_z});

			const Vec3f li = estimate_li
			(
				scene, r,
				&intersect_context,
				bounces, rand, paths[p]
			);

			if(valid(li))
			{
				c = c + li;
			}
		}
		c = c / pixel_samples;
		it[0] = c[0];
		it[1] = c[1];
		it[2] = c[2];
		++p;
	}

	rd.pathgroups.push_back(paths);
}

void rt(Scene& scene)
{
	const unsigned nthreads = 1;//std::thread::hardware_concurrency();
	std::vector<std::thread> threads(nthreads);
	RenderData renderdata("./renderdata.bin");
	const Vec2f roi_size
	{
		scene.out_image.size[0] / nthreads,
		scene.out_image.size[1]
	};
	for(unsigned ti = 0; ti < nthreads; ++ti)
	{
		const Vec2f begin{scene.out_image.begin[0] + ti*roi_size[0], 0};
		const Vec2f end  {begin + roi_size};
		OIIO::ROI roi(begin[0], end[0], begin[1], end[1]);
		threads[ti] = std::thread
		(
			&render_roi, 
			std::ref(scene), roi,
			std::ref(renderdata)
		);
	}

	for(unsigned ti = 0; ti < nthreads; ++ti) threads[ti].join();
	renderdata.disk_store_all();
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
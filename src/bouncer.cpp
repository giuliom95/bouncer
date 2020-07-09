#include "bouncer.hpp"

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

Vec2f film_space(
	const Vec2f  xy,
	const Vec2f  pixel_space,
	const Image& image
){
	const Vec2f ij = 2.0f*(
		(xy - image.begin) / (image.size - 1)
	) - 1.0f;
	const Vec2f flipper{1,-1};
	return flipper*ij + 2*(pixel_space / image.size);
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

Image::Image(const OIIO::ImageSpec& spec) :
OIIO::ImageBuf(spec)
{
	begin = Vec2f({(float)xbegin(), (float)ybegin()});
	end   = Vec2f({(float)xend(),   (float)yend()});
	size  = end - begin;
}

RTCDevice initialize_embree_device()
{
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
	_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
	return rtcNewDevice("verbose=3");
}

Bouncer::Bouncer(const boost::filesystem::path& scenepath)
	: nthreads(std::thread::hardware_concurrency())
	, embree_device(initialize_embree_device()) 
	, scene(scenepath, embree_device)
	, renderdata(nthreads)
	, out_image(OIIO::ImageSpec(
		scene.render_settings.width,
		scene.render_settings.height,
		3, OIIO::TypeDesc::FLOAT
	))
{}

Bouncer::~Bouncer()
{
	rtcReleaseDevice(embree_device);
}

void Bouncer::render()
{
	std::vector<std::thread> threads(nthreads);
	const Vec2f roi_size
	{
		out_image.size[0] / nthreads,
		out_image.size[1]
	};
	for(unsigned ti = 0; ti < nthreads; ++ti)
	{
		const Vec2f begin{out_image.begin[0] + ti*roi_size[0], 0};
		const Vec2f end  {begin + roi_size};
		OIIO::ROI roi(begin[0], end[0], begin[1], end[1]);
		threads[ti] = std::thread
		(
			&Bouncer::render_roi, 
			this, roi, ti
		);
	}

	for(unsigned ti = 0; ti < nthreads; ++ti) threads[ti].join();
	renderdata.disk_store_all("./renderdata");
}

void Bouncer::writeimage(const boost::filesystem::path& imagepath)
{
	out_image.write(imagepath.string());
}

void Bouncer::render_roi(
	const OIIO::ROI roi, 
	const unsigned thread_id
) {
	RTCIntersectContext intersect_context;
	rtcInitIntersectContext(&intersect_context);
	
	Rand rand;
	
	const unsigned pixel_samples = scene.render_settings.spp;
	const unsigned bounces = 4;

	for
	(
		OIIO::ImageBuf::Iterator<float> it(out_image, roi); 
		!it.done(); ++it
	) {
		const Vec2f xy  {(float)it.x(), (float)it.y()}; 
		Vec3f c{};
		for(unsigned s = 0; s < pixel_samples; ++s)
		{
			const Vec2f pixel_ij{rand(),        rand()};
			const Vec2f uv = film_space(xy, pixel_ij, out_image);
			RTCRay r = scene.camera.generate_ray(uv);

			Path<Vec3h> p = Path<Vec3h>();
			p.add_point(fromVec3f({r.org_x, r.org_y, r.org_z}));

			const Vec3f li = estimate_li
				(r, &intersect_context, bounces, p, rand);

			renderdata.pathgroups[thread_id].push_back(p);

			if(valid(li))
			{
				c = c + li;
			}
		}
		c = c / pixel_samples;
		it[0] = c[0];
		it[1] = c[1];
		it[2] = c[2];
	}
}

Vec3f Bouncer::estimate_li
(
	RTCRay r, 
	RTCIntersectContext* ic, 
	int bounces,
	Path<Vec3h>& path,
	Rand& rand
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

		path.add_point(fromVec3f(p));

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
			const Vec3f li = estimate_li(new_r, ic, bounces-1, path, rand);
			
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

int main()
{
	Bouncer b("../scenes/cornellbox/cornellbox.json");
	b.render();
	b.writeimage("test.exr");
}
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
	Vec2f fs = (xy + pixel_space) / image.size;
	return{fs[0], (1-fs[1])};
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
	, gatherer(nthreads, "./renderdata")
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

	BOOST_LOG_TRIVIAL(info) << "Render thread #" << thread_id << " started";

	for
	(
		OIIO::ImageBuf::Iterator<float> it(out_image, roi); 
		!it.done(); ++it
	) {
		const Vec2f xy  {(float)it.x(), (float)it.y()}; 
		Vec3f c{};
		for(unsigned s = 0; s < pixel_samples; ++s)
		{
			const Vec2f pixel_uv{rand(),        rand()};
			const Vec2f uv = film_space(xy, pixel_uv, out_image);
			RTCRay r = scene.camera.generate_ray(uv);

			//BOOST_LOG_TRIVIAL(info) << xy[0] << " " << xy[1] << " | " << pixel_uv[0] << " " << pixel_uv[1] << " | " << uv[0] << " " << uv[1];

			const Vec3f li = estimate_li
				(r, &intersect_context, bounces, rand, thread_id);

			gatherer.finalizepath(
				thread_id, 
				fromVec3f(li),
				{
					(uint16_t)xy[0], (uint16_t)xy[1],
					(half)pixel_uv[0], (half)pixel_uv[1]
				}
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
	}
}

Vec3f Bouncer::estimate_li
(
	RTCRay r, 
	RTCIntersectContext* ic, 
	int bounces,
	Rand& rand,
	const unsigned thread_id
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
		const Mat4f n_mat{t, bt, n, {}};

		gatherer.addbounce(
			thread_id, 
			fromVec3f({r.org_x, r.org_y, r.org_z})
		);

		const Material mat = scene.materials[rh.hit.geomID];
		const Vec3f kd = mat.albedo;
		const Vec3f ke = mat.emittance;

		if(bounces == 0)
		{
			return ke;
		}
		else
		{
			const Vec3f o = hemisphere_sampling(rand, n);
			RTCRay new_r = ray(p + 0.001f*n, o);
			const Vec3f li = estimate_li(
				new_r, ic, bounces-1, rand, thread_id
			);
			
			if(!valid(li))
			{
				return ke;
			}
			else 
			{
				const float alpha_g = 1.0f;
				const float ior_t = 1.2f;
				const float ior_i = 1.0f;

				const float xi_1 = rand();
				const float xi_2 = rand();

				const float alpha2_g = alpha_g*alpha_g;

				const float theta_m_num = alpha_g * sqrt(xi_1);
				const float theta_m_den = sqrt(1 - xi_1);
				const float theta_m = atan(theta_m_num / theta_m_den);
				const float phi_m = 2*PI*xi_2;
				const float sin_theta_m = sin(theta_m);
				const float cos_theta_m = cos(theta_m);
				const Vec3f m_local{
					sin_theta_m * cos(phi_m),
					sin_theta_m * sin(phi_m),
					cos_theta_m
				};
				const Vec3f m = transformVector(n_mat, m_local);

				const float lamb_term = abs(dot(n, o));

				const Vec3f i{r.dir_x, r.dir_y, r.dir_z};

				const float i_dot_n = dot(i, n);
				const float o_dot_n = dot(o, n);
				const float m_dot_n = dot(m, n);
				const float i_dot_m = dot(i, m);


				const float f_c = abs(i_dot_m);
				const float ior_frac = (ior_t*ior_t) / (ior_i*ior_i);
				const float f_g = sqrt(ior_frac - 1 + f_c*f_c);
				const float g_min_c = f_g - f_c;
				const float g_plu_c = f_g + f_c;
				const float g_min_c2 = g_min_c*g_min_c;
				const float g_plu_c2 = g_plu_c*g_plu_c;
				const float f_mult1 = g_min_c2/g_plu_c2;
				const float f_mult2_num1 = (f_c*g_plu_c - 1);
				const float f_mult2_num = f_mult2_num1*f_mult2_num1;
				const float f_mult2_den1 = (f_c*g_min_c + 1);
				const float f_mult2_den = f_mult2_den1*f_mult2_den1;
				const float f_mult2 = 1 + (f_mult2_num / f_mult2_den);
				const float f = 0.5f * f_mult1 * f_mult2;
				/*const float f_u = dot(i, o);
				const float m1_f_u = 1 - f_u;
				const float m1_f_u2 = m1_f_u * m1_f_u;
				const float m1_f_u5 = m1_f_u2 * m1_f_u2 * m1_f_u;
				const float f_lambda = 0.7f;
				const float f = f_lambda + (1-f_lambda)*m1_f_u5;*/


				const float d_num = alpha2_g*(m_dot_n > 0);
				const float cos2_theta_m = cos_theta_m*cos_theta_m;
				const float cos4_theta_m = cos2_theta_m*cos2_theta_m;
				const float tan_theta_m = sin_theta_m / cos_theta_m;
				const float tan2_theta_m = tan_theta_m*tan_theta_m;
				const float alpha2_tan2 = alpha2_g + tan2_theta_m;
				const float alpha2_tan22 = alpha2_tan2*alpha2_tan2;
				const float d_den = PI*cos4_theta_m*alpha2_tan22;
				const float d = d_num / d_den;


				float g_i = 0;
				const float g_i_step = (i_dot_m / i_dot_n) > 0;
				if(g_i_step > 0)
				{
					const float theta_i = acos(i_dot_n);
					const float tan_theta_i = tan(theta_i);
					const float tan2_theta_i = tan_theta_i*tan_theta_i;
					const float g_i_den = 1 + sqrt(1 + alpha2_g*tan2_theta_i);
					g_i = 2 / g_i_den;
				}

				float g_o = 0;
				const float g_o_step = (dot(o, m) / o_dot_n) > 0;
				if(g_o_step > 0)
				{
					const float theta_o = acos(o_dot_n);
					const float tan_theta_o = tan(theta_o);
					const float tan2_theta_o = tan_theta_o*tan_theta_o;
					const float g_o_den = 1 + sqrt(1 + alpha2_g*tan2_theta_o);
					g_o = 2 / g_o_den;
				}

				const float g = g_i*g_o;


				const float brdf = (g*d)/(4*abs(i_dot_n)*abs(o_dot_n));
				return ke + (lamb_term*(kd + PI*f*brdf*kd)*li);
			}
		}
	}
	return {-1, -1, -1};
}

int main()
{
	Bouncer b("../scenes/boxbunny/boxbunny.json");
	b.render();
	b.writeimage("boxbunny.exr");
}
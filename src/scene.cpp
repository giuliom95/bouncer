#include "scene.hpp"

void load_buffer(
			RTCGeometry		embree_geom,
			std::ifstream&	buffers_file,
	const	size_t			element_size,
	const	size_t			buffer_size,
	const	RTCBufferType	embree_buf_type,
	const	RTCFormat		embree_data_format
) {
	void* embree_buf = rtcSetNewGeometryBuffer
	(
		embree_geom, embree_buf_type, 0,
		embree_data_format, element_size, buffer_size / element_size
	);
	buffers_file.read((char*)embree_buf, buffer_size);
}

Camera load_camera(const nlohmann::json& json_camera)
{
	BOOST_LOG_TRIVIAL(info) << "Loading camera";
	return
	{
		json_camera["gate"],
		json_camera["focal"],
		json_camera["aspect"],
		{
			json_camera["eye"][0],
			json_camera["eye"][1],
			json_camera["eye"][2]
		},
		{
			json_camera["look"][0],
			json_camera["look"][1],
			json_camera["look"][2]
		},
		{
			json_camera["up"][0],
			json_camera["up"][1],
			json_camera["up"][2]
		}
	};
}

Material load_material(const nlohmann::json& json_material)
{
	const nlohmann::json& json_albedo{json_material["albedo"]};
	const nlohmann::json& json_emittance{json_material["emittance"]}; 
	return {
		{
			json_albedo[0],
			json_albedo[1],
			json_albedo[2]
		},
		{
			json_emittance[0],
			json_emittance[1],
			json_emittance[2]
		}
	};
}

Image load_render_info(const nlohmann::json& json_render_info)
{
	return Image(
		{
			json_render_info["width"],
			json_render_info["height"],
			3, OIIO::TypeDesc::FLOAT
		}
	);
}

Image::Image(const OIIO::ImageSpec& spec) :
OIIO::ImageBuf(spec)
{
	begin = Vec2f({(float)xbegin(), (float)ybegin()});
	end   = Vec2f({(float)xend(),   (float)yend()});
	size  = end - begin;
}

Scene::~Scene()
{
	BOOST_LOG_TRIVIAL(info) << "Releasing scene";
	rtcReleaseScene(embree_scene);
}

Scene::Scene(
	const boost::filesystem::path& json_path, 
	RTCDevice& embree_device
) {
	embree_scene = rtcNewScene(embree_device);

	nlohmann::json json_data;
	std::ifstream json_file{json_path.string()};
	if(!json_file) 
	{
		BOOST_LOG_TRIVIAL(fatal) <<
			"Could not open \"" << json_path.string() << "\"";
		throw std::runtime_error("Could not open scene file");
	}
	json_file >> json_data;
	json_file.close();


	boost::filesystem::path buffers_file_path = json_path;
	buffers_file_path.replace_extension(".bin");
	std::ifstream buffers_file{buffers_file_path.string()};
	if(!buffers_file) 
	{
		BOOST_LOG_TRIVIAL(fatal) <<
			"Could not open \"" << buffers_file_path.string() << "\"";
		throw std::runtime_error("Could not open scene buffers file");
	}

	out_image = load_render_info(json_data["render"]);
	
	camera = load_camera(json_data["camera"]);

	for(const nlohmann::json json_geom : json_data["geometries"]) 
	{
		BOOST_LOG_TRIVIAL(info) << "Found geometry";

		const RTCGeometry embree_geom = rtcNewGeometry
		(
			embree_device,
			RTC_GEOMETRY_TYPE_SUBDIVISION
		);

		for(const nlohmann::json json_buf : json_geom["buffers"])
		{
			std::string type = json_buf["type"];
			if(type == "faces")
			{
				BOOST_LOG_TRIVIAL(info) << "Loading faces";
				load_buffer
				(
					embree_geom, buffers_file,
					sizeof(uint32_t), (size_t)json_buf["size"], 
					RTC_BUFFER_TYPE_FACE, RTC_FORMAT_UINT
				);
			}
			else if(type == "indices")
			{
				BOOST_LOG_TRIVIAL(info) << "Loading indices";
				load_buffer
				(
					embree_geom, buffers_file,
					sizeof(uint32_t), (size_t)json_buf["size"], 
					RTC_BUFFER_TYPE_INDEX, RTC_FORMAT_UINT
				);
			}
			else if(type == "vertices")
			{
				BOOST_LOG_TRIVIAL(info) << "Loading vertices";
				load_buffer
				(
					embree_geom, buffers_file,
					3*sizeof(float), (size_t)json_buf["size"], 
					RTC_BUFFER_TYPE_VERTEX, RTC_FORMAT_FLOAT3
				);
			}
			else if(type == "creaseindices")
			{
				BOOST_LOG_TRIVIAL(info) << "Loading crease indices";
				load_buffer
				(
					embree_geom, buffers_file,
					2*sizeof(uint32_t), (size_t)json_buf["size"], 
					RTC_BUFFER_TYPE_EDGE_CREASE_INDEX, RTC_FORMAT_UINT2
				);
			}
			else if(type == "creasevalues")
			{
				BOOST_LOG_TRIVIAL(info) << "Loading crease values";
				load_buffer
				(
					embree_geom, buffers_file,
					sizeof(float), (size_t)json_buf["size"], 
					RTC_BUFFER_TYPE_EDGE_CREASE_WEIGHT, RTC_FORMAT_FLOAT
				);
			}
		}

		BOOST_LOG_TRIVIAL(info) << "Setting subdivision level";
		const bool is_smooth = json_geom["smooth"];
		if(is_smooth)
		{
			rtcSetGeometryTessellationRate(embree_geom, 6);

			rtcSetGeometrySubdivisionMode
			(
				embree_geom, 0,
				RTC_SUBDIVISION_MODE_PIN_CORNERS
			);
		}
		else
		{
			rtcSetGeometryTessellationRate(embree_geom, 0);

			rtcSetGeometrySubdivisionMode
			(
				embree_geom, 0,
				RTC_SUBDIVISION_MODE_PIN_ALL
			);
		}

		const Material mat = load_material(json_geom["material"]);
		materials.push_back(mat);

		BOOST_LOG_TRIVIAL(info) << "Committing geometry";
		rtcCommitGeometry(embree_geom);
		BOOST_LOG_TRIVIAL(info) << "Attaching geometry";
		rtcAttachGeometry(embree_scene, embree_geom);
		BOOST_LOG_TRIVIAL(info) << "Releasing geometry";
		rtcReleaseGeometry(embree_geom);
	}

	BOOST_LOG_TRIVIAL(info) << "Committing scene";
	rtcCommitScene(embree_scene);
}

Vec2f Scene::film_space(
	const Vec2f xy,
	const Vec2f pixel_space
){
	const Vec2f ij = 2.0f*(
		(xy - out_image.begin) / (out_image.size - 1)
	) - 1.0f;
	const Vec2f flipper{1,-1};
	return flipper*ij + 2*(pixel_space / out_image.size);
}
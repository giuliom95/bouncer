#include "scene.hpp"

void load_buffer(
			RTCGeometry 				embree_geom,
	const 	nlohmann::json& 			json, 
	const 	boost::filesystem::path&	parent_path,
	const	size_t						element_size,
	const	RTCBufferType				embree_buf_type,
	const	RTCFormat					embree_data_format
) {
	int						amount		= json["amount"];
	boost::filesystem::path	file_path	= json["path"];
	if(file_path.is_relative())
		file_path = parent_path / file_path;

	BOOST_LOG_TRIVIAL(info) <<
		"Loading " << amount << "(*" << element_size << 
		" bytes) elements from \"" << file_path.string() << "\"";

	void* rtc_buf = rtcSetNewGeometryBuffer
	(
		embree_geom, embree_buf_type, 0,
		embree_data_format, element_size, amount
	);
	
	std::ifstream infile{file_path.string(), std::ios::binary};
	if(!infile)
	{
		BOOST_LOG_TRIVIAL(fatal) <<
			"Could not open \"" << file_path.string() << "\"";
		throw std::runtime_error("Could not open buffer file");
	}
	infile.read((char*)rtc_buf, element_size*amount);
	infile.close();
	
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


Scene::~Scene()
{
	BOOST_LOG_TRIVIAL(info) << "Releasing scene";
	rtcReleaseScene(embree_scene);
}

void Scene::load(
	const boost::filesystem::path& json_path, 
	RTCDevice& embree_device
) {
	embree_scene = rtcNewScene(embree_device);

	const boost::filesystem::path parent_path = json_path.parent_path();
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

	camera = load_camera(json_data["camera"]);

	for(const nlohmann::json json_geom : json_data["geometries"]) 
	{
		BOOST_LOG_TRIVIAL(info) << "Found geometry";

		const RTCGeometry embree_geom = rtcNewGeometry
		(
			embree_device,
			RTC_GEOMETRY_TYPE_SUBDIVISION
		);

		BOOST_LOG_TRIVIAL(info) << "Loading indices";
		load_buffer
		(
			embree_geom, json_geom["indices"],
			parent_path, sizeof(uint32_t),
			RTC_BUFFER_TYPE_INDEX, RTC_FORMAT_UINT
		);

		BOOST_LOG_TRIVIAL(info) << "Loading vertices";
		load_buffer
		(
			embree_geom, json_geom["vertices"],
			parent_path, 3*sizeof(float),
			RTC_BUFFER_TYPE_VERTEX, RTC_FORMAT_FLOAT3
		);

		BOOST_LOG_TRIVIAL(info) << "Loading faces";
		load_buffer
		(
			embree_geom, json_geom["faces"],
			parent_path, sizeof(uint32_t),
			RTC_BUFFER_TYPE_FACE, RTC_FORMAT_UINT
		);

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

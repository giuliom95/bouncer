#include "scene.hpp"

void load_buffer(
			RTCGeometry 			embree_geom,
	const 	nlohmann::json& 		json, 
	const 	std::filesystem::path& 	parent_path,
	const	size_t					element_size,
	const	RTCBufferType			embree_buf_type,
	const	RTCFormat				embree_data_format
) {
	int						amount		= json["amount"];
	std::filesystem::path	file_path	= json["path"];
	if(file_path.is_relative())
		file_path = parent_path / file_path;

	spdlog::info
	(
		"Loading {}(*{} bytes) elements from \"{}\"", 
		amount, element_size, file_path.string()
	);

	void* rtc_buf = rtcSetNewGeometryBuffer
	(
		embree_geom, embree_buf_type, 0,
		embree_data_format, element_size, amount
	);
	
	std::ifstream infile{file_path, std::ios::binary};
	if(!infile)
	{
		spdlog::error("Could not open \"{}\"", file_path.string());
		throw std::runtime_error("Could not open buffer file");
	}
	infile.read((char*)rtc_buf, element_size*amount);
	infile.close();
	
}

Scene::~Scene()
{
	spdlog::info("Releasing scene");
	rtcReleaseScene(embree_scene);
}

void Scene::load(
	const std::filesystem::path& json_path, 
	RTCDevice& embree_device
) {
	embree_scene = rtcNewScene(embree_device);

	const std::filesystem::path parent_path = json_path.parent_path();
	nlohmann::json json_data;
	std::ifstream json_file{json_path};
	if(!json_file) 
	{
		spdlog::error("Could not open \"{}\"", json_path.string());
		throw std::runtime_error("Could not open scene file");
	}
	json_file >> json_data;
	json_file.close();


	for(const nlohmann::json json_geom : json_data["geometries"]) 
	{

		spdlog::info("Found geometry");

		const RTCGeometry embree_geom = rtcNewGeometry
		(
			embree_device,
			RTC_GEOMETRY_TYPE_SUBDIVISION
		);

		spdlog::info("Loading indices");
		load_buffer
		(
			embree_geom, json_geom["indices"],
			parent_path, sizeof(uint32_t),
			RTC_BUFFER_TYPE_INDEX, RTC_FORMAT_UINT
		);

		spdlog::info("Loading vertices");
		load_buffer
		(
			embree_geom, json_geom["vertices"],
			parent_path, 3*sizeof(float),
			RTC_BUFFER_TYPE_VERTEX, RTC_FORMAT_FLOAT3
		);

		spdlog::info("Loading faces");
		load_buffer
		(
			embree_geom, json_geom["faces"],
			parent_path, sizeof(uint32_t),
			RTC_BUFFER_TYPE_FACE, RTC_FORMAT_UINT
		);

		spdlog::info("Committing geometry");
		rtcCommitGeometry(embree_geom);
		spdlog::info("Attaching geometry");
		rtcAttachGeometry(embree_scene, embree_geom);
		spdlog::info("Releasing geometry");
		rtcReleaseGeometry(embree_geom);
	}

	spdlog::info("Committing scene");
	rtcCommitScene(embree_scene);
}

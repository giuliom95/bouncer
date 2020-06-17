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

	std::cout << logger::infotag() <<
		"Loading " << amount << "(*" << element_size << 
		" bytes) elements from \"" << file_path.string() << "\"" <<
	std::endl;

	void* rtc_buf = rtcSetNewGeometryBuffer
	(
		embree_geom, embree_buf_type, 0,
		embree_data_format, element_size, amount
	);
	
	std::ifstream infile{file_path, std::ios::binary};
	if(!infile)
	{
		std::cout << logger::errortag <<
			"Could not open \"" << file_path.string() << "\"" <<
		std::endl;
		throw std::runtime_error("Could not open buffer file");
	}
	infile.read((char*)rtc_buf, element_size*amount);
	infile.close();
	
}

Camera load_camera(const nlohmann::json& json_camera)
{
	std::cout << logger::infotag() << "Loading camera" << std::endl;
	return
	{
		json_camera["gate"],
		json_camera["focal"],
		{
			json_camera["eye"][0],
			json_camera["eye"][1],
			json_camera["eye"][2]
		},
		{
			json_camera["up"][0],
			json_camera["up"][1],
			json_camera["up"][2]
		},
		{
			json_camera["look"][0],
			json_camera["look"][1],
			json_camera["look"][2]
		}
	};
}


Scene::~Scene()
{
	std::cout << logger::infotag() << "Releasing scene" << std::endl;
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
		std::cout << logger::errortag <<
			"Could not open \"" << json_path.string() << "\"" <<
		std::endl;
		throw std::runtime_error("Could not open scene file");
	}
	json_file >> json_data;
	json_file.close();

	camera = load_camera(json_data["camera"]);

	for(const nlohmann::json json_geom : json_data["geometries"]) 
	{
		std::cout << logger::infotag() << "Found geometry" << std::endl;

		const RTCGeometry embree_geom = rtcNewGeometry
		(
			embree_device,
			RTC_GEOMETRY_TYPE_SUBDIVISION
		);

		std::cout << logger::infotag() << "Loading indices" << std::endl;
		load_buffer
		(
			embree_geom, json_geom["indices"],
			parent_path, sizeof(uint32_t),
			RTC_BUFFER_TYPE_INDEX, RTC_FORMAT_UINT
		);

		std::cout << logger::infotag() << "Loading vertices" << std::endl;
		load_buffer
		(
			embree_geom, json_geom["vertices"],
			parent_path, 3*sizeof(float),
			RTC_BUFFER_TYPE_VERTEX, RTC_FORMAT_FLOAT3
		);

		std::cout << logger::infotag() << "Loading faces" << std::endl;
		load_buffer
		(
			embree_geom, json_geom["faces"],
			parent_path, sizeof(uint32_t),
			RTC_BUFFER_TYPE_FACE, RTC_FORMAT_UINT
		);

		std::cout << logger::infotag() << "Committing geometry" << std::endl;
		rtcCommitGeometry(embree_geom);
		std::cout << logger::infotag() << "Attaching geometry" << std::endl;
		rtcAttachGeometry(embree_scene, embree_geom);
		std::cout << logger::infotag() << "Releasing geometry" << std::endl;
		rtcReleaseGeometry(embree_geom);
	}

	std::cout << logger::infotag() << "Committing scene" << std::endl;
	rtcCommitScene(embree_scene);
}

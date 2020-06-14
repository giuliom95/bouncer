#include "sceneloader.hpp"

void load_buffer(
			RTCGeometry 			rtc_geom,
	const 	nlohmann::json& 		json, 
	const 	std::filesystem::path& 	parent_path,
	const	size_t					element_size,
	const	RTCBufferType			rtc_buf_type,
	const	RTCFormat				rtc_data_format
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
		rtc_geom, rtc_buf_type, 0,
		rtc_data_format, element_size, amount
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

RTCScene load_VSSD_embree(
	const std::filesystem::path& json_path, 
	RTCDevice& rtc_device
) {
	RTCScene rtc_scene	= rtcNewScene(rtc_device);

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

		const RTCGeometry rtc_geom = rtcNewGeometry
		(
			rtc_device,
			RTC_GEOMETRY_TYPE_SUBDIVISION
		);

		spdlog::info("Loading indices");
		load_buffer
		(
			rtc_geom, json_geom["indices"],
			parent_path, sizeof(uint32_t),
			RTC_BUFFER_TYPE_INDEX, RTC_FORMAT_UINT
		);

		spdlog::info("Loading vertices");
		load_buffer
		(
			rtc_geom, json_geom["vertices"],
			parent_path, 3*sizeof(float),
			RTC_BUFFER_TYPE_VERTEX, RTC_FORMAT_FLOAT3
		);

		spdlog::info("Loading faces");
		load_buffer
		(
			rtc_geom, json_geom["faces"],
			parent_path, sizeof(uint32_t),
			RTC_BUFFER_TYPE_FACE, RTC_FORMAT_UINT
		);

		spdlog::info("Committing geometry");
		rtcCommitGeometry(rtc_geom);
		spdlog::info("Attaching geometry");
		rtcAttachGeometry(rtc_scene, rtc_geom);
		spdlog::info("Releasing geometry");
		rtcReleaseGeometry(rtc_geom);
	}

	spdlog::info("Committing scene");
	rtcCommitScene(rtc_scene);
	return rtc_scene;
}

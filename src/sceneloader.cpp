#include "sceneloader.hpp"

template <typename T>
std::vector<T> load_buffer(
	const nlohmann::json& json, 
	const std::filesystem::path& parent_path
) {
	const int 				buf_size	= json["amount"];
	std::filesystem::path	file_path	= json["path"];
	if(file_path.is_relative())
		file_path = parent_path / file_path;

	spdlog::info("Reading {} elements from \"{}\"...", buf_size, file_path.string());

	std::vector<uint32_t> data(buf_size);
	std::ifstream infile{file_path, std::ios::binary};
	if(!infile) {
		spdlog::error("Could not open \"{}\"", file_path.string());
		throw std::runtime_error("Could not open buffer file");
	}
	infile.read((char*)data.data(), buf_size*sizeof(uint32_t));
	infile.close();
	spdlog::info("Done reading buffer");

	return data;
}

bool load_VSSD_embree(const std::filesystem::path& path, RTCScene& scene) {
	const std::filesystem::path parent_path = path.parent_path();
	nlohmann::json json_data;
	std::ifstream json_file{path};
	if(!json_file) {
		spdlog::error("Could not open \"{}\"", path.string());
		return false;
	}
	json_file >> json_data;
	json_file.close();


	for(const nlohmann::json json_geom : json_data["geometries"]) {

		spdlog::info("Found geometry");

		std::vector<uint32_t> idxs = load_buffer<uint32_t>(json_geom["indices"], parent_path);
		
	}

	return true;
}

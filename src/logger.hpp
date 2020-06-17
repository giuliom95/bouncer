#ifndef _LOGGER_HPP_
#define _LOGGER_HPP_

#include <iomanip>
#include <string>
#include <chrono>

namespace logger
{
	inline std::string msgtag(const std::string tag)
	{
		const auto now     = std::chrono::system_clock::now();
		const auto now_t   = std::chrono::system_clock::to_time_t(now);
		const auto now_fmt = std::put_time(std::localtime(&now_t), "%c");
		std::stringstream sstm;
		sstm << now_fmt << " [" << tag << "]: ";
		return sstm.str();
	}

	inline std::string infotag() 
	{
		return msgtag("info");
	}

	inline std::string errortag() 
	{
		return msgtag("error");
	}
}

#endif
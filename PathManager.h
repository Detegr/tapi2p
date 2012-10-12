#pragma once
#include <string>

class PathManager
{
	private:
		static std::string m_BasePath;
	public:
		static const std::string ConfigPath();
		static const std::string KeyPath();
		static const std::string SelfKeyPath();
		static void Setup(const std::string&);
};

#pragma once
#include <string>
#include "Config.h"

class PathManager
{
	private:
		static Config*		m_Config;
		static std::string	m_BasePath;
	public:
		static Config&				GetConfig();
		static void					Destroy();
		static const std::string 	ConfigPath();
		static const std::string 	KeyPath();
		static const std::string 	SelfKeyPath();
		static void					Setup(const std::string&);
};

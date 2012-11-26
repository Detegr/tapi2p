#pragma once
#include <string>
#include "config.h"

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
		static const std::string	SocketPath();
		static void					Setup(const std::string&);
};

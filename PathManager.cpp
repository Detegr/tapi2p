#include "PathManager.h"

Config* PathManager::m_Config;
std::string PathManager::m_BasePath;

void PathManager::Setup(const std::string& basepath)
{
	m_BasePath=basepath;
}
const std::string PathManager::ConfigPath() { return m_BasePath + "/config"; }
const std::string PathManager::KeyPath() { return m_BasePath + "/keys"; }
const std::string PathManager::SelfKeyPath() { return KeyPath() + "/self"; }
Config& PathManager::GetConfig()
{
	if(!m_Config)
	{
		m_Config = new Config(ConfigPath());
	}
	return *m_Config;
}

void PathManager::Destroy()
{
	delete m_Config;
}

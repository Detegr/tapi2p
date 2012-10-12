#include "PathManager.h"

std::string PathManager::m_BasePath;

void PathManager::Setup(const std::string& basepath)
{
	m_BasePath=basepath;
}
const std::string PathManager::ConfigPath() { return m_BasePath + "/config"; }
const std::string PathManager::KeyPath() { return m_BasePath + "/keys"; }
const std::string PathManager::SelfKeyPath() { return KeyPath() + "/self"; }

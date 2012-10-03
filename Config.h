#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <unistd.h>

class ConfigItem
{
	friend class Config;
	private:
		std::string m_Section;
		std::string m_Data;
		std::string m_Value;
		ConfigItem(const std::string& s, const std::string& d) : m_Section(s), m_Data(d) {}
		ConfigItem(const std::string& s, const std::string& d, const std::string& v) : m_Section(s), m_Data(d), m_Value(v) {}
	public:
		bool operator<(const ConfigItem& ConfigItem) const;
		std::string Key() const { return m_Data; }
		std::string Value() const { return m_Value; }
};
class Config
{
	private:
		std::string m_FileName;
		std::vector<ConfigItem> m_ConfigItems;
		void M_Read();
		void M_Parse(std::string& s, std::string* cs);
		std::string& M_Trim(std::string& s);
	public:
		Config(const std::string& f, bool replace=false);
		void Set(const std::string& section, const std::string& data);
		void Set(const std::string& section, const std::string& data, const std::string& value);
		std::string Get(const std::string& section, const std::string& data) const;
		std::vector<ConfigItem> Get(const std::string& section) const;
		void Flush();
		unsigned int Size() const;
};

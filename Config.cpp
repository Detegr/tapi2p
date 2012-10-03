#include "Config.h"

Config::Config(const std::string& f, bool replace) : m_FileName(f)
{
	std::ifstream file(m_FileName.c_str());
	if(file.is_open())
	{
		file.close();
		if(replace)
		{
			unlink(m_FileName.c_str());
		}
		else M_Read();
	}
}
void Config::M_Read()
{
	std::ifstream f(m_FileName.c_str());
	if(f.is_open()) // Redundant check because of the constructor, but just in case
	{
		std::string s;
		std::string currentsection;
		while(f.good())
		{
			std::getline(f,s);
			M_Parse(s, &currentsection);
		}
	}
}

std::string& Config::M_Trim(std::string& s)
{
	const char* whitespace=" \t";
	s.erase(0, s.find_first_not_of(whitespace));
	s.erase(s.find_last_not_of(whitespace)+1);
	return s;
}

void Config::M_Parse(std::string& s, std::string* cs)
{
	if(!s.size()) return;
	if(s[0] == '[')
	{
		*cs=s.substr(1, s.find_last_of(']')-1);
	}
	else if(s.find('=') == std::string::npos)
	{
		m_ConfigItems.push_back(ConfigItem(*cs, M_Trim(s)));
	}
	else
	{
		size_t pos=s.find('=');
		std::string s1=s.substr(0, pos);
		std::string s2=s.substr(pos+1, s.npos);
		m_ConfigItems.push_back(ConfigItem(*cs, M_Trim(s1), M_Trim(s2)));
	}
}

bool ConfigItem::operator<(const ConfigItem& o) const
{
	return m_Section < o.m_Section;
}
void Config::Set(const std::string& s, const std::string& d)
{
	for(std::vector<ConfigItem>::const_iterator it=m_ConfigItems.begin(); it!=m_ConfigItems.end(); ++it)
	{
		if(it->m_Section==s && it->m_Data==d) return;
	}
	m_ConfigItems.push_back(ConfigItem(s,d));
}

void Config::Set(const std::string& s, const std::string& d, const std::string& v)
{
	for(std::vector<ConfigItem>::const_iterator it=m_ConfigItems.begin(); it!=m_ConfigItems.end(); ++it)
	{
		if(it->m_Section==s && it->m_Data==d && it->m_Value==v) return;
	}
	m_ConfigItems.push_back(ConfigItem(s,d,v));
}

void Config::Flush()
{
	std::ofstream file(m_FileName.c_str());
	if(!file.is_open()) throw std::runtime_error("Couldn't open config file");
	std::sort(m_ConfigItems.begin(), m_ConfigItems.end());
	std::string tmp;
	for(std::vector<ConfigItem>::const_iterator it=m_ConfigItems.begin(); it!=m_ConfigItems.end(); ++it)
	{
		if(file.good())
		{
			if(it->m_Section != tmp)
			{
				if(tmp.size()) file << "\n";
				tmp=it->m_Section;
				file << "[" << it->m_Section << "]";
			}
			file << "\n";
			file << "\t" << it->m_Data;
			if(it->m_Value.size())
			{
				file << "=" << it->m_Value;
			}
		}
	}
	file << "\n";
	file.close();
}

std::string Config::Get(const std::string& section, const std::string& data) const
{
	for(std::vector<ConfigItem>::const_iterator it=m_ConfigItems.begin(); it!=m_ConfigItems.end(); ++it)
	{
		if(it->m_Section == section && it->m_Data == data)
		{
			if(it->m_Value.size()) return it->m_Value;
			else return it->m_Data;
		}
	}
	return "";
}
std::vector<ConfigItem> Config::Get(const std::string& section) const
{
	std::vector<ConfigItem> ret;
	for(std::vector<ConfigItem>::const_iterator it=m_ConfigItems.begin(); it!=m_ConfigItems.end(); ++it)
	{
		ret.push_back(*it);
	}
	return ret;
}

unsigned int Config::Size() const
{
	return m_ConfigItems.size();
}

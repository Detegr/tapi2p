#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <unistd.h>

namespace dtgconfig
{
	class Operation
	{
		friend class Config;
		private:
			std::string m_Section;
			std::string m_Data;
			std::string m_Value;
			Operation(const std::string& s, const std::string& d) : m_Section(s), m_Data(d) {}
			Operation(const std::string& s, const std::string& d, const std::string& v) : m_Section(s), m_Data(d), m_Value(v) {}
		public:
			bool operator<(const Operation& operation) const;
	};
	class Config
	{
		private:
			std::string m_FileName;
			std::vector<Operation> m_Operations;
			void M_Read();
			void M_Parse(const std::string& s, std::string* cs);
		public:
			Config(const std::string& f, bool replace=false);
			void Set(const std::string& section, const std::string& data);
			void Set(const std::string& section, const std::string& data, const std::string& value);
			void Flush();
	};

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
			while(f.is_open())
			{
				std::getline(f,s);
				M_Parse(s, &currentsection);
			}
		}
	}

	void Config::M_Parse(const std::string& s, std::string* cs)
	{
		if(s[0] == '[')
		{
			*cs=s.substr(1, s.find_last_of(']'));
		}
		else if(s.find('=') != std::string::npos)
		{
			m_Operations.push_back(Operation(*cs, s));
		}
		else
		{
			size_t pos=s.find('=');
			m_Operations.push_back(Operation(*cs, s.substr(0, pos-1), s.substr(pos+1, s.npos)));
		}
	}

	bool Operation::operator<(const Operation& o) const
	{
		return m_Section < o.m_Section;
	}

	void Config::Set(const std::string& s, const std::string& d)
	{
		m_Operations.push_back(Operation(s,d));
	}

	void Config::Set(const std::string& s, const std::string& d, const std::string& v)
	{
		m_Operations.push_back(Operation(s,d,v));
	}

	void Config::Flush()
	{
		std::ofstream file(m_FileName.c_str());
		if(!file.is_open()) throw std::runtime_error("Couldn't open config file");
		std::sort(m_Operations.begin(), m_Operations.end());
		std::string tmp;
		for(std::vector<Operation>::const_iterator it=m_Operations.begin(); it!=m_Operations.end(); ++it)
		{
			if(file.good())
			{
				if(it->m_Section != tmp)
				{
					tmp=it->m_Section;
					file << "[" << it->m_Section << "]\n";
				}
				file << "\t" << it->m_Data;
				if(it->m_Value.size())
				{
					file << "=" << it->m_Value;
				}
				file << "\n";
			}
		}
	}
}

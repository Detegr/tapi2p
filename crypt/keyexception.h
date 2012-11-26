#pragma once
#include <exception>
#include <string>

class KeyException : public std::exception
{
	private:
		std::string m_What;
	public:
		explicit KeyException()
		{
			m_What="KeyException";
		}
		explicit KeyException(const std::string& w)
		{
			m_What=w;
		}
		virtual const char* what() const throw()
		{
			return m_What.c_str();
		}
		virtual ~KeyException() throw() {}
};

#include <vector>
#include "Concurrency.h"

class PipeManager
{
	private:
		static std::vector<int>			m_FdVec;
		static dtglib::C_Mutex			m_Lock;
	public:
		static void Add(int fd);
		static void Remove(int fd);
		static void Lock();
		static void Unlock();
		static int Max();
		static const std::vector<int>& Container();
};
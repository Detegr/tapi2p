#include "pipemanager.h"
#include <algorithm>

std::vector<int>	PipeManager::m_FdVec;
dtglib::C_Mutex		PipeManager::m_Lock;

void PipeManager::Add(int fd)
{
	Lock();
	m_FdVec.push_back(fd);
	Unlock();
}

void PipeManager::Remove(int fd)
{
	Lock();
	std::vector<int>::iterator pos=std::find(m_FdVec.begin(), m_FdVec.end(), fd);
	m_FdVec.erase(pos);
	Unlock();
}
void PipeManager::Lock() { m_Lock.M_Lock(); }
void PipeManager::Unlock() { m_Lock.M_Unlock(); }
const std::vector<int>& PipeManager::Container() { return m_FdVec; }

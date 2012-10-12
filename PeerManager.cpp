#include "PeerManager.h"

C_Mutex PeerManager::m_PeerLock;
std::vector<Peer*> PeerManager::m_Peers;

void PeerManager::Add(Peer* p)
{
	C_Lock(*m_PeerLock);
	m_Peers.push_back(p);
}
void PeerManager::Remove(Peer* p)
{
	C_Lock(*m_PeerLock);
	m_Peers.erase(std::find(m_Peers.begin(), m_Peers.end(), p));
	delete p;
}
std::vector<Peer*>& PeerManager::Do()
{
	m_PeerLock.M_Lock();
	return m_Peers;
}
void PeerManager::Done()
{
	m_PeerLock.M_Unlock();
}

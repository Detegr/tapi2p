#pragma once
#include "peer.h"

class PeerManager
{
	private:
		static C_Mutex				m_PeerLock;
		static std::vector<Peer*>	m_Peers;
	public:
		static void Add(Peer* p);
		static void Remove(Peer* p);
		static std::vector<Peer*>& Do();
		static void Done();
};

#pragma once
#include "dtglib/Network.h"
#include "dtglib/Packet.h"
#include "dtglib/Concurrency.h"
#include "PublicKey.h"

using namespace dtglib;

struct Peer
{
		bool			m_Connectable;
		C_Selector		m_Selector;
		C_TcpSocket 	Sock_In;
		C_TcpSocket		Sock_Out;
		C_Packet		Packet;
		C_Thread		Thread;
		RSA_PublicKey	Key;
		Peer() : m_Connectable(true) {}
		Peer(const C_TcpSocket& s) : m_Connectable(true), Sock_In(s) {}
		~Peer()
		{
			Thread.M_Join();
			Sock_In.M_Disconnect();
			Sock_Out.M_Disconnect();
		}
};

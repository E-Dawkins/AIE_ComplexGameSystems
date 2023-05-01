#pragma once
#include "RakPeerInterface.h"

class Server
{
public:
	void Run();

protected:
	void HandleNetworkMessages();
	void SendClientPing(const char* _message);
	void SendNewClientID(RakNet::SystemAddress& _address);

	RakNet::RakPeerInterface* m_pPeerInterface;
	int m_nextClientID = 1;
};


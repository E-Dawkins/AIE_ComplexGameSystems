#include "Server.h"
#include <iostream>
#include <MessageIdentifiers.h>
#include <BitStream.h>
#include <WinUser.h>
#include "GameMessages.h"
#include <string>

void Server::Run()
{
	// The port the server is hosted on
	const unsigned short PORT = 5456;

	std::cout << "Starting up the server..." << std::endl;

	m_pPeerInterface = RakNet::RakPeerInterface::GetInstance();

	// Setup socket descriptor to describe this connection
	RakNet::SocketDescriptor sd(PORT, 0);

	// Call startup - max 32 connections, on the assigned port
	m_pPeerInterface->Startup(32, &sd, 1);
	m_pPeerInterface->SetMaximumIncomingConnections(32);

	HandleNetworkMessages();
}

void Server::HandleNetworkMessages()
{
	RakNet::Packet* packet = nullptr;

	// Constantly loop and check the packets data
	while (true)
	{
		for (packet = m_pPeerInterface->Receive(); packet;
			m_pPeerInterface->DeallocatePacket(packet),
			packet = m_pPeerInterface->Receive())
		{
			switch (packet->data[0])
			{
			case ID_NEW_INCOMING_CONNECTION:
				std::cout << "Incoming connection." << std::endl;
				SendNewClientID(packet->systemAddress);
				break;
			case ID_DISCONNECTION_NOTIFICATION:
				std::cout << "A client has disconnected." << std::endl;
				break;
			case ID_CONNECTION_LOST:
				std::cout << "A client has lost connection." << std::endl;
				break;
			case ID_CLIENT_CLIENT_DATA:
			{
				RakNet::BitStream bs(packet->data, packet->length, false);
				m_pPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED,
					0, packet->systemAddress, true);
				break;
			}
			case ID_CLIENT_DISCONNECT:
				ClientDisconnect(packet);
				break;
			default:
				std::cout << "Received a message with an unknown id: " 
					<< packet->data[0] << std::endl;
				break;
			}
		}
	}
}

void Server::SendClientPing(const char* _message)
{
	// Send _message to every client connected to the server
	RakNet::BitStream bs;
	bs.Write((RakNet::MessageID)ID_SERVER_TEXT_MESSAGE);
	bs.Write(_message);

	m_pPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0,
		RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
}

void Server::SendNewClientID(RakNet::SystemAddress& _address)
{
	RakNet::BitStream bs;
	bs.Write((RakNet::MessageID)ID_SERVER_SET_CLIENT_ID);
	bs.Write(m_nextClientID);
	m_nextClientID++;

	m_pPeerInterface->Send(&bs, HIGH_PRIORITY, 
		RELIABLE_ORDERED, 0, _address, false);
}

void Server::ClientDisconnect(RakNet::Packet* _packet)
{
	RakNet::BitStream bsIn(_packet->data, _packet->length, false);
	bsIn.IgnoreBytes(sizeof(RakNet::MessageID));

	int id;
	bsIn.Read(id);

	RakNet::BitStream bs;
	bs.Write((RakNet::MessageID)ID_CLIENT_DISCONNECT);
	bs.Write(id);

	m_pPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED,
		0, _packet->systemAddress, true);
}

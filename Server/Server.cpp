#include "Server.h"
#include <iostream>
#include <MessageIdentifiers.h>
#include <BitStream.h>
#include <WinUser.h>
#include "GameMessages.h"
#include <string>
#include <thread>
#include <functional>

void Server::Run()
{
	// The port the server is hosted on
	std::cout << "Starting up the server..." << std::endl;

	m_pPeerInterface = RakNet::RakPeerInterface::GetInstance();

	// Setup socket descriptor to describe this connection
	RakNet::SocketDescriptor sd(PORT, 0);

	// Call startup - max 32 connections, on the assigned port
	m_pPeerInterface->Startup(32, &sd, 1);
	m_pPeerInterface->SetMaximumIncomingConnections(32);

	// Bind update thread
	std::thread updateThread(std::bind(&Server::UpdateObjects, this));

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

				// Read the gameobject and store it in our list
				GameObject object = GameObject();
				object.Read(packet);
				m_gameObjects[object.id] = object;

				m_pPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED,
					0, packet->systemAddress, true);
				break;
			}
			case ID_CLIENT_DISCONNECT:
				ClientDisconnect(packet);
				break;
			case ID_CLIENT_SPAWN_BULLET:
				OnSpawnBullet(packet);
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

	// Send existing gameobjects to new client - old client/s
	for (auto it = m_gameObjects.begin(); it != m_gameObjects.end(); it++)
	{
		GameObject obj = it->second;
		obj.Write(m_pPeerInterface, _address, false);
	}

	// Send us to all other clients - new client
	int id = m_nextClientID - 1;
	GameObject obj = GameObject();
	obj.networkData.SetElement("Position", glm::vec3(0));
	obj.networkData.SetElement("Color", GameObject::GetColor(id));
	obj.networkData.SetElement("Radius", 1.f);
	obj.id = id;
	obj.Write(m_pPeerInterface, _address, true);

	m_gameObjects[id] = obj;
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

	m_gameObjects.erase(id);

	m_pPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED,
		0, _packet->systemAddress, true);
}

void Server::OnSpawnBullet(RakNet::Packet* _packet)
{
	RakNet::BitStream bsIn(_packet->data, _packet->length, false);
	bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
	
	glm::vec3 pos;
	glm::vec3 vel;
	bsIn.Read((char*)&pos, sizeof(glm::vec3));
	bsIn.Read((char*)&vel, sizeof(glm::vec3));
	SpawnObject(pos, vel * 3, 0.2f);

	// Sets the spawned bullets lifetime to 5 seconds
	m_gameObjects[m_nextServerID - 1].lifetime = 5.f;
}

void Server::SpawnObject(glm::vec3 _position, glm::vec3 _velocity, float _radius)
{
	m_gameObjects[m_nextServerID] = GameObject();
	m_gameObjects[m_nextServerID].id = m_nextServerID;
	m_gameObjects[m_nextServerID].networkData.SetElement("Position", _position);
	m_gameObjects[m_nextServerID].networkData.SetElement("LocalPosition", _position);
	m_gameObjects[m_nextServerID].networkData.SetElement("Velocity", _velocity);
	m_gameObjects[m_nextServerID].networkData.SetElement("Radius", _radius);

	m_gameObjects[m_nextServerID].Write(m_pPeerInterface, 
		RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);

	m_nextServerID++;
}

void Server::Despawn(int _id)
{
	RakNet::BitStream bs;
	bs.Write((RakNet::MessageID)ID_SERVER_DESPAWN);
	bs.Write(_id);

	m_pPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0,
		RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);

	// Erase from our local list
	m_gameObjects.erase(_id);
}

float Server::GetElapsedTime()
{
	LARGE_INTEGER currentTime, elapsedMicroSeconds, frequency;

	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&currentTime);

	elapsedMicroSeconds.QuadPart = currentTime.QuadPart - m_lastTime.QuadPart;
	m_lastTime = currentTime;

	// We now have the elapsed number of ticks, as well as
	// the number of ticks-per-second. We can then use these
	// values to convert to number of elapsed microseconds.

	// Here we convert to microseconds before dividing by
	// ticks-per-second to guard against precision loss.
	elapsedMicroSeconds.QuadPart *= 1000000;
	elapsedMicroSeconds.QuadPart /= frequency.QuadPart;

	return elapsedMicroSeconds.QuadPart * 0.000001f;
}

void Server::UpdateObjects()
{
	const int deltaTime = 17; // milliseconds per broadcast
	float timeToNextUpdate = 0; // countdown until we broadcast to all game objects
	float updateFrequency = 1.f; // seconds between updates

	while (true)
	{
		// Array of objects to despawn this frame
		std::vector<int> deathRow;

		float dt = GetElapsedTime();
		timeToNextUpdate -= dt;

		for (auto it = m_gameObjects.begin(); it != m_gameObjects.end(); it++)
		{
			GameObject& obj = it->second;

			// Broadcast to every server-controlled client
			if (obj.id >= 1000)
			{
				obj.Update(dt);

				if (timeToNextUpdate < 0)
					obj.Write(m_pPeerInterface, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);

				obj.lifetime -= deltaTime * 0.001f;
				
				// If expired, store in the vector rather than erasing while iterating
				if (obj.lifetime <= 0)
					deathRow.push_back(obj.id);
			}
		}

		if (timeToNextUpdate < 0)
			timeToNextUpdate = updateFrequency;

		// Remove expired objects from our game object map
		for (int i = 0; i < (int)deathRow.size(); i++)
		{
			Despawn(deathRow[i]);
		}
		deathRow.clear();

		std::this_thread::sleep_for(std::chrono::milliseconds(deltaTime));
	}
}

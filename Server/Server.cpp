#include "Server.h"
#include <iostream>
#include <MessageIdentifiers.h>
#include <BitStream.h>
#include "GameMessages.h"
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
	unsigned int MAXCONNECTIONS = 32;
	m_pPeerInterface->Startup(MAXCONNECTIONS, &sd, 1);
	m_pPeerInterface->SetMaximumIncomingConnections(MAXCONNECTIONS);

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
				SendNewClientID(packet->systemAddress);
				break;
			case ID_DISCONNECTION_NOTIFICATION:
				std::cout << "A client has disconnected." << std::endl;
				break;
			case ID_CONNECTION_LOST:
				std::cout << "A client has lost connection." << std::endl;
				break;
			case ID_CLIENT_CLIENT_DATA:
				OnReceivedClientData(packet);
				break;
			case ID_CLIENT_DISCONNECT:
				ClientDisconnect(packet);
				break;
			case ID_CLIENT_SPAWN_GAMEOBJECT:
				OnSpawnGameObject(packet);
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
	std::cout << "Client " << m_nextClientID << " has connected." << std::endl;

	RakNet::BitStream bs;
	bs.Write((RakNet::MessageID)ID_SERVER_SET_CLIENT_ID);
	bs.Write(m_nextClientID);
	m_nextClientID++;

	m_pPeerInterface->Send(&bs, HIGH_PRIORITY, 
		RELIABLE, 0, _address, false);

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

void Server::OnReceivedClientData(RakNet::Packet* _packet)
{
	RakNet::BitStream bs(_packet->data, _packet->length, false);

	// Read the gameobject and store it in our list
	GameObject object;
	object.Read(_packet);
	m_gameObjects[object.id] = object;

	m_pPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED,
		0, _packet->systemAddress, true);
}

void Server::OnSpawnGameObject(RakNet::Packet* _packet)
{
	RakNet::BitStream bsIn(_packet->data, _packet->length, false);
	bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
	
	glm::vec3 pos;
	glm::vec3 dir;
	float vel;
	float lifetime;
	bsIn.Read((char*)&pos, sizeof(glm::vec3));
	bsIn.Read((char*)&dir, sizeof(glm::vec3));
	bsIn.Read((char*)&vel, sizeof(float));
	bsIn.Read((char*)&lifetime, sizeof(float));
	SpawnObject(pos, dir * vel, 0.2f);

	m_gameObjects[m_nextServerID - 1].lifetime = lifetime;
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
	static LARGE_INTEGER lastTime;

	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&currentTime);

	elapsedMicroSeconds.QuadPart = currentTime.QuadPart - lastTime.QuadPart;
	lastTime = currentTime;

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
	const int deltaTime = (int)std::ceil(1000.f / 300.f); // milliseconds per broadcast : ~60 tps
	float timeToNextUpdate = 0; // countdown until we broadcast to all game objects
	float updateFrequency = 1.f; // seconds between sending network data to clients

	while (true)
	{
		// Array of objects to despawn this frame
		std::vector<int> deathRow;

		float dt = GetElapsedTime();
		timeToNextUpdate -= dt;

		for (auto& member : m_gameObjects)
		{
			GameObject& obj = member.second;

			// Update every server-controlled gameobject
			if (obj.id >= 1000)
			{
				obj.Update(dt);

				// Only send network data when 'timeToNextUpdate' reaches 0
				if (timeToNextUpdate < 0)
					obj.Write(m_pPeerInterface, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);

				obj.lifetime -= deltaTime * 0.001f;

				// If expired, store in the vector rather than erasing while iterating
				if (obj.lifetime <= 0)
					deathRow.push_back(obj.id);
			}
		}

		// Reset 'timeToNextUpdate' to 'updateFrequency'
		if (timeToNextUpdate < 0)
			timeToNextUpdate = updateFrequency;

		// Remove expired objects from our game object map
		for (auto i : deathRow)
			Despawn(i);

		deathRow.clear();

		// Sleep this thread for 'deltaTime' milliseconds
		std::this_thread::sleep_for(std::chrono::milliseconds(deltaTime));
	}
}

#include "Client.h"
#include "Gizmos.h"
#include "Input.h"

#include <glm/ext.hpp>
#include <iostream>

#include <MessageIdentifiers.h>
#include <BitStream.h>

using glm::vec3;
using glm::vec4;
using glm::mat4;
using aie::Gizmos;

Client::Client()
{
	m_gameobject = GameObject();
	m_updateThread = std::thread(std::bind(&Client::update, this));
}

Client::~Client()
{
	OnClientDisconnect();
	
	// Set thread to stop updating, and join it to the main thread
	m_shouldUpdate = false;
	m_updateThread.join();

	delete m_pPeerInterface;
}

void Client::update() {
 
	while (m_shouldUpdate)
	{
		// Delay the update to fake ~60fps
		float dtMs = 1000.f / (float)FPS;
		float dtS = dtMs * 0.001;
		std::this_thread::sleep_for(std::chrono::milliseconds((int)std::ceil(dtMs)));

		FRAMECOUNT = (FRAMECOUNT + 1) % NETWORKFRAME;

		HandleNetworkMessages();

		// Update all other GameObjects
		for (auto& otherClient : m_otherClientGameObjects)
		{
			switch (m_interpolationType)
			{
			case Interpolation::LINEAR:
				Interpolation_Linear(otherClient.second, dtS);
				break;
			case Interpolation::COSINE:
				Interpolation_Cosine(otherClient.second, dtS);
				break;
			default:
				Interpolation_None(otherClient.second);
			}
		}
	}
}

#pragma region Network Methods

void Client::InitialiseClientConnection()
{
	m_pPeerInterface = RakNet::RakPeerInterface::GetInstance();

	// Create an empty socket descriptor, no data is
	// needed as we are connecting to a server
	RakNet::SocketDescriptor sd;

	// Call startup - max 1 connection (the server)
	m_pPeerInterface->Startup(1, &sd, 1);

	std::cout << "Connection to server at: " << IP << std::endl;

	// Attempt to connect to the given server
	RakNet::ConnectionAttemptResult res = 
		m_pPeerInterface->Connect(IP, PORT, nullptr, 0);

	// If we did not connect to the server, print the error
	if (res != RakNet::CONNECTION_ATTEMPT_STARTED)
	{
		std::cout << "Unable to connect. Error: " << res << std::endl;
	}
}

void Client::HandleNetworkMessages()
{
	if (m_pPeerInterface == nullptr)
		return;

	RakNet::Packet* packet;

	// Check the packet and run appropriate code path
	for (packet = m_pPeerInterface->Receive(); packet;
		m_pPeerInterface->DeallocatePacket(packet),
		packet = m_pPeerInterface->Receive())
	{
		switch (packet->data[0])
		{
		case ID_REMOTE_DISCONNECTION_NOTIFICATION:
			std::cout << "Another client has disconnected." << std::endl;
			break;
		case ID_REMOTE_CONNECTION_LOST:
			std::cout << "Another client has lost connection." << std::endl;
			break;
		case ID_REMOTE_NEW_INCOMING_CONNECTION:
			std::cout << "Another client has connected." << std::endl;
			break;
		case ID_CONNECTION_REQUEST_ACCEPTED:
			std::cout << "Our request has been accepted." << std::endl;
			break;
		case ID_NO_FREE_INCOMING_CONNECTIONS:
			std::cout << "The server is full." << std::endl;
			break;
		case ID_DISCONNECTION_NOTIFICATION:
			std::cout << "We have been disconnected." << std::endl;
			break;
		case ID_CONNECTION_LOST:
			std::cout << "Connection lost." << std::endl;
			break;
		case ID_SERVER_TEXT_MESSAGE:
		{
			RakNet::BitStream bsIn(packet->data, packet->length, false);
			bsIn.IgnoreBytes(sizeof(RakNet::MessageID));

			RakNet::RakString str;
			bsIn.Read(str);

			std::cout << str.C_String() << std::endl;
			break;
		}
		case ID_SERVER_SET_CLIENT_ID:
			OnSetClientIDPacket(packet);
			break;
		case ID_CLIENT_CLIENT_DATA:
			OnReceivedClientDataPacket(packet);
			break;
		case ID_CLIENT_DISCONNECT:
			OnReceivedClientDisconnect(packet);
			break;
		case ID_SERVER_DESPAWN:
			OnDespawn(packet);
			break;
		default:
			std::cout << "Received a message with an unknown id: "
				<< packet->data[0] << std::endl;
			break;
		}
	}
}

void Client::OnSetClientIDPacket(RakNet::Packet* _packet)
{
	RakNet::BitStream bsIn(_packet->data, _packet->length, false);
	bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
	bsIn.Read(m_gameobject.id);
	m_gameobject.networkData.SetElement("Color", GameObject::GetColor(m_gameobject.id));
	m_gameobject.networkData.SetElement("Radius", 1.f);

	std::cout << "Set client ID to: " << m_gameobject.id << std::endl;
}

void Client::SendClientObject()
{
	m_gameobject.Write(m_pPeerInterface, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
}

void Client::OnReceivedClientDataPacket(RakNet::Packet* _packet)
{
	RakNet::BitStream bsIn(_packet->data, _packet->length, false);
	bsIn.IgnoreBytes(sizeof(RakNet::MessageID));

	int clientID;
	bsIn.Read(clientID);

	if (clientID != m_gameobject.id)
	{
		GameObject object = GameObject();
		object.Read(_packet);

		if (m_otherClientGameObjects.count(object.id) == 0)
		{
			// new object - snap on first update
			m_otherClientGameObjects[object.id] = object;
		}
		else
		{
			vec3 pos = object.networkData.GetElement<vec3>("Position");
			vec3 vel = object.networkData.GetElement<vec3>("Velocity");
			vec4 col = object.networkData.GetElement<vec4>("Color");

			// existing object - copy position, color, velocity but not localPosition
			m_otherClientGameObjects[object.id].networkData.SetElement("Position", pos);
			m_otherClientGameObjects[object.id].networkData.SetElement("Color", col);
			m_otherClientGameObjects[object.id].networkData.SetElement("Velocity", vel);
		}
	}
}

void Client::OnClientDisconnect()
{
	RakNet::BitStream bs;
	bs.Write((RakNet::MessageID)ID_CLIENT_DISCONNECT);

	int id = m_gameobject.id;
	bs.Write(id);

	m_pPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED,
		0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
}

void Client::OnReceivedClientDisconnect(RakNet::Packet* _packet)
{
	RakNet::BitStream bsIn(_packet->data, _packet->length, false);
	bsIn.IgnoreBytes(sizeof(RakNet::MessageID));

	int clientID;
	bsIn.Read(clientID);

	std::cout << "Client: " << clientID << " has disconnected.";

	m_otherClientGameObjects.erase(clientID);
}

void Client::SendSpawnedObject(vec3 _spawnPos, vec3 _direction, float _velocity)
{
	RakNet::BitStream bs;
	bs.Write((RakNet::MessageID)ID_CLIENT_SPAWN_GAMEOBJECT);
	bs.Write((char*)&_spawnPos, sizeof(vec3));
	bs.Write((char*)&_direction, sizeof(vec3));
	bs.Write((char*)&_velocity, sizeof(float));

	m_pPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE, 0,
		RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
}

void Client::OnDespawn(RakNet::Packet* _packet)
{
	RakNet::BitStream bsIn(_packet->data, _packet->length, false);
	bsIn.IgnoreBytes(sizeof(RakNet::MessageID));

	int id;
	bsIn.Read(id);

	m_otherClientGameObjects.erase(id);
}

#pragma endregion

#pragma region Interpolation Methods

void Client::Interpolation_None(GameObject& _gameObject)
{
	vec3 pos = _gameObject.networkData.GetElement<vec3>("Position");
	_gameObject.networkData.SetElement("LocalPosition", pos);
}

void Client::Interpolation_Linear(GameObject& _gameObject, float _dt)
{
	vec3 localPos = _gameObject.networkData.GetElement<vec3>("LocalPosition");
	vec3 pos = _gameObject.networkData.GetElement<vec3>("Position");

	vec3 velocity = _gameObject.networkData.GetElement<vec3>("Velocity");
	vec3 targetPos = pos + (velocity * _dt);

	static float t = 0.f;
	t = (float)FRAMECOUNT * (1.f / (float)NETWORKFRAME);

	localPos = t * targetPos + (1.f - t) * localPos;

	_gameObject.networkData.SetElement("LocalPosition", localPos);
}

void Client::Interpolation_Cosine(GameObject& _gameObject, float _dt)
{
	vec3 localPos = _gameObject.networkData.GetElement<vec3>("LocalPosition");
	vec3 pos = _gameObject.networkData.GetElement<vec3>("Position");

	vec3 velocity = _gameObject.networkData.GetElement<vec3>("Velocity");
	vec3 targetPos = pos + (velocity * _dt);

	static float mu = 0.f;
	mu = (float)FRAMECOUNT * (1.f / (float)NETWORKFRAME);

	float mu2 = (1 - cosf(mu * glm::pi<float>())) * 0.5f;
	localPos = (localPos * (1 - mu2) + targetPos * mu2);

	_gameObject.networkData.SetElement("LocalPosition", localPos);
}

#pragma endregion
#include "Client.h"

#include <iostream>
#include <MessageIdentifiers.h>
#include <BitStream.h>

Client::Client()
{
	m_gameobject = GameObject();
}

Client::~Client()
{
	OnClientDisconnect();

	// Clear all the maps
	m_otherClientGameObjects.clear();
	m_onReceivedFunctions.clear();
	m_otherObjectTs.clear();

	delete m_pPeerInterface;
}

void Client::update(float _dt) {
 
	if (m_networkDelay <= 0)
		m_networkDelay = m_storedDelay;

	// Check for network messages
	HandleNetworkMessages();

	// Update all other GameObjects
	for (auto& otherClient : m_otherClientGameObjects)
	{
		//otherClient.second.Update(_dt);

		switch (m_interpolationType)
		{
		case Interpolation::LINEAR:
			Interpolation_Linear(otherClient.second, _dt);
			break;
		case Interpolation::COSINE:
			Interpolation_Cosine(otherClient.second, _dt);
			break;
		default:
			Interpolation_None(otherClient.second);
		}
	}

	m_networkDelay -= _dt;
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

	std::cout << "Connecting to server at {" << IP << "} ..." << std::endl;

	// Attempt to connect to the given server
	RakNet::ConnectionAttemptResult res =
		m_pPeerInterface->Connect(IP, PORT, nullptr, 0);

	// Connection was not started
	if (res != RakNet::CONNECTION_ATTEMPT_STARTED)
	{
		std::cout << "Unable to connect. Error: " << res << std::endl;
	}
}

void Client::AddOnReceiveCall(int _id, std::function<void(GameObject&)> _fn)
{
	m_onReceivedFunctions.insert({_id, _fn});
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
		case ID_CONNECTION_REQUEST_ACCEPTED:
			std::cout << "Our request has been accepted." << std::endl;
			break;
		case ID_NO_FREE_INCOMING_CONNECTIONS:
			std::cout << "The server is full." << std::endl;
			m_serverFull = true;
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

	if (clientID != m_gameobject.id && clientID != -1)
	{
		GameObject object = GameObject();
		object.Read(_packet);

		// Reset object's t lerp value
		m_otherObjectTs[object.id] = 0;

		// If a received function exists, call it
		if (m_onReceivedFunctions.contains(clientID))
			m_onReceivedFunctions.at(clientID)(object);

		if (m_otherClientGameObjects.count(object.id) == 0)
		{
			// new object - snap on first update
			m_otherClientGameObjects[object.id] = object;
		}
		else
		{
			// overwrite everything but local position - for interpolation
			auto localPos = m_otherClientGameObjects[object.id].networkData.GetElement<vec3>("LocalPosition");
			m_otherClientGameObjects[object.id].networkData = object.networkData;
			m_otherClientGameObjects[object.id].networkData.SetElement("LocalPosition", localPos);
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

void Client::SendGameObject(GameObject _gameObject)
{
	_gameObject.Write(m_pPeerInterface, RakNet::UNASSIGNED_SYSTEM_ADDRESS, 
		true, (RakNet::MessageID)ID_CLIENT_SPAWN_GAMEOBJECT);
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

	vec3 vel = _gameObject.networkData.GetElement<vec3>("Velocity");
	float lerpTotalFrames = (_gameObject.id >= 1000 ? 1.f : m_storedDelay) / _dt;
	vec3 targetDiff = vel * _dt * lerpTotalFrames;

	float& t = m_otherObjectTs[_gameObject.id];
	t += 1.f / lerpTotalFrames;

	localPos = pos + (t * targetDiff);

	_gameObject.networkData.SetElement("LocalPosition", localPos);
}

void Client::Interpolation_Cosine(GameObject& _gameObject, float _dt)
{
	vec3 localPos = _gameObject.networkData.GetElement<vec3>("LocalPosition");
	vec3 pos = _gameObject.networkData.GetElement<vec3>("Position");

	vec3 vel = _gameObject.networkData.GetElement<vec3>("Velocity");
	float lerpTotalFrames = (_gameObject.id >= 1000 ? 1.f : m_storedDelay) / _dt;
	vec3 targetDiff = vel * _dt * lerpTotalFrames;

	float& t = m_otherObjectTs[_gameObject.id];
	t += 1.f / lerpTotalFrames;

	float tCos = (1 - cosf(t * glm::pi<float>())) * 0.5f;
	localPos = pos + (t * targetDiff);

	_gameObject.networkData.SetElement("LocalPosition", localPos);
}

#pragma endregion
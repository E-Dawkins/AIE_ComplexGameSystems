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

bool Client::startup() {
	
	setBackgroundColour(0.25f, 0.25f, 0.25f);

	// initialise gizmo primitive counts
	Gizmos::create(10000, 10000, 10000, 10000);

	// create simple camera transforms
	m_viewMatrix = glm::lookAt(vec3(10), vec3(0), vec3(0, 1, 0));
	m_projectionMatrix = glm::perspective(glm::pi<float>() * 0.25f,
										  getWindowWidth() / (float)getWindowHeight(),
										  0.1f, 1000.f);

	m_gameobject = GameObject();
	m_facing = vec3(1, 0, 0);

	InitialiseClientConnection();

	return true;
}

void Client::shutdown() {

	OnClientDisconnect();
	Gizmos::destroy();
}

void Client::update(float deltaTime) {
 
	FRAMECOUNT = (FRAMECOUNT + 1) % NETWORKFRAME;

	// wipe the gizmos clean for this frame
	Gizmos::clear();
	std::cout << getFPS() << std::endl;

	HandleNetworkMessages();
	
	// quit if we press escape
	aie::Input* input = aie::Input::getInstance();

	vec3 pos = m_gameobject.networkData.GetElement<vec3>("Position");
	// Zeroed in case no keys are pressed
	vec3 vel = vec3(0);

	// Store previous position and velocity
	static vec3 oldPosition = m_gameobject.networkData.GetElement<vec3>("Position");
	static vec3 oldVelocity = m_gameobject.networkData.GetElement<vec3>("Velocity");

	if (input->isKeyDown(aie::INPUT_KEY_LEFT))
	{
		pos.x -= 10.f * deltaTime;
		vel.x = -10;
		m_facing = glm::vec3(-1, 0, 0);
	}
	if (input->isKeyDown(aie::INPUT_KEY_RIGHT))
	{
		pos.x += 10.f * deltaTime;
		vel.x = 10;
		m_facing = glm::vec3(1, 0, 0);
	}
	if (input->isKeyDown(aie::INPUT_KEY_UP))
	{
		pos.z -= 10.f * deltaTime;
		vel.z = -10;
		m_facing = glm::vec3(0, 0, -1);
	}
	if (input->isKeyDown(aie::INPUT_KEY_DOWN))
	{
		pos.z += 10.f * deltaTime;
		vel.z = 10;
		m_facing = glm::vec3(0, 0, 1);
	}

	m_gameobject.networkData.SetElement("Position", pos);
	m_gameobject.networkData.SetElement("Velocity", vel);

	// Only send a network message when we change
	// our movement state, and it is a network frame
	if ((oldVelocity != vel || oldPosition != pos) && FRAMECOUNT == 0)
	{
		SendClientGameObject();
		oldPosition = pos;
		oldVelocity = vel;
	}

	if (input->wasKeyPressed(aie::INPUT_KEY_SPACE))
		SendSpawnBulletPacket();

	if (input->isKeyDown(aie::INPUT_KEY_ESCAPE))
		quit();

	// Update all other GameObjects
	for (auto& otherClient : m_otherClientGameObjects)
	{
		switch (m_interpolationType)
		{
		case Interpolation::LINEAR:
			Interpolation_Linear(otherClient.second);
			break;
		case Interpolation::COSINE:
			Interpolation_Cosine(otherClient.second, deltaTime);
			break;
		default:
			Interpolation_None(otherClient.second);
		}

		//otherClient.second.Update(deltaTime);
	}
}

void Client::draw() {

	// wipe the screen to the background colour
	clearScreen();

	// update perspective in case window resized
	m_projectionMatrix = glm::perspective(glm::pi<float>() * 0.25f,
										  getWindowWidth() / (float)getWindowHeight(),
										  0.1f, 1000.f);

	// Draw body
	vec3 pos = m_gameobject.networkData.GetElement<vec3>("Position");
	vec4 color = m_gameobject.networkData.GetElement<vec4>("Color");
	float radius = m_gameobject.networkData.GetElement<float>("Radius");

	Gizmos::addSphere(pos, radius, 8, 8, color);

	// Draw other clients bodies
	for (auto& otherClient : m_otherClientGameObjects)
	{
		vec3 localPos = otherClient.second.networkData.GetElement<vec3>("LocalPosition");
		vec4 color = otherClient.second.networkData.GetElement<vec4>("Color");
		float radius = otherClient.second.networkData.GetElement<float>("Radius");

		Gizmos::addSphere(localPos, radius, 8, 8, color);
	}

	Gizmos::draw(m_projectionMatrix * m_viewMatrix);
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
	vec4 col = m_gameobject.networkData.GetElement<vec4>("Color");
	m_gameobject.networkData.SetElement("Radius", 1.f);

	std::cout << "Set client ID to: " << m_gameobject.id << std::endl;
}

void Client::SendClientGameObject()
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

void Client::SendSpawnBulletPacket()
{
	RakNet::BitStream bs;
	bs.Write((RakNet::MessageID)ID_CLIENT_SPAWN_GAMEOBJECT);

	glm::vec3 spawnPos = m_gameobject.networkData.GetElement<vec3>("Position") + m_facing;
	float velocity = 3;

	bs.Write((char*)&spawnPos, sizeof(glm::vec3));
	bs.Write((char*)&m_facing, sizeof(glm::vec3));
	bs.Write((char*)&velocity, sizeof(float));

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

void Client::Interpolation_Linear(GameObject& _gameObject)
{
	vec3 localPos = _gameObject.networkData.GetElement<vec3>("LocalPosition");
	vec3 pos = _gameObject.networkData.GetElement<vec3>("Position");

	// Close 50% of the distance each frame
	float alpha = 0.5f;
	localPos = alpha * pos + (1.f - alpha) * localPos;

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
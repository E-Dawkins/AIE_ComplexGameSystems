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

Client::Client() {

}

Client::~Client() {
}

bool Client::startup() {
	
	setBackgroundColour(0.25f, 0.25f, 0.25f);

	// initialise gizmo primitive counts
	Gizmos::create(10000, 10000, 10000, 10000);

	// create simple camera transforms
	m_viewMatrix = glm::lookAt(vec3(10), vec3(0), vec3(0, 1, 0));
	m_projectionMatrix = glm::perspective(glm::pi<float>() * 0.25f,
										  getWindowWidth() / (float)getWindowHeight(),
										  0.1f, 1000.f);

	m_gameobject.position = vec3(0, 0, 0);
	m_facing = vec3(1, 0, 0);

	InitialiseClientConnection();

	return true;
}

void Client::shutdown() {

	OnClientDisconnect();

	Gizmos::destroy();
}

void Client::update(float deltaTime) {

	// query time since application started
	float time = getTime();

	// wipe the gizmos clean for this frame
	Gizmos::clear();

	HandleNetworkMessages();
	
	// quit if we press escape
	aie::Input* input = aie::Input::getInstance();

	// Store previous velocity
	glm::vec3 oldVelocity = m_gameobject.velocity;

	// Zero it in case no keys are pressed
	m_gameobject.velocity = glm::vec3(0);

	if (input->isKeyDown(aie::INPUT_KEY_LEFT))
	{
		m_gameobject.position.x -= 10.f * deltaTime;
		m_gameobject.velocity.x = -10;
		m_facing = glm::vec3(-1, 0, 0);
	}
	if (input->isKeyDown(aie::INPUT_KEY_RIGHT))
	{
		m_gameobject.position.x += 10.f * deltaTime;
		m_gameobject.velocity.x = 10;
		m_facing = glm::vec3(1, 0, 0);
	}
	if (input->isKeyDown(aie::INPUT_KEY_UP))
	{
		m_gameobject.position.z -= 10.f * deltaTime;
		m_gameobject.velocity.z = -10;
		m_facing = glm::vec3(0, 0, -1);
	}
	if (input->isKeyDown(aie::INPUT_KEY_DOWN))
	{
		m_gameobject.position.z += 10.f * deltaTime;
		m_gameobject.velocity.z = 10;
		m_facing = glm::vec3(0, 0, 1);
	}

	// Only send a network message when we change our movement state
	if (oldVelocity != m_gameobject.velocity)
		SendClientGameObject();

	if (input->wasKeyPressed(aie::INPUT_KEY_SPACE))
		SendSpawnBulletPacket();

	if (input->isKeyDown(aie::INPUT_KEY_ESCAPE))
		quit();

	// Update all other GameObjects
	for (auto& otherClient : m_otherClientGameObjects)
	{
		otherClient.second.Update(deltaTime);

		// Close 50% of the distance each frame
		float alpha = 0.5f;
		otherClient.second.localPosition =
			alpha * otherClient.second.position +
			(1.f - alpha) * otherClient.second.localPosition;
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
	Gizmos::addSphere(m_gameobject.position,
		m_gameobject.radius, 8, 8, m_gameobject.color);

	// Draw other clients bodies
	for (auto& otherClient : m_otherClientGameObjects)
	{
		Gizmos::addSphere(otherClient.second.localPosition,
			otherClient.second.radius, 8, 8, otherClient.second.color);
	}

	Gizmos::draw(m_projectionMatrix * m_viewMatrix);
}

void Client::HandleNetworkConnections()
{

}

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
	m_gameobject.color = GameObject::GetColor(m_gameobject.id);
	m_gameobject.radius = 1.f;

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
		GameObject object;
		object.Read(_packet);

		if (m_otherClientGameObjects.count(object.id) == 0)
		{
			// new object - snap on first update
			m_otherClientGameObjects[object.id] = object;
		}
		else
		{
			// existing object - copy position, color, velocity but not localPosition
			m_otherClientGameObjects[object.id].position = object.position;
			m_otherClientGameObjects[object.id].color = object.color;
			m_otherClientGameObjects[object.id].velocity = object.velocity;
		}

		// TODO - for now just output the object position to console
		std::cout << "Client " << clientID << " at: ("
			<< object.position.x << ", "
			<< object.position.y << ", "
			<< object.position.z << ")" << std::endl;
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
	bs.Write((RakNet::MessageID)ID_CLIENT_SPAWN_BULLET);

	glm::vec3 spawnPos = m_gameobject.position + m_facing;

	bs.Write((char*)&spawnPos, sizeof(glm::vec3));
	bs.Write((char*)&m_facing, sizeof(glm::vec3));

	m_pPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0,
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

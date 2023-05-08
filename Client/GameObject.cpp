#include "GameObject.h"
#include <BitStream.h>
#include "../Server/GameMessages.h"

void GameObject::Write(RakNet::RakPeerInterface* _pPeerInterface, const RakNet::SystemAddress& _address, bool _broadcast)
{
	RakNet::BitStream bs;
	bs.Write((RakNet::MessageID)ID_CLIENT_CLIENT_DATA);
	bs.Write(id);

	bs.Write((char*)&data, sizeof(data));
		_pPeerInterface->Send(&bs, HIGH_PRIORITY, 
		RELIABLE_ORDERED, 0, _address, _broadcast);

	float temp[3] = {1, 2, 3};
	glm::vec4 temp2 = glm::vec4(2);
	networkData.push_back(temp2);

	bs.Write((char*)&networkData, sizeof(networkData));
		_pPeerInterface->Send(&bs, HIGH_PRIORITY,
		RELIABLE_ORDERED, 0, _address, _broadcast);
}

void GameObject::Read(RakNet::Packet* _packet)
{
	RakNet::BitStream bsIn(_packet->data, _packet->length, false);
	bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
	bsIn.Read(id);
	bsIn.Read((char*)&data, sizeof(data));

	Data temp;
	bsIn.Read((char*)&temp, sizeof(temp));
}

void GameObject::Update(float _deltaTime)
{
	data.position += data.velocity * _deltaTime;
}

glm::vec4 colors[] = {
	glm::vec4(0.5, 0.5, 0.5, 1),
	glm::vec4(  1,   0,   0, 1),
	glm::vec4(  0,   1,   0, 1),
	glm::vec4(  0,   0,   1, 1),
	glm::vec4(  1,   1,   0, 1),
	glm::vec4(  1,   0,   1, 1),
	glm::vec4(  0,   1,   1, 1),
	glm::vec4(  0,   0,   0, 1)
};

glm::vec4 GameObject::GetColor(int _id)
{
	return colors[_id % 7];
}

#pragma once
#include <MessageIdentifiers.h>

enum ServerMessages
{
	ID_SERVER_TEXT_MESSAGE = ID_USER_PACKET_ENUM + 1,
	ID_SERVER_SET_CLIENT_ID,
	ID_CLIENT_CLIENT_DATA,
	ID_CLIENT_DISCONNECT,
	ID_CLIENT_SPAWN_GAMEOBJECT,
	ID_SERVER_DESPAWN
};
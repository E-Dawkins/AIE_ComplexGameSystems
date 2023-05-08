#pragma once
#include "Data.h"

class PlayerData : public Data
{
public:
	ServerData server;

	struct Client
	{
		float bf;
	} client;
};
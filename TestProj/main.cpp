#include "App.h"
#include <iostream>

int main() {

	auto app = new App();

	// This is here as an example of how to setup client defaults
	std::cout << "IP to connect to: ";
	std::string ipInput;
	std::cin >> ipInput;

	ipInput = (ipInput == "self" ? "127.0.0.1" : ipInput);
	app->client->SetIP(ipInput.c_str());

	std::cout << "--Interpolation--\n  0 => None\n  1 => Linear\n  2 => Cosine\nType? ";
	std::string lerpInput;
	std::cin >> lerpInput;

	if (lerpInput == "1") app->client->SetInterpolation(Client::LINEAR);
	else if (lerpInput == "2") app->client->SetInterpolation(Client::COSINE);
	else app->client->SetInterpolation(Client::NONE);

	app->client->SetNetworkDelay(0.05f);

	app->run("AIE", 1280, 720, false);
	delete app;

	return 0;
}
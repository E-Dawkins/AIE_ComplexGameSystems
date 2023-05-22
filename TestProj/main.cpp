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
	app->client->SetInterpolation(Client::Interpolation::COSINE);
	app->client->SetFPS(60);
	app->client->SetNetworkFrameDelay(3);

	app->run("AIE", 1280, 720, false);
	delete app;

	return 0;
}
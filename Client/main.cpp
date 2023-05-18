#include "Client.h"

int main() {
	
	auto app = new Client();

	// This is here as an example of how to setup client defaults
	std::cout << "IP to connect to: ";
	std::string ipInput;
	std::cin >> ipInput;
	app->SetIP(ipInput.c_str());

	app->SetInterpolation(Client::Interpolation::NONE);
	app->SetFPS(60);
	app->SetNetworkFrameDelay(3);

	app->run("AIE", 1280, 720, false);
	delete app;

	return 0;
}
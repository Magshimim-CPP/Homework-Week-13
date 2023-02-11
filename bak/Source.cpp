#pragma comment (lib, "ws2_32.lib")

#include "WSAInitializer.h"
#include "Server.h"
#include <iostream>
#include <exception>

int main()
{
	try
	{
		//initializing a WSAInitializer object and a Server object.
		WSAInitializer wsaInit;
		Server myServer;

		myServer.serve(8826);

		// main thread --> taking care of the messages.
		thread mainThread(&Server::messagesHandler, ref(myServer));
		mainThread.detach();
		
		//connector thread --> accepting clients in an infinite loop.
		thread connectorThread(&Server::acceptClientLoop, ref(myServer));
		connectorThread.join();

	}
	catch (std::exception& e)
	{
		std::cout << "Error occured: " << e.what() << std::endl;
	}
	system("PAUSE");
	return 0;
}
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

		//initializing the server (setting up the server for client service).
		myServer.serve(8826);

		//messages thread --> taking care of the message updates.
		thread messagesThread(&Server::messagesHandler, ref(myServer));
		messagesThread.detach();
		
		//accepting thread --> accepting clients in an infinite loop.
		thread acceptingThread(&Server::acceptClientLoop, ref(myServer));
		acceptingThread.join();

	}
	catch (std::exception& e)
	{
		std::cout << "Error occured: " << e.what() << std::endl;
	}
	system("PAUSE");
	return 0;
}
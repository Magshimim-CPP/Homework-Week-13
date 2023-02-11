#pragma once
//#including libraries and header files.
#include "Helper.h"
#include <WinSock2.h>
#include <Windows.h>
#include <iostream>
#include <queue>
#include <windows.h>
#include <map>
#include <mutex>
#include <condition_variable>
#include <string>
#include <fstream>
#include <exception>
#include <thread>
#include <sstream>

//using all of the needed function and objects from the std namespace.
using std::string;
using std::mutex;
using std::thread;
using std::map;
using std::queue;
using std::ifstream;
using std::ofstream;
using std::stringstream;
using std::exception;
using std::pair;
using std::vector;
using std::condition_variable;
using std::lock_guard;
using std::unique_lock;
using std::ios_base;
using std::make_pair;
using std::sort;
using std::ref;
using std::endl;
using std::cout;

#define NONE ""

/*
The message structure according to MagshiChat messages protocol.
* string sender - the name of the user who the message.
* string receiver - the name of the user who recived message. 
* string content - the content of the message.
*/
struct MessageStructure
{
	string sender;
	string receiver;
	string content;
};

//Server class
class Server
{

public:
	Server();
	~Server();
	void serve(int port);
	void acceptClientLoop();
	void messagesHandler();

private:
	SOCKET _serverSocket;
	mutex _msgMutex;
	mutex _usersMutex;
	condition_variable _msgCodVar;
	queue<MessageStructure> _msgQueue;
	map<string, SOCKET> _usersOnline;
	//------------FUNCTIONS------------
	void clientHandler(SOCKET clientSocket);
	string allUsersStringByProtocol();
};